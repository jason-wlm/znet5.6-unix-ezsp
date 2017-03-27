// *******************************************************************
// * price-common.c
// *
// *
// * Copyright 2014 by Silicon Labs, Inc. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/util/util.h"
#include "price-common.h"
#include "enums.h"
#include "app/framework/plugin/price-common/price-common-time.h"

// Forward declaration
static void swap(int8u * a, int8u * b, int16u size);

void emberAfPluginPriceCommonSort(EmberAfPriceCommonInfo * commonInfos,
                                  int8u * dataArray,
                                  int8u dataArrayBlockSizeInByte,
                                  int8u dataArraySize){
  /* a[0] to a[dataArraySize-1] is the array to sort */
  int i,j;
  int iMin;

  /* advance the position through the entire array */
  for (j = 0; j < dataArraySize-1; j++) {
    /* find the min element in the unsorted a[j .. dataArraySize-1] */

    /* assume the min is the first element */
    iMin = j;
    /* test against elements after j to find the smallest */
    for ( i = j+1; i < dataArraySize; i++) {
      /* if this element is less, then it is the new minimum */
      if (commonInfos[i].valid && !commonInfos[iMin].valid){
        iMin = i;
      } else if ( commonInfos[i].valid && 
                  commonInfos[iMin].valid && 
                  (commonInfos[i].startTime < commonInfos[iMin].startTime) ){
        /* found new minimum; remember its index */
        iMin = i;
      }
    }

    if(iMin != j) {
      // swap common infos
      swap((int8u *)&commonInfos[j], (int8u *)&commonInfos[iMin], sizeof(EmberAfPriceCommonInfo));
      // swap block size
      swap(dataArray + j*dataArrayBlockSizeInByte,
           dataArray + iMin*dataArrayBlockSizeInByte,
           dataArrayBlockSizeInByte);
    }
  }
}

void emberAfPluginPriceCommonUpdateDurationForOverlappingEvents(EmberAfPriceCommonInfo *commonInfos,
                                                    int8u numberOfEntries ){
  int8u i;
  int32u endTime;
  if( numberOfEntries > 1 ){
    for( i=0; i<numberOfEntries-1; i++ ){
      if( commonInfos[i].valid && commonInfos[i+1].valid ){
        endTime = commonInfos[i].startTime + commonInfos[i].durationSec;
        if( endTime > commonInfos[i+1].startTime ){
          commonInfos[i].durationSec = commonInfos[i+1].startTime - commonInfos[i].startTime;
        }
      }
    }
  }
}

int32u emberAfPluginPriceCommonSecondsUntilSecondIndexActive(EmberAfPriceCommonInfo *commonInfos,
                                                             int8u numberOfEntries ){
  // This function assumes the commonInfos[] array is already sorted by start time.
  // It assumes index[0] is currently active, and checks index[1] to determine when
  // it will be active.  It returns how many seconds must elapse until index[1]
  // becomes active.
  int32u timeNow;
  int32u secondsUntilActive = 0xFFFFFFFF;

  timeNow = emberAfGetCurrentTime();
  if( (numberOfEntries > 1) && commonInfos[1].valid ){
    if( commonInfos[1].startTime >= timeNow ){
      secondsUntilActive = commonInfos[1].startTime - timeNow;
    }
    else{
      secondsUntilActive = 0;
    }
  }
  return secondsUntilActive;
}

int8u emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(EmberAfPriceCommonInfo *commonInfos,
                                                 int8u  numberOfEntries,
                                                 int32u newIssuerEventId,
                                                 int32u newStartTime,
                                                 boolean expireTimedOut ){
  int32u timeNow;
  int32u endTime;
  int32u smallestEventId = 0xFFFFFFFF;
  int8u  smallestEventIdIndex = 0xFF;
  int8u i;

  // Search the commonInfos[] table for an entry that is:
  //  - Matching (same issuerEventId & providerId)
  //  - Invalid or expired
  //  - smallest eventId
  timeNow = emberAfGetCurrentTime();

  for( i=0; i<numberOfEntries; i++ ){
    if( commonInfos[i].durationSec != ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED ){
      endTime = commonInfos[i].startTime + commonInfos[i].durationSec;
    }
    else{
      endTime = ZCL_PRICE_CLUSTER_END_TIME_NEVER;
    }

    if( (!commonInfos[i].valid) || 
        ( expireTimedOut && (endTime <= timeNow)) ){
      if( smallestEventId > 0 ){
        // Found index that is not valid.  Use the first invalid entry unless matching entry found.
        smallestEventId = 0;
        smallestEventIdIndex = i;
        commonInfos[i].valid = FALSE;
      }
    }
    else if( (commonInfos[i].startTime == newStartTime) &&
             (commonInfos[i].issuerEventId == newIssuerEventId) ){
      // Match found
      smallestEventIdIndex = i;
      break;
    }
    else if( (commonInfos[i].issuerEventId < smallestEventId) &&
             (commonInfos[i].issuerEventId < newIssuerEventId) ){
      smallestEventId = commonInfos[i].issuerEventId;
      smallestEventIdIndex = i;
    }
  }
  return smallestEventIdIndex;
}


