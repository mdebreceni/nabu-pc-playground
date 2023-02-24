// TMS9918 graphic library 
// -----------------------
// Originally from: https://github.com/michalin/TMS9918_Arduino/tree/main/examples
// Modified by DJ Sures 2022 for NABU PC
// https://nabu.ca
//
// Last updated December 13, 2022
// version 2022.12.13.01
// **********************************************************************************************

#define MODE 11
#define CSW 10
#define CSR 9
#define RESET 8
#define R1_IE 0x20
#define R1_M1 0x10
#define R1_M2 0x08
#define R1_SIZE 0x02
#define R1_MAG 0x01

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch/z80.h>
#include <stdbool.h>
#include "tms9918.h"
#include "patterns.h"

uint8_t _textBuffer[24 * 40]; // [row][col]

struct {
  uint8_t x;
  uint8_t y;
} cursor;

uint16_t _sprite_attribute_table;
uint16_t _sprite_pattern_table;
uint8_t  _sprite_size_sel; //0: 8x8 sprites 1: 16x16 sprites
uint16_t _name_table;
uint16_t _color_table_size = 0;
uint16_t _color_table;
uint16_t _pattern_table;
uint8_t  _crsr_max_x = 31;      // max number of colums (Overwritten in Text mode)
const uint8_t _crsr_max_y = 23; // max number of rows
uint8_t _vdp_mode;

uint8_t _fgcolor;
uint8_t _bgcolor;

// Writes a byte to databus for register access
void writePort(unsigned char value) {

  z80_outp(0xA1, value);
}

// Reads a byte from databus for register access
uint8_t read_status_reg() {

  return z80_inp(0xa1);
}

// Writes a byte to databus for vram access
void writeByteToVRAM(unsigned char value) {

  z80_outp(0xA0, value);
}

// Reads a byte from databus for vram access
unsigned char readByteFromVRAM() {

  return z80_inp(0xa0);
}

void setRegister(unsigned char registerIndex, unsigned char value) {

  writePort(value);

  writePort(0x80 | registerIndex);
}

void setWriteAddress(unsigned int address) {

  z80_outp(0xA1, address & 0xff);

  z80_outp(0xa1, 0x40 | (address >> 8) & 0x3f);
}

void setReadAddress(unsigned int address) {

  z80_outp(0xA1, address & 0xff);

  z80_outp(0xA1, (address >> 8) & 0x3f);
}

int vdp_init(uint8_t mode, uint8_t color, bool big_sprites, bool magnify) {

  _vdp_mode = mode;

  _sprite_size_sel = big_sprites;

  // Clear Ram
  setWriteAddress(0x0);

  for (int i = 0; i < 0x3FFF; i++)
    writeByteToVRAM(0);

  switch (mode) {
  case VDP_MODE_G1:

    setRegister(0, 0x00);
    setRegister(1, 0xC0 | (big_sprites << 1) | magnify); // Ram size 16k, activate video output
    setRegister(2, 0x05); // Name table at 0x1400
    setRegister(3, 0x80); // Color, start at 0x2000
    setRegister(4, 0x01); // Pattern generator start at 0x800
    setRegister(5, 0x20); // Sprite attriutes start at 0x1000
    setRegister(6, 0x00); // Sprite pattern table at 0x000
    _sprite_pattern_table = 0;
    _pattern_table = 0x800;
    _sprite_attribute_table = 0x1000;
    _name_table = 0x1400;
    _color_table = 0x2000;
    _color_table_size = 32;

    // Initialize pattern table with ASCII patterns
    setWriteAddress(_pattern_table + 0x100);

    for (uint16_t i = 0; i < 768; i++)
      writeByteToVRAM(ASCII[i]);

    break;

  case VDP_MODE_G2:

    setRegister(0, 0x02);
    setRegister(1, 0xC0 | (big_sprites << 1) | magnify); // Ram size 16k, Disable Int, 16x16 Sprites, mag off, activate video output
    setRegister(2, 0x0E); // Name table at 0x3800
    setRegister(3, 0xFF); // Color, start at 0x2000
    setRegister(4, 0x03); // Pattern generator start at 0x0
    setRegister(5, 0x76); // Sprite attriutes start at 0x3800
    setRegister(6, 0x03); // Sprite pattern table at 0x1800
    _pattern_table = 0x00;
    _sprite_pattern_table = 0x1800;
    _color_table = 0x2000;
    _name_table = 0x3800;
    _sprite_attribute_table = 0x3B00;
    _color_table_size = 0x1800;
    setWriteAddress(_name_table);

    for (uint16_t i = 0; i < 768; i++)
      writeByteToVRAM(i);

    break;

  case VDP_MODE_TEXT:

    setRegister(0, 0x00);
    setRegister(1, 0xD2); // Ram size 16k, Disable Int
    setRegister(2, 0x02); // Name table at 0x800
    setRegister(4, 0x00); // Pattern table start at 0x0
    _pattern_table = 0x00;
    _name_table = 0x800;
    _crsr_max_x = 39;
    setWriteAddress(_pattern_table + 0x100);

    for (uint16_t i = 0; i < 23 * 40; i++)
      _textBuffer[i] = 0x20;

    for (uint16_t i = 0; i < 768; i++)
      writeByteToVRAM(ASCII[i]);

    vdp_set_cursor2(0, 0);

    break;

  case VDP_MODE_MULTICOLOR:

    setRegister(0, 0x00);
    setRegister(1, 0xC8 | (big_sprites << 1) | magnify); // Ram size 16k, Multicolor
    setRegister(2, 0x05); // Name table at 0x1400
    // setRegister(3, 0xFF); // Color table not available
    setRegister(4, 0x01); // Pattern table start at 0x800
    setRegister(5, 0x76); // Sprite Attribute table at 0x3b00
    setRegister(6, 0x03); // Sprites Pattern Table at 0x1800
    _pattern_table = 0x800;
    _sprite_attribute_table = 0x3b00;
    _sprite_pattern_table = 0x1800;
    _name_table = 0x1400;
    setWriteAddress(_name_table); // Init name table

    for (uint8_t j = 0; j < 24; j++)
      for (uint16_t i = 0; i < 32; i++)
        writeByteToVRAM(i + 32 * (j / 4));

    break;
  default:
    return VDP_ERROR; // Unsupported mode
  }

  setRegister(7, color);

  return VDP_OK;
}

