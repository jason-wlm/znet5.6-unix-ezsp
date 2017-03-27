// *****************************************************************************
// * prepayment-topup-storage.c
// *
// * Implemented routines for storing prepayment topups.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************


#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "prepayment-topup.h"

// This function is called when a top up command is received.  The callback must determine if the top up
// command is valid or not.
//extern boolean emberAfPluginPrepaymentServerConsumerTopUpCallback(int8u originatingDevice, int8u* topUpCode);

boolean consumerTopUpIsValid( int8u *topUpCode );
void emAfPrintPublishTopUpPayload( TopUpPayload *ptopUpPayload, int8u index );

#define UTRN_HIGH_WORD_BASE_VALUE  0x669D529B
#define UTRN_LOW_WORD_BASE_VALUE   0x714A0000

#define PTUT_HEADER_BITMASK                 0xFE000000      // Subclass should be non-zero
#define PTUT_VALUE_CLASS_BITMASK_ONE_UNIT   0x01800000
#define PTUT_VALUE_BITMASK                  0x007FFC00
#define PTUT_ORIGINATOR_COUNTER_BITMASK     0x000003FF

#define PTUT_VALUE_DOWNSHIFT 10

/***********************************************************************
  NOTES about Consumer Top Up command UTRN:
  GBCS, Chapter 14 - Consumer Top Up UTRN is constructed as:
  aaab bbbc cyyy yyyy yyyy yyzz zzzz zzzz mmmm ... mmmm
  where
  [  ========   Prepayment Top Up Token (PTUT)  ========  ]
  aaa:  3 bits - PTUT Lead (fixed 0b000 value)
  bbbb: 4 bits - PTUT Sub Class (fixed 0b0000 value)
  cc:   2 bits - PTUT Value Class (0b00 = 1/100th of currency unit, 0b01 = 1 currency unit)
  yyy..yy: 13 bits - PTUT Value
  zzz..zz: 10 bits - Truncated Originator Counter
  mmm..mm: 32 bits - Supplier MAC

  first two digits of PPTD ranges from 73 to 96.
  Value ranges from 7,394,156,990,786,306,048 (0x669D529B 714A0000) 
  to  0x669D529B 714A0000 + (0x00FF FFFF FFFF FFFF) = 0x679D529B7149FFFF   (=7,466,214,584,824,233,983)
  int32u highWord, lowWord;
  int32u utrnCounter;

  highWord = (((int32u)topUpCode[0]) << 8) + topUpCode[1];
  highWord <<= 16;
  highWord += (((int32u)topUpCode[2]) << 8) + topUpCode[3];
  highWord -= UTRN_HIGH_WORD_BASE_VALUE;
  utrnCounter = highWord & PTUT_ORIGINATOR_COUNTER_BITMASK;

  Is UTRN Counter in UTRN counter cache?  (GBCS 14.3.6)
  If not in cache, add UTRN counter to cache - 100 entries, circular buffer (GBCS 14.1).

  Compare the amount with the "MaxCreditPerTopUp" attribute (D.7.2.2.1.12).  Is this amount < the attribute value?
  Compare the total credit (current + top up value) with the "MaxCreditLimit" attribute (D.7.2.2.1.11).  Is this
  amount < the attribute value?
  NOTE: These attribute values depend on the prepayment mode (currency or units), which then depend on the currency
  specified in the price cluster, or the units specified in the metering cluster.


  Low Word is the Supplier MAC.
  lowWord = (((int32u)topUpCode[4]) << 8) + topUpCode[5];
  lowWord <<= 16;
  lowWord += (((int32u)topUpCode[6]) << 8) + topUpCode[7];
  lowWord -= UTRN_LOW_WORD_BASE_VALUE;

***********************************************************************/


#define TOP_UP_ATTRIBUTE_GROUP_DELTA  0x10

