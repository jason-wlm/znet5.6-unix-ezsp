/** @file hal/micro/cortexm3/nvic-config.h
 *
 * <!-- Copyright 2014 Silicon Laboratories, Inc.                        *80*-->
 */

/** @addtogroup nvic_config
 * Nested Vectored Interrupt Controller configuration header.
 *
 * This header defines the functions called by all of the NVIC exceptions/
 * interrupts.  The following are the nine peripheral ISRs available for
 * modification.  To use one of these ISRs, it must be instantiated
 * somewhere in the HAL.  Each ISR may only be instantiated once.  It is
 * not necessary to instantiate all or any of these ISRs (a stub will
 * be automatically generated if an ISR is not instantiated).
 *
 * - \code void halTimer1Isr(void); \endcode
 * - \code void halTimer2Isr(void); \endcode
 * - \code void halSc1Isr(void);    \endcode
 * - \code void halSc1Isr(void);    \endcode
 * - \code void halAdcIsr(void);    \endcode
 * - \code void halIrqAIsr(void);   \endcode
 * - \code void halIrqBIsr(void);   \endcode
 * - \code void halIrqCIsr(void);   \endcode
 * - \code void halIrqDIsr(void);   \endcode
 *
 * @note This file should \b not be modified.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// \b NOTE NOTE NOTE NOTE NOTE NOTE - The physical layout of this file, that
// means the white space, is CRITICAL!  Since this file is \#include'ed by
// the assembly file isr-stubs.s79, the white space in this file translates
// into the white space in that file and the assembler has strict requirements.
// Specifically, there must be white space *before* each "SEGMENT()" and there
// must be an *empty line* between each "EXCEPTION()" and "SEGMENT()".
//
// \b NOTE NOTE NOTE - The order of the EXCEPTIONS in this file is critical
// since it translates to the order they are placed into the vector table in
// cstartup.
//
// The purpose of this file is to consolidate NVIC configuration, this
// includes basic exception handler (ISR) function definitions, into a single
// place where it is easily tracked and changeable.
//
// \b NOTE Our chips only implement 5 bits for priority and subpriority 
// (bits [7:3]). This means that the lowest 3 bits in any priority field will
// be forced to 0 and not used by the hardware
//
// We establish 8 levels of priority (3 bits), with 4 (2 bits) of tie break
// priority. Lower priority values are higher priorities. This means that 0
// is the highest and 7 is the lowest. The NVIC mapping is detailed below.
//

//The 'PRIGROUP' field is 3 bits within the AIRCR register and indicates the
//bit position within a given exception's 8-bit priority field to the left of
//which is the "binary point" separating the preemptive priority level (in the
//most-significant bits) from the subpriority (in the least-significant bits).
//The preemptive priority determines whether an interrupt can preempt an
//executing interrupt. The subpriority helps choose which interrupt to run if
//multiple interrupts with the same preemptive priority are active at the same
//time. If no supriority is given or there is a tie then the hardware defined
//exception number is used to break the tie.
//
//The table below shows for each PRIGROUP value (the PRIGROUP value is the
//number on the far left) how the priority bits are split and how this maps
//to the priorities on chip. A 'P' is preemption bit, 'S' is a subpriority bit
//and an 'X' indicates that that bit is not implemented.
//0=7:1= PPPPPXX.X
//1=6:2= PPPPPX.XX
//2=5:3= PPPPP.XXX
//3=4:4= PPPP.SXXX
//4=3:5= PPP.SSXXX
//5=2:6= PP.SSSXXX
//6=1:7= P.SSSSXXX
//7=0:8= SSSSSSXXX
//
//Below is the default priority configuration for the chip's interrupts
// Pri.Sub  Purpose
//   0.0    faults (highest)
//   1.0    not used
//   2.0    SysTick for idling, MAC Tmr for idling during TX, management
//          interrupt for XTAL biasing, and the SVCall
//   3.0    DISABLE_INTERRUPTS(), INTERRUPTS_OFF(), ATOMIC()
//   4.0    normal interrupts
//   5.0    not used
//   6.0    not used
//   7.0    debug backchannel, PendSV
//   7.31   reserved (lowest)
#if defined(FREE_RTOS)
  // FreeRTOS recommends this type of configuration for CortexM3 and asserts
  // in some places if it finds a different one.
  #define PRIGROUP_POSITION 2  // PPPPP.XXX
#else 
  #define PRIGROUP_POSITION 4  // PPP.SSXXX
#endif

// Priority level used by DISABLE_INTERRUPTS() and INTERRUPTS_OFF()
// Must be lower priority than pendsv
// NOTE!! IF THIS VALUE IS CHANGED, SPRM.S79 MUST ALSO BE UPDATED
#define INTERRUPTS_DISABLED_PRIORITY  (3 << (PRIGROUP_POSITION+1))  // READ NOTE ABOVE!!


//Exceptions with fixed priorities cannot be changed by software.  Simply make
//them 0 since they are high priorities anyways.
#define NVIC_FIXED 0
//Reserved exceptions are not instantiated in the hardware.  Therefore
//exception priorities don't exist so just default them to lowest level.
#define NVIC_NONE  0xFF

#ifndef SEGMENT
  #define SEGMENT()
#endif
#ifndef SEGMENT2
  #define SEGMENT2()
#endif
#ifndef PERM_EXCEPTION
  #define PERM_EXCEPTION(vectorNumber, functionName, priority) \
    EXCEPTION(vectorNumber, functionName, priority, 0)
#endif

    // SEGMENT()
    //   **Place holder required by isr-stubs.s79 to define __CODE__**
    // SEGMENT2()
    //   **Place holder required by isr-stubs.s79 to define __THUMB__**
    // EXCEPTION(vectorNumber, functionName, priorityLevel, subpriority)
    //   vectorNumber = exception number defined by hardware (not used anywhere)
    //   functionName = name of the function that the exception should trigger
    //   priorityLevel = priority level of the function
    //   supriority = Used to break ties between exceptions at the same level
    // PERM_EXCEPTION
    //   is used to define an exception that should not be intercepted by the
    //   interrupt debugging logic, or that not should have a weak stub defined.
    //   Otherwise the definition is the same as EXCEPTION

//INTENTIONALLY INDENTED AND SPACED APART! Keep it that way!  See comment above!
    SEGMENT()
    SEGMENT2()
    PERM_EXCEPTION( 1, halEntryPoint,   NVIC_FIXED           ) //Reset

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      2, halNmiIsr,       NVIC_FIXED,         0) //NMI

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      3, halHardFaultIsr, NVIC_FIXED,         0) //Hard Fault

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      4, halMemoryFaultIsr,        0,         0) //Memory Fault

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      5, halBusFaultIsr,           0,         0) //Bus Fault

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      6, halUsageFaultIsr,         0,         0) //Usage Fault

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      7, halReserved07Isr, NVIC_NONE, NVIC_NONE) //Reserved

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      8, halReserved08Isr, NVIC_NONE, NVIC_NONE) //Reserved

    SEGMENT()
    SEGMENT2()
    EXCEPTION(      9, halReserved09Isr, NVIC_NONE, NVIC_NONE) //Reserved

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     10, halReserved10Isr, NVIC_NONE, NVIC_NONE) //Reserved

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     11, halSvCallIsr,             2,         0) //SVCall

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     12, halDebugMonitorIsr,       4,         0) //Debug Monitor

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     13, halReserved13Isr, NVIC_NONE, NVIC_NONE) //Reserved

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     14, halPendSvIsr,             7,         0) //PendSV

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     15, halInternalSysTickIsr,    2,         0) //SysTick

    //The following handlers map to "External Interrupts 16 and above"
    //In the NVIC Interrupt registers, this corresponds to bits 19:0 with bit 0
    //being TIMER1 (exception 16), bit 16 being Debug (exception 32), etc.
    SEGMENT()
    SEGMENT2()
    EXCEPTION(     16, halTimer1Isr,             4,         0) //Timer 1

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     17, halTimer2Isr,             4,         0) //Timer 2

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     18, halManagementIsr,         2,         0) //Management

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     19, halBaseBandIsr,           4,         0) //BaseBand

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     20, halSleepTimerIsr,         4,         0) //Sleep Timer

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     21, halSc1Isr,                4,         0) //SC1

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     22, halSc2Isr,                4,         0) //SC2

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     23, halSecurityIsr,           4,         0) //Security

    //MAC Timer Handler must be higher priority than emRadioTransmitIsr
    // for idling during managed TX->RX turnaround to function correctly.
    // But it is >= 3 so it doesn't run when ATOMIC.
    SEGMENT()
    SEGMENT2()
    EXCEPTION(     24, halStackMacTimerIsr,      3,         0) //MAC Timer

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     25, emRadioTransmitIsr,       4,         0) //MAC TX

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     26, emRadioReceiveIsr,        4,         0) //MAC RX

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     27, halAdcIsr,                4,         0) //ADC

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     28, halIrqAIsr,               4,         0) //GPIO IRQA

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     29, halIrqBIsr,               4,         0) //GPIO IRQB

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     30, halIrqCIsr,               4,         0) //GPIO IRQC

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     31, halIrqDIsr,               4,         0) //GPIO IRQD

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     32, halDebugIsr,              7,         0) //Debug

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     33, halSc3Isr,                4,         0) //SC3

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     34, halSc4Isr,                4,         0) //SC4

    SEGMENT()
    SEGMENT2()
    EXCEPTION(     35, halUsbIsr,                4,         0) //USB

#undef SEGMENT
#undef SEGMENT2
#undef PERM_EXCEPTION

#endif //DOXYGEN_SHOULD_SKIP_THIS
