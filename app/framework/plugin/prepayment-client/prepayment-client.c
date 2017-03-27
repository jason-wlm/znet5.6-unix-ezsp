
// *****************************************************************************
// * prepayment-client.c
// *
// * Implemented routines for prepayment client.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************


#include "app/framework/include/af.h"
#include "prepayment-client.h"


void emberAfPluginPrepaymentClientChangePaymentMode( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint, int32u providerId, int32u issuerEventId, int32u implementationDateTime, int16u proposedPaymentControlConfiguration, int32u cutOffValue ){
  
  EmberStatus status;
  int8u ep = emberAfFindClusterClientEndpointIndex( srcEndpoint, ZCL_PREPAYMENT_CLUSTER_ID );
  if( ep == 0xFF ){
    emberAfAppPrintln("==== NO PREPAYMENT CLIENT ENDPOINT");
    return;
  }
  
  emberAfFillCommandPrepaymentClusterChangePaymentMode( providerId, issuerEventId, implementationDateTime, 
                                                        proposedPaymentControlConfiguration, cutOffValue );
  emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
  emberAfAppPrintln("=====   SEND PAYMENT MODE stat=0x%x", status );

}


boolean emberAfPrepaymentClusterChangePaymentModeResponseCallback( int8u friendlyCredit, int32u friendlyCreditCalendarId, 
                                                            int32u emergencyCreditLimit, int32u emergencyCreditThreshold ){

  emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  emberAfAppPrintln("RX: Change Payment Mode Response Callback");
  return TRUE;
}

boolean emberAfPrepaymentClusterPublishPrepaySnapshotCallback( int32u snapshotId, int32u snapshotTime, int8u totalSnapshotsFound,
                                                               int8u commandIndex, int8u totalNumberOfCommands, 
                                                               int32u snapshotCause, int8u snapshotPayloadType, int8u *snapshotPayload ){
  emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  emberAfAppPrintln("RX: Publish Prepay Snapshot Callback");
  return TRUE;
}

boolean emberAfPrepaymentClusterPublishTopUpLogCallback( int8u commandIndex, int8u totalNumberOfCommands, int8u *topUpPayload ){
  emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  emberAfAppPrintln("RX: Publish TopUp Log Callback");
  return TRUE;
}

boolean emberAfPrepaymentClusterPublishDebtLogCallback( int8u commandIndex, int8u totalNumberOfCommands, int8u *debtPayload ){
  emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  emberAfAppPrintln("RX: Publish Debt Log Callback");
  emberAfPrepaymentClusterPrintln("  commandIndex=%d", commandIndex);
  emberAfPrepaymentClusterPrintln("  totalNumberOfCommands=%d", totalNumberOfCommands);
  return TRUE;
}

  