boolean emberAfPrepaymentClusterConsumerTopUpCallback(int8u originatingDevice, int8u* topUpCode){
  EmberAfStatus status;
  int8u  endpoint;
  int16u attributeId;
  int32u topUpDateTime;
  int32s topUpAmount;
  int8u  topUpOriginatingDevice;
  int8u  topUpCodeRead[27];
  int8u  dataType;

  int8u i;

  topUpDateTime = emberAfGetCurrentTime();

  emberAfPrepaymentClusterPrintln("RX: Consumer Top Up Callback, time=%d", topUpDateTime);
  endpoint = emberAfCurrentEndpoint();

  if( !emberAfPluginPrepaymentServerConsumerTopUpCallback(originatingDevice, topUpCode) ){
    // TODO:  Do what?  -- Send default response?
    return FALSE;
  }


  // Before updating the Top Up #1 attribute set, push the existing #1,#2,#3,#4 attributes down.
  // So Date/Time#1 becomes Date/Time#2, Amount#1 becomes Amount#2, etc.
  // These are optional attributes, so don't care about the read/write return status.
  //for( i=0; i<4; i++ ){
  for( i=4; i>0; ){
    i--;
    attributeId = ( ZCL_TOP_UP_DATE_TIME_1_ATTRIBUTE_ID + (i * TOP_UP_ATTRIBUTE_GROUP_DELTA) );
    status = emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                                   attributeId, CLUSTER_MASK_SERVER, (int8u *)&topUpDateTime, 4, &dataType );
    if( status == EMBER_ZCL_STATUS_SUCCESS ){
      emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, (attributeId + TOP_UP_ATTRIBUTE_GROUP_DELTA),
                                  CLUSTER_MASK_SERVER, (int8u *)&topUpDateTime, dataType );
    }

    attributeId = ( ZCL_TOP_UP_AMOUNT_1_ATTRIBUTE_ID + (i * TOP_UP_ATTRIBUTE_GROUP_DELTA) );
    status = emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                                   attributeId, CLUSTER_MASK_SERVER, (int8u *)&topUpAmount, 4, &dataType );
    if( status == EMBER_ZCL_STATUS_SUCCESS ){
      emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, (attributeId + TOP_UP_ATTRIBUTE_GROUP_DELTA),
                                  CLUSTER_MASK_SERVER, (int8u *)&topUpAmount, dataType );
    }

    attributeId = ( ZCL_TOP_UP_ORIGINATING_DEVICE_1_ATTRIBUTE_ID + (i * TOP_UP_ATTRIBUTE_GROUP_DELTA) );
    status = emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                                   attributeId, CLUSTER_MASK_SERVER, &topUpOriginatingDevice, 1, &dataType );
    if( status == EMBER_ZCL_STATUS_SUCCESS ){
      emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, (attributeId + TOP_UP_ATTRIBUTE_GROUP_DELTA),
                                  CLUSTER_MASK_SERVER, &topUpOriginatingDevice, dataType );
    }

    attributeId = ( ZCL_TOP_UP_CODE_1_ATTRIBUTE_ID + (i * TOP_UP_ATTRIBUTE_GROUP_DELTA) );
    status = emberAfReadAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,
                                   attributeId, CLUSTER_MASK_SERVER, topUpCodeRead, 26, &dataType );
    if( status == EMBER_ZCL_STATUS_SUCCESS ){
      status = emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, (attributeId + TOP_UP_ATTRIBUTE_GROUP_DELTA),
                                  CLUSTER_MASK_SERVER, topUpCodeRead, dataType );
    }

  }

  // Now write the #1 attribute set with values from the top up command.

  topUpDateTime = emberAfGetCurrentTime();
  status = emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID,  ZCL_TOP_UP_DATE_TIME_1_ATTRIBUTE_ID, 
                                  CLUSTER_MASK_SERVER, (int8u *)&topUpDateTime, ZCL_UTC_TIME_ATTRIBUTE_TYPE );

  status = emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_TOP_UP_ORIGINATING_DEVICE_1_ATTRIBUTE_ID,
                                  CLUSTER_MASK_SERVER, &originatingDevice, ZCL_ENUM8_ATTRIBUTE_TYPE );

  status = emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_TOP_UP_CODE_1_ATTRIBUTE_ID, 
                                  CLUSTER_MASK_SERVER, topUpCode, ZCL_OCTET_STRING_ATTRIBUTE_TYPE );

