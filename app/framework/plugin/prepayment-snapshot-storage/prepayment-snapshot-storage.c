// *****************************************************************************
// * prepayment-snapshot-storage.c
// *
// * Implemented routines for storing prepayment snapshots.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************


#include "app/framework/include/af.h"
#include "prepayment-snapshot-storage.h"

// Dependency check
#if ( (!defined ZCL_USING_PREPAYMENT_CLUSTER_ACCUMULATED_DEBT_ATTRIBUTE) || \
      (!defined ZCL_USING_PREPAYMENT_CLUSTER_DEBT_AMOUNT_1_ATTRIBUTE) || \
      (!defined ZCL_USING_PREPAYMENT_CLUSTER_DEBT_AMOUNT_2_ATTRIBUTE) || \
      (!defined ZCL_USING_PREPAYMENT_CLUSTER_DEBT_AMOUNT_3_ATTRIBUTE) || \
      (!defined ZCL_USING_PREPAYMENT_CLUSTER_EMERGENCY_CREDIT_REMAINING_ATTRIBUTE) || \
      (!defined ZCL_USING_PREPAYMENT_CLUSTER_CREDIT_REMAINING_ATTRIBUTE) )
  // These are required for the prepayment snapshot.
  #error "Prepayment snapshots require the Accumulated Debt, Debt Amount (1-3), Emergency Credit Remaining, and Credit Remaining Attributes"
#endif

static void initSnapshots( void );
static void initSchedules( void );
static EmberAfPrepaymentSnapshotPayload *allocateSnapshot( void );
static EmberAfPrepaymentSnapshotSchedulePayload *allocateSchedule( void );
static int16u fillSnapshotPayloadBuffer(int8u *buffer, EmberAfPrepaymentSnapshotPayload *snapshot);
static EmberAfPrepaymentSnapshotPayload *findSnapshot(int32u startTime,
                                                      int32u endTime,
                                                      int8u snapshotOffset,
                                                      int32u snapshotCause, 
                                                      int8u *totalMatchesFound);
void emberAfPluginPrepaymentSnapshotPrintInfo(EmberAfPrepaymentSnapshotPayload *snapshot);



static EmberAfPrepaymentSnapshotPayload snapshots[ EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SNAPSHOT_CAPACITY ];
static EmberAfPrepaymentSnapshotSchedulePayload schedules[ EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SCHEDULE_CAPACITY ];

static int32u SnapshotIdCounter;

static void initSnapshots(){
  int8u i;
  for( i=0; i<EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SNAPSHOT_CAPACITY; i++ ){
    snapshots[i].snapshotId = INVALID_SNAPSHOT_ID;
  }
}

static void initSchedules(){
  int8u i;
  for( i=0; i<EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SCHEDULE_CAPACITY; i++ ){
    schedules[i].snapshotScheduleId = INVALID_SNAPSHOT_SCHEDULE_ID;
  }
}

void emberAfPluginPrepaymentSnapshotStorageInitCallback( int8u endpoint ){
  SnapshotIdCounter = INVALID_SNAPSHOT_ID;
  initSchedules();
  initSnapshots();
  emberAfAppPrintln("Init Snapshot Table");
}

static EmberAfPrepaymentSnapshotPayload *allocateSnapshot(){
  int8u i;
  int32u minSnapshotId = 0xFFFFFFFF;
  int8u minSnapshotIdIndex = 0xFF;

  for( i=0; i<EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SNAPSHOT_CAPACITY; i++ ){
    if( snapshots[i].snapshotId == INVALID_SNAPSHOT_ID ){
      return &snapshots[i];
    }
    else if( snapshots[i].snapshotId < minSnapshotId ){
      // Find snapshot with smallest ID if an unused snapshot doesn't exist.
      minSnapshotId = snapshots[i].snapshotId;
      minSnapshotIdIndex = i;
    }
  }
  if( minSnapshotIdIndex < EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SNAPSHOT_CAPACITY ){
    return &snapshots[minSnapshotIdIndex];
  }
  return NULL;
}

static EmberAfPrepaymentSnapshotSchedulePayload *allocateSchedule(){
  int8u i;
  for( i=0; i<EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SCHEDULE_CAPACITY; i++ ){
    if( schedules[i].snapshotScheduleId == INVALID_SNAPSHOT_SCHEDULE_ID ){
      return &schedules[i];
    }
  }
  return NULL;
}

/*
void emberAfPluginPrepaymentSnapshotServerScheduleSnapshotCallback( int8u srcEndpoint, int8u dstEndpoint,
                                                                    EmberNodeId dest,
                                                                    int8u *snapshotPayload, 
                                                                    int8u *responsePaylaod ){
  EmberAfPrepaymentSnapshotSchedulePayload *schedule;
  int8u scheduleId;
  int8u index = 0;

  //  TODO:  Confirmation support?

  // Attempt to allocate a schedule.
  schedule = allocateSchedule();
  if( schedule != NULL ){
    // Set the schedule


}*/



