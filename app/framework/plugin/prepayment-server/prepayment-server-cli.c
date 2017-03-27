// *****************************************************************************
// * prepayment-server-cli.c
// *
// * Routines for interacting with the prepayment-server.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/util.h"
#include "prepayment-server.h"
#include "prepayment-modes-table.h"
#include "prepayment-debt-log.h"
#include "prepayment-debt-schedule.h"
#include "prepayment-tick.h"
#include "app/framework/plugin/prepayment-snapshot-storage/prepayment-snapshot-storage.h"


typedef int16u PaymentControlConfiguration;
typedef int32u PrepaySnapshotPayloadCause;
typedef int8u  PrepaySnapshotPayloadType;
typedef int8u  FriendlyCredit;


void emAfPrepaymentServerCliInit( void );
void emAfPrepaymentServerCliWriteAttribute( void );
void emAfPrepaymentServerCliChangePaymentModeRelative( void );
void emAfPrepaymentServerCliVerifyPaymentMode( void );
void emAfPrepaymentServerCliVerifyAttribute( void );
void emAfPrepaymentServerCliPublishPrepaySnapshot( void );
void emAfPrepaymentServerCliAddSnapshotEvent( void );
void emAfPrepaymentReadDebtLog( void );
void emAfPrepaymentReadDebtAttributes( void );
void emAfPrepaymentGetTopUpPercentage( void );
void emAfPrepaymentCheckCalendarCli( void );
void emAfPrepaymentGetWeekdayCli( void );
void emAfPrepaymentPrintAfTime( EmberAfTimeStruct *pafTime );
void emAfPrepaymentScheduleDebtRepaymentCli( void );

void emAfPrepaymentServerCliInit(){
  int8u endpoint;
  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  emberAfPrepaymentClusterServerInitCallback( endpoint );
}

// plugin prepayment-server writeAttribute <endpoint:1> <attributeId:2> <attributeType:1> <numBytes:1> <value:4>
void emAfPrepaymentServerCliWriteAttribute(){
  int16u attributeId;
  int8u  status;
  int8u  endpoint;
  int8u  numBytes;
  int8u  attributeType;
  int32u value;

  endpoint      = (int8u) emberUnsignedCommandArgument( 0 );
  attributeId   = (int16u)emberUnsignedCommandArgument( 1 );
  attributeType = (int8u) emberUnsignedCommandArgument( 2 );
  numBytes      = (int8u) emberUnsignedCommandArgument( 3 );
  value         = (int32u)emberUnsignedCommandArgument( 4 );

  status = emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, attributeId,
                                  CLUSTER_MASK_SERVER, (int8u *)&value, attributeType );
  emberAfPrepaymentClusterPrintln("Write Attribute status=0x%x", status );
}


void emAfPrepaymentServerCliVerifyPaymentMode(){
  PaymentControlConfiguration expectedPaymentControlConfiguration;
  PaymentControlConfiguration readPaymentControlConfiguration;
  EmberAfStatus status;
  int8u endpoint;
  int8u dataType;

  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  expectedPaymentControlConfiguration = (int16u)emberUnsignedCommandArgument( 1 );
  status = emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                                ZCL_PAYMENT_CONTROL_CONFIGURATION_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                (int8u *)&readPaymentControlConfiguration, 2, &dataType );

  if( (status == EMBER_ZCL_STATUS_SUCCESS) && (expectedPaymentControlConfiguration == readPaymentControlConfiguration) ){
    emberAfPrepaymentClusterPrintln("Payment Mode Match Success - %d", readPaymentControlConfiguration );
  }
  else{
    emberAfPrepaymentClusterPrintln("Payment Mode Failed Match, status=0x%x, read=%d, exp=%d", status, readPaymentControlConfiguration, expectedPaymentControlConfiguration );
  }
}


