// *******************************************************************
// * simple-main.c
// *
// * This file provides a definition for main() for non-RTOS applications.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                                 *80*
// *******************************************************************

#include PLATFORM_HEADER
#include "app/framework/include/af.h"

int main(MAIN_FUNCTION_PARAMETERS)
{
  halInit();
  return emberAfMain(MAIN_FUNCTION_ARGUMENTS);
}

