// Life.c - John Conway's Game of Life, in 64x48 spelndor
// Copyright Mike Debreceni 2023
//
// Rules:
// * If a cell is alive, it stays alive if it has 2 or 3 neighbors
// * If a cell is dead, it springs to life if it has 3 neighbors
//
// Based on Hello World C example from NABU.ca Homebrew
//
// https://nabu.ca/homebrew-c-tutorial
// https://cloud.nabu.ca/homebrew/Hello-World-C.zip

#define FONT_STANDARD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arch/z80.h>
#include <input.h>

#include "tms9918.h"

#define X_RES_PIXELS 64.0
#define Y_RES_PIXELS 48.0

#define SPRITE_X_MAXPOS 256.0
#define SPRITE_Y_MAXPOS 192.0

#define SPRITE_LARGE true
#define SPRITE_SMALL false

uint8_t cursor_sprite_small[] = {0xf0, 0x90, 0x90, 0xf0,
                                 0x00, 0x00, 0x00, 0x00};

int16_t sprite_handle = 0;
int16_t cursor_x = X_RES_PIXELS / 2;
int16_t cursor_y = Y_RES_PIXELS / 2;

uchar in_KeyDebounce = 0;         // Number of ticks before a keypress is acknowledged. Set to 1 for no debouncing.
uchar in_KeyStartRepeat = 20;     // Number of ticks after first time key is registered (after debouncing) before a key starts repeating.
uchar in_KeyRepeatPeriod = 20;    // Repeat key rate measured in ticks.
int in_KbdState;                  // Reserved variable holds in_GetKey() state

char lifeGrid[64][48];
char neighborCount[64][48];
char cols_to_scan[64];
char rows_to_scan[48];

int16_t cursor_x_to_screen(int cursor_x) {
    int16_t offset = 32;
    return 4 * cursor_x + offset;
}

int16_t cursor_y_to_screen(int cursor_y) {
    int16_t offset = 0;
    return 4 * cursor_y + offset;
}

void centerCursor(void) {
    cursor_x = X_RES_PIXELS / 2;
    cursor_y = Y_RES_PIXELS / 2;
    vdp_sprite_set_position(sprite_handle, cursor_x_to_screen(cursor_x),
                            cursor_y_to_screen(cursor_y));
}

int countNeighbors(int x, int y) {
    // count neighbors for cell at position (x, y)
    // xn, yn are x,y of neighbor to test
    // we will scan x-1,y-1 through x+1, y+1.  We will check that xn, yn are in
    // bounds, and not the cell at x,y
    //
    // [   ][   ][   ]
    // [   ][x,y][   ]
    // [   ][   ][   ]
    int16_t neighbors = 0;
    if(x < 0 || x > (X_RES_PIXELS - 1)) return 0;
    if(y < 0 || y > (Y_RES_PIXELS - 1)) return 0;
    vdp_sprite_set_position(sprite_handle, cursor_x_to_screen(x), cursor_y_to_screen(y));

    for (int xn = x - 1; xn <= x + 1; xn++) {
        for (int yn = y - 1; yn <= y + 1; yn++) {
            if (xn != x || yn != y) {
                // don't count ourselves
                if (xn >= 0 && xn < 64 && yn >= 0 &&
                    yn < Y_RES_PIXELS) {  // check if we're in bounds
                    if (lifeGrid[xn][yn] == true) {
                        neighbors++;
                    }
                }
            }
        }
    }
    return neighbors;
}

void setRowsToScan(void) {
    for (int y = 0; y < (Y_RES_PIXELS - 1); y++) {
        if (rows_to_scan[y + 1]) rows_to_scan[y] = (char) 1;
    }
    for (int y = (Y_RES_PIXELS - 1); y > 0; y--) {
        if (rows_to_scan[y - 1]) rows_to_scan[y] = (char) 1;
    }

}

