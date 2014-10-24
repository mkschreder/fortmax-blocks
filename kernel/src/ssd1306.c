/*******
 * I2C asynchronous driver for ssd1306 series 128x64 point displays
 *
 * License: GPLv3
 * Author: Martin K. Schröder
 * Email: info@fortmax.se
 * Website: http://oskit.se
 */

#ifdef __AVR__
#include <avr/io.h>
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif

#include <stdlib.h>

#include <kernel/device.h>
#include <kernel/i2c.h>
#include <kernel/ssd1306.h>
#include <kernel/ssd1306_priv.h>
#include <kernel/uart.h>

#include <string.h>

#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ABS(x) ((x >= 0) ? x : -x)

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH  128
#define FRAGMENT_WIDTH 128
#define FRAGMENT_HEIGHT 8

typedef struct
{
  long x, y;
  unsigned char color;
} Point2D;

Point2D p0, p1, p2, p00, p01, p02;
long contourx[FRAGMENT_HEIGHT][2];

// Scans a side of a triangle setting min X and max X in ContourX[][]
// (using the Bresenham's line drawing algorithm).
void sc_ScanLine(long frag_top, long frag_bottom, long x1, long y1, long x2, long y2)
{
  long sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

  sx = x2 - x1;
  sy = y2 - y1;

  if (sx > 0) dx1 = 1;
  else if (sx < 0) dx1 = -1;
  else dy1 = 0;

  if (sy > 0) dy1 = 1;
  else if (sy < 0) dy1 = -1;
  else dy1 = 0;

  m = ABS(sx);
  n = ABS(sy);
  dx2 = dx1;
  dy2 = 0;

  if (m < n)
  {
    m = ABS(sy);
    n = ABS(sx);
    dx2 = 0;
    dy2 = dy1;
  }

  x = x1; y = y1;
  cnt = m + 1;
  k = n / 2;

  while (cnt--)
  {
    if ((y >= frag_top) && (y < frag_bottom))
    {
			long line = y - frag_top; 
      if (x < contourx[line][0]) contourx[line][0] = x;
      if (x > contourx[line][1]) contourx[line][1] = x;
    }

    k += n;
    if (k < m)
    {
      x += dx2;
      y += dy2;
    }
    else
    {
      k -= m;
      x += dx1;
      y += dy1;
    }
  }
}

void __drawGraphics(void *ptr)
{
	for(int frag_top = 0; frag_top < SCREEN_HEIGHT; frag_top += FRAGMENT_HEIGHT){
		for(int frag_left = 0; frag_left < SCREEN_WIDTH; frag_left += FRAGMENT_WIDTH){
			long frag_bottom = frag_top + 8;

			for (int y = 0; y < FRAGMENT_HEIGHT; y++)
			{
				contourx[y][0] = LONG_MAX; // min X
				contourx[y][1] = LONG_MIN; // max X
			}
			
			sc_ScanLine(frag_top, frag_bottom, p0.x, p0.y, p1.x, p1.y);
			sc_ScanLine(frag_top, frag_bottom, p1.x, p1.y, p2.x, p2.y);
			sc_ScanLine(frag_top, frag_bottom, p2.x, p2.y, p0.x, p0.y);

			for(int col = 0; col < FRAGMENT_WIDTH; col++){
				unsigned char byte = 0;
				// construct the vertical column byte for the fragment
				for(int line = 0; line < FRAGMENT_HEIGHT; line++){
					// pixel is set if the scan line interval exists
					if(contourx[line][1] >= contourx[line][0] &&
						(frag_left + col) >= contourx[line][0] &&
						(frag_left + col) < contourx[line][1]){
						byte |= 1;
						//screen[frag_top + line][frag_left + col] = '#'; 
					}
					byte = byte << 1; // shift left
				}
				//*data++ = byte; 
				//frag.data[col] = byte; 
			}
		}
	}
}

// initialize the triangle
int sc_Init(void)
{
	//srand(4567);
	long cx = 64, cy = 32;
	
	p00.x = -16; p00.y = -16;
	p01.x = 0; p01.y = 32;
	p02.x = 16; p02.y = -16;
	
	p0.x = cx - 16; //rand() % SCREEN_WIDTH;
	p0.y = cy;//rand() % SCREEN_HEIGHT;

	p1.x = cx; //rand() % SCREEN_WIDTH;
	p1.y = cy - 16; //rand() % SCREEN_HEIGHT;

	p2.x = cx + 16; //rand() % SCREEN_WIDTH;
	p2.y = cy; //rand() % SCREEN_HEIGHT;

  return 0;
}

