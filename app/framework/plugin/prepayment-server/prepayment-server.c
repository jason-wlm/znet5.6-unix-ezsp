
// *****************************************************************************
// * prepayment-server.c
// *
// * Implemented routines for prepayment server.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************


#include "app/framework/include/af.h"
#include "prepayment-server.h"
#include "prepayment-debt-log.h"
#include "prepayment-debt-schedule.h"
#include "prepayment-modes-table.h"
#include "../calendar-client/calendar-client.h"



typedef int16u PaymentControlConfiguration;
typedef int8u  FriendlyCredit;

extern void emberAfPluginPrepaymentSnapshotStorageInitCallback( int8u endpoint );
extern int8u emberAfPluginPrepaymentServerGetSnapshotCallback( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint,
                                                        int32u startTime,
                                                        int32u endTime,
                                                        int8u  snapshotOffset,
                                                        int32u snapshotCause );


void emberAfPrepaymentClusterServerInitCallback( int8u endpoint ){
  int8u ep = emberAfFindClusterServerEndpointIndex( endpoint, ZCL_PREPAYMENT_CLUSTER_ID );

  emberAfPluginPrepaymentSnapshotStorageInitCallback( endpoint );
  emInitPrepaymentModesTable();
  emberAfPluginPrepaymentServerInitDebtLog();
  emberAfPluginPrepaymentServerInitDebtSchedule();

  if( ep == 0xFF ){
    return;
  }
}


boolean emberAfPrepaymentClusterSelectAvailableEmergencyCreditCallback( int32u commandIssueDateTime,
                                                                        int8u originatingDevice ){
  emberAfPrepaymentClusterPrintln("Rx: Select Available Emergency Credit");
  return TRUE;
}



#define CUTOFF_UNCHANGED 0xFFFFFFFF
boolean emberAfPrepaymentClusterChangePaymentModeCallback(int32u providerId,
                                                          int32u issuerEventId,
                                                          int32u implementationDateTime,
                                                          PaymentControlConfiguration proposedPaymentControlConfiguration,
                                                          int32u cutOffValue){

  // The requester can be obtained with emberAfResponseDestination;
  EmberNodeId nodeId;
  int8u endpoint;
  int8u srcEndpoint, dstEndpoint;
  FriendlyCredit friendlyCredit;
  int32u friendlyCreditCalendarId;
  int32u emergencyCreditLimit;
  int32u emergencyCreditThreshold;
  int8u  dataType;
  int8u  i;

  emberAfPrepaymentClusterPrintln("RX: ChangePaymentMode, pid=0x%4x, eid=0x%4x, cfg=0x%2x", providerId, issuerEventId, proposedPaymentControlConfiguration);
  endpoint = emberAfCurrentEndpoint();

  if( cutOffValue != CUTOFF_UNCHANGED ){
#ifdef ZCL_USING_PREPAYMENT_CLUSTER_CUT_OFF_VALUE_ATTRIBUTE
    emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                          ZCL_CUT_OFF_VALUE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (int8u *)&cutOffValue, ZCL_INT32S_ATTRIBUTE_TYPE );
#endif
  }

  emberAfPrepaymentSchedulePrepaymentMode( emberAfCurrentEndpoint(), providerId, issuerEventId, implementationDateTime,
                                           proposedPaymentControlConfiguration );

  // Setup the friendly credit & emergency credit limit attributes.
#ifdef EMBER_AF_PLUGIN_CALENDAR_CLIENT
  i = emberAfPluginCalendarClientGetCalendarIndexByType( endpoint, EMBER_ZCL_CALENDAR_TYPE_FRIENDLY_CREDIT_CALENDAR );
  friendlyCredit = ( i < EMBER_AF_PLUGIN_CALENDAR_CLIENT_CALENDARS ) ? 0x01 : 0x00;
  friendlyCreditCalendarId = emberAfPluginCalendarClientGetCalendarId( endpoint, i );
#else
  friendlyCredit = 0x00;
  friendlyCreditCalendarId = EMBER_AF_PLUGIN_CALENDAR_CLIENT_INVALID_CALENDAR_ID;
#endif

#if (!defined ZCL_USING_PREPAYMENT_CLUSTER_EMERGENCY_CREDIT_LIMIT_ALLOWANCE_ATTRIBUTE) || \
    (!defined ZCL_USING_PREPAYMENT_CLUSTER_EMERGENCY_CREDIT_THRESHOLD_ATTRIBUTE)
#error "Prepayment Emergency Credit Limit/Allowance and Threshold attributes required for this plugin!"
#endif
  emberAfReadAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                        ZCL_EMERGENCY_CREDIT_LIMIT_ALLOWANCE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                        (int8u *)&emergencyCreditLimit, 4, &dataType );
  emberAfReadAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                        ZCL_EMERGENCY_CREDIT_THRESHOLD_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                        (int8u *)&emergencyCreditThreshold, 4, &dataType );
  nodeId = emberAfCurrentCommand()->source;
  srcEndpoint = emberAfGetCommandApsFrame()->destinationEndpoint;
  dstEndpoint = emberAfGetCommandApsFrame()->sourceEndpoint;
  emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );
  
  emberAfFillCommandPrepaymentClusterChangePaymentModeResponse( friendlyCredit, friendlyCreditCalendarId, 
                                                                 emergencyCreditLimit, emergencyCreditThreshold );
  emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
  return TRUE;
}

