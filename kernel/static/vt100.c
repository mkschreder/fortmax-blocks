/**
	This file is part of FORTMAX kernel.

	FORTMAX kernel is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	FORTMAX kernel is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FORTMAX kernel.  If not, see <http://www.gnu.org/licenses/>.

	Copyright: Martin K. Schröder (info@fortmax.se) 2014
*/

#include <avr/io.h>
#include <ctype.h>
#include "vt100.h"
#include "ili9340.h"

#define VT100_SCREEN_WIDTH 240
#define VT100_SCREEN_HEIGHT 320
#define VT100_CHAR_WIDTH 6
#define VT100_CHAR_HEIGHT 8

#define STATE(NAME, TERM, EV, ARG) void NAME(struct vt100 *TERM, uint8_t EV, uint16_t ARG)


// states 
enum {
	STATE_IDLE,
	STATE_ESCAPE,
	STATE_COMMAND
};

// events that are passed into states
enum {
	EV_CHAR = 1,
};

#define MAX_COMMAND_ARGS 4
static struct vt100 {
	uint16_t screen_width, screen_height; 
	int16_t cursor_x, cursor_y;
	int16_t saved_cursor_x, saved_cursor_y; 
	int8_t char_width, char_height;
	uint16_t back_color, front_color;
	int16_t scroll; 
	// command arguments that get parsed as they appear in the terminal
	uint8_t narg; uint16_t args[MAX_COMMAND_ARGS];
	// current arg pointer (we use it for parsing) 
	uint8_t carg;
	
	void (*state)(struct vt100 *term, uint8_t ev, uint16_t arg); 
} term;

STATE(_st_idle, term, ev, arg);
STATE(_st_command, term, ev, arg);

void _vt100_reset(void){
	term.screen_width = 240;
  term.screen_height = 320;
  term.char_height = 8;
  term.char_width = 6;
  term.back_color = 0x0000;
  term.front_color = 0xffff;
  term.cursor_x = term.cursor_y = term.saved_cursor_x = term.saved_cursor_y = 0;
  term.narg = 0;
  term.scroll = 0; 
  term.state = _st_idle;
  ili9340_setFrontColor(term.front_color);
	ili9340_setBackColor(term.back_color);
}

void _vt100_moveCursor(struct vt100 *term, int16_t xx, int16_t yy){
	/*uint16_t x = term->cursor_x * term->char_width;
	uint16_t y = term->cursor_y * term->char_height;

	// fill current cursor with bg color
	ili9340_fillRect(x, y, term->char_width, term->char_height, term->back_color);
*/
	term->cursor_x = (xx < 0)?0:xx;
	term->cursor_y = (yy < 0)?0:yy;
/*
	x = term->cursor_x * term->char_width;
	y = term->cursor_y * term->char_height;

	ili9340_fillRect(x, y, term->char_width, term->char_height, term->front_color); */
}

void _vt100_drawCursor(struct vt100 *t){
	//uint16_t x = t->cursor_x * t->char_width;
	//uint16_t y = t->cursor_y * t->char_height;

	//ili9340_fillRect(x, y, t->char_width, t->char_height, t->front_color); 
}

void _vt100_scroll(struct vt100 *t, int16_t lines){
	t->scroll = (t->scroll + lines) % 320;
	
	ili9340_setScrollStart(t->scroll);

	uint16_t x = t->cursor_x * t->char_width;
	uint16_t y = t->cursor_y * t->char_height;
	
	// clear bottom line
	if(lines > 0) {
		ili9340_fillRect(x, y - lines, t->screen_width, lines, t->back_color);
	} else {
		ili9340_fillRect(x, 0, t->screen_width, lines, t->back_color);
	}
}

