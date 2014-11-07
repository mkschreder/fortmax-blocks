#include <avr/sleep.h>
#include <avr/power.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "crypto/sha256.h"
#include "random.h"
#include "nrf24l01.h"
#include "rfnet.h"

/** Half duplex comm

MASTERSTART
SLAVEACK
MASTERACK
MASTERFIN

- master picks random nonce and sends it to slave in first packet
- slave always acks packet received from master and sends with it it's data to master
- master checks the ack and sends either a fin or masterack with more data to slave
- if fin is sent, the slave does not ack it. 

if master sends masterstart and does not receives slaveack, master resends masterstart. If slave sends ack and does not receive either masterack or masterfin, slave resends the slaveack. if slave does not get the masterfin, it will send slaveack once again until timeout. In that case master will retransmit masterfin again until the slave stops sending slaveack. If master is not listening or has switched to communicatin with another slave on another channel, then slave will eventually timeout. All packets with valid checksum but wrong sequence numbers that are picked up by the master are answered by the master with masterfin. Slaves never send data on the air unless they are asked to do so by the master. 

All nodes that operate on a network address can listen in on the communications. If multiple masters are on the bus, they can listen in until they get a MASTERFIN packet and then start transmitting. But for simplicity it is better to only have one master that instructs slaves to start sending data. If slave has no data, it will transmit slaveack with size 0. 

Basically the master sends out masterstart, waits 1ms (for the slave to encrypt response) then tries to read. If no data is available, master resends the packet again, waits 1ms and reads response. Slave then asnvers with slaveack with nonce set to nonce of previous master packet plus one. If master receives any other nonce, it rejects the packet and resends again until it gets the correct nonce or times out. When correct nonce is received, master sends out masterack with the received nonce plus one. Slave does the same - waits for correct nonce before sendin out more data.

The rf driver takes as input the address on which to do all communication and the key or password that is used for encryption. Any subsequent call to send/recv will send or receive on that address. The send and receive operation is a simultaneous process - just like with SPI. so the method that is used for sending a buffer, will send the buffer and replace the data with bytes received from the other side. 
*/
/*
// default address 
uint8_t rf_addr[5] = {0x3b, 0x31, 0xbb, 0x23, 0x39};

// aes256 key for this device pair
uint8_t device_key[32] = {
		0x62, 0xd9, 0x60, 0xdb, 0x66, 0xdd, 0x64, 0xdf,
		0x6a, 0xd1, 0x68, 0xd3, 0x6e, 0xd5, 0x6c, 0xd7,
		0x72, 0xc9, 0x70, 0xcb, 0x76, 0xcd, 0x74, 0xcf,
		0x7a, 0xc1, 0x78, 0xc3, 0x7e, 0xc5, 0x7c, 0xc7
};*/


#define RFNET_BUFFER_SIZE 16
#define RFNET_MAX_PIPES 6
#define RFNET_READ_TIMEOUT 200

#define DEVICE_PASSWORD "pleasechangeme"


// packet flags
#define PTF_SYN 0x10
#define PTF_ACK 0x20
#define PTF_FIN 0x40
#define PTF_RST 0x80

enum {
	PT_READ = 1, // sends packet with nonce and addr, gets packet with data and nonce+1
	PT_READ_RESP, // response to a read command
	PT_WRITE, // sends packet with client nonce + 1, addr and data
	PT_WRITE_RESP, // response to a write command
	PT_MASTERSTART, 
	PT_SLAVEACK,
	PT_MASTERACK,
	PT_MASTERFIN
	//PT_REQ_WRITE,
	//PT_DATA
};

typedef uint16_t nonce_t; 

typedef void (*rfnet_write_callback_t)(int8_t con, const uint8_t *data, uint16_t size, void *ctx); 
typedef void (*rfnet_read_callback_t)(int8_t con, uint8_t *data, uint16_t size, void *ctx); 

struct rfnet_connection {
	rfnet_mac_t 	addr; 
	aes256_ctx_t 	key; 
	uint32_t 			timeout; 
	uint8_t 			state; 
	rfnet_write_callback_t 	write; 
	rfnet_read_callback_t 	read; 
}; 

