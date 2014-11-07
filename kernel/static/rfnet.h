#pragma once

#include <avr/io.h>
#include "crypto/aes/aes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
  #define STATIC_ASSERT(e,m) \
    { enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(!!(e)) }; }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
  #define STATIC_ASSERT(e,m) \
    { enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }; }
#endif

typedef unsigned char rfnet_mac_t[5]; 
typedef unsigned char rfnet_key_t[32]; 

struct rfnet_device {
	/*struct {
		uint8_t busy : 1; // operation in progress
		uint8_t read : 1; // read in progress
		uint8_t write : 1; // write in progress
	} flags;*/
	aes256_ctx_t ctx;
	uint16_t nonce; 
}; 

struct rfnet {
	// two communication interfaces one for admin and other readonly (user)
	struct rfnet_device root_dev; 
	//struct rfnet_device user_dev; 
	//aes256_ctx_t root_ctx;
	//aes256_ctx_t user_ctx; 
	uint8_t (*get_field)(uint8_t field_id, uint8_t **data, uint8_t *size); 
	struct {
		uint32_t sent; 
		uint32_t received; 
		uint32_t decrypted; 
		uint32_t send_failed; 
		uint32_t dropped; 
	} stats; 
};

#define RFNET_ERR_READ_REQ_SEND_FAIL 	-1
#define RFNET_ERR_TIMEOUT 						-2
#define RFNET_ERR_ALL_NOT_READ 				-3
#define RFNET_ERR_BUFFER_SEND_FAILED  -4
#define RFNET_ERR_NONCE_MISMATCH 			-5
#define RFNET_ERR_INVALID_PACKET			-6
#define RFNET_ERR_CONNECT_TIMEOUT 		-7

void rfnet_init(struct rfnet *net, const char *root_pw, 
	volatile uint8_t *outport, volatile uint8_t *ddrport, 
	uint8_t cepin, uint8_t cspin); 
void rfnet_dev_init(struct rfnet_device *dev, const char *pw); 
int8_t rfnet_sync_read(struct rfnet *net, struct rfnet_device *dev, 
	uint8_t field_id, uint8_t *outbuf, int16_t size); 
int8_t rfnet_process_packets(struct rfnet *net); 

/*
void packet_init(struct packet *pack);
void packet_encrypt(aes256_ctx_t *ctx, struct packet *pack, encrypted_packet_t *ep);
uint8_t packet_decrypt(aes256_ctx_t *ctx, encrypted_packet_t *ep, struct packet *pack);
void packet_read_hs_req(struct packet *p, uint16_t *challenge);
void packet_read_hs_resp(struct packet *p, uint16_t *resp);
void packet_prep_hs_resp(struct packet *p, uint16_t response);
*/
//void fx_random(char *buf, uint16_t size); 
//void enterSleep(void);


#ifdef __cplusplus
}
#endif
