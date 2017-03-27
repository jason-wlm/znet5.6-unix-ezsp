// *******************************************************************
// * price-client-consolidated-bills.c
// *
// * The Price client plugin is responsible for keeping track of the current
// * and future prices.
// * This file handles consolidated billing.
// *
// * Copyright 2014 by Silicon Labs. All rights reserved.              *80*
// *******************************************************************

#include "../../include/af.h"
#include "../../util/common.h"
#include "price-client.h"


//  CONSOLIDATED BILL

typedef struct{
  int32u providerId;
  int32u issuerEventId;
  int32u adjustedStartTimeUtc;  // Holds the converted start time (END OF WEEK, START OF DAY, etc)
  int32u adjustedEndTimeUtc;    // Holds the calculated end time
  int32u billingPeriodStartTime;
  int32u billingPeriodDuration;
  int8u  billingPeriodDurationType;
  int8u  tariffType;
  int32u consolidatedBill;
  int16u currency;
  int8u  billTrailingDigit;
  boolean valid;
} EmberAfPriceConsolidatedBill;

EmberAfPriceConsolidatedBill ConsolidatedBillsTable[EMBER_AF_PRICE_CLUSTER_CLIENT_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE];

void emberAfPriceInitConsolidatedBillsTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }

  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
    ConsolidatedBillsTable[ep][i].valid = FALSE;
  }
}

#define CANCELLATION_START_TIME  0xFFFFFFFF
/*
static boolean eventsOverlap( int32u newEventStartTime, int32u newEventEndTime, int32u oldEventStartTime, int32u oldEventEndTime ){
  return( ((newEventStartTime >= oldEventStartTime) && (newEventStartTime <= oldEventEndTime)) ||
      ((newEventEndTime >= oldEventStartTime) && (newEventEndTime <= oldEventEndTime)) ||
      ((newEventStartTime <= oldEventStartTime) && (newEventEndTime > oldEventStartTime)) );
}

static boolean removeOverlappingAndValidateConsolidatedBills( int32u providerId, int32u issuerEventId, int32u startTimeUtc, int32u durationSeconds, int8u tariffType ){
  int8u i;
  boolean cancelIndex;
  int32u endTimeUtc;
  int8u  billIsValid = TRUE;

  endTimeUtc = startTimeUtc + durationSeconds;
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
    cancelIndex = FALSE;
    if( ConsolidatedBillsTable[i].valid && (ConsolidatedBillsTable[i].providerId == providerId) &&
        (ConsolidatedBillsTable[i].tariffType == tariffType) && (ConsolidatedBillsTable[i].issuerEventId <= issuerEventId) ){
      if( (ConsolidatedBillsTable[i].issuerEventId == issuerEventId) &&
          (ConsolidatedBillsTable[i].billingPeriodStartTime == CANCELLATION_START_TIME) ){
        // Cancel action received, and matching entry found.   Cancel event.
        //cancelIndex = TRUE;
        ConsolidatedBillsTable[i].valid = FALSE;
      }
      else if( eventsOverlap( startTimeUtc, (startTimeUtc + durationSeconds),
                           ConsolidatedBillsTable[i].adjustedStartTimeUtc,
                           ConsolidatedBillsTable[i].adjustedEndTimeUtc) ){
        if( issuerEventId > ConsolidatedBillsTable[i].issuerEventId ){
          // Overlapping event found with smaller event ID - remove.
          cancelIndex = TRUE;
        }
        else{
          // Overlapping event found with larger event ID - new event is invalid
          billIsValid = FALSE;
        }
      }
    }
    if( cancelIndex == TRUE ){
      ConsolidatedBillsTable[i].valid = FALSE;
    }
  }
  return billIsValid;
}
*/

