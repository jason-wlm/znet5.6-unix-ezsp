// *****************************************************************************
// * prepayment-modes-table.h
// *
// * Implemented routines for storing future prepayment modes.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************

#ifndef _PREPAYMENT_MODES_TABLE_H_
#define _PREPAYMENT_MODES_TABLE_H_


/**
 * @brief Initializes the prepayment mode table.
 *
 **/
void emInitPrepaymentModesTable( void );


/**
 * @brief Schedules a prepayment mode at some current or future time.
 * @param endpoint The endpoint of the device that supports the prepayment server.
 * @param providerId A unique identifier for the commodity supplier to whom this command relates.
 * @param issuerEventId A unique identifier that identifies this prepayment mode event.  Newer events should have a larger issuerEventId than older events.
 * @param dateTimeUtc The UTC time that indicates when the payment mode change should be applied.  A value of 0x00000000 indicates the command should be executed immediately.  A value of 0xFFFFFFFF indicates an existing change payment mode command with the same provider ID and issuer event ID should be cancelled.
 * @param paymentControlConfig A bitmap that indicates which actions should be taken when switching the payment mode.
 *
 **/
void emberAfPrepaymentSchedulePrepaymentMode( int8u endpoint, int32u providerId, int32u issuerEventId, int32u dateTimeUtc, int16u paymentControlConfig );


/**
 * @brief Determines the number of remaining seconds until the next prepayment mode event is scheduled to occur.
 * @param timeNowUtc The current UTC time.
 * @return Returns the number of remaining seconds until the next prepayment mode event.
 *
 **/
int32u emberAfPrepaymentServerSecondsUntilPaymentModeEvent( int32u timeNowUtc );


/**
 * @brief Sets the current payment mode on the prepayment server.
 * @param endpoint The endpoint of the prepayment server.
 *
 **/
void emberAfPrepaymentServerSetPaymentMode( int8u endpoint );



//void updatePaymentControlConfiguration( int8u endpoint, int16u paymentControlConfig );



#endif  // #ifndef _PREPAYMENT_MODES_TABLE_H_