// sends the character to the display and updates cursor position
void _vt100_putc(struct vt100 *t, uint8_t ch){
	uint16_t x = t->cursor_x++ * t->char_width;
	uint16_t y = t->cursor_y * t->char_height;

	// automatically scroll the display
	if(y >= VT100_SCREEN_HEIGHT){
		_vt100_scroll(t, t->char_height);
		// update coordinate for next character to be rendered
		
		// go back one line so we are still on the bottom line of the display
		t->cursor_y--; y -= t->char_height;
	}

	// check if we have gone off the screen and automatically move the cursor
	// to next line. We also fill remaining pixels of the line with background color
	if(x + (t->char_width << 1) > t->screen_width){
		// fill rest of line
		ili9340_fillRect(x, y, t->screen_width - x, t->char_height, t->back_color); 
		t->cursor_x = 0; t->cursor_y++;
	}

	ili9340_setFrontColor(t->front_color);
	ili9340_setBackColor(t->back_color); 
	ili9340_drawChar(x, y, ch);

	_vt100_drawCursor(t); 
}

STATE(_st_command_arg, term, ev, arg){
	switch(ev){
		case EV_CHAR: {
			if(isdigit(arg)){ // a digit argument
				term->args[term->narg] = term->args[term->narg] * 10 + (arg - '0');
			} else if(arg == ';') { // separator
				term->narg++;
			} else { // no more arguments
				// go back to command state 
				term->narg++;
				term->state = _st_command;
				// execute command state as well to make sure we catch this char
				_st_command(term, ev, arg); 
			}
			break;
		}
	}
}

STATE(_st_command, term, ev, arg){
	switch(ev){
		case EV_CHAR: {
			if(isdigit(arg)){ // start of an argument
				_st_command_arg(term, ev, arg);
				term->state = _st_command_arg;
			} else if(arg == ';'){ // arg separator. 
				// skip
			} else {
				switch(arg){
					case 'f': 
					case 'H': { // move cursor to position (default 0;0)
						_vt100_moveCursor(term,
								(term->narg >= 1)?term->args[0]:0,
								(term->narg == 2)?term->args[1]:0); 
						break;
					}
					case 'm': { // sets colors. Accepts up to 3 args
						while(term->narg){
							term->narg--; 
							int n = term->args[term->narg];
							static const uint16_t colors[] = {
								0x0000, // black
								0xf800, // red
								0x0780, // green
								0xfe00, // yellow
								0x001f, // blue
								0xf81f, // magenta
								0x07ff, // cyan
								0xffff // white
							};
							if(n == 0){ // all attributes off
								term->front_color = 0xffff;
								term->back_color = 0x0000;
								
								ili9340_setFrontColor(term->front_color);
								ili9340_setBackColor(term->back_color);
							}
							if(n >= 30 && n < 38){ // fg colors
								term->front_color = colors[n-30]; 
								ili9340_setFrontColor(term->front_color);
							} else if(n >= 40 && n < 48){
								term->back_color = colors[n-40]; 
								ili9340_setBackColor(term->back_color); 
							}
						}
						break;
					}
					case 'A': {// move cursor up
						int n = (term->narg > 0)?term->args[term->narg - 1]:1;
						_vt100_moveCursor(term, term->cursor_x, term->cursor_y - n); 
						break;
					}
					case 'B': { // cursor down
						int n = (term->narg > 0)?term->args[term->narg - 1]:1;
						_vt100_moveCursor(term, term->cursor_x, term->cursor_y + n); 
						break;
					}
					case 'C': { // cursor down
						int n = (term->narg > 0)?term->args[term->narg - 1]:1;
						_vt100_moveCursor(term, term->cursor_x + n, term->cursor_y); 
						break;
					}
					case 'D': { // cursor down
						int n = (term->narg > 0)?term->args[term->narg - 1]:1;
						_vt100_moveCursor(term, term->cursor_x - n, term->cursor_y); 
						break;
					}
					case 's':{// save cursor pos
						term->saved_cursor_x = term->cursor_x;
						term->saved_cursor_y = term->cursor_y;
						break;
					}
					case 'u':{// restore cursor pos
						_vt100_moveCursor(term, term->saved_cursor_x, term->saved_cursor_y);
						break;
					}
					case 'K':{// clear line from cursor right/left
						uint16_t x = term->cursor_x * term->char_width;
						uint16_t y = term->cursor_y * term->char_height;
						if(term->narg == 0 || (term->narg == 1 && term->args[0] == 0)){
							ili9340_fillRect(x, y, term->screen_width - x, term->char_height, term->back_color);
						} else if(term->narg == 1 && term->args[0] == 1){
							ili9340_fillRect(0, y, x, term->char_height, term->back_color);
						} else if(term->narg == 1 && term->args[0] == 2){
							ili9340_fillRect(0, y, term->screen_width, term->char_height, term->back_color);
						}
						break;
					}
					case 'J':{// clear screen from cursor up or down
						uint16_t x = term->cursor_x * term->char_width;
						uint16_t y = term->cursor_y * term->char_height;
						if(term->narg == 0 || (term->narg == 1 && term->args[0] == 0)){
							ili9340_fillRect(0, y, term->screen_width, term->screen_height - y, term->back_color);
						} else if(term->narg == 1 && term->args[0] == 1){
							ili9340_fillRect(0, 0, term->screen_width, y, term->back_color);
						} else if(term->narg == 1 && term->args[0] == 2){
							ili9340_fillRect(0, 0, term->screen_width, term->screen_height, 0x0000);
							_vt100_moveCursor(term, 0, 0); 
						}
						break;
					}
					case 'c':{ // reset
						_vt100_reset();
						break; 
					}
					case '=':{ // argument follows... 
						//term->state = _st_screen_mode;
						break; 
					}
				}
				term->state = _st_idle; 
			} // else
			break;
		}
	}
}