boolean emberAfPriceClusterPublishConsolidatedBillCallback(int32u providerId,
                                                           int32u issuerEventId,
                                                           int32u billingPeriodStartTime,
                                                           int32u billingPeriodDuration,
                                                           int8u billingPeriodDurationType,
                                                           int8u tariffType,
                                                           int32u consolidatedBill,
                                                           int16u currency,
                                                           int8u billTrailingDigit){
  int32u adjustedStartTime;
  int32u adjustedDuration;
  int8u  i;
  // int8u  availableIndex;
  // int32u  availableStartTime;
  int32u timeNow = emberAfGetCurrentTime();
  int8u  isValid = 1;

  int8u  smallestEventIdIndex;
  int32u smallestEventId = 0xFFFFFFFF;

  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }

  emberAfPriceClusterPrintln("RX: PublishConsolidatedBill eventId=%d,  timeNow=0x%4x", issuerEventId, timeNow );

  if( billingPeriodStartTime != CANCELLATION_START_TIME ){
    if( billingPeriodStartTime == 0 ){
      billingPeriodStartTime = timeNow;
    }
    adjustedStartTime = emberAfPluginPriceCommonClusterGetAdjustedStartTime( billingPeriodStartTime, billingPeriodDurationType );
    adjustedDuration = emberAfPluginPriceCommonClusterConvertDurationToSeconds( billingPeriodStartTime, billingPeriodDuration, billingPeriodDurationType );
  }
  else{
    adjustedStartTime = CANCELLATION_START_TIME;
  }
  //isValid = removeOverlappingAndValidateConsolidatedBills( providerId, issuerEventId, adjustedStartTime, adjustedDuration, tariffType );

  if( isValid ){
    // Initialize these.
    //availableStartTime = 0;
    //availableIndex = EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE;
    smallestEventIdIndex = EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE;
    
    // Search table for matching entry, invalid entry, lowestEventId
    for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
      emberAfPriceClusterPrintln(" == i=%d, val=%d, event=%d, start=0x%4x,  timeNow=0x%4x", 
        i, ConsolidatedBillsTable[ep][i].valid, ConsolidatedBillsTable[ep][i].issuerEventId,
        ConsolidatedBillsTable[ep][i].billingPeriodStartTime, timeNow );
      if( billingPeriodStartTime == CANCELLATION_START_TIME ){
        if( (ConsolidatedBillsTable[ep][i].providerId == providerId) &&
            (ConsolidatedBillsTable[ep][i].issuerEventId == issuerEventId) ){
          ConsolidatedBillsTable[ep][i].valid = FALSE;
          goto kickout;
        }
      }

      else if( ConsolidatedBillsTable[ep][i].valid &&
          (ConsolidatedBillsTable[ep][i].issuerEventId == issuerEventId) ){
        // Matching entry - reuse index
        // availableIndex = i;
        break;  // DONE
      } 
      else if( ConsolidatedBillsTable[ep][i].valid == FALSE ){
      //else if( (ConsolidatedBillsTable[i].valid == FALSE) ||
        //  (ConsolidatedBillsTable[i].adjustedEndTimeUtc < timeNow) ){
        // invalid or expired - mark with very far-out start time
        //availableIndex = i;
        //availableStartTime = 0xFFFFFFFF;
        smallestEventIdIndex = i;
        smallestEventId = 0;    // Absolutely use this index unless a match is found.
        emberAfPriceClusterPrintln("    INVALID");
      }
      //else if( ConsolidatedBillsTable[i].adjustedStartTimeUtc > availableStartTime ){
      else if( ConsolidatedBillsTable[ep][i].issuerEventId < issuerEventId
               && ConsolidatedBillsTable[ep][i].issuerEventId < smallestEventId ){
        //availableIndex = i;
        //availableStartTime = ConsolidatedBillsTable[i].adjustedStartTimeUtc;
        smallestEventIdIndex = i;
        smallestEventId = ConsolidatedBillsTable[ep][i].issuerEventId;
        emberAfPriceClusterPrintln("    TIME...");
      }
    }
    // Populate the available index fields.
    i = smallestEventIdIndex;
    if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE ){
      emberAfPriceClusterPrintln("  === DONE, i=%d", i);
      ConsolidatedBillsTable[ep][i].providerId = providerId;
      ConsolidatedBillsTable[ep][i].issuerEventId = issuerEventId;
      ConsolidatedBillsTable[ep][i].adjustedStartTimeUtc = adjustedStartTime;
      ConsolidatedBillsTable[ep][i].adjustedEndTimeUtc = adjustedStartTime + adjustedDuration;
      ConsolidatedBillsTable[ep][i].billingPeriodStartTime = billingPeriodStartTime;
      ConsolidatedBillsTable[ep][i].billingPeriodDuration = billingPeriodDuration;
      ConsolidatedBillsTable[ep][i].billingPeriodDurationType = billingPeriodDurationType;
      ConsolidatedBillsTable[ep][i].tariffType = tariffType;
      ConsolidatedBillsTable[ep][i].consolidatedBill = consolidatedBill;
      ConsolidatedBillsTable[ep][i].currency = currency;
      ConsolidatedBillsTable[ep][i].billTrailingDigit = billTrailingDigit;
      ConsolidatedBillsTable[ep][i].valid = TRUE;
      emAfPricePrintConsolidatedBillTableIndex( endpoint, i );
    }
  }
