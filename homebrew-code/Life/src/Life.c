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
    for(int i=0; i<48; i++) {
        rows_to_scan[i] = 0;
    }
    for(int i=0; i<64; i++) {
        cols_to_scan[i] = 0;
    }
    //memset(cols_to_scan, 0, X_RES_PIXELS);
    //memset(rows_to_scan, 0, Y_RES_PIXELS);

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

bool runGeneration(bool (*callback_func)(void)) {
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

    for(int i=0; i<64; i++) {
        col_was_active[i] = 0;
    }
    for(int i=0; i<48; i++) {
        row_was_active[i] = 0;
    }
//    memset(col_was_active, 0, X_RES_PIXELS);
//    memset(row_was_active, 0, Y_RES_PIXELS);

    if(callback_func != NULL) {
        // preserve support for callback function
        keepgoing = callback_func();
    }

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
    for(int i=0; i<64; i++) {
        cols_to_scan[i] = col_was_active[i]; 
    }
    for(int i=0; i<48; i++) {
        rows_to_scan[i] = row_was_active[i];
    }
    //memcpy(cols_to_scan, col_was_active, X_RES_PIXELS);
    //memcpy(rows_to_scan, row_was_active, Y_RES_PIXELS);
    return keepgoing;
}

bool editGrid(void) {
    bool shouldKeepEditing = true;
    bool shouldKeepRunning = true;
    
    sprite_handle = vdp_sprite_init(0, 0, VDP_MAGENTA);
    while (shouldKeepEditing) {
        vdp_sprite_set_position(sprite_handle, cursor_x_to_screen(cursor_x),
                        cursor_y_to_screen(cursor_y));
        char key = isKeyPressed();

        switch (key) {
            case 'w':
                cursor_y--;
                if (cursor_y < 0) cursor_y = 0;
                break;
            case 'a':
                cursor_x--;
                if (cursor_x < 0) cursor_x = 0;
                break;
            case 's':
                cursor_y++;
                if (cursor_y > (Y_RES_PIXELS - 1)) cursor_y = (Y_RES_PIXELS - 1);
                break;
            case 'd':
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

bool handle_input(void) {
    // FIXME:  This is a callback that handles input, but is not actually used right now. Consider removing.
    
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
    int count=0;
    while (keepgoing == true) {
        count++;
        vdp_plot_color(0, 0, VDP_CYAN);
        // z80_delay_ms(500);
        if(count % 1 == 0)  {  // debug - call isKeyPressed() less frequently to see if that helps
            // force plotting of entire grid for debugging to detect scribbling on LifeGrid array - SLOW
            // plotGrid();  
            // diagnostic red dot before calling isKeyPressed
            vdp_plot_color(0, 0, VDP_LIGHT_RED);   
            //  mysteriously crash Life by calling this
     //       ch = isKeyPressed();     
        }
        if(count >= 100) count = 0;
        vdp_plot_color(0, 0, VDP_DARK_RED);
        // z80_delay_ms(500);
        vdp_plot_color(0, 0, VDP_DARK_BLUE);
        
        // Press Space or Go to enter editor
        if (ch == ' ' || ch == 0x0d) {
            editGrid();
        }
        
        runGeneration(NULL);
        setRowsToScan();
        setColsToScan();

        plotColsToScan();
        plotRowsToScan();
    }
}
