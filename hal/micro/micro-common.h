/** @file hal/micro/micro-common.h
 * 
 * @brief Minimal Hal functions common across all microcontroller-specific files.
 * See @ref micro for documentation.
 *
 * <!-- Copyright 2009 by Ember Corporation. All rights reserved.-->   
 */
 
/** @addtogroup micro
 * Many of the supplied example applications use these microcontroller functions.
 * See hal/micro/micro-common.h for source code.
 *
 *@{
 */

#ifndef __MICRO_COMMON_H__
#define __MICRO_COMMON_H__


/** @brief Initializes microcontroller-specific peripherals.
*/
void halInit(void);

/** @brief Restarts the microcontroller and therefore everything else.
*/
void halReboot(void);

/** @brief Powers up microcontroller peripherals and board peripherals.
*/
void halPowerUp(void);

/** @brief Powers down microcontroller peripherals and board peripherals.
*/
void halPowerDown(void);

/** @brief Resumes microcontroller peripherals and board peripherals.
*/
void halResume(void);

/** @brief Suspends microcontroller peripherals and board peripherals.
*/
void halSuspend(void);


/** @brief The value that must be passed as the single parameter to 
 *  ::halInternalDisableWatchDog() in order to successfully disable the watchdog 
 *  timer.
 */ 
#define MICRO_DISABLE_WATCH_DOG_KEY 0xA5

/** @brief Enables the watchdog timer.
 */
void halInternalEnableWatchDog(void);

/** @brief Disables the watchdog timer.
 *
 * @note To prevent the watchdog from being disabled accidentally, 
 * a magic key must be provided.
 * 
 * @param magicKey  A value (::MICRO_DISABLE_WATCH_DOG_KEY) that enables the function.
 */
void halInternalDisableWatchDog(int8u magicKey);

/** @brief Determines whether the watchdog has been enabled or disabled.
 *
 * @return A boolean value indicating if the watchdog is current enabled.
 */
boolean halInternalWatchDogEnabled( void );

#ifdef DOXYGEN_SHOULD_SKIP_THIS
/** @brief Enumerations for the possible microcontroller sleep modes.
 * - SLEEPMODE_RUNNING
 *     Everything is active and running.  In practice this mode is not
 *     used, but it is defined for completeness of information.
 * - SLEEPMODE_IDLE
 *     Only the CPU is idled.  The rest of the chip continues running
 *     normally.  The chip will wake from any interrupt.
 * - SLEEPMODE_WAKETIMER
 *     The sleep timer clock sources remain running.  The RC is always
 *     running and the 32kHz XTAL depends on the board header.  Wakeup
 *     is possible from both GPIO and the sleep timer.  System time
 *     is maintained.  The sleep timer is assumed to be configured
 *     properly for wake events.
 * - SLEEPMODE_MAINTAINTIMER
 *     The sleep timer clock sources remain running.  The RC is always
 *     running and the 32kHz XTAL depends on the board header.  Wakeup
 *     is possible from only GPIO.  System time is maintained.
 *       NOTE: This mode is not available on EM2XX chips.
 * - SLEEPMODE_NOTIMER
 *     The sleep timer clock sources (both RC and XTAL) are turned off.
 *     Wakeup is possible from only GPIO.  System time is lost.
 */
enum SleepModes
#else
typedef int8u SleepModes;
enum
#endif
{
  SLEEPMODE_RUNNING = 0,
  SLEEPMODE_IDLE = 1,
  SLEEPMODE_WAKETIMER = 2,
  SLEEPMODE_MAINTAINTIMER = 3,
  SLEEPMODE_NOTIMER = 4,
  
  //The following SleepModes are deprecated on EM2xx and EM3xx chips.  Each
  //micro's halSleep() function will remap these modes to the appropriate
  //replacement, as necessary.
  SLEEPMODE_RESERVED = 6,
  SLEEPMODE_POWERDOWN = 7,
  SLEEPMODE_POWERSAVE = 8,
};



