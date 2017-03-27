// Copyright 2014 Silicon Laboratories, Inc.
//-----------------------------------------------------------------------------

#include "hal/micro/cortexm3/compiler/iar.h"
#include "stack/include/ember-types.h"
#include "stack/include/event.h"
#include "hal/hal.h"
#include "hal/micro/infrared-led-driver.h"
#include "hal/micro/infrared-led-driver-emit.h"
#include "hal/micro/infrared-led-driver-sird.h"
#include "hal/micro/infrared-led-driver-uird.h"

#define DEFAULT_FRAME_INTERVAL_IN_MS    ( 100)

//****************************************************************************

typedef enum
{
  IDLE,
  START,
  REPEATE,
  STOP,
} STATE_t;

EmberEventControl halInfraredLedEventControl;

static int32u     irStartTimeInMs;
static int32u     irRepeateIntervalInMs = DEFAULT_FRAME_INTERVAL_IN_MS;
static int8u      decodeStatus;

static int8u      *irdPtrCurrent = NULL;
static int8u      irdLenCurrent = 0;
static STATE_t    state = IDLE;
static int8u      repeateCount = 0;
static boolean    reqStart = 0;
static boolean    reqStop = 0;

static int8u      uirdDecryptBuf[HAL_INFRARED_LED_UIRD_DB_STRUCT_SIZE];

//****************************************************************************
// Local functions

// Return the number of milliseconds to the start of the next frame.
static int32u irGetDelayToNextFrameInMs(int32u intervalInMs)
{
  int32u now;
  int32u elapsed;

  now = halCommonGetInt32uMillisecondTick();
  elapsed = elapsedTimeInt32u(irStartTimeInMs, now);
  return (elapsed >= intervalInMs) ? 1 : (intervalInMs - elapsed);
}

// Decode external data.
// The format must be specified in the irDbFormat,
// ird must point to the database and irdLen must specify the length.
static int infraredLedDecodeData(HalInfraredLedDbFormat irDbFormat,
                                 const int8u *ird,
                                 int8u irdLen)
{
  int length;

  decodeStatus = 0;
  halInternalInfraredLedEmitClr();
  if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_UIRD) {
    length = halInfraredLedUirdDecode(ird, irdLen);
    if (length <= 0) {
      decodeStatus = HAL_INFRARED_LED_STATUS_DECODE_ERROR;
    }
  } else if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_UIRD_ENCRYPTED) {
    if (irdLen > HAL_INFRARED_LED_UIRD_DB_STRUCT_SIZE) {
      decodeStatus = HAL_INFRARED_LED_STATUS_INVALID_LENGTH;
    } else {
      // decrypt
      length = halInfraredLedUirdDecrypt(uirdDecryptBuf, ird, irdLen);
      if (length == 0) {
        decodeStatus = HAL_INFRARED_LED_STATUS_DECRYPT_ERROR;
      } else {
        // Decode from the decrypt buffer
        length = halInfraredLedUirdDecode(uirdDecryptBuf, irdLen);
        if (length <= 0) {
          decodeStatus = HAL_INFRARED_LED_STATUS_DECODE_ERROR;
        }
      }
    }
  } else if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_SIRD) {
    length = halInfraredLedsirdDecode(ird, irdLen);
    if (length <= 0) {
      decodeStatus = HAL_INFRARED_LED_STATUS_DECODE_ERROR;
    }
  } else {
    decodeStatus = 1;
  }
  if (decodeStatus != 0) {
    halInternalInfraredLedEmitClr();
  }
  return decodeStatus;
}

// Decode ir data in the local database.
// The format must be specified in the irDbFormat,
// the index is specified in the dbIndex.
static int infraredLedDecodeIndex(HalInfraredLedDbFormat irDbFormat,
                                  int8u dbIndex)
{
  int8u *irdPtr;
  int8u irdLen;

  decodeStatus = HAL_INFRARED_LED_STATUS_OK;
  if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_SIRD) {
    if (dbIndex >= HAL_INFRARED_LED_SIRD_DB_ENTRIES) {
      decodeStatus = HAL_INFRARED_LED_STATUS_INDEX_OUT_OF_RANGE;
    } else {
      irdPtr = ( int8u *)halInfraredLedSirdDb[ dbIndex].pDat;
      irdLen = halInfraredLedSirdDb[ dbIndex].uLen;
    }
  } else if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_UIRD) {
    if (dbIndex >= HAL_INFRARED_LED_UIRD_DB_ENTRIES) {
      decodeStatus = HAL_INFRARED_LED_STATUS_INDEX_OUT_OF_RANGE;
    } else {
      irdPtr = (int8u *)&halInfraredLedUirdDatabase[ dbIndex];
      irdLen = HAL_INFRARED_LED_UIRD_DB_STRUCT_SIZE;
    }
  } else if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_UIRD_ENCRYPTED) {
    if (dbIndex >= HAL_INFRARED_LED_UIRD_ENCR_DB_ENTRIES) {
      decodeStatus = HAL_INFRARED_LED_STATUS_INDEX_OUT_OF_RANGE;
    } else {
      irdPtr = (int8u *)&halInfraredLedUirdEncryptDatabase[dbIndex];
      irdLen = HAL_INFRARED_LED_UIRD_ENCR_DB_STRUCT_SIZE;
    }
  } else {
    decodeStatus = HAL_INFRARED_LED_STATUS_UNKNOWN_FORMAT;
  }
  if (decodeStatus == 0) {
    infraredLedDecodeData(irDbFormat, irdPtr, irdLen);
  }
  return decodeStatus;
}