void vdp_dumpFontToScreen() {

  for (uint8_t j = 0; j < 24; j++)
    for (uint16_t i = 0; i < 32; i++)
      writeByteToVRAM(i + 32 * (j / 4));
}

void vdp_colorize(uint8_t fg, uint8_t bg) {

  if (_vdp_mode != VDP_MODE_G2)
    return;

  uint16_t name_offset = cursor.y * (_crsr_max_x + 1) + cursor.x; // Position in name table
  uint16_t color_offset = name_offset << 3;                       // Offset of pattern in pattern table
  setWriteAddress(_color_table + color_offset);

  for (uint8_t i = 0; i < 8; i++)
    writeByteToVRAM((fg << 4) + bg);
}

void vdp_plot_hires(uint8_t x, uint8_t y, uint8_t color1, uint8_t color2) {

  uint16_t offset = 8 * (x / 8) + y % 8 + 256 * (y / 8);
  setReadAddress(_pattern_table + offset);
  uint8_t pixel = readByteFromVRAM();
  setReadAddress(_color_table + offset);
  uint8_t color = readByteFromVRAM();

  if (color1 != NULL) {

    pixel |= 0x80 >> (x % 8); //Set a "1"
    color = (color & 0x0F) | (color1 << 4);
  } else {

    pixel &= ~(0x80 >> (x % 8)); //Set bit as "0"
    color = (color & 0xF0) | (color2 & 0x0F);
  }

  setWriteAddress(_pattern_table + offset);
  writeByteToVRAM(pixel);
  setWriteAddress(_color_table + offset);
  writeByteToVRAM(color);
}

void vdp_plot_color(uint8_t x, uint8_t y, uint8_t color) {

  if (_vdp_mode == VDP_MODE_MULTICOLOR) {

    uint16_t addr = _pattern_table + 8 * (x / 2) + y % 8 + 256 * (y / 8);
    setReadAddress(addr);
    uint8_t dot = readByteFromVRAM();
    setWriteAddress(addr);

    if (x & 1) // Odd columns
      writeByteToVRAM((dot & 0xF0) + (color & 0x0f));
    else
      writeByteToVRAM((dot & 0x0F) + (color << 4));
  } else if (_vdp_mode == VDP_MODE_G2) {

    // Draw bitmap
    uint16_t offset = 8 * (x / 2) + y % 8 + 256 * (y / 8);
    setReadAddress(_color_table + offset);
    uint8_t color_ = readByteFromVRAM();

    if ((x & 1) == 0) //Even 
    {

      color_ &= 0x0F;
      color_ |= (color << 4);
    } else {

      color_ &= 0xF0;
      color_ |= color & 0x0F;
    }

    setWriteAddress(_pattern_table + offset);
    writeByteToVRAM(0xF0);
    setWriteAddress(_color_table + offset);
    writeByteToVRAM(color_);
    // Colorize
  }
}