#ifdef CORTEXM3_EMBER_MICRO
  #ifdef EMBER_MICRO_PORT_F_GPIO
    #define WAKEPORTD         int8u portD;
    #define WAKEPORTE         int8u portE;
    #define WAKEPORTF         int8u portF;
    #define GPIO_MASK_SIZE    48
    #define GPIO_MASK         0xFFFFFFFFFFFF
    typedef int64u GpioMaskType;
    typedef int64u WakeMask;
  #elif defined (EMBER_MICRO_PORT_E_GPIO)
    #define WAKEPORTD         int8u portD;
    #define WAKEPORTE         int8u portE;
    #define WAKEPORTF 
    #define GPIO_MASK_SIZE    40
    #define GPIO_MASK         0xFFFFFFFFFF
    typedef int64u GpioMaskType;
    typedef int64u WakeMask;
  #elif defined (EMBER_MICRO_PORT_D_GPIO)
    #define WAKEPORTD       int8u portD;
    #define WAKEPORTE 
    #define WAKEPORTF 
    #define GPIO_MASK_SIZE  32
    #define GPIO_MASK       0xFFFFFFFF
    typedef int32u GpioMaskType;
    typedef int64u WakeMask;
  #else
    #define WAKEPORTD 
    #define WAKEPORTE 
    #define WAKEPORTF 
    #define GPIO_MASK_SIZE    24
    #define GPIO_MASK         0xFFFFFF
    typedef int32u GpioMaskType;
    typedef int32u WakeMask;
  #endif

  #define WAKE_GPIO_MASK      GPIO_MASK
  #define WAKE_GPIO_SIZE      GPIO_MASK_SIZE

  //We don't have a real register to hold this composite information.
  //Pretend we do so halGetWakeInfo can operate like halGetResetInfo.
  //This "register" is only ever set by halSleep.
  // [31] = WakeInfoValid
  // [30] = SleepSkipped
  // [29] = CSYSPWRUPREQ
  // [28] = CDBGPWRUPREQ
  // [27] = WAKE_CORE
  // [26] = TIMER_WAKE_WRAP
  // [25] = TIMER_WAKE_COMPB
  // [24] = TIMER_WAKE_COMPA
  // [23:0] = corresponding GPIO activity
  #define WAKEINFOVALID_INTERNAL_WAKE_EVENT_BIT (GPIO_MASK_SIZE+7)
  #define SLEEPSKIPPED_INTERNAL_WAKE_EVENT_BIT  (GPIO_MASK_SIZE+6)
  #define CSYSPWRUPREQ_INTERNAL_WAKE_EVENT_BIT  (GPIO_MASK_SIZE+5)
  #define CDBGPWRUPREQ_INTERNAL_WAKE_EVENT_BIT  (GPIO_MASK_SIZE+4)
  #define WAKE_CORE_INTERNAL_WAKE_EVENT_BIT     (GPIO_MASK_SIZE+3)
  #define WRAP_INTERNAL_WAKE_EVENT_BIT          (GPIO_MASK_SIZE+2)
  #define CMPB_INTERNAL_WAKE_EVENT_BIT          (GPIO_MASK_SIZE+1)
  #define CMPA_INTERNAL_WAKE_EVENT_BIT          (GPIO_MASK_SIZE+0)
  //This define shifts events from the PWRUP_EVENT register into the proper
  //place in the halInternalWakeEvent variable
  #define INTERNAL_WAKE_EVENT_BIT_SHIFT         (GPIO_MASK_SIZE-4)
  typedef union
  {
    struct
    {
      int8u portA;
      int8u portB;
      int8u portC;
      WAKEPORTD
      WAKEPORTE
      WAKEPORTF
      union {
        struct
        {
          int8u TIMER_WAKE_COMPA : 1;
          int8u TIMER_WAKE_COMPB : 1;
          int8u TIMER_WAKE_WRAP  : 1;
          int8u WAKE_CORE_B      : 1;
          int8u CDBGPWRUPREQ     : 1;
          int8u CSYSPWRUPREQ     : 1;
          int8u SleepSkipped     : 1;
          int8u WakeInfoValid    : 1;
        }bits;
        int8u byte;
      } internal;
    } events;
    WakeMask eventflags;
  } WakeEvents;
  #undef WAKEPORTD
  #undef WAKEPORTE
  #undef WAKEPORTF
#elif defined (CORTEXM3_EFM32_MICRO)
  typedef int32u WakeEvents;
  typedef int32u WakeMask;
#else // to cover simulation targets
  #define GPIO_MASK_SIZE    24
  #define GPIO_MASK         0xFFFFFF
  #define WAKE_GPIO_MASK    GPIO_MASK
  #define WAKE_GPIO_SIZE    GPIO_MASK_SIZE
  typedef int32u WakeEvents;
  typedef int32u WakeMask;
#endif // CORTEXM3_EMBER_MICRO

#define WAKE_MASK_INVALID (-1)

/** @note The preprocessor symbol WAKE_EVENT_SIZE has been deprecated.
 * Please use WakeMask instead.
 */
#define WAKE_EVENT_SIZE   WakeMask

/** @brief Puts the microcontroller to sleep in a specified mode.
 *
 * @note This routine always enables interrupts.
 *
 * @param sleepMode  A microcontroller sleep mode 
 * 
 * @sa ::SleepModes
 */
void halSleep(SleepModes sleepMode);

/** @brief Blocks the current thread of execution for the specified
 * amount of time, in microseconds.
 *
 * The function is implemented with cycle-counted busy loops
 * and is intended to create the short delays required when interfacing with
 * hardware peripherals.
 *
 * The accuracy of the timing provided by this function is not specified,
 * but a general rule is that when running off of a crystal oscillator it will
 * be within 10us.  If the micro is running off of another type of oscillator
 * (e.g. RC) the timing accuracy will potentially be much worse.
 *
 * @param us  The specified time, in microseconds. 
              Values should be between 1 and 65535 microseconds.
 */
void halCommonDelayMicroseconds(int16u us);

#define DEBUG_TOGGLE(n)

#endif //__MICRO_COMMON_H__

/** @} END micro group  */
  
