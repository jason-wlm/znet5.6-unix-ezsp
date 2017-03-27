// Copyright 2014 Silicon Laboratories, Inc.
#include "hal/micro/cortexm3/compiler/iar.h"
#include "stack/include/ember-types.h"
#include "hal/hal.h"
#include "hal/micro/infrared-led-driver-emit.h"

//****************************************************************************
// Macro definitions

#define TIM_OC1M_LOW   (0x4 << TIM_OC1M_BIT)
#define TIM_OC1M_HIGH  (0x5 << TIM_OC1M_BIT)
#define TIM_OC1M_PWM   (0x6 << TIM_OC1M_BIT)

// These values match the EM3x platforms.
// These compensations can not be larger than 4 without changing the usage.
#define COMPENSATE_MARK_TIME    (3)
#define COMPENSATE_SPACE_TIME   (3)

//****************************************************************************
// Variables

HalInternalInfraredLedEmitHeader_t  halInternalInfraredLedEmitHeader;
HalInternalInfraredLedEmitTiming_t  halInternalInfraredLedEmitTimingBuf[HAL_INFRARED_LED_EMIT_TIMING_BUFFER_SIZE];
int8u                               halInternalInfraredLedEmitTimingCnt;
int8u                               halInternalInfraredLedEmitBitIdxBuf[HAL_INFRARED_LED_EMIT_BITS_BUFFER_SIZE];
HalInternalInfraredLedEmitFrame_t   halInternalInfraredLedEmitFrameBuf[HAL_INFRARED_LED_EMIT_FRAME_TYPE_COUNT];
int8u                               halInternalInfraredLedEmitToggleT;
int8u                               halInternalInfraredLedEmitToggleS;
int8u                               halInternalInfraredLedEmitToggleFirst;
HalInternalInfraredLedEmitFrame_t   halInternalInfraredLedEmitToggleBuf[HAL_INFRARED_LED_EMIT_TOGGLE_SIZE];

static int8u                        toggleOfs;

//****************************************************************************
// Static functions

// Delay the number of microseconds specified by delay.
static void delayMicrosecondsInt32u(int32u delay)
{
    while (delay > 0xFFFF) {
      halCommonDelayMicroseconds(0xFFFF);
      delay -= 0xFFFF;
    }
    halCommonDelayMicroseconds(delay);
}

// initialize the timer
static void initialize(void)
{
  // IR LED is driven by PB6, controlled by timer 1 channel 1
  // Assume SYSCLK is running off of a 24 MHz crystal, so PCLK is 12 MHz
  // disable counter clock
  TIM1_CR1 &= ~TIM_CEN;
  // disable all capture/compare channels
  TIM1_CCER = 0x0;
  // buffer auto-reload (must be on for PWM mode 1) and set rest to default
  TIM1_CR1 = TIM_ARBE | TIM1_CR1_RESET;
  // disable slave mode, reset to default
  TIM1_SMCR = TIM1_SMCR_RESET;
  // set up channel as compare output with buffer
  TIM1_CCMR1 = TIM_OC1M_PWM | TIM_OC1PE;
  // enable compare channel
  TIM1_CCER = TIM_CC1E;
  // set prescaler to give us finest resolution (0.083 microseconds)
  // counter clock CK_CNT is CK_PSC / (2 ^ TIM_PSC)
  TIM1_PSC = 0;
  // zero the reload register, set later based on carrier period
  TIM1_ARR = 0;
  // zero the compare register, set later based on carrier duty cycle
  TIM1_CCR1 = 0;
  // disable interrupts
  INT_TIM1CFG = INT_TIM1CFG_RESET;
  // clear pending interrupts
  INT_TIM1FLAG = INT_TIM1FLAG_RESET;
  // force update of registers and re-initialize counter
  TIM1_EGR |= TIM_UG;
  // place PB6 output under control of timer
  GPIO_PBCFGH = (GPIO_PBCFGH & ~PB6_CFG) | (GPIOCFG_OUT_ALT<<PB6_CFG_BIT);
  // enable counter clock
  TIM1_CR1 |= TIM_CEN;
}

// Set the emitter under manual control.
static void takeManualControl(void)
{
  // place PB6 output under controlled of GPIO_PBOUT
  GPIO_PBCFGH = (GPIO_PBCFGH & ~PB6_CFG) | (GPIOCFG_OUT<<PB6_CFG_BIT);
}

// Check if the emitter is under manual control.
static boolean checkManualControl(void)
{
  return ((GPIO_PBCFGH & PB6_CFG) == (GPIOCFG_OUT << PB6_CFG_BIT));
}

// Called at frame begin. Initializes the timer.
static void frameBegin()
{
  initialize();
  if (halInternalInfraredLedEmitHeader.bUseCarrier) {
    int32u carrierPeriod = halInternalInfraredLedEmitHeader.wCarrierPeriod;
    TIM1_ARR = (carrierPeriod * 12 + 4) / 8 - 1;
  } else {
    // Run at the highest possible frequency
    TIM1_ARR = 1;
  }
  TIM1_EGR |= TIM_UG;
}

// Called at frame end. Turns off the timer.
static void frameEnd()
{
  TIM1_CCR1 = 0;
  TIM1_ARR = 0;
  TIM1_EGR |= TIM_UG;
}

// Emit one mark.
static void emitFrameMark()
{
  if (halInternalInfraredLedEmitHeader.bUseCarrier) {
    TIM1_CCR1 = TIM1_ARR / 3; // 30% carrier duty
  } else {
    // Set the compare register to something higher than the reload register
    TIM1_CCR1 = TIM1_ARR + 1;
  }
}

