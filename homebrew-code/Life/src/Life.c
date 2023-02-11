// Life.c - John Conway's Game of Life, in 64x48 spelndor
// mode Copyright Mike Debreceni 2023
//
// Rules:
// * If a cell is alive, it stays alive if it has 2 or 3 neighbors
// * If a cell is dead, it springs to life if it has 3 neighbors
//
// Based on Hello World C example from NABU.ca Homebrew
//
// https://nabu.ca/homebrew-c-tutorial
// https://cloud.nabu.ca/homebrew/Hello-World-C.zip

static void orgit() __naked { 
	__asm 
	org 0x140D 
	nop 
	nop 
	nop 
	__endasm; }

void main2();

void main() { main2(); }

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

uint8_t cursor_sprite_small[] = {0xf0, 0x90, 0x90, 0xf0, 0x00, 0x00, 0x00, 0x00};

int16_t sprite_handle = 0;
int16_t cursor_x = 32;
int16_t cursor_y = 24;

char lifeGrid[64][48];
char neighborCount[64][48];

int16_t cursor_x_to_screen(int cursor_x) {
    int16_t offset = 0;
    return 4 * cursor_x + offset;
}

int16_t cursor_y_to_screen(int cursor_y) {
    int16_t offset = 0;
    return 4 * cursor_y + offset;
}

void centerCursor(void) {
    cursor_x = 32;
    cursor_y = 24;
    vdp_sprite_set_position(sprite_handle, cursor_x_to_screen(cursor_x), cursor_y_to_screen(cursor_y));
}

int countNeighbors(int x, int y) {
    // count neighbors for cell at position (x, y)
    // xn, yn are x,y of neighbor to test
    // we will scan x-1,y-1 through x+1, y+1.  We will check that xn, yn are in bounds, and not the cell at x,y
    // 
    // [   ][   ][   ]
    // [   ][x,y][   ]
    // [   ][   ][   ]
    int16_t neighbors = 0;

    for(int xn = x - 1; xn <= x + 1; xn++) {
        for(int yn = y - 1; yn <= y + 1; yn++) {
            if(xn != x || yn != y) {  
                // don't count ourselves
                if(xn >= 0 && xn < 64 && yn >= 0 && yn < 48) {// check if we're in bounds
                    if(lifeGrid[xn][yn] == true) {
                        neighbors++;
                    }
                }
            }
        }
    }
    return neighbors;
}


void initGrid(void) {
    memset(lifeGrid, 0, sizeof(lifeGrid));

    // F pentomino
    lifeGrid[32][22] = true;
    lifeGrid[33][22] = true;
    lifeGrid[33][23] = true;
    lifeGrid[34][23] = true;
    lifeGrid[33][24] = true;

    // Glider
    lifeGrid[10][10] = true;
    lifeGrid[11][11] = true;
    lifeGrid[11][12] = true;
    lifeGrid[9][12] = true;
    lifeGrid[10][12] = true;

    // Blinker
    lifeGrid[13][21] = true;
    lifeGrid[13][22] = true;
    lifeGrid[13][23] = true;
    
    // Square 
    lifeGrid[52][42] = true;
    lifeGrid[53][42] = true;
    lifeGrid[52][43] = true;
    lifeGrid[53][43] = true;
    
}

void plotGrid(void) {
    for(int x=0; x<64; x++) {
        for(int y=0; y<48; y++) {
            if(lifeGrid[x][y]) {
                vdp_plot_color(x, y, VDP_LIGHT_YELLOW);
            } else {
                vdp_plot_color(x, y, VDP_DARK_BLUE);
            }
        }
    }
}

int runGeneration(bool (*callback_func)(void)) {
    bool keepgoing = true;
    // count neighbors
    for(int x = 0; x < 64; x++) {
        for(int y = 0; y < 48; y++) {
            neighborCount[x][y] = countNeighbors(x,y);
        }
    }
    
    // calculate next generation
    for (int x = 0; x < 64; x++) {
        for(int y = 0; y < 48; y++) {
            int c = neighborCount[x][y];
            if(lifeGrid[x][y] == true) {
                // cell is currently alive
                // * If a cell is alive, it stays alive if it has 2 or 3 neighbors
                if( c < 2 || c > 3) {
                    lifeGrid[x][y] = false;
                    vdp_plot_color(x, y, VDP_DARK_BLUE);
                }
            } else {
                // cell is dead.
                // * If a cell is dead, it springs to life if it has 3 neighbors
                if (c == 3) {
                    lifeGrid[x][y] = true;
                    vdp_plot_color(x, y, VDP_LIGHT_YELLOW);
                }
            }
        }
    }

    return 1;
}

bool handle_input(void) {
    static uint8_t lastStatus;
    static uint8_t stepSize;
    char key = 0;
    char keypressed = isKeyPressed();
    bool shouldKeepGoing = true;

    if (keypressed) {
        key = LastKeyPressed;
    }

    if (stepSize < 1 || stepSize > 9) stepSize = 4;

    switch (key) {
        // we will either be in run mode or edit mode.
        // WASD and SPACE will work only in edit mode
        // GO will toggle between run and edit mode
        
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            stepSize = key - '0';
            break;
        case 'w':
            cursor_y -= stepSize;
            break;
        case 'a':
            cursor_x -= stepSize;
            break;
        case 's':
            cursor_y += stepSize;
            break;
        case 'd':
            cursor_x += stepSize;
            break;
        case ' ':
            // update cell at current location
            break;
        case 0x0d:  // ENTER or GO
            shouldKeepGoing = false;
            break;
        case 148:
            LastKeyPressed = 148;
            break;
    }
    if (shouldKeepGoing) {
        if (cursor_x < 32) cursor_x = 32;
        if (cursor_x >= 272) cursor_x = 272;

        if (cursor_y <= 0) cursor_y = 0;
        if (cursor_y > 180) cursor_y = 180;
        vdp_sprite_set_position(sprite_handle, cursor_x, cursor_y);
    } else {
        stepSize = 4;
    }

    return shouldKeepGoing;
}


void main2() {

    vdp_init(VDP_MODE_MULTICOLOR, VDP_BLACK, SPRITE_SMALL, false);
    for (int i = 0; i < 256; i++) {
        vdp_set_sprite_pattern(i, cursor_sprite_small);
    }
    //sprite_handle = vdp_sprite_init(0, 0, VDP_WHITE);
    //vdp_sprite_set_position(sprite_handle, cursor_x, cursor_y);

    bool keepgoing = true;
    initGrid();
    plotGrid();
    while (true) {
        runGeneration(NULL);
    }
}