STATE(_st_escape, term, ev, arg){
	switch(ev){
		case EV_CHAR: {
			switch(arg){
				case '[': { // command
					// prepare command state and switch to it
					term->narg = 0;
					for(int c = 0; c < MAX_COMMAND_ARGS; c++)
						term->args[c] = 0; // need to reset so we get correct reading!
					term->state = _st_command;
					break;
				}
				case 'D': { // scroll window up one line
					_vt100_scroll(term, term->char_height);
					break;
				}
				case 'M': { // scroll window down one line
					_vt100_scroll(term, -term->char_height);
					break;
				}
				case 'E': { // move to next line
					break;
				}
				case '7': { // save cursor vt100 code
					term->saved_cursor_x = term->cursor_x;
					term->saved_cursor_y = term->cursor_y;
					break;
				}
				case '8': { // restore cursor vt100 code
					_vt100_moveCursor(term, term->saved_cursor_x, term->saved_cursor_y);
					break;
				}
				default: { // unknown sequence - return to normal mode
					term->state = _st_idle;
					break;
				}
			}
			break;
		}
		default: {
			
		}
	}
}

STATE(_st_idle, term, ev, arg){
	switch(ev){
		case EV_CHAR: {
			switch(arg){
				case 0x1b: {// escape
					term->state = _st_escape;
					break;
				}
				case '\n': {
					_vt100_moveCursor(term, 0, term->cursor_y + 1); 
					break;
				}
				case '\r': {
					_vt100_moveCursor(term, 0, term->cursor_y); 
					break;
				}
				case '\b': {
					if(term->cursor_x == 0){
						if(term->cursor_y > 0) term->cursor_y--;
					} else {
						term->cursor_x--;
					}
					ili9340_drawChar(term->cursor_x * term->char_width,
						term->cursor_y * term->char_height,
						' ');
					break;
				}
				default: {
					_vt100_putc(term, arg);
					break;
				}
			}
			break;
		}
		default: {}
	}
}

void vt100_init(void){
	_vt100_reset(); 
}

void vt100_putc(uint8_t c){
	term.state(&term, EV_CHAR, 0x0000 | c);
}
