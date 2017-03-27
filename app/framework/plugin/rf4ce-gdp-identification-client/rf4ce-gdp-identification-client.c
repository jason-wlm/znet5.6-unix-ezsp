// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"

// This plugin does not have a separate -cli.c file, so this error is here.
#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE GDP Identification Client plugin is not compatible with the legacy CLI.
#endif

#ifndef EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT
  #error The RF4CE GDP Identification Client plugin can only be used on devices configured as identification clients.
#endif

// These are duplicated from EmberAfRf4ceGdpClientNotificationIdentifyFlags,
// but in the form of #defines so they can be used by the preprocessor.
#define FLAG_FLASH_LIGHT 0x02
#define FLAG_MAKE_SOUND  0x04
#define FLAG_VIBRATE     0x08

#if (EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_CAPABILITIES & FLAG_FLASH_LIGHT)
  #if defined(EMBER_AF_API_LED) && !defined(UNIX_HOST)
    #ifdef BOARD_GDP_IDENTIFICATION_LED
      #define LED BOARD_GDP_IDENTIFICATION_LED
    #elif HAL_HOST
      #define LED BOARDLED0
    #else
      #define LED BOARDLED2
    #endif
    #define FLASH_LIGHT()         halToggleLed(LED)
    #define STOP_FLASHING_LIGHT() halClearLed(LED)
  #else
    #define FLASH_LIGHT()         emberAfCorePrintln("~FLASH~")
    #define STOP_FLASHING_LIGHT()
  #endif
#else
  #define FLASH_LIGHT()
  #define STOP_FLASHING_LIGHT()
#endif

#if (EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_CAPABILITIES & FLAG_MAKE_SOUND)
  #ifdef EMBER_AF_API_BUZZER
    static int8u PGM beep[] = {
      NOTE_B4, 1,
      0,       0,
    };
    // TODO: The tune cannot start with a pause (i.e., {0, x}), even if the
    // pause is meant to serve as terminator (i.e., {0, 0}).  Instead, start
    // with a real note, but with a duration of zero, to silence the buzzer.
    static int8u PGM silence[] = {
      NOTE_B4, 0,
      0,       0,
    };
    #define MAKE_SOUND()        halPlayTune_P(beep,    TRUE)
    #define STOP_MAKING_SOUND() halPlayTune_P(silence, TRUE)
  #else
    #define MAKE_SOUND()        emberAfCorePrintln("~BEEP~")
    #define STOP_MAKING_SOUND()
  #endif
#else
  #define MAKE_SOUND()
  #define STOP_MAKING_SOUND()
#endif

#if (EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_CAPABILITIES & FLAG_VIBRATE)
  #define VIBRATE()        emberAfCorePrintln("~VIBRATE~")
  #define STOP_VIBRATING()
#else
  #define VIBRATE()
  #define STOP_VIBRATING()
#endif

EmberEventControl emberAfPluginRf4ceGdpIdentificationClientActionEventControl;
EmberEventControl emberAfPluginRf4ceGdpIdentificationClientTimeoutEventControl;

static boolean identifyStopOnAction = FALSE;
static boolean identifyFlashLight = FALSE;
static boolean identifyMakeSound = FALSE;
static boolean identifyVibrate = FALSE;

void emberAfRf4ceGdpIdentificationClientDetectedUserInteraction(void)
{
  if (identifyStopOnAction) {
    emberEventControlSetActive(emberAfPluginRf4ceGdpIdentificationClientTimeoutEventControl);
  }
}

void emberAfPluginRf4ceGdpIdentifyCallback(EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                           int16u timeS)
{
  identifyStopOnAction = READBITS(flags, EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_STOP_ON_ACTION);
  identifyFlashLight = READBITS(flags, EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_FLASH_LIGHT);
  identifyMakeSound = READBITS(flags, EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_MAKE_SOUND);
  identifyVibrate = READBITS(flags, EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_VIBRATE);

  if (timeS == 0x0000) {
    emberEventControlSetActive(emberAfPluginRf4ceGdpIdentificationClientTimeoutEventControl);
  } else {
    // Note that sixteen bits for seconds means the delay will always be less
    // than the maximum permissable duration, but, to be safe, cap it anyway.
    int32u timeMs = timeS * MILLISECOND_TICKS_PER_SECOND;
    if (EMBER_MAX_EVENT_CONTROL_DELAY_MS < timeMs) {
      timeMs = EMBER_MAX_EVENT_CONTROL_DELAY_MS;
    }
    emberEventControlSetActive(emberAfPluginRf4ceGdpIdentificationClientActionEventControl);
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpIdentificationClientTimeoutEventControl,
                                timeMs);
  }
}

void emberAfPluginRf4ceGdpIdentificationClientActionEventHandler(void)
{
  if (identifyFlashLight) {
    FLASH_LIGHT();
  }
  if (identifyMakeSound) {
    MAKE_SOUND();
  }
  if (identifyVibrate) {
    VIBRATE();
  }
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpIdentificationClientActionEventControl,
                              MILLISECOND_TICKS_PER_SECOND);
}

void emberAfPluginRf4ceGdpIdentificationClientTimeoutEventHandler(void)
{
  identifyStopOnAction = FALSE;
  if (identifyFlashLight) {
    STOP_FLASHING_LIGHT();
    identifyFlashLight = FALSE;
  }
  if (identifyMakeSound) {
    STOP_MAKING_SOUND();
    identifyMakeSound = FALSE;
  }
  if (identifyVibrate) {
    STOP_VIBRATING();
    identifyVibrate = FALSE;
  }
  emberEventControlSetInactive(emberAfPluginRf4ceGdpIdentificationClientActionEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpIdentificationClientTimeoutEventControl);
}