void vdp_set_sprite_pattern(uint8_t number, const uint8_t* sprite) {

  if (_sprite_size_sel) {

    setWriteAddress(_sprite_pattern_table + 32 * number);

    for (uint8_t i = 0; i < 32; i++) {
      writeByteToVRAM(sprite[i]);
    }
  } else {

    setWriteAddress(_sprite_pattern_table + 8 * number);

    for (uint8_t i = 0; i < 8; i++) {
      writeByteToVRAM(sprite[i]);
    }
  }

}

void vdp_sprite_color(uint16_t addr, uint8_t color) {

  setReadAddress(addr + 3);

  uint8_t ecclr = readByteFromVRAM() & 0x80 | (color & 0x0F);

  setWriteAddress(addr + 3);

  writeByteToVRAM(ecclr);
}

//Sprite_attributes vdp_sprite_get_attributes(uint16_t addr) {
//
//  Sprite_attributes attrs;
//
//  setReadAddress(addr);
//
//  attrs.y = readByteFromVRAM();
//  attrs.x = readByteFromVRAM();
//  attrs.name_ptr = readByteFromVRAM();
//  attrs.ecclr = readByteFromVRAM();
//
//  return attrs;
//}

void vdp_sprite_get_position(uint16_t addr, uint16_t xpos, uint8_t ypos) {

  setReadAddress(addr);

  ypos = readByteFromVRAM();

  uint8_t x = readByteFromVRAM();

  readByteFromVRAM();

  uint8_t eccr = readByteFromVRAM();

  if ((eccr & 0x80) != 0)
    xpos = x;
  else
    xpos = x + 32;
}

uint16_t vdp_sprite_init(uint8_t name, uint8_t priority, uint8_t color) {

  uint16_t addr = _sprite_attribute_table + 4 * priority;
  setWriteAddress(addr);
  writeByteToVRAM(0);
  writeByteToVRAM(0);

  if (_sprite_size_sel)
    writeByteToVRAM(4 * name);
  else
    writeByteToVRAM(name);

  writeByteToVRAM(0x80 | (color & 0xF));

  return addr;
}

uint8_t vdp_sprite_set_position(uint16_t addr, uint16_t x, uint8_t y) {

  uint8_t ec, xpos;

  if (x < 144) {

    ec = 1;
    xpos = x;
  } else {

    ec = 0;
    xpos = x - 32;
  }

  setReadAddress(addr + 3);
  uint8_t color = readByteFromVRAM() & 0x0f;

  setWriteAddress(addr);
  writeByteToVRAM(y);
  writeByteToVRAM(xpos);
  setWriteAddress(addr + 3);
  writeByteToVRAM((ec << 7) | color);

  return read_status_reg();
}

void vdp_print(uint8_t* text) {

  for (uint16_t i = 0; text[i] != 0x00; i++) {

    switch (text[i]) {

    case '\n':
      vdp_set_cursor2(cursor.x, ++cursor.y);
      break;
    case '\r':
      vdp_set_cursor2(0, cursor.y);
      break;
    case 0x00:
      break;
      break;
    default:
      vdp_write(text[i], true);
      //      vdp_colorize(_fgcolor, _bgcolor);
    }
  }
}

void vdp_newLine() {

  vdp_set_cursor2(0, ++cursor.y);
}

void vdp_set_bdcolor(uint8_t color) {

  setRegister(7, color);
}

void vdp_set_pattern_color(uint16_t index, uint8_t fg, uint8_t bg) {

  if (_vdp_mode == VDP_MODE_G1) {
    index &= 31;
  }

  setWriteAddress(_color_table + index);
  writeByteToVRAM((fg << 4) + bg);
}

void vdp_set_cursor2(uint8_t col, uint8_t row) {

  if (col == 255) //<0
  {
    col = _crsr_max_x;
    row--;
  } else if (col > _crsr_max_x) {
    col = 0;
    row++;
  }

  if (row == 255) {
    row = _crsr_max_y;
  } else if (row > _crsr_max_y) {
    row = 0;
  }

  cursor.x = col;
  cursor.y = row;
}

void vdp_set_cursor(uint8_t direction) {

  switch (direction) {
  case VDP_CSR_UP:
    vdp_set_cursor2(cursor.x, cursor.y - 1);
    break;
  case VDP_CSR_DOWN:
    vdp_set_cursor2(cursor.x, cursor.y + 1);
    break;
  case VDP_CSR_LEFT:
    vdp_set_cursor2(cursor.x - 1, cursor.y);
    break;
  case VDP_CSR_RIGHT:
    vdp_set_cursor2(cursor.x + 1, cursor.y);
    break;
  }
}

