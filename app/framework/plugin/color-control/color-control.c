// *******************************************************************
// * color-control.c
// *
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

typedef struct {
  boolean active;
  int8u commandId;
  boolean hueMoveDirection; // TRUE for up
  boolean satMoveDirection; // TRUE for up
  boolean acceleratedHue; // Used for HueAndSat; true if Hue move > Sat move
  int8u acceleratedMoveRate;
  int8u hueMoveToLevel;
  int8u saturationMoveToLevel;
  int32u eventDuration; // All time fields in milliseconds
  int32u elapsedTime;
  int32u transitionTime;
} ColorControlState;

static ColorControlState stateTable[EMBER_AF_COLOR_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT];

static ColorControlState *getColorControlState(int8u endpoint);

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
static void colorControlClearRemainingTime(int8u endpoint);
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE

static void colorControlSetHue(int8u endpoint, int8u hue);
static void colorControlSetSaturation(int8u endpoint, int8u saturation);
static void colorControlSetColorModeToZero(void);
static EmberAfStatus colorControlReadCurrentHue(int8u endpoint, int8u* hue);
static EmberAfStatus colorControlReadCurrentSaturation(int8u endpoint,
                                                       int8u* saturation);
static boolean colorControlHueAndSaturationHandler(int8u endpoint);

static ColorControlState *getColorControlState(int8u endpoint)
{
  int8u index = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_COLOR_CONTROL_CLUSTER_ID);
  return (index == 0xFF ? NULL : &stateTable[index]);
}

void emberAfColorControlClusterServerInitCallback(int8u endpoint)
{
  ColorControlState *state = getColorControlState(endpoint);
  if (state == NULL) {
    return;
  }

  state->active = FALSE;
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  colorControlClearRemainingTime(endpoint);
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
}

void emberAfColorControlClusterServerTickCallback(int8u endpoint)
{
  ColorControlState *state = getColorControlState(endpoint);
  EmberAfStatus status;
  int8u hue, sat;
  boolean hueUp = state->hueMoveDirection;
  boolean satUp = state->satMoveDirection;

  if (state == NULL) {
    return;
  }

  status = colorControlReadCurrentHue(endpoint, &hue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: could not read current hue %x", 
                                      status);
    return;
  }
  status = colorControlReadCurrentSaturation(endpoint, &sat);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: could not read current saturation %x", 
                                      status);
    return;
  }

  switch(state->commandId) {
    case ZCL_MOVE_TO_HUE_AND_SATURATION_COMMAND_ID:
      state->active = colorControlHueAndSaturationHandler(endpoint);
      break;
    case ZCL_STEP_HUE_COMMAND_ID:
    case ZCL_MOVE_TO_HUE_COMMAND_ID:
    case ZCL_MOVE_HUE_COMMAND_ID:
      hue = hueUp ? hue + 1 : hue - 1;
      if (hue == 0xFF) {
        hue = hueUp ? 0x00 : 0xFE;
      }
      colorControlSetHue(endpoint, hue);
      if (state->commandId != ZCL_MOVE_HUE_COMMAND_ID) {
        state->active = (hue != state->hueMoveToLevel);
      }
      break;
    case ZCL_STEP_SATURATION_COMMAND_ID:
    case ZCL_MOVE_TO_SATURATION_COMMAND_ID:
    case ZCL_MOVE_SATURATION_COMMAND_ID:
      sat = satUp ? sat + 1 : sat - 1;
      if (sat == 0xFF) {
        sat = satUp ? 0xFE : 0x00;
      }
      colorControlSetSaturation(endpoint, sat);
      if (state->commandId != ZCL_MOVE_SATURATION_COMMAND_ID) {
        state->active = (sat != state->saturationMoveToLevel
                         || sat == 0x00
                         || sat == 0xFE);
      }
      break;
    default:
      emberAfColorControlClusterPrintln("ERR: unknown color control command.");
      return;
  }

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  {
    int16u remainingTime;

    state->elapsedTime = (state->active 
                          ? state->elapsedTime + state->eventDuration
                          : state->transitionTime); 

    // If we're done, we should clear the remaining time
    if (state->elapsedTime == state->transitionTime) {
      colorControlClearRemainingTime(endpoint);
      return;
    }

    // The remainingTime attribute expects values in tenths of seconds;
    // we maintain in milliseconds
    remainingTime = state->elapsedTime / 100;
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_COLOR_CONTROL_CLUSTER_ID,
                                         ZCL_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                         (int8u *)&remainingTime,
                                         ZCL_INT16U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {                                                                             
      emberAfColorControlClusterPrintln("ERR: writing remaining time %x", status);                                        
      return;                                                                                                             
    }    
  }
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE

  if (!state->active) {
    return;
  }

  //schedule the next tick
  emberAfScheduleServerTick(endpoint,
                            ZCL_COLOR_CONTROL_CLUSTER_ID,
                            state->eventDuration);
}

