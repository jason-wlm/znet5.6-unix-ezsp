// Copyright 2013 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"
#include "app/framework/util/af-event.h"
#include "idle-sleep.h"

#ifdef EZSP_HOST
  #define MAX_SLEEP_VALUE_MS MAX_TIMER_MILLISECONDS_HOST
#else
  #define MAX_SLEEP_VALUE_MS 0xFFFFFFFFUL
#endif

#ifdef EMBER_AF_PLUGIN_IDLE_SLEEP_STAY_AWAKE_WHEN_NOT_JOINED
  #define STAY_AWAKE_WHEN_NOT_JOINED_DEFAULT TRUE
#else
  #define STAY_AWAKE_WHEN_NOT_JOINED_DEFAULT FALSE
#endif
boolean emAfStayAwakeWhenNotJoined = STAY_AWAKE_WHEN_NOT_JOINED_DEFAULT;

boolean emAfForceEndDeviceToStayAwake = FALSE;

// NO PRINTFS.  This may be called in ISR context.
void emberAfForceEndDeviceToStayAwake(boolean stayAwake)
{
  emAfForceEndDeviceToStayAwake = stayAwake;
}

#ifdef EMBER_AF_PLUGIN_IDLE_SLEEP_USE_BUTTONS
void emberAfHalButtonIsrCallback(int8u button, int8u state)
{
  if (state == BUTTON_PRESSED) {
    emberAfForceEndDeviceToStayAwake(button == BUTTON0);
  }
}
#endif

#ifdef EMBER_TEST
  extern boolean doingSerialTx[];
  #define simulatorDoingSerialTx(port) doingSerialTx[port]
#else
  #define simulatorDoingSerialTx(port) FALSE
#endif

boolean emAfOkToIdleOrSleep(void)
{
  int8u i;

  if (emAfForceEndDeviceToStayAwake) {
    return FALSE;
  }

  if (emAfStayAwakeWhenNotJoined) {
    boolean awake = FALSE;
    for (i = 0; !awake && i < EMBER_SUPPORTED_NETWORKS; i++) {
      if (emberAfPushNetworkIndex(i) == EMBER_SUCCESS) {
        awake = (emberAfNetworkState() != EMBER_JOINED_NETWORK);
        emberAfPopNetworkIndex();
      }
    }
    if (awake) {
      return FALSE;
    }
  }

#ifdef EM_NUM_SERIAL_PORTS
  for (i = 0; i < EM_NUM_SERIAL_PORTS; i++) {
    if (!emberSerialUnused(i)
        && (emberSerialReadAvailable(i) != 0
            || emberSerialWriteUsed(i) != 0
            || simulatorDoingSerialTx(i))) {
      return FALSE;
    }
  }
#else
  if (!emberSerialUnused(APP_SERIAL)
      && (emberSerialReadAvailable(APP_SERIAL) != 0
          || emberSerialWriteUsed(APP_SERIAL) != 0
          || simulatorDoingSerialTx(APP_SERIAL))) {
    return FALSE;
  }
#endif

  return (emberAfGetCurrentSleepControlCallback() != EMBER_AF_STAY_AWAKE);
}


static EmberAfEventSleepControl defaultSleepControl = EMBER_AF_OK_TO_SLEEP;

EmberAfEventSleepControl emberAfGetCurrentSleepControlCallback(void)
{
  EmberAfEventSleepControl sleepControl = defaultSleepControl;
#ifdef EMBER_AF_GENERATED_EVENT_CONTEXT
  int8u i;
  for (i = 0; i < emAfAppEventContextLength; i++) {
    EmberAfEventContext *context = &emAfAppEventContext[i];
    if (emberEventControlGetActive(*context->eventControl)
        && sleepControl < context->sleepControl) {
      sleepControl = context->sleepControl;
    }
  }
#endif
  return sleepControl;
}

EmberAfEventSleepControl emberAfGetDefaultSleepControlCallback(void)
{
  return defaultSleepControl;
}

void emberAfSetDefaultSleepControlCallback(EmberAfEventSleepControl sleepControl)
{
  defaultSleepControl = sleepControl;
}
