static void orgit() __naked {
  __asm
  org     0x140D
    nop
    nop
    nop
    __endasm;
}

void main2();

void main() {
  main2();
}

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <z80.h>
#include "tms9918.h"
#include "nabu.h"

#define MAX_ITERATION 50
#define X_RES 64.0
#define Y_RES 48.0

#define SPRITE_LARGE true
#define SPRITE_SMALL false

uint8_t cursor_sprite_small[] = {
	0xff,
	0x81,
	0x81,
	0x81,
	0x81,
	0x81,
	0x81,
	0xff
};


uint8_t cursor_sprite_large[32] = {
	0xff, 0xff,
       	0x80, 0x80, 
	0x80, 0x80,
       	0x80, 0x80, 

	0x80, 0x80,
       	0x80, 0x80, 
	0x80, 0x80,
       	0xff, 0xff, 
	
	0xff, 0xff,
       	0x01, 0x01, 
	0x01, 0x01,
       	0x01, 0x01, 

	0x01, 0x01,
       	0x01, 0x01, 
	0x01, 0x01,
       	0xff, 0xff, 
};

int16_t sprite_handle = 0;
int16_t cursor_xpos = 160 - 8;
int16_t cursor_ypos = 96 - 8;
int16_t cursor_xdir = 1;
int16_t cursor_ydir = 1;

// min and max values for real component of c
float cr_min = -2;
float cr_max = 2;

// min and max values for imaginary componet of c
float ci_min = -2;
float ci_max = 2;

int iterationCount(float cr, float ci, bool (*callback_func)(void)) {
	int i = 0;
	float zr=0, zi=0;
	float temp;
	bool keepgoing = true;

	while(keepgoing && (zr * zr + zi * zi < 4) && (i < MAX_ITERATION)) {
		if(callback_func != NULL) {
			keepgoing = callback_func();
		}
		temp = zr * zr - zi * zi + cr;
		zi = 2 * zr * zi + ci;
		zr = temp;
		i++;
	}

	return i;
}

bool handle_input(void) {

	if (cursor_xpos < 32) cursor_xdir = 1;
	if (cursor_xpos > 272) cursor_xdir = -1;
	cursor_xpos += cursor_xdir;
	
	if(cursor_ypos <= 0) cursor_ydir = 1;
	if(cursor_ypos > 160) cursor_ydir = -1;
	cursor_ypos += cursor_ydir;

	vdp_sprite_set_position(sprite_handle, cursor_xpos, cursor_ypos);

	return true;

}

int iterToColor(int i) {
	int color = 1; // BLACK by default
	if(i < MAX_ITERATION) {
		color = i % 14 + 2;  // range 2-15
	}
	return color;
}

float pixel_x_to_cr(int x, float cr_min, float cr_max) {
	float cr = x / X_RES * (cr_max - cr_min) + cr_min;
	return cr;
}

float pixel_y_to_ci(int y, float ci_min, float ci_max) {
	float ci = y / Y_RES * (ci_max - ci_min) + ci_min;
	return ci;
}


void main2() {

	vdp_init(VDP_MODE_MULTICOLOR, VDP_DARK_BLUE, SPRITE_LARGE, false);
	for(int i=0; i<256; i++) {
		vdp_set_sprite_pattern(i, cursor_sprite_large);
	}

	sprite_handle = vdp_sprite_init(0, 0, VDP_WHITE);
	vdp_sprite_set_position(sprite_handle, cursor_xpos, cursor_ypos);

	for (int y=0; y<48; y++) {
		float ci = pixel_y_to_ci(y, ci_min, ci_max);
		for (int x=0; x<64; x++) {
			vdp_plot_color(x, y, VDP_WHITE);
			float cr = pixel_x_to_cr(x, cr_min, cr_max);
			int i = iterationCount(cr, ci, &handle_input);
			int color = iterToColor(i);
			vdp_plot_color(x, y, color);
		}
	}
	while(true) {
		z80_delay_ms(1000);
	}
}