#define DISPLAY_ADDRESS 0x78

#define TEXT_W (21)
#define TEXT_H (8)
#define TEXTSZ (TEXT_W * TEXT_H)
#define TEXTPACKETSZ (8)
#define TEXTCHAR_W (5)
#define PACKETSZ (TEXTPACKETSZ * 6 + 1) // 8 chars * 6 bytes each + 1 command byte
#define debug(msg) {} //uart_printf("SSD1306: %s\n", msg);


enum {
	SSD_IDLE, 
	SSD_TX_INIT,
	SSD_TX_START,
	SSD_TX_DATA
};

// types of requests:
// init: one long command request
// scroll: one small command request
// data: one command, then data request
// text: 
typedef struct ssd1306_device {
	struct {
		uint8_t in_use : 1; 
		uint8_t busy : 1;
		uint8_t need_sync : 1;
	} flags;

	uint8_t front[TEXTSZ];
	uint8_t back[TEXTSZ];
	//uint8_t _commands[32]; // command buffer

	struct i2c_command i2c_request;
	uint8_t _buffer[FRAGMENT_WIDTH + 1]; // PACKETSZ
	
	uint8_t state;
	const uint8_t *cur_buf;
	long frag_left, frag_top; 
	uint16_t cur_idx;
	uint16_t cur_length;
	uint8_t cur_char; // buffering for current char
	
} ssd1306_device_t;

static ssd1306_device_t _device[1];
static handle_t i2c = 0;

static void __update(void *arg);

handle_t ssd1306_open(id_t id){
	ssd1306_device_t *dev = &_device[0];
	if(dev->flags.in_use) return INVALID_HANDLE;
	dev->flags.in_use = 1;
	dev->flags.busy = 0;

	dev->cur_idx = 0;
	dev->state = SSD_TX_INIT;
	dev->cur_buf = cmd_init;
	dev->cur_length = sizeof(cmd_init);

	debug("OPEN");
	sc_Init();
	
	__update(dev); 
	
	return dev;
}

static uint16_t _frame_count = 0; 
uint16_t ssd1306_get_frame_count(){
	return _frame_count;
}