void emAfPrepaymentServerCliVerifyAttribute(){
  EmberAfStatus status;
  int32u readAttributeValue;
  int32u expectedAttributeValue;
  int16u attributeId;

  int8u endpoint;
  int8u dataType;
  int8u attributeSize;

  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  attributeId = (int16u)emberUnsignedCommandArgument( 1 );
  attributeSize = (int8u)emberUnsignedCommandArgument( 2 );
  expectedAttributeValue = (int32u)emberUnsignedCommandArgument( 3 );
  status = emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                                attributeId, CLUSTER_MASK_SERVER,
                                (int8u *)&readAttributeValue, attributeSize, &dataType );

  if( (status == EMBER_ZCL_STATUS_SUCCESS) && (expectedAttributeValue == readAttributeValue) ){
    emberAfPrepaymentClusterPrintln("Attribute Read Match Success - %d", status );
  }
  else{
    emberAfPrepaymentClusterPrintln("Attribute Read Failed Match status=0x%x, read=%d, exp=%d", status, readAttributeValue, expectedAttributeValue );
  }
}


#define MAX_SNAPSHOT_PAYLOAD_LEN  24
void emAfPrepaymentServerCliPublishPrepaySnapshot(){
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int32u snapshotTableIndex = (int32u)emberUnsignedCommandArgument(3);

  emberAfPluginPrepaymentSnapshotStoragePublishSnapshot(nodeId,
                                                        srcEndpoint, 
                                                        dstEndpoint, 
                                                        snapshotTableIndex);
}


void emAfPrepaymentServerCliAddSnapshotEvent(){
  int32u snapshotCause;
  int8u  endpoint;

  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  snapshotCause = (int32u)emberUnsignedCommandArgument( 1 );
  emberAfPrepaymentClusterPrintln("CLI Add Snapshot Event, endpoint=%d cause=0x%4x", endpoint, snapshotCause );

  emberAfPluginPrepaymentSnapshotStorageTakeSnapshot( endpoint, snapshotCause );

}

void emAfPrepaymentReadDebtLog(){
  int8u index;
  index = (int8u)emberUnsignedCommandArgument( 0 );
  emberAfPluginPrepaymentPrintDebtLogIndex( index );
}


void emAfPrepaymentReadDebtAttributes(){
  int8u endpoint;
  int8u index;
  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  index = (int8u)emberUnsignedCommandArgument( 1 );
  emberAfPluginPrepaymentPrintDebtAttributes( endpoint, index );
}

void emAfPrepaymentGetTopUpPercentage(){
  int8u  endpoint;
  int32u topUpValue;

  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  topUpValue = (int32u)emberUnsignedCommandArgument( 1 );
  emberAfPluginPrepaymentGetDebtRecoveryTopUpPercentage( endpoint, topUpValue );
}

void emAfPrepaymentCheckCalendarCli(){
  int32u utcTime;
  int32u calcUtcTime;
  EmberAfTimeStruct afTime;

  utcTime = (int32u)emberUnsignedCommandArgument( 0 );
  emberAfFillTimeStructFromUtc( utcTime, &afTime );
  emAfPrepaymentPrintAfTime( &afTime );
  
  calcUtcTime = emberAfGetUtcFromTimeStruct( &afTime );
  if( calcUtcTime == utcTime ){
    emberAfPrepaymentClusterPrintln("= UTC Times Match, 0x%4x", calcUtcTime );
  }
  else{
    emberAfPrepaymentClusterPrintln(" ERROR: UTC Times Don't Match, 0x%4x != 0x%4x", utcTime, calcUtcTime );
  }
}

void emAfPrepaymentGetWeekdayCli(){
  int8u weekday;
  int32u utcTime;

  utcTime = (int32u)emberUnsignedCommandArgument(0);
  weekday = emberAfGetWeekdayFromUtc( utcTime );
  emberAfPrepaymentClusterPrintln("UTC Time=0x%4x, Weekday=%d", utcTime, weekday );
}