// TODO:   How to extract the amount from the Top Up command??

//  topUpAmount = (((int32u)topUpCode[0]) << 8) + topUpCode[1];
//  topUpAmount <<= 16;
//  topUpAmount += (((int32u)topUpCode[2]) << 8) + topUpCode[3];
//  topUpAmount -= UTRN_HIGH_WORD_BASE_VALUE;
 // topUpAmount &= PTUT_VALUE_BITMASK;
 // topUpAmount >>= PTUT_VALUE_DOWNSHIFT;
 // status = emberAfWriteAttribute( endpoint, ZCL_PREPAYMENT_CLUSTER_ID, ZCL_TOP_UP_AMOUNT_1_ATTRIBUTE_ID, 
 //                                 CLUSTER_MASK_SERVER, topUpAmount, ZCL_INT32S_ATTRIBUTE_TYPE );


  return TRUE;
}


#define MAX_TOP_UPS   5

boolean emberAfPrepaymentClusterGetTopUpLogCallback( int32u latestEndTime, int8u numberOfRecords ){

  EmberNodeId nodeId;
  int8u srcEndpoint, dstEndpoint;

  if( numberOfRecords == 0 ){
    // Requesting maximum possible.
    numberOfRecords = MAX_TOP_UPS;
  }

  
  emberAfPrepaymentClusterPrintln("RX: GetTopUpLog, endTime=0x%4x, numRec=%d", latestEndTime, numberOfRecords );

  nodeId = emberAfCurrentCommand()->source;
  srcEndpoint = emberAfGetCommandApsFrame()->destinationEndpoint;
  dstEndpoint = emberAfGetCommandApsFrame()->sourceEndpoint;

  emberAfPluginSendPublishTopUpLog( nodeId, srcEndpoint, dstEndpoint, latestEndTime, numberOfRecords );

  return TRUE;

}

static int8u NumTopUpPayloads;
#define CMD_INDEX_LAST_CMD  0xFE