#define SNAPSHOT_PAYLOAD_TYPE_DEBT_CREDIT_STATUS 0x00

int32u emberAfPluginPrepaymentSnapshotStorageTakeSnapshot( int8u endpoint, int32u snapshotCause ){
  EmberAfPrepaymentSnapshotPayload *snapshot;
  int32u snapshotId = INVALID_SNAPSHOT_ID;
  
  snapshot = allocateSnapshot();
  if( snapshot != NULL ){
    if( SnapshotIdCounter == INVALID_SNAPSHOT_ID ){
      SnapshotIdCounter++;
    }
    snapshotId = SnapshotIdCounter++;
    snapshot->snapshotId = snapshotId;
    snapshot->snapshotCauseBitmap = snapshotCause;
    snapshot->snapshotTime = emberAfGetCurrentTime();

    // TODO:  Should snapshotType be passed in as a parameter to this function?
    snapshot->snapshotType = SNAPSHOT_PAYLOAD_TYPE_DEBT_CREDIT_STATUS;

    emberAfAppPrintln("Storing Snapshot, addr=%d, id=%d, bitmap=0x%2x, type=%d, time=0x%4x", (int32u)snapshot,
        snapshot->snapshotId, snapshot->snapshotCauseBitmap, snapshot->snapshotType, snapshot->snapshotTime );

    emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_ACCUMULATED_DEBT_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER, (int8u *)&snapshot->accumulatedDebt,
                          sizeof(snapshot->accumulatedDebt), NULL );
    emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_DEBT_AMOUNT_1_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER, (int8u *)&snapshot->type1DebtRemaining,
                          sizeof(snapshot->type1DebtRemaining), NULL );
    emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_DEBT_AMOUNT_2_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER, (int8u *)&snapshot->type2DebtRemaining,
                          sizeof(snapshot->type2DebtRemaining), NULL );
    emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_DEBT_AMOUNT_3_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER, (int8u *)&snapshot->type3DebtRemaining,
                          sizeof(snapshot->type3DebtRemaining), NULL );
    emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_EMERGENCY_CREDIT_REMAINING_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER, (int8u *)&snapshot->emergencyCreditRemaining,
                          sizeof(snapshot->emergencyCreditRemaining), NULL );
    emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_CREDIT_REMAINING_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER, (int8u *)&snapshot->creditRemaining,
                          sizeof(snapshot->creditRemaining), NULL );
  }
  else{
    emberAfAppPrintln("Add Snapshot Event Failed");
  }
  return snapshotId;
} 


#define MAX_SNAPSHOT_PAYLOAD_LEN  24

EmberStatus emberAfPluginPrepaymentSnapshotStoragePublishSnapshot(EmberNodeId nodeId, 
                                                                  int8u srcEndpoint,
                                                                  int8u dstEndpoint,
                                                                  int32u snapshotId){
  int8u snapshotPayload[ MAX_SNAPSHOT_PAYLOAD_LEN ];
  EmberAfPrepaymentSnapshotPayload *snapshot = NULL;
  int16u index = 0;
 
  for(index=0; index<EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SNAPSHOT_CAPACITY; index++){
    EmberAfPrepaymentSnapshotPayload * cur = &snapshots[index];
    if (cur->snapshotId == snapshotId){
      snapshot = cur;
    }
  }

  if (snapshot == NULL){
    emberAfAppPrintln("Unable to find entries with snapshotId=%d", snapshotId);
    return EMBER_ERR_FATAL;
  }
  
  emberAfPluginPrepaymentSnapshotPrintInfo(snapshot);
  int16u payloadSize = fillSnapshotPayloadBuffer(snapshotPayload, snapshot);

  emberAfFillCommandPrepaymentClusterPublishPrepaySnapshot(snapshot->snapshotId,
                                                           snapshot->snapshotTime,
                                                           1,
                                                           0,
                                                           1,
                                                           snapshot->snapshotCauseBitmap,
                                                           snapshot->snapshotType,
                                                           snapshotPayload,
                                                           payloadSize);

  emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );
  emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );   
  return EMBER_SUCCESS;
}
                                                                  