void emAfPrepaymentPrintAfTime( EmberAfTimeStruct *pafTime ){
  emberAfPrepaymentClusterPrintln("== AF TIME ==");
  emberAfPrepaymentClusterPrintln("  Year=%d", pafTime->year );
  emberAfPrepaymentClusterPrintln("  Month=%d", pafTime->month );
  emberAfPrepaymentClusterPrintln("  Day=%d", pafTime->day );
  emberAfPrepaymentClusterPrintln("  Hour=%d", pafTime->hours );
  emberAfPrepaymentClusterPrintln("  Min=%d", pafTime->minutes );
  emberAfPrepaymentClusterPrintln("  Sec=%d", pafTime->seconds );
}
  

#define SECONDS_PER_DAY (3600 * 24)
extern emDebtScheduleEntry DebtSchedule[];

void emAfPrepaymentScheduleDebtRepaymentCli(){
  int8u  endpoint;
  int8u  debtType;
  int16u collectionTime;
  int32u issuerEventId;
  int32u collectionTimeSec;
  int32u startTime;
  int8u  collectionFrequency;
  int8u  i;

  endpoint = (int8u)emberUnsignedCommandArgument( 0 );
  issuerEventId = (int32u)emberUnsignedCommandArgument( 1 );
  debtType = (int8u)emberUnsignedCommandArgument( 2 );
  collectionTime = (int16u)emberUnsignedCommandArgument( 3 );
  startTime = (int32u)emberUnsignedCommandArgument( 4 );
  collectionFrequency = (int8u)emberUnsignedCommandArgument( 5 );

  emberAfPluginPrepaymentServerScheduleDebtRepayment( endpoint, issuerEventId, debtType, collectionTime, startTime, collectionFrequency );

  // Convert collectionTime (mins) to seconds for future comparisons.
  collectionTimeSec = ((int32u)collectionTime) * 60;

  // After calling the ScheduleDebtRepayment() function, verify a couple things.
  if( debtType >= 3 ){
    emberAfPrepaymentClusterPrintln("Debt type out of bounds");
  }
  else{
    i = debtType;
    if( (DebtSchedule[i].firstCollectionTimeSec >= startTime) &&
        (DebtSchedule[i].issuerEventId == issuerEventId) &&
        (DebtSchedule[i].collectionFrequency == collectionFrequency) &&
        (DebtSchedule[i].nextCollectionTimeUtc >= startTime) &&
        ((DebtSchedule[i].nextCollectionTimeUtc % SECONDS_PER_DAY) == collectionTimeSec) &&
        ((DebtSchedule[i].firstCollectionTimeSec % SECONDS_PER_DAY) == collectionTimeSec) ){
      emberAfPrepaymentClusterPrintln("Valid Debt Schedule");
    }
    else{
      emberAfPrepaymentClusterPrintln("INVALID Debt Schedule");
      emberAfPrepaymentClusterPrintln("  first=%d, startTime=%d", DebtSchedule[i].firstCollectionTimeSec, startTime );
      emberAfPrepaymentClusterPrintln("  issuerEvtId=%d, %d", DebtSchedule[i].issuerEventId, issuerEventId );
      emberAfPrepaymentClusterPrintln("  collFreq=%d, %d", DebtSchedule[i].collectionFrequency, collectionFrequency );
      emberAfPrepaymentClusterPrintln("  nextColl=%d, startTime=%d", DebtSchedule[i].nextCollectionTimeUtc, startTime );
      emberAfPrepaymentClusterPrintln("  nextMOD=%d, collectTimeSec=%d", (DebtSchedule[i].nextCollectionTimeUtc % SECONDS_PER_DAY), collectionTimeSec );
      emberAfPrepaymentClusterPrintln("  firstMOD=%d, collectTimeSec=%d", (DebtSchedule[i].firstCollectionTimeSec % SECONDS_PER_DAY), collectionTimeSec );
   }
  }
}




