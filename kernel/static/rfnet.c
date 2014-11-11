#include <avr/sleep.h>
#include <avr/power.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "crypto/sha256.h"
//#include "random.h"
#include "nrf24l01.h"
#include "rfnet.h"
#include "time.h"
#include "uart.h"

/** Half duplex comm

MASTERSTART - send first data packet to slave
SLAVEACK		- return data to master and ack previous packet
MASTERACK 	- send continuation data to slave
MASTERFIN		- signal slave that this is the last packet
SLAVEFIN		- signal master that we have also closed the connection

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
#define RFNET_MAX_PIPES 1
//#define RFNET_READ_TIMEOUT 200
#define RFNET_RETRY_COUNT 		20
#define RFNET_RETRY_TIMEOUT 	20000


// packet flags
#define PTF_SRT 0x10
#define PTF_ACK 0x20
#define PTF_FIN 0x40
#define PTF_RST 0x80

struct __attribute__((__packed__)) packet {
	uint8_t type; 				// packet type
	rfnet_mac_t from_addr; // from which device is this packet?
	// sequence number is automatically incremented at slave and sent back
	// to master with every update
	uint16_t sequence_number; 
	uint8_t data_size; // number of bytes in data that are valid data
	uint8_t data[RFNET_PACKET_DATA_SIZE];
	uint16_t checksum; // crc16 of the data
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
	STATIC_ASSERT(sizeof(struct packet) == NRF24L01_PAYLOAD, "Packet size must be same as nrf payload size!");
	STATIC_ASSERT(sizeof(struct packet) == sizeof(encrypted_packet_t), "Packet size must match aes encryption buffer size of 16 bytes");
	/*STATIC_ASSERT((((uint8_t*)&p.sum) - ((uint8_t*)&p)) !=
		(sizeof(struct packet) - sizeof(p.sum)),
			"Checksum field must be placed at the end of packet struct!");*/
	STATIC_ASSERT(sizeof(struct packet) >= NRF24L01_PAYLOAD, "Transport layer block size must be same as packet size!"); 
}

static void packet_build(struct packet *pack, 
	rfnet_mac_t from, 
	uint8_t type, 
	uint8_t *data, 
	uint8_t size, 
	uint16_t nonce){
	memset(pack, 0, sizeof(struct packet));
	memcpy(pack->from_addr, from, sizeof(rfnet_mac_t)); 
	pack->type = type; 
	pack->data_size = (size > RFNET_PACKET_DATA_SIZE)?RFNET_PACKET_DATA_SIZE:size;
	if(data) memcpy(pack->data, data, pack->data_size); 
	pack->sequence_number = nonce; 
}

static void packet_encrypt(aes256_ctx_t *ctx, struct packet *pack, encrypted_packet_t *ep){
	// checksum is done on whole packet
	pack->checksum = gen_crc16((uint8_t*)pack, sizeof(struct packet) - 
		sizeof(pack->checksum));
	memcpy(ep, pack, sizeof(struct packet));
	// encrypt the whole packet 
	// packets must always be sent fully encrypted
	aes256_enc(ep, ctx);
	aes256_enc(((uint8_t*)ep) + 16, ctx);
}

static uint8_t packet_decrypt(aes256_ctx_t *ctx, encrypted_packet_t *ep, struct packet *pack){
	struct packet pp; 
	struct packet *p = &pp; 
	// dec() is destructive so we need to create local copy
	memcpy(p, ep, sizeof(encrypted_packet_t)); 
	aes256_dec(p, ctx);
	aes256_dec(((uint8_t*)p) + 16, ctx);
	uint8_t valid = 0; 
	if(p->checksum == gen_crc16((uint8_t*)p, sizeof(struct packet) - sizeof(p->checksum)))
		valid = 1; 
	if(valid) memcpy((char*)pack, p, sizeof(struct packet));
	return valid; 
}

#define TX_PIPE 0
#define RX_PIPE 1

typedef uint16_t nonce_t; 

typedef enum {
	EV_FAIL, 
	EV_ENTER, 
	EV_LEAVE, 
	EV_PUT_DATA, 
	EV_GOT_DATA, 
	EV_GOT_ACK, 
	EV_RETRIES_EXPENDED, 
	EV_TIMEOUT
} con_event_t; 

#define CON_STATE(NAME, CON, EV, DATA, SIZE) \
	static uint8_t NAME(struct rfnet_connection *CON, \
	con_event_t EV, uint8_t *DATA, uint8_t SIZE)

struct rfnet_connection; 
typedef uint8_t (*rfnet_state_proc_t)(
	struct rfnet_connection *con, con_event_t ev,
	uint8_t *data, uint8_t size);