void plotRowsToScan(void) {
    for (int y = 0; y < Y_RES_PIXELS; y++) {
        vdp_plot_color(0, y, VDP_DARK_BLUE);
    }
    // for(int idy=0; idy<Y_RES_PIXELS && rows_to_scan[idy] != (char) -1; idy++) {
    for (int y = 0; y < Y_RES_PIXELS; y++) {
        if (rows_to_scan[y]) {
            vdp_plot_color(0, y, VDP_LIGHT_GREEN);
        }
    }
}

void setColsToScan(void) {
    // mark neighbors to left of marked columns
    for (int x = 0; x < (X_RES_PIXELS - 1); x++) {
        if (cols_to_scan[x + 1]) cols_to_scan[x] = 1;
    }

    // mark neighbors to right of marked colums
    for (int x = (X_RES_PIXELS - 1); x > 0; x--) {
        if (cols_to_scan[x - 1]) cols_to_scan[x] = 1;
    }
}
void plotColsToScan(void) {
    for (int x = 0; x < 64; x++) {
        vdp_plot_color(x, 0, VDP_DARK_BLUE);
    }

    for (int x = 0; x < 64; x++) {
        if (cols_to_scan[x]) {
            vdp_plot_color(x, 0, VDP_LIGHT_GREEN);
        }
    }
}

void initActiveRowsColsFromLifeGrid(void) {
    memset(cols_to_scan, 0, X_RES_PIXELS);
    memset(rows_to_scan, 0, Y_RES_PIXELS);

    for (int x = 0; x < X_RES_PIXELS; x++) {
        for (int y = 0; y < Y_RES_PIXELS; y++) {
            if (lifeGrid[x][y]) {
                cols_to_scan[x] = true;
                rows_to_scan[y] = true;
            }
        }
    }
    setRowsToScan();
    setColsToScan();
}

void initGrid(void) {
    memset(lifeGrid, 0, Y_RES_PIXELS * X_RES_PIXELS);

    // F pentomino
    lifeGrid[32][22] = true;
    lifeGrid[33][22] = true;
    lifeGrid[33][23] = true;
    lifeGrid[34][23] = true;
    lifeGrid[33][24] = true;

    // Glider
    // lifeGrid[10][10] = true;
    // lifeGrid[11][11] = true;
    // lifeGrid[11][12] = true;
    // lifeGrid[9][12] = true;
    // lifeGrid[10][12] = true;

    // Blinker
    // lifeGrid[13][21] = true;
    // lifeGrid[13][22] = true;
    // lifeGrid[13][23] = true;

    // Square
    // lifeGrid[52][42] = true;
    // lifeGrid[53][42] = true;
    // lifeGrid[52][43] = true;
    // lifeGrid[53][43] = true;

    initActiveRowsColsFromLifeGrid();
}

void plotGrid(void) {
    for (int x = 0; x < X_RES_PIXELS; x++) {
        for (int y = 0; y < Y_RES_PIXELS; y++) {
            if (lifeGrid[x][y]) {
                vdp_plot_color(x, y, VDP_LIGHT_YELLOW);
            } else {
                vdp_plot_color(x, y, VDP_DARK_BLUE);
            }
        }
    }
}