void vdp_textcolor(uint8_t fg, uint8_t bg) {

  _fgcolor = fg;

  _bgcolor = bg;

  if (_vdp_mode == VDP_MODE_TEXT)
    setRegister(7, (fg << 4) + bg);
}

void vdp_write(uint8_t chr, bool advanceNextChar) {

  if (chr >= 0x20 && chr <= 0x7d) {

    uint16_t name_offset = cursor.y * (_crsr_max_x + 1) + cursor.x; // Position in name table
    uint16_t pattern_offset = name_offset << 3;                    // Offset of pattern in pattern table

    if (_vdp_mode == VDP_MODE_G2) {

      setWriteAddress(_pattern_table + pattern_offset);

      for (uint8_t i = 0; i < 8; i++)
        writeByteToVRAM(ASCII[((chr - 32) << 3) + i]);

    } else {

      // G1 and text mode
      setWriteAddress(_name_table + name_offset);

      writeByteToVRAM(chr);

      _textBuffer[cursor.y * (_crsr_max_x + 1) + cursor.x] = chr;
    }

    if (advanceNextChar)
      vdp_set_cursor(VDP_CSR_RIGHT);
  }
}

void vdp_writeCharAtLocation(uint8_t x, uint8_t y, uint8_t c) {

  uint16_t name_offset = y * (_crsr_max_x + 1) + x; // Position in name table

  _textBuffer[y * (_crsr_max_x + 1) + x] = c;

  setWriteAddress(_name_table + name_offset);

  writeByteToVRAM(c);
}

uint8_t vdp_getCharAtLocationVRAM(uint8_t x, uint8_t y) {

  uint16_t name_offset = y * (_crsr_max_x + 1) + x; // Position in name table

  setReadAddress(_name_table + name_offset);

  return readByteFromVRAM();
}

uint8_t vdp_getCharAtLocationBuf(uint8_t x, uint8_t y) {

  return _textBuffer[y * (_crsr_max_x + 1) + x];
}

void vdp_setCharAtLocationBuf(uint8_t x, uint8_t y, uint8_t c) {

  _textBuffer[y * (_crsr_max_x + 1) + x] = c;
}

void vdp_ScrollTextUp(uint8_t topRow, uint8_t bottomRow) {

  uint16_t name_offset = topRow * (_crsr_max_x + 1);

  setWriteAddress(_name_table + name_offset);

  for (uint8_t y = topRow; y < bottomRow; y++)
    for (uint8_t x = 0; x < 40; x++) {

      _textBuffer[name_offset] = _textBuffer[name_offset + _crsr_max_x + 1];

      writeByteToVRAM(_textBuffer[name_offset]);

      name_offset++;
    }

  if (bottomRow < _crsr_max_y)
    for (uint8_t x = 0; x < 40; x++) {

      _textBuffer[name_offset] = 0x20;

      writeByteToVRAM(0x20);

      name_offset++;
    }
}

void vdp_ClearRows(uint8_t topRow, uint8_t bottomRow) {

  uint16_t name_offset = topRow * (_crsr_max_x + 1);

  setWriteAddress(_name_table + name_offset);

  for (uint8_t y = topRow; y < bottomRow; y++)
    for (uint8_t x = 0; x < 40; x++) {

      _textBuffer[name_offset] = 0x20;

      writeByteToVRAM(0x20);

      name_offset++;
    }
}

void vdp_writeUInt8(uint8_t v) {

  uint8_t tb[3];
  itoa(v, tb, 10);
  vdp_print(tb);
}

void vdp_writeInt8(int8_t v) {

  uint8_t tb[4];
  itoa(v, tb, 10);
  vdp_print(tb);
}

void vdp_writeUInt8ToBinary(uint8_t v) {

  uint8_t str[9];

  for (uint8_t i = 0; i < 8; i++)
    if (v >> i & 1)
      str[7 - i] = '1';
    else
      str[7 - i] = '0';

  str[8] = 0x00;

  vdp_print(str);
}

int vdp_init_textmode(uint8_t fgcolor, uint8_t bgcolor) {

  return vdp_init(VDP_MODE_TEXT, (fgcolor << 4) | (bgcolor & 0x0f), 0, 0);
}

int vdp_init_g1(uint8_t fgcolor, uint8_t bgcolor) {

  return vdp_init(VDP_MODE_G1, (fgcolor << 4) | (bgcolor & 0x0f), 0, 0);
}

int vdp_init_g2(bool big_sprites, bool scale_sprites) {

  return vdp_init(VDP_MODE_G2, 0x0, big_sprites, scale_sprites);
}

int vdp_init_multicolor() {

  return vdp_init(VDP_MODE_MULTICOLOR, 0, 0, 0);
}