int8u emberAfPluginPriceCommonFindValidEntries(int8u* validEntries,
                                   int8u numberOfEntries,
                                   EmberAfPriceCommonInfo* commonInfos,
                                   int32u earliestStartTime,
                                   int32u minIssuerEventId,
                                   int8u numberOfRequestedCommands ){
  int8u numberOfValidCommands = 0;
  int32u startTrackingLastStartTime = -1;
  int8u i = 0;
  int8u validEntryCount = 0;

  for (i=0; i<numberOfEntries; i++){
    if (commonInfos[i].valid){
      validEntryCount++;
    }
  }
  emberAfPriceClusterPrintln("Total number of active entries in table: %d", validEntryCount);

  for (i=0; i<numberOfEntries; i++){
    if (commonInfos[i].valid == 0){
      continue;
    }

    if ((minIssuerEventId != 0xFFFFFFFF) && (minIssuerEventId > commonInfos[i].issuerEventId)) {
      continue;
    }

    if (earliestStartTime <= commonInfos[i].startTime){
      validEntries[numberOfValidCommands] = i;
      numberOfValidCommands++;
    } else { // keep track of the startTime before currentTime.
             // in case there's no startTime after currentTime,
             // the last startTime will be valid.
      if (startTrackingLastStartTime == -1){
        startTrackingLastStartTime = 1;
        validEntries[numberOfValidCommands] = i;
        numberOfValidCommands++;
      } else if (startTrackingLastStartTime == 1){ // update lastStartTime
        validEntries[numberOfValidCommands - 1] = i;
      }
    }
  }

  if( (numberOfRequestedCommands != 0x0) &&
      (numberOfRequestedCommands < numberOfValidCommands) ){
    numberOfValidCommands = numberOfRequestedCommands;
  }

  emberAfPriceClusterPrint("Valid entries index: ");
  for (i=0; i<numberOfValidCommands; i++){
    emberAfPriceClusterPrint("%d, ", validEntries[i]);
  }
  emberAfPriceClusterPrintln("");
  return numberOfValidCommands;
}

int8u emberAfPluginPriceCommonServerGetActiveIndex(EmberAfPriceCommonInfo *commonInfos,
                                       int8u numberOfEntries ){

  int32u timeNow;
  int32u endTime;
  int32u maxCurrentEventId = 0;
  int8u  maxCurrentEventIdIndex = 0xFF;
  int8u  i;

  // TODO:  This function doesn't work if the index+1 item has started up.
  // This function is fooled by index 0 and returns that as the active index.
  // Need to build in a means into all price commands to invalidate their entries
  // at the appropriate time (when index+1 starts up).
  // End TODO.

  timeNow = emberAfGetCurrentTime();
  for( i=0; i<numberOfEntries; i++ ){
    if( commonInfos[i].valid &&
        (commonInfos[i].startTime <= timeNow) ){
      if( commonInfos[i].durationSec == ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED ){
        endTime = ZCL_PRICE_CLUSTER_END_TIME_NEVER;
      } else {
        endTime = commonInfos[i].startTime + commonInfos[i].durationSec;
      }

      if( (endTime >= timeNow) &&
          (commonInfos[i].issuerEventId > maxCurrentEventId) ){
        maxCurrentEventId = commonInfos[i].issuerEventId;
        maxCurrentEventIdIndex = i;
      }
    }
  }
  return maxCurrentEventIdIndex;
}

int8u emberAfPluginPriceCommonServerGetFutureIndex(EmberAfPriceCommonInfo *commonInfos,
                                       int8u numberOfEntries, 
                                       int32u * secUntilFutureEvent){
  int32u minDeltaSec = 0xFFFFFFFF;
  int32u minDeltaIndex = ZCL_PRICE_INVALID_INDEX;
  int32u deltaSec;
  int32u timeNow;
  int8u i;

  timeNow = emberAfGetCurrentTime();

  // Look for the nearest time of the next change starting either NOW, or in the future.
  for( i=0; i<numberOfEntries; i++ ){
    if( commonInfos[i].valid &&
        (commonInfos[i].startTime >= timeNow) ){
      deltaSec = commonInfos[i].startTime - timeNow;
      if( deltaSec < minDeltaSec ){
        minDeltaSec = deltaSec;
        minDeltaIndex = i;
      }
    }
  }

  if (secUntilFutureEvent != NULL){
    *secUntilFutureEvent = minDeltaSec;
  }
  return minDeltaIndex;
}

static void swap(int8u * a, int8u * b, int16u size){
  int8u tmp;
  int16u i;
  for (i=0; i<size; i++){
    if (a[i] != b[i]){
      tmp = a[i];
      a[i] = b[i];
      b[i] = tmp;
    }
  }
}