bool runGeneration(void) {
    bool keepgoing = true;
    
    char col_was_active[X_RES_PIXELS];
    char row_was_active[Y_RES_PIXELS];
    // count neighbors
    for (int x = 0; x < X_RES_PIXELS; x++) {
        if (cols_to_scan[x]) {
            for (int y = 0; y < Y_RES_PIXELS; y++) {
                if (rows_to_scan[y]) {
                    neighborCount[x][y] = countNeighbors(x, y);
                }
            }
        }
    }

    memset(col_was_active, 0, X_RES_PIXELS);
    memset(row_was_active, 0, Y_RES_PIXELS);

    // calculate next generation
    for (int x = 0; x < X_RES_PIXELS; x++) {
        // col_was_active[x] = true;

        if (cols_to_scan[x]) {
            for (int y = 0; y < Y_RES_PIXELS; y++) {
                // row_was_active[y] = true;
                if (rows_to_scan[y]) {
                    int c = neighborCount[x][y];
                    if (lifeGrid[x][y] == true) {
                        // cell is currently alive
                        // * If a cell is alive, it stays alive if it has 2 or 3
                        // neighbors
                        if (c < 2 || c > 3) {
                            lifeGrid[x][y] = false;
                            vdp_plot_color(x, y, VDP_DARK_BLUE);
                            col_was_active[x] = true;
                            row_was_active[y] = true;
                        }
                    } else {
                        // cell is dead.
                        // * If a cell is dead, it springs to life if it has 3
                        // neighbors
                        if (c == 3) {
                            lifeGrid[x][y] = true;
                            vdp_plot_color(x, y, VDP_LIGHT_YELLOW);
                            col_was_active[x] = true;
                            row_was_active[y] = true;
                        }
                    }
                }
            }
        }
    }
    
    memcpy(cols_to_scan, col_was_active, X_RES_PIXELS);
    memcpy(rows_to_scan, row_was_active, Y_RES_PIXELS);
    
    return keepgoing;
}

bool editGrid(void) {
    bool shouldKeepEditing = true;
    bool shouldKeepRunning = true;
    
    sprite_handle = vdp_sprite_init(0, 0, VDP_MAGENTA);
    while (shouldKeepEditing) {
        vdp_sprite_set_position(sprite_handle, cursor_x_to_screen(cursor_x),
                        cursor_y_to_screen(cursor_y));
        char key = getk();

        switch (key) {
            case 'w': case 'W':
                cursor_y--;
                if (cursor_y < 0) cursor_y = 0;
                break;
            case 'a': case 'A':
                cursor_x--;
                if (cursor_x < 0) cursor_x = 0;
                break;
            case 's': case 'S':
                cursor_y++;
                if (cursor_y > (Y_RES_PIXELS - 1)) cursor_y = (Y_RES_PIXELS - 1);
                break;
            case 'd': case 'D':
                cursor_x++;
                if (cursor_x > (X_RES_PIXELS - 1)) cursor_x = (X_RES_PIXELS - 1);
                break;
            case ' ':
                // update cell at current location
                if (lifeGrid[cursor_x][cursor_y]) {
                    lifeGrid[cursor_x][cursor_y] = false;
                    vdp_plot_color(cursor_x, cursor_y, VDP_DARK_BLUE);
                } else {
                    lifeGrid[cursor_x][cursor_y] = true;
                    vdp_plot_color(cursor_x, cursor_y, VDP_LIGHT_YELLOW);
                }
                break;
            case 0x0d:  // ENTER or GO
                shouldKeepEditing = false;
                break;
        }
    }
    sprite_handle = vdp_sprite_init(0, 0, VDP_WHITE);
    vdp_sprite_set_position(sprite_handle, cursor_x_to_screen(cursor_x),
                            cursor_y_to_screen(cursor_y));

    initActiveRowsColsFromLifeGrid();
    return shouldKeepRunning;
}

int main(void) {
    char ch = 0;
    bool keepgoing = true;
    vdp_init(VDP_MODE_MULTICOLOR, VDP_BLACK, SPRITE_SMALL, false);
    for (int i = 0; i < 256; i++) {
        vdp_set_sprite_pattern(i, cursor_sprite_small);
    }
    sprite_handle = vdp_sprite_init(0, 0, VDP_WHITE);

    initGrid();
    plotGrid();
    editGrid();

    while (keepgoing == true) {
        vdp_plot_color(0, 0, VDP_CYAN);
        // z80_delay_ms(500);
    	// mysteriously crash Life by calling this
            ch = getk();     
        
        // Press Space or Go to enter editor
        if (ch == ' ' || ch == 0x0d) {
            editGrid();
	    ch = 0;
        }
        
        runGeneration();
        setRowsToScan();
        setColsToScan();

        plotColsToScan();
        plotRowsToScan();
    }
}
