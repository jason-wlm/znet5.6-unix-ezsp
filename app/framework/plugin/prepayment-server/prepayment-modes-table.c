// *****************************************************************************
// * prepayment-modes-table.c
// *
// * Implemented routines for storing future prepayment modes.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************

#include "app/framework/include/af.h"
#include "prepayment-server.h"
#include "prepayment-modes-table.h"
#include "prepayment-tick.h"

#define CHANGE_PAYMENT_MODE_DATETIME_NOW    0x00000000
#define CHANGE_PAYMENT_MODE_DATETIME_CANCEL 0xFFFFFFFF
#define INVALID_PAYMENT_CONTROL_CONFIG      0xFFFF
#define INVALID_UTC_TIME  0xFFFFFFFF
#define END_TIME_NEVER    0xFFFFFFFF
#define MAX_NUM_PENDING_PAYMENT_MODES   EMBER_AF_PLUGIN_PREPAYMENT_SERVER_MAX_PENDING_PAYMENT_MODES

typedef struct{
  int32u providerId;
  int32u issuerEventId;
  int32u startTimeUtc;
  int32u endTimeUtc;
  int16u paymentControlConfig;
  int8u  endpoint;
  int8u  active;
} emPendingPaymentMode;

static emPendingPaymentMode PendingPaymentModes[ MAX_NUM_PENDING_PAYMENT_MODES ];

void initPrepaymentModesTableIndex( int8u x );
void updatePaymentControlConfiguration( int8u endpoint, int16u paymentControlConfig );
int16u emAfPrepaymentModesGetCurrentMode( void );
void scheduleNextMode( int8u endpoint );


void emInitPrepaymentModesTable(){
  int8u x;
  for( x=0; x<MAX_NUM_PENDING_PAYMENT_MODES; x++ ){
    initPrepaymentModesTableIndex( x );
  }
}

void initPrepaymentModesTableIndex( int8u x ){
  if( x < MAX_NUM_PENDING_PAYMENT_MODES ){
    PendingPaymentModes[x].providerId = 0;
    PendingPaymentModes[x].issuerEventId = 0;
    PendingPaymentModes[x].startTimeUtc = INVALID_UTC_TIME;
    PendingPaymentModes[x].endTimeUtc = INVALID_UTC_TIME;
    PendingPaymentModes[x].paymentControlConfig = INVALID_PAYMENT_CONTROL_CONFIG;
    PendingPaymentModes[x].endpoint = 0;
    PendingPaymentModes[x].active = 0;
  }
}

void emberAfPrepaymentSchedulePrepaymentMode( int8u endpoint, int32u providerId, int32u issuerEventId, int32u startTimeUtc, int16u paymentControlConfig ){
  int8u  x;
  int8u  oldestIndex = 0xFF;
  int32u oldestEventId = 0xFFFFFFFF;
  int32u endTimeUtc = END_TIME_NEVER;

  if( startTimeUtc == CHANGE_PAYMENT_MODE_DATETIME_NOW ){
    startTimeUtc = emberAfGetCurrentTime();
  }

  // Adjust overlapping start/end times with this event based on event IDs.
  // Also, find an index to store the event - the first unused.
  for( x=0; x<MAX_NUM_PENDING_PAYMENT_MODES; x++ ){
    if( startTimeUtc == CHANGE_PAYMENT_MODE_DATETIME_CANCEL ){
      // CANCEL operation - look for matching provider ID & event ID.
      if( (providerId == PendingPaymentModes[x].providerId) && (issuerEventId == PendingPaymentModes[x].issuerEventId) ){
        // Found a match that should be cancelled.
        emberAfPrepaymentClusterPrintln("Cancelling scheduled prepayment mode.");
        initPrepaymentModesTableIndex( x );
        break;
      }
    }
    else{
      if( (PendingPaymentModes[x].startTimeUtc == INVALID_UTC_TIME) && (oldestEventId != 0) ){
        // Unused index
        oldestIndex = x;
        oldestEventId = 0;
      }
      else{
        // Update END time on any overlapping events with smaller event IDs to avoid overlapping.
        // Also search for an index to add the new event.
        if( (issuerEventId > PendingPaymentModes[x].issuerEventId) &&
            (startTimeUtc < PendingPaymentModes[x].endTimeUtc) ){
          PendingPaymentModes[x].endTimeUtc = startTimeUtc;
        }
        else if( (PendingPaymentModes[x].issuerEventId > issuerEventId) &&
                 (PendingPaymentModes[x].startTimeUtc > startTimeUtc) ){
          // Found payment mode with later start time, and larger event ID.
          endTimeUtc = PendingPaymentModes[x].startTimeUtc;
        }
        //else if( PendingPaymentModes[x].issuerEventId < oldestEventId ){
          // We could possibly store a new event over the oldest event ID, but this seems bad.
          //oldestIndex = x;
          //oldestEventId = PendingPaymentModes[x].issuerEventId;
        //}
      }
    }
  }
  if( oldestIndex < MAX_NUM_PENDING_PAYMENT_MODES ){
    x = oldestIndex;
    PendingPaymentModes[x].endpoint = endpoint;
    PendingPaymentModes[x].providerId = providerId;
    PendingPaymentModes[x].issuerEventId = issuerEventId;
    PendingPaymentModes[x].startTimeUtc = startTimeUtc;
    PendingPaymentModes[x].endTimeUtc = endTimeUtc;
    PendingPaymentModes[x].paymentControlConfig = paymentControlConfig;
    emberAfPrepaymentClusterPrintln("Adding prepayment mode, x=%d", x );
  }
  //scheduleNextMode( endpoint );
  emberAfPrepaymentClusterScheduleTickCallback( endpoint, PREPAYMENT_TICK_CHANGE_PAYMENT_MODE_EVENT );
}


