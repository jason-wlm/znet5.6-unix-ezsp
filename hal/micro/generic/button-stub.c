/**
 * File: hal/micro/generic/button-stub.c
 * Description: stub implementation of button functions.
 *
 * Copyright 2015 Silicon Laboratories, Inc.
 */

#include PLATFORM_HEADER
#include "stack/include/ember-types.h"
#include "hal/hal.h"

void halInternalInitButton(void)
{
}

int8u halButtonState(int8u button)
{
  return BUTTON_RELEASED;
}

int8u halButtonPinState(int8u button)
{
  return BUTTON_RELEASED;
}

int16u halGpioIsr(int16u interrupt, int16u pcbContext)
{
  return 0;
}

int16u halTimerIsr(int16u interrupt, int16u pcbContext)
{
  return 0;
}

void simulatedButtonIsr(int8u button, boolean isPress)
{
}
