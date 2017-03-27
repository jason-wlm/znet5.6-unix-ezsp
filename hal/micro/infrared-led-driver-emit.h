// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __INFRARED_LED_DRIVER_EMIT_H__
#define __INFRARED_LED_DRIVER_EMIT_H__

#include PLATFORM_HEADER


#define HAL_INFRARED_LED_EMIT_TIMING_BUFFER_SIZE       ( 16)
#define HAL_INFRARED_LED_EMIT_BITS_BUFFER_SIZE         ( 200)
#define HAL_INFRARED_LED_EMIT_TOGGLE_SIZE              ( 5)
#define HAL_INFRARED_LED_EMIT_FRAME_TYPE_COUNT         ( 3)


enum {
  HAL_INFRARED_LED_EMIT_PRESS_FRAME = 0,
  HAL_INFRARED_LED_EMIT_REPEAT_FRAME,
  HAL_INFRARED_LED_EMIT_RELEASE_FRAME
};

// Header structure
typedef struct {
  int8u   bRepeatMode;
  int8u   bUseCarrier;
  int8u   bMarkSpace;
  int8u   uRepeatCount;
  int16u  wCarrierPeriod;
} HalInternalInfraredLedEmitHeader_t;

// Timing buffer structure
typedef struct {
  int16u mark;
  int16u space;
} HalInternalInfraredLedEmitTiming_t;

// Frame data structure
typedef struct {
  int8u *pBitIdxBuf;
  int8u  uBitCount;
} HalInternalInfraredLedEmitFrame_t;


extern HalInternalInfraredLedEmitHeader_t   halInternalInfraredLedEmitHeader;
extern HalInternalInfraredLedEmitTiming_t   halInternalInfraredLedEmitTimingBuf[ HAL_INFRARED_LED_EMIT_TIMING_BUFFER_SIZE];
extern int8u                                halInternalInfraredLedEmitTimingCnt;
extern int8u                                halInternalInfraredLedEmitBitIdxBuf[ HAL_INFRARED_LED_EMIT_BITS_BUFFER_SIZE];
extern HalInternalInfraredLedEmitFrame_t    halInternalInfraredLedEmitFrameBuf[ HAL_INFRARED_LED_EMIT_FRAME_TYPE_COUNT];
extern int8u                                halInternalInfraredLedEmitToggleT;
extern int8u                                halInternalInfraredLedEmitToggleS;
extern int8u                                halInternalInfraredLedEmitToggleFirst;
extern HalInternalInfraredLedEmitFrame_t    halInternalInfraredLedEmitToggleBuf[ HAL_INFRARED_LED_EMIT_TOGGLE_SIZE];


void halInternalInfraredLedEmitClr( void);
void halInternalInfraredLedEmitPress(void);
void halInternalInfraredLedEmitRepeat(void);
void halInternalInfraredLedEmitRelease(void);
void halInternalInfraredLedEmitToggleReset();
void halInternalInfraredLedEmitToggleStepToNext();

void    halInternalInfraredLedEmitTakeManualControl(void);
boolean halInternalInfraredLedEmitCheckManualControl(void);


#endif // __INFRARED_LED_DRIVER_EMIT_H__