void updatePaymentControlConfiguration( int8u endpoint, int16u paymentControlConfig ){
  emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                         ZCL_PAYMENT_CONTROL_CONFIGURATION_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                        (int8u *)&paymentControlConfig, ZCL_BITMAP16_ATTRIBUTE_TYPE );
}

int32u emberAfPrepaymentServerSecondsUntilPaymentModeEvent( int32u timeNowUtc ){
  int32u minDelaySec = EVENT_TIME_NO_PENDING_EVENTS;
  int8u x;
  int32u minNextTimeUtc = INVALID_UTC_TIME;
  int32u nextTimeUtc;

  for( x=0; x<MAX_NUM_PENDING_PAYMENT_MODES; x++ ){
    nextTimeUtc = INVALID_UTC_TIME;

    if( PendingPaymentModes[x].endTimeUtc <= timeNowUtc ){
      // expire this entry
      initPrepaymentModesTableIndex( x );
    }
    else if( PendingPaymentModes[x].active ){
      // This event is current - next event when event expires
      //minDelaySec = PendingPaymentModes[i].endTimeUtc;
      nextTimeUtc = PendingPaymentModes[x].endTimeUtc;
    }
    else if( (PendingPaymentModes[x].startTimeUtc <= timeNowUtc) &&
             (PendingPaymentModes[x].endTimeUtc >= timeNowUtc) ){
      // Found an entry that is not active that should be running now.
      // NOTE:  Earlier 'else if' checks to see if entry is already active.
      // Mark it as active and set nextTimeUtc to NOW.
      // We can abort the for loop immediately since we have an event that needs to run NOW.
      PendingPaymentModes[x].active = TRUE;
      minDelaySec = 0;
      break;
    }
    else if( PendingPaymentModes[x].startTimeUtc != INVALID_UTC_TIME ){
      nextTimeUtc = PendingPaymentModes[x].startTimeUtc;
    }
    
    if( nextTimeUtc < minNextTimeUtc ){
      minNextTimeUtc = nextTimeUtc;
    }
  }
  if( minNextTimeUtc != INVALID_UTC_TIME ){
    if( minNextTimeUtc > timeNowUtc ){
      minDelaySec = minNextTimeUtc - timeNowUtc;
    }
    else{
      minDelaySec = 0;
    }
  }
  return minDelaySec;
}



void emberAfPrepaymentServerSetPaymentMode( int8u endpoint ){
  int16u paymentControlConfig = emAfPrepaymentModesGetCurrentMode();
  if( paymentControlConfig != INVALID_PAYMENT_CONTROL_CONFIG ){
    // Eventually, this will be used to change prepayment modes, etc.
    // For now, it exists to make sure I understand how to invoke it.
    updatePaymentControlConfiguration( endpoint, paymentControlConfig );
  }
}


int16u emAfPrepaymentModesGetCurrentMode(){
  int8u x;
  int32u now;
  int32u largestEventId = 0;
  int8u largestEventIdIndex = 0xFF;
  int16u currentPaymentControlConfig = INVALID_PAYMENT_CONTROL_CONFIG;

  now = emberAfGetCurrentTime();

  // Invalidate all old entries and get the most recent paymentControlConfiguration.
  for( x=0; x<MAX_NUM_PENDING_PAYMENT_MODES; x++ ){
    if( PendingPaymentModes[x].endTimeUtc <= now ){
      initPrepaymentModesTableIndex( x );
    }
    if( (PendingPaymentModes[x].startTimeUtc <= now) &&
        (PendingPaymentModes[x].endTimeUtc > now) ){
      if( PendingPaymentModes[x].issuerEventId > largestEventId ){
        largestEventId = PendingPaymentModes[x].issuerEventId;
        largestEventIdIndex = x;
      }
    }
  }
  if( largestEventIdIndex < MAX_NUM_PENDING_PAYMENT_MODES ){
    x = largestEventIdIndex;
    currentPaymentControlConfig = PendingPaymentModes[x].paymentControlConfig;
    PendingPaymentModes[x].active = TRUE;
  }
  return currentPaymentControlConfig;
}