// Clear remaining time
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
static void colorControlClearRemainingTime(int8u endpoint)
{
  int16u data = 0xFFFF;
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                                                     ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                     ZCL_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                                     (int8u *)&data,
                                                     ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing remaining time %x", status);
    return;
  }
}
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE

// Sets the hue attribute
static void colorControlSetHue(int8u endpoint, int8u hue)
{
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                                                     ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                     ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID,
                                                     (int8u *)&hue,
                                                     ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing current hue %x", status); emberAfColorControlClusterFlush();
    return;
  }
  emberAfDebugPrintln("hue=%x", hue);
}

// Sets the saturation attribute
static void colorControlSetSaturation(int8u endpoint, int8u saturation)
{
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                                                     ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                     ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID,
                                                     (int8u *)&saturation,
                                                     ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing current saturation %x",
                                      status);
    return;
  }
  emberAfDebugPrintln("saturation=%x", saturation); 
}

static void colorControlSetColorModeToZero(void)
{
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE
  // Set the optional Color Mode attribute to zero as per the spec.  If an
  // error occurs, log it, but ignore it.
  int8u colorMode = 0;
  EmberAfStatus status = emberAfWriteServerAttribute(emberAfCurrentEndpoint(),
                                                     ZCL_COLOR_CONTROL_CLUSTER_ID, 
                                                     ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID, 
                                                     &colorMode,
                                                     ZCL_ENUM8_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing color mode%x", status);
  }
  emberAfDebugPrintln("colorMode=%x", colorMode); emberAfColorControlClusterFlush();
#else
  emberAfDebugPrintln("no color mode attribute"); emberAfColorControlClusterFlush();
#endif //ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE
}

// returns a ZCL status, EMBER_ZCL_STATUS_SUCCESS when it works. Sets newHue to the
// current Hue attribute value
static EmberAfStatus colorControlReadCurrentHue(int8u endpoint, int8u* hue)
{
  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                                                    ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                    ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID,
                                                    hue,
                                                    sizeof(int8u));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: reading current hue %x", status);
  }
  return status;
}

// returns a ZCL status, EMBER_ZCL_STATUS_SUCCESS when it works. Sets newHue to the
// current Hue attribute value
static EmberAfStatus colorControlReadCurrentSaturation(int8u endpoint,
                                                       int8u* saturation)
{
  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                                                    ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                    ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID,
                                                    saturation,
                                                    sizeof(int8u));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: reading current saturation %x",
                                      status);
  }
  return status;
}

/****************** COMMAND HANDLERS **********************/