boolean emberAfPrepaymentClusterEmergencyCreditSetupCallback(int32u issuerEventId,
                                                             int32u startTime,
                                                             int32u emergencyCreditLimit,
                                                             int32u emergencyCreditThreshold){
  emberAfPrepaymentClusterPrintln("Rx: Emergency Credit Setup");
#ifdef ZCL_USING_PREPAYMENT_CLUSTER_EMERGENCY_CREDIT_LIMIT_ALLOWANCE_ATTRIBUTE
    emberAfWriteAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                          ZCL_EMERGENCY_CREDIT_LIMIT_ALLOWANCE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (int8u *)&emergencyCreditLimit, ZCL_INT32U_ATTRIBUTE_TYPE );
#else
  #error "Prepayment Emergency Credit Limit Allowance attribute is required for this plugin."
#endif

#ifdef ZCL_USING_PREPAYMENT_CLUSTER_EMERGENCY_CREDIT_THRESHOLD_ATTRIBUTE
    emberAfWriteAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                          ZCL_EMERGENCY_CREDIT_THRESHOLD_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (int8u *)&emergencyCreditThreshold, ZCL_INT32U_ATTRIBUTE_TYPE );
#else
  #error "Prepayment Emergency Credit Threshold attribute is required for this plugin."
#endif
  return TRUE;
}

enum{
  CREDIT_ADJUSTMENT_TYPE_INCREMENTAL = 0x00,
  CREDIT_ADJUSTMENT_TYPE_ABSOLUTE    = 0x01,
};

boolean emberAfPrepaymentClusterCreditAdjustmentCallback(int32u issuerEventId,
                                                         int32u startTime,
                                                         int8u creditAdjustmentType,
                                                         int32u creditAdjustmentValue){
  int32u currTimeUtc;
  int32s currCreditAdjustmentValue;
  int8u  dataType;

  emberAfPrepaymentClusterPrintln("Rx: Credit Adjustment");
#ifdef ZCL_USING_PREPAYMENT_CLUSTER_CREDIT_REMAINING_ATTRIBUTE
  if( creditAdjustmentType == CREDIT_ADJUSTMENT_TYPE_INCREMENTAL ){
    // Read current value, then add it to the adjustment.
    emberAfReadAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                          ZCL_CREDIT_REMAINING_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (int8u *)&currCreditAdjustmentValue, 4, &dataType );
    currCreditAdjustmentValue += creditAdjustmentValue;
  }

  emberAfWriteAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                         ZCL_CREDIT_REMAINING_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                         (int8u *)&creditAdjustmentValue, ZCL_INT32S_ATTRIBUTE_TYPE );
#else
  #error "Prepayment Credit Adjustment attribute is required for this plugin."
#endif

#ifdef ZCL_USING_PREPAYMENT_CLUSTER_CREDIT_REMAINING_TIMESTAMP_ATTRIBUTE
  // This one is optional - we'll track it if supported.
  currTimeUtc = emberAfGetCurrentTime();
  emberAfWriteAttribute( emberAfCurrentEndpoint(), ZCL_PREPAYMENT_CLUSTER_ID,
                         ZCL_CREDIT_REMAINING_TIMESTAMP_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                         (int8u *)&currTimeUtc, ZCL_UTC_TIME_ATTRIBUTE_TYPE );
#endif

  return TRUE;
}


#define MAX_SNAPSHOT_PAYLOAD_LEN  24
boolean emberAfPrepaymentClusterGetPrepaySnapshotCallback( int32u earliestStartTime, int32u latestEndTime,
                                                           int8u snapshotOffset, int32u snapshotCause ){

  EmberNodeId nodeId;
  int8u srcEndpoint, dstEndpoint;

  emberAfPrepaymentClusterPrintln("RX: GetPrepaySnapshot, st=0x%4x, offset=%d, cause=%d", earliestStartTime, snapshotOffset, snapshotCause );
  nodeId = emberAfCurrentCommand()->source;
  srcEndpoint = emberAfGetCommandApsFrame()->destinationEndpoint;
  dstEndpoint = emberAfGetCommandApsFrame()->sourceEndpoint;
  emberAfPrepaymentClusterPrintln("... from 0x%2x, ep=%d", nodeId, dstEndpoint );

  emberAfPluginPrepaymentServerGetSnapshotCallback( nodeId, srcEndpoint, dstEndpoint, 
                                                    earliestStartTime, latestEndTime, snapshotOffset, snapshotCause );
  return TRUE;

}