void emberAfPluginSendPublishTopUpLog( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint, 
                                       int32u latestEndTime, int8u numberOfRecords ){
  
  // Figure out how many records to send, then create
  // TopUpPayload structure array and populate response.
  // Then call emberAfPluginPrepaymentServerPublishTopUpLog().
  EmberAfStatus status;
  TopUpPayload topUpPayloads[ MAX_TOP_UPS ];
  int8u topUpCodes[27 * MAX_TOP_UPS];
  int32u topUpAmount;
  int32u topUpTime;
  int16u attributeId;
  int8u *ptopUpCode;

  int8u i;
  int8u matchCnt = 0;

  if( numberOfRecords == 0 ){
    numberOfRecords = MAX_TOP_UPS;
  }

  emberAfPrepaymentClusterPrintln("Send Publish Top Up Log, %d entries", numberOfRecords );
  ptopUpCode = topUpCodes;

  for( i=0; i<MAX_TOP_UPS; i++ ){
    // Read TOP UP CODE string, TopUpAmount U32, TopUpTime UTC_Time.
    attributeId = ZCL_TOP_UP_DATE_TIME_1_ATTRIBUTE_ID + (i*0x10);
    status = emberAfReadAttribute( srcEndpoint, ZCL_PREPAYMENT_CLUSTER_ID, attributeId,
                          CLUSTER_MASK_SERVER, (int8u *)&topUpTime, 4, NULL );

    if( (status == EMBER_ZCL_STATUS_SUCCESS) && (topUpTime <= latestEndTime) ){

      attributeId = ZCL_TOP_UP_CODE_1_ATTRIBUTE_ID + (i*0x10);
      status = emberAfReadAttribute( srcEndpoint, ZCL_PREPAYMENT_CLUSTER_ID, attributeId,
                            CLUSTER_MASK_SERVER, ptopUpCode, 26, NULL );
      
      attributeId = ZCL_TOP_UP_AMOUNT_1_ATTRIBUTE_ID + (i*0x10);
      status |= emberAfReadAttribute( srcEndpoint, ZCL_PREPAYMENT_CLUSTER_ID, attributeId,
                            CLUSTER_MASK_SERVER, (int8u *)&topUpAmount, 4, NULL );
      
      if( status == EMBER_ZCL_STATUS_SUCCESS ){
//        emberAfPutStringInResp( topUpCode );
//        emberAfPutInt32uInResp( topUpAmount );
//        emberAfPutInt32uInResp( topUpTime );
        topUpPayloads[matchCnt].topUpCode = ptopUpCode;
        topUpPayloads[matchCnt].topUpAmount = topUpAmount;
        topUpPayloads[matchCnt].topUpTime = topUpTime;
        matchCnt++;
        ptopUpCode += 26;
        if( matchCnt >= numberOfRecords ){
          break;
        }
      }
      else{
        // failure status - abort since higher groups of these attributes are also not supported.
        break;
      }
    }
  }
  NumTopUpPayloads = matchCnt;
  if( matchCnt > 0 ){
    // Command is now ready to be sent.
    emberAfPluginPrepaymentServerPublishTopUpLog( nodeId, srcEndpoint, dstEndpoint, CMD_INDEX_LAST_CMD, 1, topUpPayloads );
  }
  else{
    emberAfPrepaymentClusterPrintln("No Matching Top Up Log Entries");
    emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_NOT_FOUND );
  }
}


void emAfPrintPublishTopUpPayload( TopUpPayload *ptopUpPayload, int8u index ){
  emberAfPrepaymentClusterPrintln("= Top Up Payload %d", index );
  emberAfPrepaymentClusterPrintln("  Code=%s",  ptopUpPayload->topUpCode );
  emberAfPrepaymentClusterPrintln("  Amount=%d",  ptopUpPayload->topUpAmount );
  emberAfPrepaymentClusterPrintln("  Time=%d",  ptopUpPayload->topUpTime );
}


// Called when a Get Top Up Log command is received from a prepayment client, or
// when a new top up is performed.
// NOTE:  TopUpPayload is a struct type.  See se-meter-gas/gen/simulation/af-structs.h
void emberAfPluginPrepaymentServerPublishTopUpLog( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint,
                                                   int8u commandIndex, int8u totalNumberOfCommands,
                                                   TopUpPayload *topUpPayload  ){
  EmberStatus status;
  int8u i;

  emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );

  // cmdIndex=0xFE, totalNumCmds=1.   APS Fragmentation will deal with message fragmenting if necessary.
  emberAfFillExternalBuffer( (ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_PUBLISH_TOP_UP_LOG_COMMAND_ID,
                              "uu",
                              commandIndex, totalNumberOfCommands );

  for( i=0; i<NumTopUpPayloads; i++ ){
    emberAfPutStringInResp( topUpPayload[i].topUpCode );
    emberAfPutInt32uInResp( topUpPayload[i].topUpAmount );
    emberAfPutInt32uInResp( topUpPayload[i].topUpTime );
    emAfPrintPublishTopUpPayload( &topUpPayload[i], i );
  }

  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
  if( status == EMBER_SUCCESS ){
    emberAfPrepaymentClusterPrintln("Sent Publish Top Up Log, %d top ups", NumTopUpPayloads );
  }
  else{
    emberAfPrepaymentClusterPrintln("Error in Publish Top Up Log %x", status );
  }

}