struct __attribute__((__packed__)) packet {
	uint8_t type; 
	uint8_t data[10];
	uint16_t nonce; // unique nonce for this packet
	uint8_t size; // number of bytes in data that are valid data
	uint16_t sum; // crc16 of the data
}; 

typedef struct {
	unsigned char data[sizeof(struct packet)];
} encrypted_packet_t;

static uint16_t gen_crc16(const uint8_t *data, uint16_t size)
{
	uint16_t crc = 0;
	for(int c = 0; c < size; c++){
		crc = _crc16_update(crc, *data++);
	}
	return crc; 
}

/// this function does nothing. 
static void ___validate_packet_sizes___(void){
	//struct packet p; 
	STATIC_ASSERT(sizeof(struct packet) == 16, "Packet size must be 16 bytes!");
	STATIC_ASSERT(sizeof(struct packet) == sizeof(encrypted_packet_t), "Packet size must match aes encryption buffer size of 16 bytes");
	/*STATIC_ASSERT((((uint8_t*)&p.sum) - ((uint8_t*)&p)) !=
		(sizeof(struct packet) - sizeof(p.sum)),
			"Checksum field must be placed at the end of packet struct!");*/
	STATIC_ASSERT(sizeof(struct packet) >= NRF24L01_PAYLOAD, "Transport layer block size must be same as packet size!"); 
}

static void packet_init(struct packet *pack){
	memset(pack, 0, sizeof(struct packet));
}

static uint8_t packet_validate(struct packet *pack){
	return pack->sum == gen_crc16((uint8_t*)pack, sizeof(struct packet) - 2);
}

static void packet_encrypt(aes256_ctx_t *ctx, struct packet *pack, encrypted_packet_t *ep){
	pack->sum = gen_crc16((uint8_t*)pack, sizeof(struct packet) - 2);
	memcpy(ep, pack, sizeof(struct packet));
	aes256_enc(ep, ctx);
}

static uint8_t packet_decrypt(aes256_ctx_t *ctx, encrypted_packet_t *ep, struct packet *pack){
	encrypted_packet_t _ep;
	// dec() is destructive so we need to create local copy
	memcpy(&_ep, ep, sizeof(encrypted_packet_t)); 
	aes256_dec(&_ep, ctx);
	uint8_t valid = packet_validate((struct packet *)&_ep);
	if(valid) memcpy((char*)pack, &_ep, sizeof(struct packet));
	return valid; 
}

static uint8_t rfnet_send_packet(
		struct rfnet *net, struct rfnet_device *dev, struct packet *pack){
	uint8_t ret = 0;
	encrypted_packet_t ep; 
	
	packet_encrypt(&dev->ctx, pack, &ep);
	
	for(int c = 0; c < RFNET_READ_TIMEOUT; c++){
		ret = nrf24l01_write((uint8_t*)&ep);

		if(ret == 1){
			// success
			net->stats.sent++;
			return 1; 
		} else if(ret == 2) {
			// retry
		} else {
			break;  
		}
		_delay_ms(1); 
	}
	net->stats.send_failed++; 
	return 0; 
}

static uint8_t rfnet_recv_packet(struct rfnet *net, struct rfnet_device *dev, struct packet *pack){
	uint8_t pipe = 0xff; 
	uint8_t buffer[NRF24L01_PAYLOAD]; 
	
	//if data is ready on our interface
	if(nrf24l01_readready(&pipe) && pipe < 7) { 
		//read buffer
		nrf24l01_read(buffer);
		
		net->stats.received++; 
		
		if(!packet_decrypt(&dev->ctx, (encrypted_packet_t*)buffer, pack)){
			return 0; 
		}
		
		net->stats.decrypted++; 
		
		if(pack->nonce != dev->nonce){
			net->stats.dropped++; 
			return RFNET_ERR_NONCE_MISMATCH; 
		}
		return 1; 
	}
	return 0; 
}

void rfnet_dev_init(struct rfnet_device *dev, const char *pw){
	sha256_hash_t hash; 
	sha256(&hash, pw, strlen(pw)); 
	
	aes256_init(hash, &dev->ctx); 
	dev->nonce = 0; 
}