static void __update(void *arg) {
	//DDRD |= _BV(4);
	//PORTD |= _BV(4);
	
	ssd1306_device_t *dev = (ssd1306_device_t*)arg;

	uint8_t tx_count = 2; 
	switch(dev->state){
		case SSD_TX_START:
			debug("START");
			dev->_buffer[0] = 0x00; // command
			dev->_buffer[1] = pgm_read_byte(&dev->cur_buf[dev->cur_idx++]);
			tx_count = 2; 
			break; 
		case SSD_TX_INIT:
			debug("INIT"); 
			dev->_buffer[0] = 0x00; // command
			dev->_buffer[1] = pgm_read_byte(&dev->cur_buf[dev->cur_idx++]);
			tx_count = 2; 
			break; 
		case SSD_TX_DATA:
			//debug("DATA");
			// fill out packet with graphical font data
			{
				/*tx_count = PACKETSZ;
				dev->_buffer[0] = 0x40;
				// fill rest of the buffer with data for current fragment
				for(int c = 1; c < tx_count; c++)
				{
					if(dev->cur_idx == 0) dev->cur_char = dev->cur_buf[0];
					uint8_t char_col = dev->cur_idx & 0x07;
					
					if(char_col < TEXTCHAR_W){ // next four cols are just font stuff
						dev->_buffer[c] = pgm_read_byte(&font[dev->cur_char * 5 + (char_col)]);
						dev->cur_idx++;
					} else if(char_col == TEXTCHAR_W){ // last one is letter separator
						dev->_buffer[c] = 0;
						dev->cur_idx &= ~0x07; // next char;
						dev->cur_idx += 8;
						dev->cur_char = dev->cur_buf[dev->cur_idx >> 3];
					}
				}*/
			}
			// fill buffer with graphical data
			{
				tx_count = FRAGMENT_WIDTH + 1;
				dev->_buffer[0] = 0x40;
				
				long frag_bottom = dev->frag_top + 8;
				
				for (int y = 0; y < FRAGMENT_HEIGHT; y++)
				{
					contourx[y][0] = LONG_MAX; // min X
					contourx[y][1] = LONG_MIN; // max X
				}
				
				sc_ScanLine(dev->frag_top, frag_bottom, p0.x, p0.y, p1.x, p1.y);
				sc_ScanLine(dev->frag_top, frag_bottom, p1.x, p1.y, p2.x, p2.y);
				sc_ScanLine(dev->frag_top, frag_bottom, p2.x, p2.y, p0.x, p0.y);

				unsigned char *data = &dev->_buffer[1]; 
				for(int col = 0; col < FRAGMENT_WIDTH; col++){
					unsigned char byte = 0;
					// construct the vertical column byte for the fragment
					for(int line = 0; line < FRAGMENT_HEIGHT; line++){
						// pixel is set if the scan line interval exists
						long x = dev->frag_left + col; 
						if(contourx[line][1] >= contourx[line][0] &&
							x >= contourx[line][0] &&
							x < contourx[line][1]){
							byte |= (1 << line);
							//screen[frag_top + line][frag_left + col] = '#'; 
						}
						//byte = byte << 1; // shift left
					}
					*data++ = byte; 
				}
				dev->frag_left += FRAGMENT_WIDTH;
				if(dev->frag_left > (SCREEN_WIDTH - FRAGMENT_WIDTH)){
					dev->frag_left = 0;
					dev->frag_top += FRAGMENT_HEIGHT;
				}
				dev->cur_idx += (FRAGMENT_HEIGHT * FRAGMENT_WIDTH); 
			}
			break; 
	};

	dev->i2c_request = (i2c_command_t){
		.addr = DISPLAY_ADDRESS,
		.buf = dev->_buffer,
		.wcount = tx_count,
		.rcount = 0,
		.callback = __update,
		.arg = dev
	};

	// update state
	if(dev->cur_idx >= dev->cur_length){
		dev->cur_idx = 0;
		
		switch(dev->state){
			case SSD_TX_START:
				debug("START -> DATA"); 
				dev->state = SSD_TX_DATA;
				// swap buffers
				memcpy(dev->front, dev->back, sizeof(dev->front));

				// prepare for text transfer
				{
					dev->cur_buf = dev->front; 
					dev->cur_length = TEXT_H * TEXT_W * 8;
				}
				// prapare for graphical transfer
				{
					static float angle = 0;
					static float angle2 = 0;
					static float mov_l = 0, mov_u = 0; 
					angle += 0.05;
					angle2 += 0.05;
					mov_l += 0.1;
					mov_u += 0.2; 
					float scale = 1.2 + sin(angle2);
					
					long cx = 64 + 40 * sin(mov_l), cy = 32 + 20 * sin(mov_u);
					p0.x = cx + scale * ((p00.x * cos(angle)) - (p00.y * sin(angle)));
					p0.y = cy + scale * ((p00.y * cos(angle)) + (p00.x * sin(angle))); 
					p1.x = cx + scale * ((p01.x * cos(angle)) - (p01.y * sin(angle)));
					p1.y = cy + scale * ((p01.y * cos(angle)) + (p01.x * sin(angle))); 
					p2.x = cx + scale * ((p02.x * cos(angle)) - (p02.y * sin(angle)));
					p2.y = cy + scale * ((p02.y * cos(angle)) + (p02.x * sin(angle))); 
					dev->frag_left = 0;
					dev->frag_top = 0; 
					//dev->cur_buf = dev->frag;
					dev->cur_length = SCREEN_WIDTH * SCREEN_HEIGHT;
				}
				_frame_count++; 
				break; 
			case SSD_TX_INIT:
			case SSD_TX_DATA:
				debug("INIT|DATA -> START"); 
				dev->state = SSD_TX_START;
				dev->cur_buf = cmd_home;
				dev->cur_length = sizeof(cmd_home);
				break; 
		};
	}
	
	i2c_transfer(i2c, &dev->i2c_request);
}

int16_t ssd1306_xy_printf(handle_t h, uint8_t x, uint8_t y, const char *fmt, ...){
	uint16_t n; 
	va_list vl; 
	va_start(vl, fmt);
	struct ssd1306_device *dev = (struct ssd1306_device*)h;
	uint8_t start = y * TEXT_W + x;
	
	n = vsnprintf((char*)(dev->back + start), TEXTSZ-start, fmt, vl);
	
	va_end(vl);

	dev->flags.need_sync = 1; 
	return n; 
}

int16_t ssd1306_xy_puts(handle_t h,
	uint8_t x, uint8_t y, const char *str){
	struct ssd1306_device *dev = (struct ssd1306_device*)h;

	ssd1306_xy_printf(dev, x, y, str); 

	dev->flags.need_sync = 1; 
	return 1; 
}

void ssd1306_reset(handle_t h){
	struct ssd1306_device *dev = (struct ssd1306_device*)h;
	
	for(int c = 0; c < TEXTSZ; c++){
		dev->back[c] = ' ';
	}
}

CONSTRUCTOR(ssd1306_setup){
	i2c = i2c_open(0);
}

