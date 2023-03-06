#ifndef VDP_H
#define VDP_H

enum VDP_COLORS {
  VDP_TRANSPARENT = 0,
  VDP_BLACK = 1,
  VDP_MED_GREEN = 2,
  VDP_LIGHT_GREEN = 3,
  VDP_DARK_BLUE = 4,
  VDP_LIGHT_BLUE = 5,
  VDP_DARK_RED = 6,
  VDP_CYAN = 7,
  VDP_MED_RED = 8,
  VDP_LIGHT_RED = 9,
  VDP_DARK_YELLOW = 10,
  VDP_LIGHT_YELLOW = 11,
  VDP_DARK_GREEN = 12,
  VDP_MAGENTA = 13,
  VDP_GRAY = 14,
  VDP_WHITE = 15
};

enum VDP_MODES {
  VDP_MODE_G1 = 0,
  VDP_MODE_G2 = 1,
  VDP_MODE_MULTICOLOR = 2,
  VDP_MODE_TEXT = 3,
};

enum VDP_CSR {
  VDP_CSR_UP = 0,
  VDP_CSR_DOWN = 1,
  VDP_CSR_LEFT = 2,
  VDP_CSR_RIGHT = 3
};

/**
 * VDP Status flags from status register
 */
// Coincidence flag, set when sprites overlap
#define VDP_FLAG_COIN 0x20 
/// 5th sprite flag, set when more than 4 sprite per line 
#define VDP_FLAG_S5 0x40  

 /** Struct
  * 4-Byte record defining sprite attributes
  */
typedef struct {
  uint8_t x; //Sprite X position
  uint8_t y; //Sprite Y position
  uint8_t name_ptr; //Sprite name in pattern table
  uint8_t ecclr; //Bit 7: Early clock bit, bit 3:0 color
} Sprite_attributes;

/**
 * VDP status
 */
#define VDP_OK 0
#define VDP_ERROR 1

 /**
  * initialize the VDP
  * Not all parameters are useful for all modes. Refer to documentation
  *
  * @param mode VDP_MODE_G1 | VDP_MODE_G2 | VDP_MODE_MULTICOLOR | VDP_MODE_TEXT
  * @param color
  * @param big_sprites true: Use 16x16 sprites false: use 8x8 sprites
  * @param magnify true: Scale sprites up by 2
  * @return int
  */
int vdp_init(uint8_t mode, uint8_t color, bool big_sprites, bool magnify);

/**
 * @brief Initializes the VDP in text mode
 *
 * @param fgcolor Text color default: default black
 * @param bgcolor Background color: default white
 * @returns VDP_ERROR | VDP_SUCCESS
 */
int vdp_init_textmode(uint8_t fgcolor, uint8_t);

/**
 * @brief Initializes the VDP in Graphic Mode 1
 *
 * @param fgcolor Text color default: default black
 * @param bgcolor Background color: default white
 * @returns VDP_ERROR | VDP_SUCCESS
 * @deprecated Not really useful if more than 4k Video ram is available
 */
int vdp_init_g1(uint8_t fgcolor, uint8_t bgcolor);

/**
 * @brief Initializes the VDP in Graphic Mode 2
 *
 * @param big_sprites true: use 16x16 sprites false: use 8x8 sprites
 * @param scale_sprites Scale sprites up by 2
 * @returns VDP_ERROR | VDP_SUCCESS
 */
int vdp_init_g2(bool big_sprites, bool scale_sprites);

/**
 * @brief Initializes the VDP in 64x48 Multicolor Mode
 *
 * @returns VDP_ERROR | VDP_SUCCESS
 * @deprecated Not really useful if more than 4k Video ram is available
 */
int vdp_init_multicolor();

/**
 * @brief Set foreground and background color of the pattern at the current cursor position
 * Only available in Graphic mode 2
 * @param fgcolor Foreground color
 * @param bgcolor Background color
 */
void vdp_colorize(uint8_t fgcolor, uint8_t bgcolor);

/**
 * @brief Plot a point at position (x,y), where x <= 255. The full resolution of 256 by 192 is available.
 * Only two different colors are possible whithin 8 neighboring pixels
 * VDP_MODE G2 only
 *
 * @param x
 * @param y
 * @param color1 Color of pixel at (x,y). If NULL, plot a pixel with color2
 * @param color2 Color of the pixels not set or color of pixel at (x,y) when color1 == NULL
 */
void vdp_plot_hires(uint8_t x, uint8_t y, uint8_t color1, uint8_t color2);

/**
 * @brief Plot a point at position (x,y), where x <= 64. In Graphics mode2, the resolution is 64 by 192 pixels, neighboring pixels can have different colors.
 * In Multicolor  mode, the resolution is 64 by 48 pixels
 *
 * @param x
 * @param y
 * @param color
 */
void vdp_plot_color(uint8_t x, uint8_t y, uint8_t color);

void writeByteToVRAM(unsigned char value);

uint8_t readByteFromVRAM();

void writePort(unsigned char value);

/**
 * @brief Print string at current cursor position. These Escape sequences are supported:
 * <ul>
 * <li>\\n (newline) </li>
 * <li>\\r (carriage return)</li>
 * <li>Graphic Mode 2 only: \\033[<fg>;[<bg>]m sets the colors and optionally the background of the subsequent characters </li>
 * </ul>
 * Example: vdp_print("\033[4m Dark blue on transparent background\n\r\033[4;14m dark blue on gray background");
 * @param text Text to print
 */