struct rfnet_connection {
	rfnet_mac_t 				tx_addr; 
	rfnet_mac_t 				rx_addr;
	aes256_ctx_t 				*key; 
	aes256_ctx_t 				tx_key; 
	aes256_ctx_t 				rx_key; 	// the key of the other device
	encrypted_packet_t	tx_buffer; // buffer for outgoing data
	rfnet_state_proc_t 	state; // current state of connection
	timeout_t 					timeout; // time of the next timeout (used by state)
	uint8_t							retries; 
	nonce_t 						sequence_number; 
	//rfnet_data_buffer_ready_proc_t 	cb_tx_buffer_empty; 
	rfnet_request_handler_proc_t 	cb_handle_request; 
	rfnet_response_handler_proc_t cb_handle_response; 
	rfnet_error_proc_t 	cb_error; 
	/*rfnet_mac_t 		addr; // address of the other device
	rfnet_mac_t 		to; 
	timeout_t 			timeout; // time of the next timeout (used by state)
	uint16_t				timeout_interval;  			
	uint8_t					retries; 
	nonce_t 				nonce; 
	encrypted_packet_t	buffer; // buffer for outgoing data
	rfnet_state_proc_t state; // current state of connection
	//rfnet_data_buffer_ready_proc_t 	cb_tx_buffer_empty; 
	rfnet_rx_data_ready_proc_t 	cb_rx_buffer_ready; 
	rfnet_error_proc_t 	cb_error; */
}; 

// device configuration that is maintained for each device on hub side
struct rfnet_device_config {
	uint8_t 				valid; 
	rfnet_mac_t 		device_addr; 
	rfnet_pw_t 			passwd; 
}; 

struct rfnet {
	struct rfnet_connection connection;
}; 

static struct rfnet _net; 

static const char *err_string(rfnet_error_t err){
	static const char *errors[] = {
		"INFO", 
		"RFNET_ERR_READ_REQ_SEND_FAIL",  
		"RFNET_ERR_TIMEOUT", 
		"RFNET_ERR_ALL_NOT_READ", 
		"RFNET_ERR_BUFFER_SEND_FAILED", 
		"RFNET_ERR_NONCE_MISMATCH", 
		"RFNET_ERR_INVALID_PACKET", 
		"RFNET_ERR_CONNECT_TIMEOUT"
	}; 
	uint8_t id = (err * -1) % 8; 
	return errors[id]; 
}

static void con_error(struct rfnet_connection *con, rfnet_error_t err){
	if(con->cb_error){
		con->cb_error(err, err_string(err)); 
	}
}

static void con_debug(struct rfnet_connection *con, const char *str){
	if(con->cb_error){
		con->cb_error(0, str); 
	}
}

CON_STATE(ST_IDLE, 				con, ev, data, size); 
CON_STATE(ST_SEND_DATA, 	con, ev, data, size); 

static uint8_t _default_handle_response(const uint8_t *data, uint8_t size){
	return 0; 
}

static void con_init(
		struct rfnet_connection *con, 
		rfnet_mac_t addr, 
		const char *pw, 
		rfnet_request_handler_proc_t cb_data_ready, 
		rfnet_error_proc_t cb_error){
	sha256_hash_t hash; 
	sha256(&hash, pw, strlen(pw)); 
	
	aes256_init(hash, &con->rx_key); 
	memcpy(&con->tx_key, &con->rx_key, sizeof(con->tx_key)); 
	con->key = &con->rx_key; 
	memcpy(con->rx_addr, addr, sizeof(rfnet_mac_t)); 
	memcpy(con->tx_addr, addr, sizeof(rfnet_mac_t)); 
	
	con->timeout = 0; 
	//con->timeout_interval = 0; 
	con->retries = 0; 
	con->state = ST_IDLE; 
	
	//con->cb_tx_buffer_empty = 0; 
	con->cb_handle_request = cb_data_ready; 
	con->cb_handle_response = _default_handle_response; 
	con->cb_error = cb_error; 
}


static void con_connect(
	struct rfnet_connection *con, 
	rfnet_mac_t addr, 
	const char *pw, 
	rfnet_response_handler_proc_t cb_resp
){
	sha256_hash_t hash; 
	sha256(&hash, pw, strlen(pw)); 
	
	aes256_init(hash, &con->tx_key); 
	memcpy(con->tx_addr, addr, sizeof(rfnet_mac_t)); 
	
	con->cb_handle_response = cb_resp; 
}

