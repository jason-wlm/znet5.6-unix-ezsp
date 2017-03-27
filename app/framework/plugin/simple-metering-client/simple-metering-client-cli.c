// *******************************************************************
// * simple-metering-client-cli.c
// *
// *
// * Copyright 2014 by Silicon Labs. All rights reserved.                  *80*
// *******************************************************************

#include "../../include/af.h"
#include "../../util/common.h"

#if !defined(EMBER_AF_GENERATE_CLI)

void emAfPluginSimpleMeteringClientCliSchSnapshot(void);
void emAfPluginSimpleMeteringClientCliStartSampling(void);
void emAfPluginSimpleMeteringClientCliGetSampledData(void);
void emAfPluginSimpleMeteringClientCliLocalChangeSupply(void);

EmberCommandEntry emberAfPluginSimpleMeteringClientCommands[] = {
  emberCommandEntryAction("sch-snapshot",
                          emAfPluginSimpleMeteringClientCliSchSnapshot,
                          "vuuwuuuwwuw",
                          "Schedule a snapshot."),
  emberCommandEntryAction("start-sampling",
                          emAfPluginSimpleMeteringClientCliStartSampling,
                          "vuuwwuvv",
                          "Send a start sampling command to a metering server."),
  emberCommandEntryAction("get-sampled-data",
                          emAfPluginSimpleMeteringClientCliGetSampledData,
                          "vuuvwuv",
                          "Send a start sampling command to a metering server."),
  emberCommandEntryAction("local-change-supply",
                          emAfPluginSimpleMeteringClientCliLocalChangeSupply,
                          "vuuu",
                          "Send a start sampling command to a metering server."),
  emberCommandEntryTerminator(),
};

#endif /* !defined(EMBER_AF_GENERATE_CLI) */

void emAfPluginSimpleMeteringClientCliSchSnapshot(void)
{
  EmberStatus status;
  int8u payload[13];
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  int32u issuerId = (int32u)emberUnsignedCommandArgument(3);
  int8u commandIndex = (int8u)emberUnsignedCommandArgument(4);
  int8u numberOfCommands = (int8u)emberUnsignedCommandArgument(5);
  int8u snapshotScheduleId = (int8u)emberUnsignedCommandArgument(6);
  int32u startTime = (int32u)emberUnsignedCommandArgument(7);
  int32u snapshotSchedule = (int32u)emberUnsignedCommandArgument(8);
  int8u snapshotType = (int8u)emberUnsignedCommandArgument(9);
  int32u snapshotCause = (int32u)emberUnsignedCommandArgument(10);


  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);

  payload[0] = snapshotScheduleId;
  emberAfCopyInt32u((int8u *)payload, 1, startTime);
  emberAfCopyInt24u((int8u *)payload, 5, snapshotSchedule);
  payload[8] = snapshotType;
  emberAfCopyInt32u((int8u *)payload, 9, snapshotCause);

  emberAfFillCommandSimpleMeteringClusterScheduleSnapshot(issuerId,
                                                          commandIndex,
                                                          numberOfCommands,
                                                          payload,
                                                          13);

  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);
}

void emAfPluginSimpleMeteringClientCliStartSampling(void)
{
  EmberStatus status;
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  int32u issuerId = (int32u)emberUnsignedCommandArgument(3);
  int32u startTime = (int32u)emberUnsignedCommandArgument(4);
  int8u sampleType = (int8u)emberUnsignedCommandArgument(5);
  int16u sampleRequestInterval = (int16u)emberUnsignedCommandArgument(6);
  int16u maxNumberOfSamples = (int16u)emberUnsignedCommandArgument(7);

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND \
                             | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER), \
                            ZCL_SIMPLE_METERING_CLUSTER_ID, \
                            ZCL_START_SAMPLING_COMMAND_ID, \
                            "wwuvv", \
                            issuerId, \
                            startTime, \
                            sampleType, \
                            sampleRequestInterval, \
                            maxNumberOfSamples);
  // emberAfFillCommandSimpleMeteringClusterStartSampling(issuerId,
  //                                                      startTime,
  //                                                      sampleType,
  //                                                      sampleRequestInterval,
  //                                                      maxNumberOfSamples);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);
}

void emAfPluginSimpleMeteringClientCliGetSampledData(void)
{
  EmberStatus status;
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  int16u sampleId = (int16u)emberUnsignedCommandArgument(3);
  int32u startTime = (int32u)emberUnsignedCommandArgument(4);
  int8u sampleType = (int8u)emberUnsignedCommandArgument(5);
  int16u numberOfSamples = (int16u)emberUnsignedCommandArgument(6);

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  
  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND \
                           | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                           | ZCL_DISABLE_DEFAULT_RESPONSE_MASK), \
                          ZCL_SIMPLE_METERING_CLUSTER_ID, \
                          ZCL_GET_SAMPLED_DATA_COMMAND_ID, \
                          "vwuv", \
                          sampleId, \
                          startTime, \
                          sampleType, \
                          numberOfSamples);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);
}

void emAfPluginSimpleMeteringClientCliLocalChangeSupply(void)
{
  EmberStatus status;
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);  
  int8u proposedChange = (int8u)emberUnsignedCommandArgument(3);
  
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND \
                           | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                           | ZCL_DISABLE_DEFAULT_RESPONSE_MASK), \
                          ZCL_SIMPLE_METERING_CLUSTER_ID, \
                          ZCL_LOCAL_CHANGE_SUPPLY_COMMAND_ID, \
                          "u", \
                          proposedChange);

  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);
  
}