void vdp_print(uint8_t *text);

/**
 * @brief Set backdrop color
 *
 * @param color
 */
void vdp_set_bdcolor(uint8_t);

/**
 * @brief Set the color of patterns at the cursor position
 *
 * @param index VDP_MODE_G2: Number of pattern to set the color, VDP_MODE_G1: one of 32 groups of 8 subsequent patterns
 * @param fg Pattern foreground color
 * @param bg Pattern background color
 */
void vdp_set_pattern_color(uint16_t index, uint8_t fgcolor, uint8_t bgcolor);

/**
 * @brief Position the cursor at the specified position
 *
 * @param col column
 * @param row row
 */
void vdp_set_cursor2(uint8_t col, uint8_t row);

/**
 * @brief Move the cursor along the specified direction
 *
 * @param direction {VDP_CSR_UP|VDP_CSR_DOWN|VDP_CSR_LEFT|VDP_CSR_RIGHT}
 */
void vdp_set_cursor(uint8_t direction);

/**
 * @brief set foreground and background color of the characters printed after this function has been called.
 * In Text Mode and Graphics Mode 1, all characters are changed. In Graphics Mode 2, the escape sequence \\033[<fg>;<bg>m can be used instead.
 * See vdp_print()
 *
 * @param fg Foreground color
 * @param bg Background color
 */
void vdp_textcolor(uint8_t fgcolor, uint8_t bgcolor);

/**
 * @brief Write ASCII character at current cursor position
 *
 * @param chr Pattern at the respective location of the  pattern memory. Graphic Mode 1 and Text Mode: Ascii code of character
 */
void vdp_write(uint8_t chr, bool advanceNextChar);

/**
 * @brief Write a sprite into the sprite pattern table
 *
 * @param name Reference of sprite 0-255 for 8x8 sprites, 0-63 for 16x16 sprites
 * @param sprite Array with sprite data. Type uint8_t[8] for 8x8 sprites, uint8_t[32] for 16x16 sprites
 */
void vdp_set_sprite_pattern(uint8_t name, const uint8_t* sprite);

/**
 * @brief Set the sprite color
 *
 * @param handle Sprite Handle returned by vdp_sprite_init()
 * @param color
 */
void vdp_sprite_color(uint16_t handle, uint8_t color);

/**
 * @brief Get the sprite attributes
 *
 * @param handle Sprite Handle returned by vdp_sprite_init()
 * @return Sprite_attributes
 */
// Sprite_attributes vdp_sprite_get_attributes(uint16_t handle);

/// <summary>
/// Add a new line (move down and to line start)
/// </summary>
void vdp_newLine();

/// <summary>
/// Display the numeric value of the variable
/// </summary>
void vdp_writeUInt8(uint8_t v);

/// <summary>
/// Get the character in text mode at the specified location directly from VRAM, which is slow
/// </summary>
uint8_t vdp_getCharAtLocationVRAM(uint8_t x, uint8_t y);

/// <summary>
/// In text mode, there is a buffer copy of the screen
/// </summary>
uint8_t vdp_getCharAtLocationBuf(uint8_t x, uint8_t y);

/// <summary>
/// Set the character in memory buffer at location. This does not update the screen!
/// It is used by the text scroll methods
/// </summary
void vdp_setCharAtLocationBuf(uint8_t x, uint8_t y, uint8_t c);

/// <summary>
/// Scroll all lines up between topRow and bottomRow
/// </summary>
void vdp_ScrollTextUp(uint8_t topRow, uint8_t bottomRow);

/// <summary>
/// Clear the rows with 0x20 between topRow and bottomRow
/// </summary>
void vdp_ClearRows(uint8_t topRow, uint8_t bottomRow);

/// <summary>
/// Write a character at the specified location
/// </summary>
void vdp_writeCharAtLocation(uint8_t x, uint8_t y, uint8_t c);

void vdp_dumpFontToScreen();

/// <summary>
/// Display the numeric value of the variable
/// </summary>
void vdp_writeInt8(int8_t v);

/// <summary>
/// Display the binary value of the variable
/// </summary>
void vdp_writeUInt8ToBinary(uint8_t v);

/**
 * @brief Get the current position of a sprite
 *
 * @param handle Sprite Handle returned by vdp_sprite_init()
 * @param xpos Reference to x-position
 * @param ypos Reference to y-position
 */
void vdp_sprite_get_position(uint16_t handle, uint16_t xpos, uint8_t ypos);

/**
 * @brief Activate a sprite
 *
 * @param name Number of the sprite as defined in vdp_set_sprite()
 * @param priority 0: Highest priority; 31: Lowest priority
 * @param color
 * @returns     Sprite Handle
 */
uint16_t vdp_sprite_init(uint8_t name, uint8_t priority, uint8_t color);

/**
 * @brief Set position of a sprite
 *
 * @param handle  Sprite Handle returned by vdp_sprite_init()
 * @param x
 * @param y
 * @returns     true: In case of a collision with other sprites
 */
uint8_t vdp_sprite_set_position(uint16_t handle, uint16_t x, uint8_t y);

#endif