static void con_process_events(struct rfnet_connection *con){
	if(!con->state) return; 
	if(con->retries > 0 && timeout_expired(con->timeout)){
		//rfnet_mac_t from; 
		con->state(con, EV_TIMEOUT, 0, 0); 
		con->timeout = timeout_from_now(RFNET_RETRY_TIMEOUT); 
		con->retries--; 
		if(con->retries == 0)
			con->state(con, EV_RETRIES_EXPENDED, 0, 0); 
	}
}
static void con_enter_state(
		struct rfnet_connection *con, 
		rfnet_state_proc_t state, 
		uint8_t *data, 
		uint8_t size){
	//rfnet_mac_t from; 
	if(!con->state) return; 
	con->state(con, EV_LEAVE, 0, 0); 
	con->retries = 0; 
	con->timeout = 0; 
	con->state = state; 
	con->state(con, EV_ENTER, data, size); 
}

static void con_set_retries(struct rfnet_connection *con, uint8_t retries, uint16_t interval){
	con->timeout = timeout_from_now(interval); 
	con->retries = retries;
}
/*
static void _con_send_buffer(struct rfnet_connection *con){
	//memcpy(con->to, to, sizeof(rfnet_mac_t)); 
	
	//for(int c = 0; c < 5; c++) uart_printf("%02x", to[c]); 
	//uart_printf("\n"); 
	
	nrf24l01_settxaddr(con->addr); 
	// try to send (if this step fails then packet did not arrive!)
	if(nrf24l01_write((uint8_t*)&con->buffer) == 2){
		// without auto ack enabled, we never get here
		con_debug(con, "MAXRT"); 
	}
}
*/
CON_STATE(ST_IDLE, con, ev, data, size){
	switch(ev){
		// enter/leave state
		case EV_LEAVE: 
		case EV_ENTER: {
			break; 
		}
		// we want to write data to the connection
		case EV_PUT_DATA: { // user code writes packet to the socket
			// switch to send start state and pass the incoming data to it
			con_debug(con, "IDL:DATA"); 
			
			// send start to the address specified by this connection
			con_enter_state(con, ST_SEND_DATA, data, size); 
			
			break; 
		}
		case EV_GOT_DATA: { 
			// we got data from the other end of rf connection
			//con_debug(con, "IDL:START"); 
			
			// invoke callback that will fill output buffer which we can send back
			uint8_t outdata[RFNET_PACKET_DATA_SIZE]; 
			uint8_t outsize = 0; 
			if(con->cb_handle_request)
				outsize = con->cb_handle_request(data, size, 
					outdata, RFNET_PACKET_DATA_SIZE); 
			
			{ // SEND RESPONSE 
				struct packet pack; 
				
				// build packet from us addressed to the same address as us
				packet_build(&pack, 
					con->rx_addr, 
					PTF_ACK, 
					outdata, 
					outsize, 
					con->sequence_number + 1); 
					
				packet_encrypt(&con->rx_key, &pack, &con->tx_buffer); 
				
				nrf24l01_settxaddr(con->rx_addr); 
				nrf24l01_write((uint8_t*)&con->tx_buffer); 
				//uart_printf("sent to: "); 
				//for(int c = 0; c < 5; c++) uart_printf("%02x", con->rx_addr[c]); 
				
				
			}
			// we always send response on the same address
			//_con_send_buffer(con); 
			
			// we always only respond once. If packet gets lost another start 
			// will be sent and we can try resending. 
			break; 
		}
		default: { 
			//con_error(con, RFNET_ERR_INVALID_EVENT); 
			//return 0; 
			break; 
		}
	}
	return 1; 
}

/// state that sends out a data packet and retries to send it a few times
CON_STATE(ST_SEND_DATA, con, ev, data, size){
	switch(ev){
		case EV_ENTER: {
			con_debug(con, "SRT:ENTER"); 
			struct packet pack; 
			
			packet_build(&pack, 
				con->rx_addr, 
				PTF_SRT, 
				data, 
				size, 
				con->sequence_number
			); 
			
			con->key = &con->tx_key; 
			packet_encrypt(&con->tx_key, &pack, &con->tx_buffer); 
			
			nrf24l01_settxaddr(con->tx_addr); 
			nrf24l01_write((uint8_t*)&con->tx_buffer); 
			// send the buffer to address of selected connection 
			//_con_send_buffer(con); 
			con_set_retries(con, RFNET_RETRY_COUNT, RFNET_RETRY_TIMEOUT); 
			
			break; 
		}
		case EV_LEAVE: {
			con_debug(con, "SRT:LV"); 
			con->key = &con->rx_key; 
			nrf24l01_settxaddr(con->rx_addr); 
			break; 
		}
		case EV_GOT_ACK: { 
			// the other side has sent us valid data response
			// notify the user of the data we have received and ask for more
			con_debug(con, "SRT:ACK"); 
			//uint8_t outsize = 0; 
			if(con->cb_handle_response){
				// send rx buffer ready but no data can be sent back
				con->cb_handle_response(data, size); 
			}
			// and return to idle state
			con_enter_state(con, ST_IDLE, data, size); 
			break; 
		}
		case EV_TIMEOUT: { // the timeout timer has timedout
			//PORTD &= ~_BV(4); 
			//con_debug(con, "SRT:TOUT"); 
			// resend the encrypted packet
			
			//nrf24l01_settxaddr(con->tx_addr); 
			nrf24l01_write((uint8_t*)&con->tx_buffer); 
			break; 
		}
		case EV_RETRIES_EXPENDED: { // all retries have not resulted in ack!
			//con_debug(con, "START:RETRYZERO"); 
			con_error(con, RFNET_ERR_TIMEOUT); 
			con_enter_state(con, ST_IDLE, 0, 0); 
			break; 
		}
		default: {
			//con_error(con, RFNET_ERR_INVALID_EVENT); 
			// all other events are ignored
		}
	}
	return 1; 
}

