// Mandelbrot.c - Render the Mandelbrot Set with a NABU PC in 64 x 48 multicolor mode
// Copyright Mike Debreceni 2022
// 
//
// Based on Hello World C example from NABU.ca Homebrew
// 
// https://nabu.ca/homebrew-c-tutorial
// https://cloud.nabu.ca/homebrew/Hello-World-C.zip


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

#define FONT_STANDARD
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <z80.h>
#include "nabu.h"
#include "tms9918.h"

#define MAX_ITERATION 50

#define X_RES_PIXELS 64.0
#define Y_RES_PIXELS 48.0

#define SPRITE_X_MAXPOS 256.0
#define SPRITE_Y_MAXPOS 192.0

#define SPRITE_LARGE true
#define SPRITE_SMALL false

uint8_t cursor_sprite_small[] = {
	0xff,
	0x81,
	0x81,
	0x81,
	0x81,
	0xff,
	0x00,
	0x00
};


uint8_t cursor_sprite_large[32] = {
	0xff, 0xff,
    0x80, 0x80, 
	0x80, 0x80,
    0x80, 0x80, 

	0x80, 0x80,
    0xff, 0xff, 
	0x00, 0x00,
	0x00, 0x00,

	0xff, 0xff,
    0x01, 0x01, 
	0x01, 0x01,
    0x01, 0x01, 
	0x01, 0x01,
    0xff, 0xff, 
	0x00, 0x00,
	0x00, 0x00,

};

int16_t sprite_handle = 0;
int16_t cursor_xpos = 160 - 8;
int16_t cursor_ypos = 96 - 6;
int16_t cursor_xdir = 1;
int16_t cursor_ydir = 1;

// min and max values for real component of c
float cr_min = -2;
float cr_max = 2;

// min and max values for imaginary componet of c
float ci_min = -2;
float ci_max = 2;

void centerCursor(void) {
    cursor_xpos = 160 - 8;
    cursor_ypos = 96 - 6;
    cursor_xdir = 1;
    cursor_ydir = 1;	
	vdp_sprite_set_position(sprite_handle, cursor_xpos, cursor_ypos);

}

int iterationCount(float cr, float ci, bool (*callback_func)(void)) {
	// return number of iterations for testing if cr,ci is in Mandelbrot set
	// return -1 if callback_func returns false (indicating we should interrupt calculation)
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

	if(keepgoing == true) return i;
	else return -1;
}

bool handle_input(void) {
	static uint8_t lastStatus;
	static uint8_t stepSize;
	char key=0;
	char keypressed = isKeyPressed();
	bool shouldKeepGoing = true;

	if(keypressed) {
		key = LastKeyPressed;
	}

    if (stepSize < 1 || stepSize > 9) stepSize = 4;

	switch(key) {
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                        stepSize = key - '0';
			break;
		case 'w':
			cursor_ypos-=stepSize;
			break;
		case 'a':
			cursor_xpos-=stepSize;
			break;
		case 's':
			cursor_ypos+=stepSize;
			break;
		case 'd':
			cursor_xpos+=stepSize;
			break;
		case 0x0d:  // ENTER or GO
			shouldKeepGoing = false;
			break;
		case 148:
			LastKeyPressed = 148;
			break;
	}
    if(shouldKeepGoing) {
	    if(cursor_xpos < 32) cursor_xpos = 32;
	    if(cursor_xpos >= 272) cursor_xpos = 272;

	    if(cursor_ypos <= 0) cursor_ypos = 0;
	    if(cursor_ypos > 180) cursor_ypos = 180;
	    vdp_sprite_set_position(sprite_handle, cursor_xpos, cursor_ypos);
	} else {
		stepSize = 4;
	}
	
	return shouldKeepGoing;

}

int iterToColor(int i) {
	int color = 1; // BLACK by default
	if(i < MAX_ITERATION) {
		color = i % 14 + 2;  // range 2-15
	}
	return color;
}

float pixel_x_to_cr(int x, float cr_min, float cr_max) {
	float cr = x / X_RES_PIXELS * (cr_max - cr_min) + cr_min;
	return cr;
}

float pixel_y_to_ci(int y, float ci_min, float ci_max) {
	float ci = y / Y_RES_PIXELS * (ci_max - ci_min) + ci_min;
	return ci;
}

float sprite_x_to_cr(int x, float cr_min, float cr_max) {
	float cr = x / SPRITE_X_MAXPOS * (cr_max - cr_min) + cr_min;
	return cr;
}

float sprite_y_to_ci(int y, float ci_min, float ci_max) {
	float ci = y / SPRITE_Y_MAXPOS * (ci_max - ci_min) + ci_min;
	return ci;
}

void main2() {
	float new_cr_min = cr_min;
	float new_cr_max = cr_max;
	float new_ci_min = ci_min;
	float new_ci_max = ci_max;

	while(true) {
	vdp_init(VDP_MODE_MULTICOLOR, VDP_DARK_BLUE, SPRITE_LARGE, false);
	for(int i=0; i<256; i++) {
		vdp_set_sprite_pattern(i, cursor_sprite_large);
	}

	sprite_handle = vdp_sprite_init(0, 0, VDP_WHITE);
	vdp_sprite_set_position(sprite_handle, cursor_xpos, cursor_ypos);

	bool keepgoing = true;

	for (int y=0; keepgoing && y<48; y++) {
		float ci = pixel_y_to_ci(y, ci_min, ci_max);
		for (int x=0; keepgoing && x<64; x++) {
			vdp_plot_color(x, y, VDP_WHITE);
			float cr = pixel_x_to_cr(x, cr_min, cr_max);
			int i = iterationCount(cr, ci, &handle_input);
			if( i != -1) {
			        int color = iterToColor(i);
			        vdp_plot_color(x, y, color);
			} else {
				keepgoing = false;
			}
		}
	}
	while(keepgoing) {
		keepgoing = handle_input();
		z80_delay_ms(100);
	}

	new_cr_min = sprite_x_to_cr(cursor_xpos - 32, cr_min, cr_max);
    new_cr_max = sprite_x_to_cr(cursor_xpos - 32 + 16, cr_min, cr_max);
	new_ci_min = sprite_y_to_ci(cursor_ypos, ci_min, ci_max);
	new_ci_max = sprite_y_to_ci(cursor_ypos + 12, ci_min, ci_max);

	cr_min = new_cr_min;
	cr_max = new_cr_max;
	ci_min = new_ci_min;
	ci_max = new_ci_max;
	centerCursor();
	}
}
