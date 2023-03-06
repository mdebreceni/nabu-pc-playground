#ifndef NABU_H
#define NABU_H

#define AYDATA   0x40
#define AYLATCH  0x41
#define HCCA     0x80
#define KEYBOARD 0x90
#define IOPORTA  0x0e
#define IOPORTB  0x0f

inline void nop();

void ayWrite(uint8_t reg, uint8_t val);

uint8_t isKeyPressed();

uint8_t getChar();

/// <summary>
/// Initializes the HCCA interrupt to know when there is data to read
/// </summary>
void hcca_ReceiveModeStart();

/// <summary>
/// Always check if there is data before calling hcca_readbyte()
/// </summary>
bool hcca_IsDataAvailable();

void hcca_ReceiveModeStop();

void hcca_TransmitModeStart();

bool hcca_IsTransmitBufferEmpty();

void hcca_TransmitModeStop();

void hcca_WriteByte(uint8_t c);

void hcca_WriteString(uint8_t* str);

void hcca_WriteBytes(uint8_t* str, uint8_t len);

uint8_t hcca_readByte();

void beep(int pitch, uint16_t ms);

#include "nabu.c"

#endif
