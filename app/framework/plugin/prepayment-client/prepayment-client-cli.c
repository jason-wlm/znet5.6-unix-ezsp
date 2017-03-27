// *****************************************************************************
// * prepayment-server-cli.c
// *
// * Routines for interacting with the prepayment-server.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "prepayment-client.h"


/******
  TEMPORARY FIX -
    According to Bob, we believe these types should be auto-generated and put into enums.h
    Currently that is not the case.  For now, define these here.
 ******/

typedef int16u PaymentControlConfiguration;
typedef int32u PrepaySnapshotPayloadCause;
typedef int8u  PrepaySnapshotPayloadType;
typedef int8u  FriendlyCredit;

void emAfPrepaymentClientCliChangePaymentMode( void );

void emAfPrepaymentClientCliChangePaymentMode(){
  EmberNodeId nodeId;
  int8u srcEndpoint, dstEndpoint;
  int32u providerId, issuerEventId;
  int32u implementationDateTime;
  PaymentControlConfiguration proposedPaymentControlConfiguration;
  int32u cutOffValue;


  nodeId = (EmberNodeId)emberUnsignedCommandArgument( 0 );
  srcEndpoint = (int8u)emberUnsignedCommandArgument( 1 );
  dstEndpoint = (int8u)emberUnsignedCommandArgument( 2 );
  providerId = (int32u)emberUnsignedCommandArgument( 3 );
  issuerEventId = (int32u)emberUnsignedCommandArgument( 4 );

  implementationDateTime = (int32u)emberUnsignedCommandArgument( 5 );
  proposedPaymentControlConfiguration = (PaymentControlConfiguration)emberUnsignedCommandArgument( 6 );
  cutOffValue = (int32u)emberUnsignedCommandArgument( 7 );

  //emberAfAppPrintln("RX Publish Prepay Snapshot Cmd, varLen=%d", i );
  emberAfAppPrintln("Change Payment Mode, srcEp=%d, dstEp=%d, addr=%2x", srcEndpoint, dstEndpoint, nodeId );
  emberAfPluginPrepaymentClientChangePaymentMode( nodeId, srcEndpoint, dstEndpoint, providerId, issuerEventId,
                                                  implementationDateTime, proposedPaymentControlConfiguration,
                                                  cutOffValue );
}