void rfnet_init(struct rfnet *net, const char *root_pw, 
	volatile uint8_t *outport, volatile uint8_t *ddrport, 
	uint8_t cepin, uint8_t cspin){
	// to avoid compiler warning
	___validate_packet_sizes___(); 
	
	rfnet_dev_init(&net->root_dev, root_pw); 
	
	/*sha256_hash_t hash; 
	sha256(&hash, root_pw, strlen(root_pw)); 
	
	aes256_init(hash, &net->root_ctx); */
	net->get_field = 0; 
	net->stats.sent = net->stats.received = net->stats.decrypted = 0; 
	
	nrf24l01_init(outport, ddrport, cepin, cspin); 
	//nrf24l01_init(&PORTC, &DDRC, PC2, PC3); 
}


static void _rfnet_init_syn_packet(struct packet *pack, struct rfnet_device *dev){
	dev->nonce = rand(); 
	packet_init(pack); 
	pack->type = PTF_SYN; 
	pack->nonce = dev->nonce; 
	pack->size = 0; 
	dev->nonce++; 
}

static void _rfnet_init_synack_packet(struct packet *pack, struct rfnet_device *dev){
	packet_init(pack); 
	pack->type = PTF_SYN | PTF_ACK; 
	pack->nonce = ++dev->nonce; 
	pack->size = 0; 
}

static void _rfnet_init_ack_packet(struct packet *pack, struct rfnet_device *dev){
	packet_init(pack); 
	pack->type = PTF_ACK; 
	pack->nonce = ++dev->nonce; 
	pack->size = 0; 
}

static void _rfnet_init_read_packet(struct packet *pack, struct rfnet_device *dev, uint8_t field){
	packet_init(pack); 
	pack->type = PT_READ; 
	pack->data[0] = field; 
	pack->nonce = dev->nonce++; 
	pack->size = 1; 
}

static void _rfnet_init_data_packet(struct packet *pack, struct rfnet_device *dev, uint8_t *data, uint8_t size){
	packet_init(pack); 
	pack->type = PT_READ_RESP; 
	memcpy(pack->data, data, size); 
	pack->size = size; 
	pack->nonce = ++dev->nonce; 
}

/** 
 * Connects to another node by agreeing on a common nonce
 */
static int8_t rfnet_connect(struct rfnet *net, struct rfnet_device *dev, uint8_t field){
	struct packet syn;
	struct packet ack; 
	
	// send syn 
	_rfnet_init_syn_packet(&syn, dev); 
	
	// wait for ack and retry
	for(int c = 0; c < 4; c++){
		rfnet_send_packet(net, dev, &syn); 
		for(int wait = 0; wait < 40; wait++){
			if(rfnet_recv_packet(net, dev, &ack)){
				if(ack.type & (PTF_SYN | PTF_ACK))
					return 1; 
				else {
					// otherwise resend syn
					//rfnet_send_packet(net, dev, &syn); 
				}
			}
			_delay_ms(1); 
		}
	}
	
	return 0; 
}

/// Sets the address of our listening pipe
void rfnet_setaddress(const rfnet_mac_t addr){
	
}

/// Sets the encryption key for our listening pipe
void rfnet_setpassword(const char *pw){
	
}

/// Sets up a context for a node that has hw address addr, key, and method that is called when new packets are decoded from the node. 
int8_t rfnet_connect(const rfnet_mac_t addr, const char *pw, 
	// called when the node tries to send data to us
	rfnet_write_callback_t write, 
	// called when the driver requests data to send to the node
	rfnet_read_callback_t read, 
	void *ctx){
	
}

/// performs an active write to the connection
int8_t rfnet_write(int8_t con); 

/** 
	encrypts the data and sends it out on the address of the device
	awaits ack from the device with possible data packet. 
**/
void rfnet_writeread(struct rfnet_device *dev, const uint8_t *datain, uint8_t *dataout, uint16_t size){
	
}

void rfnet_read(uint8_t *data, uint16_t size){
	
}

