/** @file stack/platform/micro/debug-channel.h
* 
* @brief Functions that provide access to the debug channel.
 *
 * <!-- Copyright 2005 by Ember Corporation. All rights reserved.-->   
 */

#ifndef __DEBUG_CHANNEL_H__
#define __DEBUG_CHANNEL_H__

/** @name Debug Channel Functions */

//@{

EmberStatus halStackDebugPutBuffer(EmberMessageBuffer buffer);
int8u emDebugAddInitialFraming(int8u *buff, int16u debugType);

#if DEBUG_LEVEL >= BASIC_DEBUG
EmberStatus emDebugInit(void);
void emDebugPowerDown(void);
void emDebugPowerUp(void);
boolean halStackDebugActive(void);
#else
#define emDebugInit() EMBER_SUCCESS
#define emDebugPowerDown()
#define emDebugPowerUp()
#endif

//@}  // end of Debug Channel Functions

#endif //__DEBUG_CHANNEL_H__