// Move hue to a given hue, taking transitionTime until completed.
boolean emberAfColorControlClusterMoveToHueCallback(int8u hue,
                                                    int8u direction,
                                                    int16u transitionTime)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int8u currentHue, currentSaturation;

  emberAfColorControlClusterPrintln("ColorControl: MoveToHue (%x, %x, %2x)",
                                    hue,
                                    direction,
                                    transitionTime);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the color specified is not achievable by the hardware, then the
  // color shall not be set and a ZCL default response command shall be
  // generated with status code equal to INVALID_VALUE.
  status = colorControlReadCurrentSaturation(emberAfCurrentEndpoint(), 
                                             &currentSaturation);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }
  if(!emberAfPluginColorControlIsColorSupportedCallback(hue, currentSaturation)) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto send_default_response;
  }

  status = colorControlReadCurrentHue(emberAfCurrentEndpoint(), &currentHue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  // Nothing to do, prevents divide-by-zero
  if ( hue == currentHue ) {
    status = EMBER_ZCL_STATUS_SUCCESS;
    goto send_default_response;
  }

  // As hue is effectively measured on a circle, the new hue may be moved to in
  // either direction.  The direction of hue change is given by the Direction
  // field. If Directionis "Shortest distance," the direction is taken that
  // involves the shortest path round the circle.  This case corresponds to
  // expected normal usage.  If Direction is "Longest distance," the direction
  // is taken that involves the longest path round the circle.  This case can be
  // used for "rainbow effects."  In both cases, if both distances are the same,
  // the Up direction shall be taken.
  switch (direction) {
    case EMBER_ZCL_HUE_DIRECTION_SHORTEST_DISTANCE:
      state->hueMoveDirection = ((int8u) (hue - currentHue) <= 127 ? TRUE : FALSE);
      break;
    case EMBER_ZCL_HUE_DIRECTION_LONGEST_DISTANCE:
      state->hueMoveDirection = ((int8u) (hue - currentHue) >= 127 ? TRUE : FALSE);
      break;
    case EMBER_ZCL_HUE_DIRECTION_UP:
      state->hueMoveDirection = TRUE;
      break;
    case EMBER_ZCL_HUE_DIRECTION_DOWN:
      state->hueMoveDirection = FALSE;
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  state->commandId = ZCL_MOVE_TO_HUE_COMMAND_ID;
  state->elapsedTime = 0;
  state->hueMoveToLevel = hue;
  state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10; 
  state->eventDuration = state->transitionTime / ((state->hueMoveDirection) ? 
                                                  (hue - currentHue) : 
                                                  (currentHue - hue));
  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  // Schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  state->active = TRUE;
  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}

// Move hue continuously at the given rate. If mode is stop, then stop.
boolean emberAfColorControlClusterMoveHueCallback(int8u moveMode, int8u rate)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;

  emberAfColorControlClusterPrintln("ColorControl: MoveHue (%x, %x)",
                                    moveMode,
                                    rate);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the rate is set to 0, then the command shall have no effect
  // and a ZCL default response command shall be generated with status 
  // code equal to INVALID_VALUE.

  if (rate == 0) {
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto send_default_response;
  }

  state->active = TRUE;

  switch (moveMode) {
    case EMBER_ZCL_HUE_MOVE_MODE_STOP:
      state->active = FALSE;
      emberAfDeactivateServerTick(emberAfCurrentEndpoint(),
                                  ZCL_COLOR_CONTROL_CLUSTER_ID);
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
      colorControlClearRemainingTime(emberAfCurrentEndpoint());
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
      status = EMBER_ZCL_STATUS_SUCCESS;
      goto send_default_response;
    case EMBER_ZCL_HUE_MOVE_MODE_UP:
      state->hueMoveDirection = TRUE;
      break;
    case EMBER_ZCL_HUE_MOVE_MODE_DOWN:
      state->hueMoveDirection = FALSE;
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  state->commandId = ZCL_MOVE_HUE_COMMAND_ID;
  if (state->active) {
    state->eventDuration = MILLISECOND_TICKS_PER_SECOND / rate;
  }

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

// Step hue by one step, taking time as specified.
boolean emberAfColorControlClusterStepHueCallback(int8u stepMode,
                                                  int8u stepSize,
                                                  int8u transitionTime)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int8u currentHue, currentSaturation;

  emberAfColorControlClusterPrintln("ColorControl: StepHue (%x, %x, %x)",
                                    stepMode,
                                    stepSize,
                                    transitionTime);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  status = colorControlReadCurrentHue(emberAfCurrentEndpoint(), &currentHue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  // Add or subtract the step size to/from the current hue to get the final hue.
  // However, the range for hues is 0x00 to 0xFE, so if we land on 0xFF or if we
  // roll past it, we have to add or subtract one to the final hue, effectively
  // skipping over the invalid 0xFF.
  switch (stepMode) {
    case EMBER_ZCL_HUE_STEP_MODE_UP:
      state->hueMoveToLevel = currentHue + stepSize;
      if (stepSize == 0xFF - currentHue) {
        state->hueMoveToLevel++;
      }
      state->hueMoveDirection = TRUE;
      break;
    case EMBER_ZCL_HUE_STEP_MODE_DOWN:
      state->hueMoveToLevel = currentHue - stepSize;
      if (stepSize == currentHue - 1) {
        state->hueMoveToLevel--;
      }
      state->hueMoveDirection = FALSE;
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  // If the color specified is not achievable by the hardware, then the
  // color shall not be set and a ZCL default response command shall be
  // generated with status code equal to INVALID_VALUE.
  status = colorControlReadCurrentSaturation(emberAfCurrentEndpoint(), 
                                             &currentSaturation);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  if(!emberAfPluginColorControlIsColorSupportedCallback(state->hueMoveToLevel, 
                                                        currentSaturation)
     || stepSize == 0 ) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto send_default_response;
  }

  state->commandId = ZCL_STEP_HUE_COMMAND_ID;
  state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10; 
  state->elapsedTime = 0;
  state->eventDuration = state->transitionTime / stepSize;

  state->active = TRUE;

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

// Move saturation to a given saturation, taking transitionTime until completed.
boolean emberAfColorControlClusterMoveToSaturationCallback(int8u saturation,
                                                           int16u transitionTime)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int8u currentHue, currentSaturation;

  emberAfColorControlClusterPrintln("ColorControl: MoveToSaturation (%x, %2x)",
                                    saturation,
                                    transitionTime);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the color specified is not achievable by the hardware, then the
  // color shall not be set and a ZCL default response command shall be
  // generated with status code equal to INVALID_VALUE.
  status = colorControlReadCurrentHue(emberAfCurrentEndpoint(), 
                                      &currentHue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }
  if(!emberAfPluginColorControlIsColorSupportedCallback(currentHue, saturation)) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto send_default_response;
  }

  status = colorControlReadCurrentSaturation(emberAfCurrentEndpoint(),
                                             &currentSaturation);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  // Nothing to do, prevent divide-by-zero
  if ( saturation == currentSaturation ) {
    status = EMBER_ZCL_STATUS_SUCCESS;
    goto send_default_response;
  }

  state->commandId = ZCL_MOVE_TO_SATURATION_COMMAND_ID;
  state->elapsedTime = 0;
  state->saturationMoveToLevel = saturation;
  state->satMoveDirection = 
    (saturation > currentSaturation ? TRUE : FALSE);
  state->eventDuration = state->transitionTime / ((state->satMoveDirection) ? 
                                                  (saturation - currentSaturation) : 
                                                  (currentSaturation - saturation));

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

// Move sat continuously at the given rate. If mode is stop, then stop.
boolean emberAfColorControlClusterMoveSaturationCallback(int8u moveMode, int8u rate)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;

  emberAfColorControlClusterPrintln("ColorControl: MoveSaturation (%x, %x)",
                                    moveMode,
                                    rate);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the rate is set to 0, then the command shall have no effect
  // and a ZCL default response command shall be generated with status 
  // code equal to INVALID_VALUE.

  if (rate == 0) {
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto send_default_response;
  }

  state->active = TRUE;

  switch (moveMode) {
    case EMBER_ZCL_SATURATION_MOVE_MODE_STOP:
      state->active = FALSE;
      emberAfDeactivateServerTick(emberAfCurrentEndpoint(),
                                  ZCL_COLOR_CONTROL_CLUSTER_ID);
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
      colorControlClearRemainingTime(emberAfCurrentEndpoint());
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
      status = EMBER_ZCL_STATUS_SUCCESS;
      goto send_default_response;
    case EMBER_ZCL_SATURATION_MOVE_MODE_UP:
      state->satMoveDirection = TRUE;
      break;
    case EMBER_ZCL_SATURATION_MOVE_MODE_DOWN:
      state->satMoveDirection = FALSE;
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  state->commandId = ZCL_MOVE_SATURATION_COMMAND_ID;
  if (state->active) {
    state->eventDuration = MILLISECOND_TICKS_PER_SECOND / rate;
  }

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;
  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

// Step sat by one step, taking time as specified.
boolean emberAfColorControlClusterStepSaturationCallback(int8u stepMode,
                                                         int8u stepSize,
                                                         int8u transitionTime)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int8u currentHue, currentSaturation;

  emberAfColorControlClusterPrintln("ColorControl: StepSaturation (%x, %x, %x)",
                                    stepMode,
                                    stepSize,
                                    transitionTime);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  status = colorControlReadCurrentSaturation(emberAfCurrentEndpoint(),
                                             &currentSaturation);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  // Add or subtract the step size to/from the current saturation to get the
  // final saturation.  However, the range for saturations is 0x00 to 0xFE, so
  // don't increment or decrement past those bounds.
  switch (stepMode) {
    case EMBER_ZCL_SATURATION_STEP_MODE_UP:
      state->saturationMoveToLevel = (stepSize > 0xFE - currentSaturation
                                      ? 0xFE
                                      : currentSaturation + stepSize);
      state->satMoveDirection = TRUE;
      break;
    case EMBER_ZCL_SATURATION_STEP_MODE_DOWN:
      state->saturationMoveToLevel = (stepSize > currentSaturation
                                      ? 0x00
                                      : currentSaturation - stepSize);
      state->satMoveDirection = FALSE;
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  // If the color specified is not achievable by the hardware, then the
  // color shall not be set and a ZCL default response command shall be
  // generated with status code equal to INVALID_VALUE.
  status = colorControlReadCurrentHue(emberAfCurrentEndpoint(), 
                                      &currentHue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  if(!emberAfPluginColorControlIsColorSupportedCallback(currentHue, 
                                                        state->saturationMoveToLevel)
     || stepSize == 0 ) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto send_default_response;
  }

  state->commandId = ZCL_STEP_SATURATION_COMMAND_ID;
  state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10;
  state->elapsedTime = 0;
  state->eventDuration = state->transitionTime / stepSize;

  state->active = TRUE;

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

// Move hue and saturation to a given values, taking time as specified.
boolean emberAfColorControlClusterMoveToHueAndSaturationCallback(int8u hue,
                                                                 int8u saturation,
                                                                 int16u transitionTime)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int8u currentSaturation, currentHue, hueDiff, satDiff;

  emberAfColorControlClusterPrintln("ColorControl: MoveToHueAndSaturation (%x, %x, %2x)",
                                    hue,
                                    saturation,
                                    transitionTime);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the color specified is not achievable by the hardware, then the
  // color shall not be set and a ZCL default response command shall be
  // generated with status code equal to INVALID_VALUE.
  if (!emberAfPluginColorControlIsColorSupportedCallback(hue, saturation)) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto send_default_response;
  }

  status = colorControlReadCurrentSaturation(emberAfCurrentEndpoint(),
                                             &currentSaturation);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  status = colorControlReadCurrentHue(emberAfCurrentEndpoint(), &currentHue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }

  state->commandId = ZCL_MOVE_TO_HUE_AND_SATURATION_COMMAND_ID;
  state->hueMoveToLevel = hue;
  state->hueMoveDirection = (hue - currentHue <= 127 ? TRUE : FALSE);
  hueDiff = (state->hueMoveDirection ? hue - currentHue : currentHue - hue);

  state->saturationMoveToLevel = saturation;
  state->satMoveDirection = (saturation - currentSaturation <= 127 ? TRUE : FALSE);
  satDiff = (state->satMoveDirection 
             ? saturation - currentSaturation 
             : currentSaturation - saturation);

  if (hueDiff == 0) {
    return emberAfColorControlClusterMoveToSaturationCallback(saturation,
                                                              transitionTime);
  } else if (satDiff == 0) {
    return emberAfColorControlClusterMoveToHueCallback(hue,
                                                       EMBER_ZCL_HUE_DIRECTION_SHORTEST_DISTANCE,
                                                       transitionTime);
  }

  state->elapsedTime = 0;
  state->acceleratedHue = hueDiff > satDiff;
  state->acceleratedMoveRate = (state->acceleratedHue
                                ? hueDiff / satDiff
                                : satDiff / hueDiff);

  state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10; 
  state->eventDuration = state->transitionTime / ((state->acceleratedHue) 
                                                  ? hueDiff
                                                  : satDiff);

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToZero();

  state->active = TRUE;

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

static boolean colorControlHueAndSaturationHandler(int8u endpoint)
{
  EmberAfStatus status;
  ColorControlState *state = getColorControlState(endpoint);
  int8u hue, sat, hueDiff, satDiff;
  boolean hueUp = state->hueMoveDirection;
  boolean satUp = state->satMoveDirection;
  boolean proceed = TRUE;

  if (state == NULL) {
    return FALSE;
  }

  status = colorControlReadCurrentHue(endpoint, &hue);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: could not read current hue %x", 
                                      status);
    return FALSE;
  }
  status = colorControlReadCurrentSaturation(endpoint, &sat);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: could not read current saturation %x", 
                                      status);
    return FALSE;
  }

  // Calculate the hue and saturation differentials
  hueDiff = (hue > state->hueMoveToLevel) 
    ? (0xFF - (hue - state->hueMoveToLevel))
    : (state->hueMoveToLevel - hue);
  satDiff = (sat > state->saturationMoveToLevel) 
    ? (0xFF - (sat - state->saturationMoveToLevel))
    : (state->saturationMoveToLevel - sat);

  // See where we are relative to our expected correction threshold
  if (state->acceleratedHue) {
    if ( satDiff == 0 ) return FALSE;
    hueDiff = hueDiff / satDiff;
    if (hueDiff > state->acceleratedMoveRate) {
      hueDiff = state->acceleratedMoveRate - 1;
    } else {
      hueDiff = state->acceleratedMoveRate + 1;
    }

    sat = satUp ? sat + 1 : sat - 1;
    if (sat == 0xFF) {
      sat = satUp ? 0x00 : 0xFE;
    }
    // Are we finished?
    if (sat == state->saturationMoveToLevel) {
      hue = state->hueMoveToLevel;
      proceed = FALSE;
      goto set_values;
    }
    hue = hueUp ? hue + hueDiff : hue - hueDiff;
    if (hue == 0xFF) {
      hue = hueUp ? 0x00 : 0xFE;
    }
  } else {
    if ( hueDiff == 0 ) return FALSE;
    satDiff = satDiff / hueDiff;
    if (satDiff > state->acceleratedMoveRate) {
      satDiff = state->acceleratedMoveRate - 1;
    } else {
      satDiff = state->acceleratedMoveRate + 1;
    }

    hue = hueUp ? hue + 1 : hue - 1;
    if (hue == 0xFF) {
      hue = hueUp ? 0x00 : 0xFE;
    }
    // Are we finished?
    if (hue == state->hueMoveToLevel) {
      sat = state->saturationMoveToLevel;
      proceed = FALSE;
      goto set_values;
    }
    sat = satUp ? sat + satDiff : sat - satDiff;
    if (sat == 0xFF) {
      sat = satUp ? 0x00 : 0xFE;
    }
  }

set_values:
  colorControlSetHue(endpoint, hue);
  colorControlSetSaturation(endpoint, sat);
  return proceed;
}