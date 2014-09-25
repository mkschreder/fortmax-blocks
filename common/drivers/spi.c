/*
spi lib 0x01

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include "spi.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define SPI_is_master (SPCR & _BV(MSTR))
#define SPI_is_slave (!SPI_is_master)



#define SShi {SPI_PORT |= _BV(SPI_SS);}
#define SSlo {SPI_PORT &= ~_BV(SPI_SS);}

#define CEhi {SPI_PORT |= _BV(SPI_CE);}
#define CElo {SPI_PORT &= ~_BV(SPI_CE);}

#define SSout {SPI_DDR |= _BV(SPI_SS);}
#define SSin {SPI_DDR &= ~_BV(SPI_SS);}

#define CEout {SPI_DDR |= _BV(SPI_CE);}
#define CEin {SPI_DDR &= ~_BV(SPI_CE);}

#define MISOout {SPI_DDR |= _BV(SPI_MISO);}
#define MISOin {SPI_DDR &= ~_BV(SPI_MISO);}

#define SS (SPI_PIN & _BV(SPI_SS))
#define CE (SPI_PIN & _BV(SPI_CE)) 

#define LEDon {PORTC |= _BV(PC0); DDRC |= _BV(PC0); }
#define LEDoff {PORTC &= ~_BV(PC0); DDRC &= ~_BV(PC0); }

#define E_NOTREADY 0
#define E_READY 0x02

// start condition is CE going from high to low and SS being low
#define START { \
	SPI_DDR |= (_BV(SPI_SS) | _BV(SPI_CE)); \
	SPI_PORT &= ~(_BV(SPI_SS) | _BV(SPI_CE)); \
}

#define EXT_RD (0x20)
#define EXT_WR (0x30)

typedef union {
	struct {
		uint16_t address;
		uint8_t command;
		//uint8_t data[16];
	};
	struct {
		//char packet[19];
	}; 
} packet_t;

// used by slave
static volatile char *_slave_buffer;
static volatile uint8_t _slave_buffer_size = 0; 
static volatile uint8_t _slave_buffer_ptr;
static volatile uint16_t _slave_addr = 0;

static packet_t _packet;
static volatile uint8_t _rx_count = 0; 
static volatile uint8_t _prev_pinb = 0;

void bus_master_init(){
	SPI_DDR &= ~((1<<SPI_MISO)); //input
	SPI_DDR |= (_BV(SPI_MOSI) | _BV(SPI_SS) | _BV(SPI_SCK) | _BV(SPI_CE)); //output
	// enable pullups
	SPI_PORT |= (_BV(SPI_SS) | _BV(SPI_CE));
	
	SPCR = ((1<<SPE)|               // SPI Enable
					(0<<SPIE)|              // SPI Interupt Enable
					(0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
					(1<<MSTR)|              // Master/Slave select
					(0<<SPR1)|(0<<SPR0)|    // SPI Clock Rate
					(0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
					(0<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)

	SPSR = (0<<SPI2X); // Double SPI Speed Bit
	
	// default is SS high and CE high
	SPI_PORT |= _BV(SPI_CE) | _BV(SPI_SS); 
}

/** initialize spi interface in SPI_EXT mode **/
void bus_slave_init(uint16_t base_addr, char *buf, uint8_t size){
	//_slave_addr = base_addr & 0x7fff;
	_slave_addr = base_addr;
	_slave_buffer = buf;
	_slave_buffer_size = size;
	
	// Enable PCINT interrupt on PB0 (int0) and PB1 (int1) 
	PCICR |= (1<<PCIE0);  
	PCMSK0 = (1<<PCINT0) | (1<<PCINT2);

	SPI_DDR &= ~(_BV(SPI_MOSI) | _BV(SPI_SS) | _BV(SPI_CE) | _BV(SPI_SCK)); //input
	// enable pullups on CE and SS
	SPI_PORT |= (_BV(SPI_CE) | _BV(SPI_SS));
	
	// MISO is also input by default so that we don't interfere with other slaves. 
	MISOin; 
	
	SPCR = ((1<<SPE)|               // SPI Enable
					(1<<SPIE)|              // SPI Interupt Enable
					(0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
					(0<<MSTR)|              // Master/Slave select
					(0<<SPR1)|(0<<SPR0)|    // SPI Clock Rate
					(0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
					(0<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)

	SPSR = (0<<SPI2X); // Double SPI Speed Bit
}

uint8_t bus_exchange_byte(uint8_t data) {
    SPDR = data;
    while((SPSR & (1<<SPIF)) == 0);
    return SPDR;
}

uint16_t ext_get_address(){ return _packet.address; }
uint16_t ext_state(){ return 0;}

// writes data to an expansion slot peripheral
uint8_t bus_write(uint16_t addr, const char *data, uint8_t size){
	//if(!ext_start(addr & 0x7fff)) return 0; 
	if(size > 0x0f) size = 0x0f;
	
	SSout;
	SSlo;
	_delay_us(10);

	bus_exchange_byte(addr);
	_delay_us(5);
	
	bus_exchange_byte(addr >> 8);
	_delay_us(5);

	bus_exchange_byte(EXT_WR | (size & 0x0f));
	_delay_us(5);
	
	// transfer data
	for(uint8_t c = 0; c < size; c++){
		//SSlo;
		bus_exchange_byte(data[c]);
		//SShi;
		_delay_us(10);
	}

	SShi;
	_delay_us(10); 
}

#define BYTE_DELAY_US 5
// reads data from expansion slot peripheral
uint8_t bus_read(uint16_t addr, char *data, uint8_t size){
	uint8_t retry = 10;
	uint8_t rx_count = 0; 
	while(retry--){
		cli();
		
		SSlo;
		_delay_us(BYTE_DELAY_US);

		bus_exchange_byte(addr);
		_delay_us(BYTE_DELAY_US);

		bus_exchange_byte(addr >> 8);
		_delay_us(BYTE_DELAY_US);

		bus_exchange_byte(EXT_RD | (size & 0x0f));
		_delay_us(BYTE_DELAY_US);

		// when slave gets the last address byte, it loads it's data register with previous
		// "next" byte. When it gets the command, it loads it with our status. 
		uint8_t stat = bus_exchange_byte(0);
		_delay_us(BYTE_DELAY_US);

		if(stat != E_READY){
			SShi;
			
			sei();
			
			_delay_us(50); 
			continue; 
		}
		
		//bus_exchange_byte(0);
		__asm("nop"); __asm("nop"); __asm("nop");
		
		// transfer data
		while(rx_count < size){
			data[rx_count] = bus_exchange_byte(0);
			rx_count++; 
			_delay_us(BYTE_DELAY_US); 
		}
		
		SShi;
		break; 
	}
	_delay_us(BYTE_DELAY_US);
	sei();
	
	return rx_count; 
}

enum states {
	BUS_IDLE,
	BUS_ADDRESS,
	BUS_COMMAND,
	BUS_RX_DATA,
	BUS_TX_DATA,
	BUS_STATE_COUNT
};

enum events {
	EV_BUS_START,
	EV_BUS_END,
	EV_BUS_DATA,
	EV_COUNT
};

typedef struct bus_s {
	uint8_t rx_bytes;
	uint16_t address;
	uint8_t state;
	uint8_t command;
	uint8_t out_byte;
	uint8_t in_byte; 
} bus_t;

bus_t _bus = {
	.rx_bytes = 0,
	.address = 0,
	.state = BUS_IDLE, 
	.command = 0
}; 

static void _bus_reset();

static void _bus_idle_start(){
	// called when ss goes low
}

static void _bus_idle_rx_byte(){
	_bus.address |= _bus.in_byte;
	_bus.state = BUS_ADDRESS; 
}

static void _bus_address_rx_byte(){
	_bus.address |= _bus.in_byte << 8;
	if((_bus.address >= _slave_addr) &&
		(_bus.address < (_slave_addr + _slave_buffer_size))){
		// if the address matches our address then we enter command state
		MISOout;
		_bus.out_byte = E_READY; 
		_bus.state = BUS_COMMAND;
	} else {
		_bus_reset(); 
	}
}

static void _bus_command_rx_byte(){
	_bus.command = _bus.in_byte & 0xf0;
	uint8_t size = _bus.in_byte & 0x0f;
	
	if(_bus.command == EXT_RD)
		_bus.state = BUS_TX_DATA;
	else if(_bus.command == EXT_WR)
		_bus.state = BUS_RX_DATA;
		
	_slave_buffer_ptr = 0; 
}

static void _bus_rx_data(){
	if(_slave_buffer_ptr < _slave_buffer_size){
		_slave_buffer[_slave_buffer_ptr++] = _bus.in_byte;
		_bus.out_byte = _bus.in_byte; // send it back to master for inspection
	} else {
		_bus_reset();
	}
}

static void _bus_tx_data(){
	
	//PORTB |= _BV(1);
	if(_slave_buffer_ptr < _slave_buffer_size){
		_bus.out_byte = _slave_buffer[_slave_buffer_ptr++];
	} else {
		_bus_reset();
	}
	//PORTB &= ~_BV(1);
}

static void _bus_reset(){
	_bus.address = 0;
	_bus.command = 0; 
	_bus.state = BUS_IDLE;
	MISOin; 
}

static void _bus_nop(){
	
}

// start, end, data
void (*const state_table [BUS_STATE_COUNT][EV_COUNT]) () = {
	{ _bus_idle_start, 	_bus_reset, _bus_idle_rx_byte,  }, // idle
	{ _bus_nop, 				_bus_reset, _bus_address_rx_byte }, // address
	{ _bus_nop, 				_bus_reset, _bus_command_rx_byte }, // command
	{ _bus_nop, 				_bus_reset, _bus_rx_data }, // rx
	{ _bus_nop, 				_bus_reset, _bus_tx_data }, // tx
};

/// Enabled for the bus transfers
ISR(PCINT0_vect){
	DDRB |= _BV(1); 
	//PORTB |= _BV(1);
	
	uint8_t changed = _prev_pinb ^ SPI_PIN; 
	_prev_pinb = SPI_PIN;

	if(SPI_is_slave && (changed & _BV(SPI_SS))){
		if(SS){
			state_table[_bus.state][EV_BUS_END]();
		} else {
			state_table[_bus.state][EV_BUS_START]();
		}
	}
	/*
	if(SPI_is_slave && (changed & _BV(SPI_SS))){
		// if ss went low then we have a start of transmission

		MISOin;
		
		if(!SS){
			uint8_t _rx_count = 0;
			uint16_t _addr = 0;
			uint8_t _command = 0;
			uint8_t _slave_buffer_ptr = 0;
			uint8_t _out_data = 0;
			
			while(!SS){
				uint8_t data; 
				while(!SS && (SPSR & (1<<SPIF)) == 0) data = SPDR;
				// not that this swap MUST happen right away before master starts
				// sending the next byte. The timing is critical! 
				SPDR = _out_data; 

				PORTB |= _BV(1);
				if(_rx_count < 2){
					if(_rx_count){
						_addr |= (((uint16_t)data) << 8);
						if((_addr >= _slave_addr) &&
								(_addr < (_slave_addr + _slave_buffer_size))){
									
							_out_data = E_READY; 
							MISOout; 
						}
					}
					else
						_addr |= data;
					
				} else if(_rx_count == 2){
					_command = data;
				} else if(_rx_count > 2){
					uint8_t cmd = _command & 0xf0;
					uint8_t size = _command & 0x0f; 
					if(cmd == EXT_RD && (_slave_buffer_ptr < _slave_buffer_size)){
						_out_data = _slave_buffer[_slave_buffer_ptr++];
					} else if(cmd == EXT_WR && (_slave_buffer_ptr < _slave_buffer_size)){
						_slave_buffer[_slave_buffer_ptr++] = data;
						_out_data = data; // send it back to master for inspection
					}
				}
				_rx_count++;
				PORTB &= ~_BV(1);
			}
		}
		MISOin; 
	}
	LEDoff; */
/*
	if(SPI_is_slave && (changed & _BV(SPI_SS))){
		// SS pulled low - reset all logic
		if(!SS){
			//_slave_buffer_ptr = 0;
			//_addr_buffer = 0x0000;
			//_rx_byte_cnt = 0;
			//_cmd = 0;
			_slave_buffer_ptr = 0; 
			_packet.address = 0x0000;
			_packet.command = 0;
			_rx_count = 0;
			
			MISOin;
		}
		// SS went high - end of transmission
		else {
			
			MISOin;
		}
	}

	LEDoff;
	
	PORTB &= ~_BV(1);*/
}

/// This ISR is only used when data is received in slave mode.
/// Takes 1.5us
/// Interrupt latency: 2us
/// SPI transmission time: 4us at clk/8
ISR(SPI_STC_vect)
{
	PORTB |= _BV(1);
	DDRB |= _BV(1);
	
	_bus.in_byte = SPDR; 
	state_table[_bus.state][EV_BUS_DATA]();
	SPDR = _bus.out_byte;

	PORTB &= ~_BV(1); 
	/*
	DDRC |= _BV(3); 
	
	uint8_t data = SPDR;
	//_packet.packet[_rx_count] = data;

	PORTC |= _BV(3); 
	//if(_rx_count > 0x0f) return;
	
	
	//uint8_t size = _packet.command & 0x0f;

	if(_rx_count < 2){
		
		if(_rx_count == 0)
			_packet.address |= data;
		else {
			_packet.address |= data << 8;

			// If the address is received and it is valid, then we load the
			// data register with a flag to specify whether we are ready to
			// receive the next packet and make our MISO pin an output. 
			if((_packet.address >= _slave_addr) &&
				(_packet.address < (_slave_addr + _slave_buffer_size))){
					
				SPDR = E_READY;
				MISOout;
			}
		}
		
	} else if(_rx_count == 2){
		_packet.command = data;
	} else {
		if((_packet.command & 0xf0) == EXT_RD && (_slave_buffer_ptr < _slave_buffer_size)){
			SPDR = _slave_buffer[_slave_buffer_ptr++];
		} else if((_packet.command & 0xf0) == EXT_WR && (_slave_buffer_ptr < _slave_buffer_size)){
			_slave_buffer[_slave_buffer_ptr++] = data;
			SPDR = data; // send it back to master for inspection
		}
	}
	_rx_count++; 
	PORTC &= ~_BV(3); 
	/*
	PORTC |= _BV(3);
	uint8_t data = SPDR;
	_packet.packet[_rx_count] = data;

	uint8_t size = (_packet.command & 0x0f); 
	// compare address and generate affirmation 
	if(_rx_count == 1
		 && (_packet.address >= _slave_addr) &&
		(_packet.address < (_slave_addr + _slave_buffer_size))){
		
		SPDR = E_READY;
		MISOout;
	} else if(_rx_count == 2){
		//(_packet.address + size) < (_slave_addr + _slave_buffer_size)){
		for(uint8_t c = 0; c < 16; c++){
			if(c == size) break;
			_packet.data[c] = _slave_buffer[_packet.address - _slave_addr + c];
		}
		
		}
	} else {
		SPDR = _packet.packet[_rx_count];
		//_packet.packet[_rx_count] = data; 
	}
	_rx_count++;
	
	PORTC &= ~_BV(3); */
	/*
	if(_rx_byte_cnt < 2){
		if(_rx_byte_cnt == 0)
			_addr_buffer |= data;
		else {
			_addr_buffer |= data << 8;

			// If the address is received and it is valid, then we load the
			// data register with a flag to specify whether we are ready to
			// receive the next packet and make our MISO pin an output. 
			if((_addr_buffer >= _slave_addr) &&
				(_addr_buffer < (_slave_addr + _slave_buffer_size))){
				SPDR = E_READY;
				MISOout;
			}
		}
	} else if(_rx_byte_cnt == 2){
		_cmd = data;
	} else {
		if(_cmd == EXT_RD && (_slave_buffer_ptr < _slave_buffer_size)){
			SPDR = _slave_buffer[_slave_buffer_ptr++];
		} else if(_cmd == EXT_WR && (_slave_buffer_ptr < _slave_buffer_size)){
			_slave_buffer[_slave_buffer_ptr++] = data;
			SPDR = data; // send it back to master for inspection
		}
	}

	_rx_byte_cnt++;
	*/
}



/*
 * SOFTWARE SPI
 * 
void spi_init(){
	SPI_DDR &= ~((1<<SPI_MISO)); //input
	SPI_DDR |= ((1<<SPI_MOSI) | (1<<SPI_SS) | (1<<SPI_SCK)); //output
}

uint8_t spi_writereadbyte(uint8_t data){
	uint8_t byte_in = 0;
	
	for (uint8_t bit = 0x80; bit; bit >>= 1) {
			//write_MOSI((byte_out & bit) ? HIGH : LOW);
			if(data & bit)
				SPI_PORT |= _BV(SPI_MOSI);
			else
				SPI_PORT &= ~_BV(SPI_MOSI);

			//delay(SPI_SCLK_LOW_TIME);
			_delay_us(1);
			
			SPI_PORT |= _BV(SPI_SCK); 
			//write_SCLK(HIGH);

			if(SPI_PIN & _BV(SPI_MISO))
				byte_in |= bit;
				
			//if (read_MISO() == HIGH)
			//		byte_in |= bit;

			//delay(SPI_SCLK_HIGH_TIME);
			_delay_us(1);
			
			//write_SCLK(LOW);
			SPI_PORT &= ~_BV(SPI_SCK); 
	}

	return byte_in;
}
*/

/*
 * spi initialize
 */
 /*
void spi_init() {
    SPI_DDR &= ~((1<<SPI_MOSI) | (1<<SPI_MISO) | (1<<SPI_SS) | (1<<SPI_SCK)); //input
    SPI_DDR |= ((1<<SPI_MOSI) | (1<<SPI_SS) | (1<<SPI_SCK)); //output

    SPCR = ((1<<SPE)|               // SPI Enable
            (0<<SPIE)|              // SPI Interupt Enable
            (0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
            (1<<MSTR)|              // Master/Slave select
            (0<<SPR1)|(1<<SPR0)|    // SPI Clock Rate
            (0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
            (0<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)

    SPSR = (1<<SPI2X); // Double SPI Speed Bit
}*/