// Emit one space.
static void emitFrameSpace()
{
  TIM1_CCR1 = 0;
}

// Emit one frame according to the frame definition.
static void emitFrame(HalInternalInfraredLedEmitFrame_t Frame)
{
  int8u  *pBitIdxBuf;
  int8u  uTimIndex;
  static int32u delayMark;
  static int32u delaySpace;

  if (Frame.uBitCount == 0) {
    return;
  }
  pBitIdxBuf = Frame.pBitIdxBuf;
  frameBegin();
  for (; Frame.uBitCount > 0; Frame.uBitCount--) {
    uTimIndex = (pBitIdxBuf != 0) ? *pBitIdxBuf : 0;
    delayMark = halInternalInfraredLedEmitTimingBuf[uTimIndex].mark;
    delaySpace = halInternalInfraredLedEmitTimingBuf[uTimIndex].space;
    // output mark
    if (delayMark > 0) {
      emitFrameMark();
      delayMark *= 4;   // Convert to usec.
      delayMark -= COMPENSATE_MARK_TIME;
      delayMicrosecondsInt32u(delayMark);
    }
    // output space
    if (delaySpace > 0) {
      emitFrameSpace();
      delaySpace *= 4;   // Convert to usec.
      delaySpace -= COMPENSATE_SPACE_TIME;
      delayMicrosecondsInt32u(delaySpace);
    }
    if (pBitIdxBuf != 0) {
      pBitIdxBuf++;
    }
  }
  frameEnd();
}

// Get the repeate frame modified with a toggle frame if the toggle is active.
static HalInternalInfraredLedEmitFrame_t getRepeateFrameModifiedWithToggleFrame()
{
  HalInternalInfraredLedEmitFrame_t frame;
  HalInternalInfraredLedEmitFrame_t toggle;

  frame = halInternalInfraredLedEmitFrameBuf[HAL_INFRARED_LED_EMIT_REPEAT_FRAME];
  if ((halInternalInfraredLedEmitToggleS > 0) && (toggleOfs > 0)) {
    int i;

    toggle = halInternalInfraredLedEmitToggleBuf[toggleOfs - 1];
    for (i=0; i < toggle.uBitCount; i++) {
      frame.pBitIdxBuf[halInternalInfraredLedEmitToggleFirst + i] = toggle.pBitIdxBuf[i];
    }
  }

  return frame;
}

//****************************************************************************
// Global functions

// Clears all variables and hardware used to emit ir.
void halInternalInfraredLedEmitClr(void)
{
  int8u i;

  // Clear variables
  halInternalInfraredLedEmitHeader.bRepeatMode = 0;
  halInternalInfraredLedEmitHeader.bUseCarrier = 0;
  halInternalInfraredLedEmitHeader.bMarkSpace = 0;
  halInternalInfraredLedEmitHeader.uRepeatCount = 0;
  halInternalInfraredLedEmitHeader.wCarrierPeriod = 0;
  halInternalInfraredLedEmitTimingCnt = 0;
  for (i=0; i < HAL_INFRARED_LED_EMIT_TIMING_BUFFER_SIZE; i++) {
    halInternalInfraredLedEmitTimingBuf[i].mark = 0;
    halInternalInfraredLedEmitTimingBuf[i].space = 0;
  }
  for (i = 0; i < 3; i++) {
    halInternalInfraredLedEmitFrameBuf[i].pBitIdxBuf = 0;
    halInternalInfraredLedEmitFrameBuf[i].uBitCount = 0;
  }
  halInternalInfraredLedEmitToggleT = 0;
  halInternalInfraredLedEmitToggleS = 0;
  // Clear hardware
  initialize();
}

// Emits one press frame - if present.
void halInternalInfraredLedEmitPress(void)
{
  HalInternalInfraredLedEmitFrame_t frame;

  if (halInternalInfraredLedEmitFrameBuf[HAL_INFRARED_LED_EMIT_PRESS_FRAME].uBitCount > 0) {
    frame = halInternalInfraredLedEmitFrameBuf[HAL_INFRARED_LED_EMIT_PRESS_FRAME];
  } else {
    frame = getRepeateFrameModifiedWithToggleFrame();
  }
  emitFrame(frame);
}

// Emits one repeate frame - if present
void halInternalInfraredLedEmitRepeat(void)
{
  HalInternalInfraredLedEmitFrame_t frame;

  frame = getRepeateFrameModifiedWithToggleFrame();
  emitFrame(frame);
}

// Emits one release frame - if present
void halInternalInfraredLedEmitRelease(void)
{
  HalInternalInfraredLedEmitFrame_t frame;

  frame = halInternalInfraredLedEmitFrameBuf[HAL_INFRARED_LED_EMIT_RELEASE_FRAME];
  emitFrame( frame);
}

// Resets the toggle counter.
void halInternalInfraredLedEmitToggleReset()
{
  toggleOfs = 0;
}

// Steps the toggle counter to the next toggle.
// If it passes the maximum value, it will start from 0 again.
void halInternalInfraredLedEmitToggleStepToNext()
{
  if (halInternalInfraredLedEmitToggleS > 0) {
    toggleOfs++;
    if (toggleOfs > halInternalInfraredLedEmitToggleS) {
      toggleOfs = 0;
    }
  }
}

//****************************************************************************
// Used by cli

// Set manual control of the emitter.
void halInternalInfraredLedEmitTakeManualControl(void)
{
  takeManualControl();
}

// Check if the emitter is under manual control.
boolean halInternalInfraredLedEmitCheckManualControl(void)
{
  return checkManualControl();
}

