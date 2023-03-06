// ****************************************************************************************
// NABU C Library
// DJ Sures (c) 2022
// https://nabu.ca
// 
// Last updated December 13, 2022
// version 2022.12.13.01
// **********************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <z80.h> // https://github.com/z88dk/z88dk/blob/master/include/_DEVELOPMENT/sdcc/z80.h

inline void nop() {
  __asm
  NOP
    __endasm;
}

void ayWrite(uint8_t reg, uint8_t val) {

  z80_outp(AYLATCH, reg);
  z80_outp(AYDATA, val);
}

uint8_t isKeyPressed() {



  uint8_t status = z80_inp(KEYBOARD + 1);
  uint8_t retval = 0;
  uint8_t inKey = 0;
  if (status & 0x02) {

    inKey = z80_inp(KEYBOARD);

    if (inKey >= 0x90 && inKey <= 0x95)
      retval = 0x00;
    else 
      retval = inKey;
  }

  return retval;;
}

uint8_t getChar() {

  uint8_t retVal = 0;

  while (retVal == 0)
    retVal = isKeyPressed();

  return retVal;
}

void hcca_ReceiveModeStart() {

  z80_outp(AYLATCH, IOPORTA);

  z80_outp(AYDATA, 0x80);
}

bool hcca_IsDataAvailable() {

  z80_outp(AYLATCH, IOPORTB);

  uint8_t r = z80_inp(AYDATA);

  return (r & 0x02) == 0x00;
}

void hcca_ReceiveModeStop() {

  z80_outp(AYLATCH, IOPORTA);

  z80_outp(AYDATA, 0);
}

void hcca_TransmitModeStart() {

  z80_outp(AYLATCH, IOPORTA);

  z80_outp(AYDATA, 0x40);
}

bool hcca_IsTransmitBufferEmpty() {

  z80_outp(AYLATCH, IOPORTB);

  uint8_t r = z80_inp(AYDATA);

  return (r & 64) == 0;
}

void hcca_TransmitModeStop() {

  z80_outp(AYLATCH, IOPORTA);

  z80_outp(AYDATA, 0);
}

void hcca_WriteByte(uint8_t c) {

  hcca_ReceiveModeStop();

  hcca_TransmitModeStart();

  z80_outp(HCCA, c);

  hcca_TransmitModeStop();

  hcca_ReceiveModeStart();
}

void hcca_WriteString(uint8_t* str) {

  for (int i = 0; str[i] != 0x00; i++)
    hcca_WriteByte(str[i]);
}

void hcca_WriteBytes(uint8_t* str, uint8_t len) {

  for (int i = 0; i < len; i++)
    hcca_WriteByte(str[i]);
}

uint8_t hcca_readByte() {

  hcca_ReceiveModeStop();

  int8_t r = z80_inp(HCCA);

  hcca_ReceiveModeStart();

  return r;
}

void beep(int pitch, uint16_t ms) {

  ayWrite(0, pitch);
  ayWrite(1, 1);

  z80_delay_ms(ms);

  ayWrite(0, 0);
  ayWrite(1, 0);
}