void rfnet_init(
		rfnet_mac_t addr, // listen address
		const char *root_pw, // password for this device
		volatile uint8_t *outport, volatile uint8_t *ddrport, uint8_t cepin, uint8_t cspin, 
		rfnet_request_handler_proc_t data_proc, 
		rfnet_error_proc_t cb_error){
	
	struct rfnet *net = &_net; 
	
	// to avoid compiler warning
	___validate_packet_sizes___(); 
	
	// initialize the radio
	nrf24l01_init(outport, ddrport, cepin, cspin); 
	
	// read configuration from eeprom
	/*
	struct rfnet_device_config config; 
	eeprom_read_block(&config, 0, sizeof(struct rfnet_device_config)); 
	
	eeprom_busy_wait(); 
	uint8_t *d = (uint8_t*)&config; 
	for(int c = 0; c < sizeof(config); c++){
		uart_printf("0x%02x", d[c]); 
	}
	uart_printf("\n"); 
	
	config.valid = 1; 
	memcpy(config.device_addr, addr, sizeof(rfnet_mac_t)); 
	snprintf(config.passwd, sizeof(rfnet_pw_t), "%s", "ThePassWord!"); 
	eeprom_update_block(&config, 0, sizeof(config)); 
	eeprom_busy_wait(); 
	*/
	// setup a connection for listening on the wire
	con_init(&net->connection, addr, root_pw, data_proc, cb_error); 
	
	// pipe 1 is always active as receive pipe
	
	nrf24l01_setrxaddr(0, addr); 
	
	time_init(); 
	
	//net->stats.sent = net->stats.received = net->stats.decrypted = 0; 
	
	//nrf24l01_init(&PORTC, &DDRC, PC2, PC3); 
}


void rfnet_process_events(void){
	// check for any received packets
	uint8_t pipe = 0xff; 
	uint8_t max_packets = 10; 
	
	//poll radio for incoming packets
	for(;;){
		if(nrf24l01_readready(&pipe) && pipe < 7 && --max_packets) { 
			encrypted_packet_t ep; 
			struct packet pack; 
			
			//read buffer
			nrf24l01_read((uint8_t*)&ep);
			
			//net->stats.received++; 
			
			// determine which connection the packet belongs to
			struct rfnet_connection *con = &_net.connection; 
			
			// try decrypting the packet using the connection key
			if(!packet_decrypt(con->key, &ep, &pack)){
				con_debug(&_net.connection, "DECRFAIL0"); 
				continue; 
			}
			
			// this is where we need to test nonce
			// TODO 
			
			// generate appropriate event for the connection
			if(pack.type & PTF_SRT){
				// got incoming data packet
				//con->nonce = pack.nonce + 1; 
				con->state(con, EV_GOT_DATA, pack.data, pack.data_size); 
			}
			else if(pack.type & PTF_ACK){
				// got data response packet
				//con->nonce = pack.nonce + 1; 
				con->state(con, EV_GOT_ACK, pack.data, pack.data_size); 
			}
			else {
				con_debug(&_net.connection, "INVPACK"); 
			}
		//} else {
		}
		break; 
	}
	// process connection events
	con_process_events(&_net.connection); 
}

// writes a block to the connection of size RFNET_PACKET_DATA_SIZE
uint8_t rfnet_send(uint8_t *data, uint8_t size){
	struct rfnet_connection *con = &_net.connection; 
	
	// check so that the connection is ready to accept data
	if(!con->state || con->state != ST_IDLE) return 0; 
	
	uint8_t s = (size > RFNET_PACKET_DATA_SIZE)?RFNET_PACKET_DATA_SIZE:size; 
	if(con->state(con, EV_PUT_DATA, data, s)){
		return s; 
	} 
	return 0; 
}

/// adds a new connection to list of connections
uint8_t rfnet_connect(
		rfnet_mac_t addr,  // address of the other device
		const char *pw, 		// password for the other device
		rfnet_response_handler_proc_t data_handler){

	con_connect(&_net.connection, addr, pw, data_handler); 
	
	return 1; 
}

