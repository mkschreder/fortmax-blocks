/*************************************************************************
* Asynchronous I/O I2C library (based on the old i2cmaster and a few others.
* The library is interrupt based and uses the async framework for running
* callbacks from within the main loop. use this with async.c found in the
* same folder!
*
* Authod: Martin K. Schröder
* Website: http://oskit.se
* Email: info@fortmax.se
**************************************************************************/
#include <inttypes.h>
#include <compat/twi.h>

#include <kernel/device.h>
#include <kernel/i2c.h>
#include <kernel/gpio.h>

/* I2C clock in Hz */
#define SCL_CLOCK  10000L

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/twi.h>
#include <stdlib.h>

#include <kernel/i2c.h>

#define I2C_TIMEOUT 100000L

#define SCL_CLOCK  10000L
#define TWI_TWBR ((uint8_t)(((F_CPU/SCL_CLOCK)-16)/2))  /* must be > 10 for stable operation */

#define TWCR_START  (_BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE)) //0xA5 //send START 
#define TWCR_SEND   (_BV(TWINT) | _BV(TWEN) | _BV(TWIE)) //0x85 //poke TWINT flag to send another byte 
#define TWCR_RACK   (_BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE)) //0xC5 //receive byte and return ACK to slave  
#define TWCR_RNACK  (_BV(TWINT) | _BV(TWEN) | _BV(TWIE)) //0x85 //receive byte and return NACK to slave
#define TWCR_RST    (_BV(TWEN)) //0x04 //reset TWI
#define TWCR_STOP   (_BV(TWINT) | _BV(TWSTO) | _BV(TWEN)) //0x94 //send STOP,interrupt off, signals completion

#define TWI_BUFFER_SIZE 17  //SLA+RW (1 byte) +  16 data bytes (message size)

#define ZERO  0x00
#define ONE   0x01

struct i2c_device {
	i2c_command_t cmd;
	
	uint8_t busy;
	uint8_t _rw_count; 
	uint8_t _buf_ptr; 
};

static volatile struct i2c_device _devices[1];

volatile uint8_t _i2c_write_done = 0, _i2c_read_done = 0;

static void __i2c_read(struct i2c_device *dev);

ISR(TWI_vect){
	volatile struct i2c_device *dev = &_devices[0]; 

  uint8_t status = TWSR & 0xfc; 
  switch (status) {
    case TW_START:          //START has been xmitted, fall thorough
    case TW_REP_START:      //Repeated START was xmitted
			// load up device address
      TWDR = dev->cmd.addr;  
      dev->_buf_ptr = 0;
      TWCR = TWCR_SEND;     //send SLA+RW
      break;
    case TW_MT_SLA_ACK:     //SLA+W was xmitted and ACK rcvd, fall through 
    case TW_MT_DATA_ACK:                //Data byte was xmitted and ACK rcvd
      if (dev->_buf_ptr < dev->_rw_count){  //send data till done
        TWDR = dev->cmd.buf[dev->_buf_ptr++];  //load next and postincrement index
        TWCR = TWCR_SEND;               //send next byte 
      }
      else {
				if(dev->cmd.rcount == 0){
					//there is no read op pending so send stop 
					TWCR = TWCR_STOP;
				} else {
					__i2c_read(dev); 
				}
				_i2c_write_done = 1; 
			}
      break;
    case TW_MR_DATA_ACK:
			//Data byte has been rcvd, ACK xmitted, fall through
      dev->cmd.buf[dev->_buf_ptr++] = TWDR;    
    case TW_MR_SLA_ACK:
			//SLA+R xmitted and ACK rcvd
			if (dev->_buf_ptr < (dev->_rw_count -1)){
				TWCR = TWCR_RACK; // ACK each byte
			} else {
				TWCR = TWCR_RNACK; //NACK last byte
			}
      break; 
    case TW_MR_DATA_NACK: //Data byte was rcvd and NACK xmitted
      dev->cmd.buf[dev->_buf_ptr] = TWDR;      //save last byte to buffer
      TWCR = TWCR_STOP;                 //initiate a STOP
      _i2c_read_done = 1; 
     // _i2c_ready = 1; 
      break;      
    case TW_MT_ARB_LOST:                //Arbitration lost 
      TWCR = TWCR_START;                //initiate RESTART 
      break;
    default:                            //Error occured, save TWSR 
      //twi_state = TWSR;
      _i2c_read_done = 1;
      _i2c_write_done = 1; 
      TWCR = TWCR_RST;                  //Reset TWI, disable interupts 
  }//switch
}//TWI_isr


uint8_t twi_busy(void){
  return (bit_is_set(TWCR,TWIE)); //if interrupt is enabled, twi is busy
}

static void __i2c_write(struct i2c_device *dev){
	//uart_puts("i2write\n"); 

  _i2c_write_done = 0; 
	dev->cmd.addr = (dev->cmd.addr & ~TW_READ); 
	dev->_rw_count = dev->cmd.wcount;
	dev->_buf_ptr = 0; 

	TWCR = TWCR_START;                    //initiate START
}

static void __i2c_read(struct i2c_device *dev){
  _i2c_read_done = 0;
  
	dev->cmd.addr = (dev->cmd.addr | TW_READ); //set twi bus address, mark as write 
	dev->_rw_count = dev->cmd.rcount;              //load size of xfer
	dev->_buf_ptr = 0;
	
	TWCR = TWCR_START;                    //initiate START
}

static void __i2c_completed(void *ptr){
	//uart_puts("i2done!\n");
	
	struct i2c_device *dev = (struct i2c_device*)ptr;
	dev->busy = 0; 
	if(dev->cmd.callback){
		async_schedule(dev->cmd.callback, dev->cmd.arg, 0);
	}
}

int8_t i2c_transfer(handle_t handle, i2c_command_t cmd){
	struct i2c_device *dev = (struct i2c_device*)handle;
  if(dev->busy) return EBUSY;

  dev->busy = 1;
	dev->cmd = cmd;
	//dev->arg = dev;

	_i2c_write_done = 0; 
	_i2c_read_done = 0; 
	
	//async_wait(dev->cmd.callback, 0, dev->cmd.arg, &_i2c_ready, I2C_TIMEOUT);
	
	// setup completion callback depending on requested operation 
	if(cmd.wcount > 0) {
		if(cmd.rcount) {
			// if the operation involves a read as well then we need to add an extra step!
			//async_wait(__i2c_read, 0, dev, &_i2c_write_done, I2C_TIMEOUT);
			async_wait(__i2c_completed, __i2c_completed, dev, &_i2c_read_done, I2C_TIMEOUT * 2);
		} else {
			async_wait(__i2c_completed, __i2c_completed, dev, &_i2c_write_done, I2C_TIMEOUT);
		}
		__i2c_write(dev); 
	} else if(cmd.rcount > 0){
		// do normal read in one stage
		async_wait(__i2c_completed, __i2c_completed, dev, &_i2c_read_done, I2C_TIMEOUT);
		__i2c_read(dev); 
	}
  return SUCCESS; 
}

CONSTRUCTOR(i2c_init){
	TWDR = 0xFF;     //release SDA, default contents
  TWSR = 0x00;     //prescaler value = 1
  TWBR = TWI_TWBR; //defined in twi_master.h 
}

handle_t i2c_open(id_t id){
	return &_devices[0];
}

int16_t i2c_close(handle_t h){
	return SUCCESS;
}

static void i2c_tick(void){
	
}

DECLARE_DRIVER(i2c); 
