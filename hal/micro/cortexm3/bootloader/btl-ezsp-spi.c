/*
 * File: btl-ezsp-spi.c
 * Description: em35x bootloader serial interface functions for ezsp spi
 *
 *
 * Copyright 2009 by Ember Corporation. All rights reserved.                *80*
 */

#include PLATFORM_HEADER  // should be iar.h
#include "bootloader-common.h"
#include "bootloader-gpio.h"
#include "bootloader-serial.h"
#include "btl-ezsp-spi-protocol.h"
#include "stack/include/ember-types.h"
#include "hal.h"

boolean spipFramePending;       // halHostCommandBuffer[] contains a frame
boolean spipResponsePending;    // have data to send in next spi session
int8u preSpiComBufIndex;        // index for reading from halHostCommandBuffer[]

void serInit(void)
{
  halHostSerialInit();
}

void serPutFlush(void)
{
  //stub
}
void serPutChar(int8u ch)
{
  //stub
  (void)ch;
}
void serPutStr(const char *str)
{
  //stub
  (void)*str;
}
void serPutBuf(const int8u buf[], int8u size)
{
  //stub
  (void)*buf; (void)size;
}
void serPutHex(int8u byte)
{
  //stub
  (void)byte;
}
void serPutHexInt(int16u word)
{
  //stub
  (void)word;
}

// get char if available, else return error
BL_Status serGetChar(int8u* ch)
{
  if (serCharAvailable()) {
    *ch = halHostCommandBuffer[XMODEM_FRAME_START_INDEX + preSpiComBufIndex];
    preSpiComBufIndex++;
    return BL_SUCCESS;
  } else {
    return BL_ERR;
  }
}

boolean serCharAvailable(void)
{
  if (spipFramePending) {           // already have frame?
    return TRUE;
  }
  if (!halHostSerialTick(FALSE)) {  // see if a frame was just received
    return FALSE;
  }
  if (spipResponsePending) {        // need to send a response?
    halHostCallback(FALSE);         // send it at once
    halHostSerialTick(TRUE);
    spipResponsePending = FALSE;
    return FALSE;
  }
  spipFramePending = TRUE;          // flag frame available
  preSpiComBufIndex = 0;            // and set read index to the frame start
  return TRUE;
}

void serGetFlush(void)
{
  //stub
}