/*** 
 * Synchronized read operation
 * 
 * - we send a PT_READ packet with field id to the node
 * - we start listening for packets from the node
 * - node answers with first PT_READ_RESP packet containing the data
 * - we send PT_ACK to the node. 
 * - node sends next PT_READ_RESP
 * 
 * Possible pitfalls: 
 * - our PT_READ packet does not reach the node
 * -- We timeout and return error
 * - nodes PT_READ_RESP does not reach us
 * -- We timeout and return error
 * - another node sends out ACK to our node
 * -- We get packet with wrong nonce so we return error
 * - our ack does not reach the node in time
 * -- We timeout and return error
 **/
 
int8_t rfnet_sync_read(struct rfnet *net, struct rfnet_device *dev, 
	uint8_t field_id, uint8_t *outbuf, int16_t size){
	struct packet pack; 
	
	// clear any pending acks or other stuff in the queue
	//nrf24l01_flushTXfifo(); 
	
	// send syn and wait for synack (indicating the other end is ready)
	if(!rfnet_connect(net, dev, field_id)){
		return RFNET_ERR_CONNECT_TIMEOUT; 
	}
	
	// send read request and wait for one or more read_resp
	_rfnet_init_read_packet(&pack, dev, field_id); 
	for(int c = 0; c < 5; c++){
		if(rfnet_send_packet(net, dev, &pack))
			break; 
		if(c == 4)
			return RFNET_ERR_READ_REQ_SEND_FAIL; 
		_delay_ms(5); 
	}
	
	int16_t timeout = RFNET_READ_TIMEOUT; 
	int16_t read_count = 0; 
	
	while(timeout > 0 && read_count < size){
		if(rfnet_recv_packet(net, dev, &pack)){
			if(pack.type == PT_READ_RESP && pack.size > 0){
				
				uint8_t cs = ((read_count + pack.size) >= size)?(size-read_count):pack.size;
				
				memcpy(outbuf, pack.data, cs); 
				read_count += cs; 
				outbuf+=cs; 
				
				// send ack
				_rfnet_init_ack_packet(&pack, dev); 
				if(!rfnet_send_packet(net, dev, &pack)){
					return RFNET_ERR_BUFFER_SEND_FAILED; 
				}
				
				if(read_count < size){
					timeout = RFNET_READ_TIMEOUT; 
					
					continue; 
				} else {
					//nrf24l01_flushRXfifo(); 
					break; 
				}
			} else {
				return RFNET_ERR_INVALID_PACKET;
			}
		} 
		_delay_ms(1); 
		timeout-=1; 
	}
	if(timeout <= 0) return RFNET_ERR_TIMEOUT; 
	if(read_count < size) return RFNET_ERR_ALL_NOT_READ; 
	return 1; 
}

void rfnet_write(void){
	
}

int8_t rfnet_process_packets(struct rfnet *net){
	struct packet pack; 
	if(rfnet_recv_packet(net, &net->root_dev, &pack)){
		// connection request
		if(pack.type & PTF_SYN){
			// store nonce for future reference
			net->root_dev.nonce = pack.nonce + 1; 
			// send back syn ack
			_rfnet_init_synack_packet(&pack, &net->root_dev); 
			rfnet_send_packet(net, &net->root_dev, &pack); 
		} else if(pack.type == PT_READ && net->get_field){
			// read: 
			// data[0] = field_id
		
			uint8_t *field_data; 
			uint8_t field_size = 0; 
			
			if(net->get_field(pack.data[0], &field_data, &field_size)){
				while(field_size > 0){ 
					uint8_t size = (field_size > sizeof(pack.data))?sizeof(pack.data):field_size; 
					
					struct packet pdata;
					
					// prepare a data packet and send it 
					_rfnet_init_data_packet(&pdata, &net->root_dev, field_data, size); 
					if(!rfnet_send_packet(net, &net->root_dev, &pdata)){
						return RFNET_ERR_BUFFER_SEND_FAILED; 
					}
					
					field_size -= size; 
					field_data += size; 
					
					// wait for ack response
					uint8_t cont = 0; 
					for(int tries = 0; tries < RFNET_READ_TIMEOUT; tries++){
						if(rfnet_recv_packet(net, &net->root_dev, &pack) && 
							pack.type & PTF_ACK){
							cont = 1; 
							break; 
						}
						_delay_ms(1); 
					}
					if(!cont) return RFNET_ERR_TIMEOUT; 
				}
			} 
		}
	}
	return 1; 
}


