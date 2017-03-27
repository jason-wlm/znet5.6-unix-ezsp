/**
 * File: hal/micro/generic/diagnostic-stub.c
 * Description: stubs for HAL diagnostic functions.
 *
 * Copyright 2014 Silicon Laboratories, Inc.
 */

#include PLATFORM_HEADER

void halPrintCrashData(int8u port)
{
}

void halPrintCrashDetails(int8u port)
{
}

void halPrintCrashSummary(int8u port)
{
}

void halStartPCDiagnostics(void)
{
}

void halStopPCDiagnostics(void)
{
}

int16u halGetPCDiagnostics(void)
{
  return 0;
}