kickout:
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

void emAfPricePrintConsolidatedBillTableIndex( int8u endpoint, int8u i ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Print PublishConsolidatedBill [%d]", i);
    emberAfPriceClusterPrintln("  isValid=%d", ConsolidatedBillsTable[ep][i].valid );
    emberAfPriceClusterPrintln("  providerId=%d", ConsolidatedBillsTable[ep][i].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=%d", ConsolidatedBillsTable[ep][i].issuerEventId );
    emberAfPriceClusterPrintln("  billingPeriodStartTime=0x%4x", ConsolidatedBillsTable[ep][i].billingPeriodStartTime );
    emberAfPriceClusterPrintln("  billingPeriodDuration=0x%d", ConsolidatedBillsTable[ep][i].billingPeriodDuration );
    emberAfPriceClusterPrintln("  billingPeriodDurationType=0x%X", ConsolidatedBillsTable[ep][i].billingPeriodDurationType );
    emberAfPriceClusterPrintln("  tariffType=%d", ConsolidatedBillsTable[ep][i].tariffType );
    emberAfPriceClusterPrintln("  consolidatedBill=%d", ConsolidatedBillsTable[ep][i].consolidatedBill );
    emberAfPriceClusterPrintln("  currency=%d", ConsolidatedBillsTable[ep][i].currency );
    emberAfPriceClusterPrintln("  billTrailingDigit=%d", ConsolidatedBillsTable[ep][i].billTrailingDigit );
  }
}

int8u emAfPriceConsolidatedBillTableGetIndexWithEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
    if( ConsolidatedBillsTable[ep][i].valid && (issuerEventId == ConsolidatedBillsTable[ep][i].issuerEventId) ){
      break;
    }
  }
  return i;
}

int8u emAfPriceConsolidatedBillTableGetCurrentIndex( int8u endpoint ){
  int32u currTime = emberAfGetCurrentTime();
  int32u largestEventId = 0;
  int8u  largestEventIdIndex = EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE;
  int8u  i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }

  emberAfPriceClusterPrintln("=======  GET CURRENT INDEX, timeNow=0x%4x", currTime );

  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
    if( ConsolidatedBillsTable[ep][i].valid && 
        (ConsolidatedBillsTable[ep][i].adjustedStartTimeUtc <= currTime) &&
        (ConsolidatedBillsTable[ep][i].issuerEventId >= largestEventId) ){
        largestEventId = ConsolidatedBillsTable[ep][i].issuerEventId;
        largestEventIdIndex = i;
    }
  }
  return largestEventIdIndex;
}
      