int8u emberAfPluginPrepaymentServerGetSnapshotCallback( EmberNodeId nodeId,
                                                        int8u srcEndpoint,
                                                        int8u dstEndpoint,
                                                        int32u startTime,
                                                        int32u endTime,
                                                        int8u  snapshotOffset,
                                                        int32u snapshotCause ){

  EmberAfPrepaymentSnapshotPayload *snapshot;
  int8u snapshotPayload[ MAX_SNAPSHOT_PAYLOAD_LEN ];
  int16u payloadSize = 0;
  int8u  totalMatchesFound;

  snapshot = findSnapshot( startTime, endTime, snapshotOffset, snapshotCause, &totalMatchesFound );
  if( snapshot != NULL ){
    payloadSize = fillSnapshotPayloadBuffer( snapshotPayload, snapshot );
    emberAfAppPrintln("Found Snapshot, addr=%d, id=%d, matches=%d, bitmap=0x%2x, type=%d, time=0x%4x",
                      (int32u)snapshot,
                      snapshot->snapshotId,
                      totalMatchesFound,
                      snapshot->snapshotCauseBitmap,
                      snapshot->snapshotType,
                      snapshot->snapshotTime );
    
    emberAfPluginPrepaymentSnapshotPrintInfo( snapshot );

    emberAfFillCommandPrepaymentClusterPublishPrepaySnapshot(snapshot->snapshotId,
                                                             snapshot->snapshotTime,
                                                             totalMatchesFound,
                                                             0,
                                                             0,
                                                             snapshot->snapshotCauseBitmap,
                                                             snapshot->snapshotType,
                                                             snapshotPayload,
                                                             payloadSize );

    emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );
    emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
  }
  else{
    emberAfAppPrintln("Snapshot not found, st=0x%4x, end=0x%4x, cause=0x%4x", startTime, endTime, snapshotCause );
    emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_NOT_FOUND );
  }
  return payloadSize;
}

void emberAfPluginPrepaymentSnapshotPrintInfo( EmberAfPrepaymentSnapshotPayload *snapshot ){
  emberAfAppPrintln("= Prepayment Snapshot =");
  emberAfAppPrintln(" id=%d", snapshot->snapshotId );
  emberAfAppPrintln(" bitmap=0x%2x", snapshot->snapshotCauseBitmap );
  emberAfAppPrintln(" time=0x%4x", snapshot->snapshotTime );
  emberAfAppPrintln(" type=%d", snapshot->snapshotType );
  emberAfAppPrintln(" accumDebt=%d", snapshot->accumulatedDebt );
  emberAfAppPrintln(" type1Debt=%d", snapshot->type1DebtRemaining );
  emberAfAppPrintln(" type2Debt=%d", snapshot->type2DebtRemaining );
  emberAfAppPrintln(" type3Debt=%d", snapshot->type3DebtRemaining );
  emberAfAppPrintln(" emergCredit=%d", snapshot->emergencyCreditRemaining );
  emberAfAppPrintln(" credit=%d", snapshot->creditRemaining);
}




static int16u fillSnapshotPayloadBuffer( int8u *buffer, EmberAfPrepaymentSnapshotPayload *snapshot ){
  int16u index = 0;
  emberAfCopyInt32u( buffer, index, snapshot->accumulatedDebt );
  index += 4;
  emberAfCopyInt32u( buffer, index, snapshot->type1DebtRemaining);
  index += 4;
  emberAfCopyInt32u( buffer, index, snapshot->type2DebtRemaining);
  index += 4;
  emberAfCopyInt32u( buffer, index, snapshot->type3DebtRemaining);
  index += 4;
  emberAfCopyInt32u( buffer, index, snapshot->emergencyCreditRemaining);
  index += 4;
  emberAfCopyInt32u( buffer, index, snapshot->creditRemaining);
  index += 4;
  return index;
}

static EmberAfPrepaymentSnapshotPayload *findSnapshot(int32u startTime,
                                                      int32u endTime,
                                                      int8u snapshotOffset,
                                                      int32u snapshotCause, 
                                                      int8u *totalMatchesFound){
  EmberAfPrepaymentSnapshotPayload *matchSnapshot = NULL;
  int8u i;
  int8u matchCount = 0;

  // Keep this function simple to improve testability.  Caller must do something with the snapshot.  
  //snapshot = findSnapshot( startTime, endTime, snapshotOffset, snapshotCause );
  for( i=0; i<EMBER_AF_PLUGIN_PREPAYMENT_SNAPSHOT_STORAGE_SNAPSHOT_CAPACITY; i++ ){
    if( (snapshots[i].snapshotTime >= startTime) && (snapshots[i].snapshotTime <= endTime) ){
      if( (snapshotCause == SNAPSHOT_CAUSE_ALL_SNAPSHOTS) || 
          ((snapshotCause & snapshots[i].snapshotCauseBitmap) == snapshotCause) ){
        matchCount++;
        if( snapshotOffset == 0 ){
          //return &snapshots[i];
          matchSnapshot = &snapshots[i];
          break;
        }
        else{
          snapshotOffset--;
        }
      }
    }
  }
  *totalMatchesFound = matchCount;
  return matchSnapshot;
}