//****************************************************************************
// Driver interface

// Initialize the driver.
void halInfraredLedInitialize(void)
{
  reqStart = FALSE;
  reqStop = FALSE;
  state = IDLE;
  irdPtrCurrent = 0;
  irdLenCurrent = 0;
  halInternalInfraredLedEmitClr();
}

// This is the state machine that emits one frame at a time according to the
// format in the ir-database. It is controlled by the halInfraredLedStart and
// halInfraredLedStop functions and the event set by itself - only.
void halInfraredLedEventHandler(void)
{
  int32u delayMs;

  emberEventControlSetInactive(halInfraredLedEventControl);

  switch(state) {
  case IDLE:
    if (reqStart) {
      state = START;
      emberEventControlSetDelayMS( halInfraredLedEventControl, 1);
    } else if (reqStop) {
      state = STOP;
      emberEventControlSetDelayMS( halInfraredLedEventControl, 1);
    }
    break;

  case START:
    reqStart = FALSE;
    halInternalInfraredLedEmitPress();
    repeateCount = halInternalInfraredLedEmitHeader.uRepeatCount;
    if (repeateCount > 0) {
      repeateCount--;
    }
    state = REPEATE;
    delayMs = irGetDelayToNextFrameInMs(irRepeateIntervalInMs);
    emberEventControlSetDelayMS(halInfraredLedEventControl, delayMs);
    break;

  case REPEATE:
    if (repeateCount > 0) {
      repeateCount--;
      halInternalInfraredLedEmitRepeat();
      delayMs = irGetDelayToNextFrameInMs( irRepeateIntervalInMs);
      emberEventControlSetDelayMS( halInfraredLedEventControl, delayMs);
    } else if (reqStop) {
      state = STOP;
      emberEventControlSetDelayMS(halInfraredLedEventControl, 1);
    }
    else if (halInternalInfraredLedEmitHeader.bRepeatMode) {
      halInternalInfraredLedEmitRepeat();
      delayMs = irGetDelayToNextFrameInMs(irRepeateIntervalInMs);
      emberEventControlSetDelayMS(halInfraredLedEventControl, delayMs);
    } else {
      state = IDLE;
    }
    break;

  case STOP:
    halInternalInfraredLedEmitRelease();
    halInternalInfraredLedEmitToggleStepToNext();
    reqStop = FALSE;
    state = IDLE;
    irdPtrCurrent = NULL;
    irdLenCurrent = 0;
    break;
  }
}

// Start the transmissions of a sequence of IR frames. The format must
// be specified in the IR database entry.
int halInfraredLedStart(HalInfraredLedDbFormat irDbFormat,
                        const int8u *ird,
                        int8u irdLen)
{
static int8u *irdPtrPrevious;
static int8u irdLenPrevious;
int iSta;

  // Is there a requests in progress?
  if ((irdPtrCurrent != 0) || (irdLenCurrent != 0)) {
    return 1;
  }
  irdPtrCurrent = (int8u *)ird;
  irdLenCurrent = irdLen;
  // Is the current request different from the previous?
  if ((ird != irdPtrPrevious) || (irdLen != irdLenPrevious)) {
    //This is a new format
    irdPtrPrevious = (int8u *)ird;
    irdLenPrevious = irdLen;
    // Always reset the toggle counter for the SIRD format
    if (irDbFormat == HAL_INFRARED_LED_DB_FORMAT_SIRD) {
      halInternalInfraredLedEmitToggleReset();
    }
  }
  if (ird == 0) {
    // If the pointer is 0, irdLen shall be interpreted as the index.
    iSta = infraredLedDecodeIndex(irDbFormat, irdLen);
  } else {
    iSta = infraredLedDecodeData(irDbFormat, ird, irdLen);
  }
  if (iSta == 0) {
    reqStart = TRUE;
    if (state == IDLE) {
      emberEventControlSetActive(halInfraredLedEventControl);
    }
  }
  return iSta;
}

// Stop the ongoing sequence of IR transmissions and transmit the final
// IR frame. The format must be specified in the IR database entry.
int halInfraredLedStop(HalInfraredLedDbFormat irDbFormat,
                       const int8u *ird,
                       int8u irdLen)
{
  int iSta;

  // Is it the current request that is stopped?
  if ((ird != irdPtrCurrent) || (irdLen != irdLenCurrent)) {
    return 1;
  }
  if (ird == 0) {
    // If the pointer is 0, irdLen shall be interpreted as the index.
    iSta = infraredLedDecodeIndex(irDbFormat, irdLen);
  } else {
    iSta = infraredLedDecodeData(irDbFormat, ird, irdLen);
  }
  if (iSta == 0) {
    reqStop = TRUE;
    if (state == IDLE) {
      emberEventControlSetActive(halInfraredLedEventControl);
    }
  }
  return iSta;
}
