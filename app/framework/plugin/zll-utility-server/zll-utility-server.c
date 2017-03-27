// *******************************************************************
// * zll-utility-server.c
// *
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"

static int8u getMaxLength(void)
{
  int8u maxLength = emberAfMaximumApsPayloadLength(EMBER_OUTGOING_DIRECT,
                                                   emberAfResponseDestination,
                                                   &emberAfResponseApsFrame);
  if (EMBER_AF_RESPONSE_BUFFER_LEN < maxLength) {
    maxLength = EMBER_AF_RESPONSE_BUFFER_LEN;
  }
  return maxLength;
}

boolean emberAfZllCommissioningClusterGetGroupIdentifiersRequestCallback(int8u startIndex)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u total = emberAfPluginZllCommissioningGroupIdentifierCountCallback(endpoint);
  int8u i, *count, maxLength;

  emberAfZllCommissioningClusterPrintln("RX: GetGroupIdentifiersRequest 0x%x",
                                        startIndex);

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                             | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT
                             | EMBER_AF_DEFAULT_RESPONSE_POLICY_RESPONSES),
                            ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                            ZCL_GET_GROUP_IDENTIFIERS_RESPONSE_COMMAND_ID,
                            "uu",
                            total,
                            startIndex);

  count = &appResponseData[appResponseLength];
  emberAfPutInt8uInResp(0); // temporary count

  maxLength = getMaxLength();
  for (i = startIndex; i < total && appResponseLength + 3 <= maxLength; i++) {
    EmberAfPluginZllCommissioningGroupInformationRecord record;
    if (emberAfPluginZllCommissioningGroupIdentifierCallback(endpoint,
                                                             i,
                                                             &record)) {
      emberAfPutInt16uInResp(record.groupId);
      emberAfPutInt8uInResp(record.groupType);
      (*count)++;
    }
  }

  emberAfSendResponse();
  return TRUE;
}

boolean emberAfZllCommissioningClusterGetEndpointListRequestCallback(int8u startIndex)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u total = emberAfPluginZllCommissioningEndpointInformationCountCallback(endpoint);
  int8u i, *count, maxLength;

  emberAfZllCommissioningClusterPrintln("RX: GetEndpointListRequest 0x%x",
                                        startIndex);

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                             | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT
                             | EMBER_AF_DEFAULT_RESPONSE_POLICY_RESPONSES),
                            ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                            ZCL_GET_ENDPOINT_LIST_RESPONSE_COMMAND_ID,
                            "uu",
                            total,
                            startIndex);

  count = &appResponseData[appResponseLength];
  emberAfPutInt8uInResp(0); // temporary count

  maxLength = getMaxLength();
  for (i = startIndex; i < total && appResponseLength + 8 <= maxLength; i++) {
    EmberAfPluginZllCommissioningEndpointInformationRecord record;
    if (emberAfPluginZllCommissioningEndpointInformationCallback(endpoint,
                                                                 i,
                                                                 &record)) {
      emberAfPutInt16uInResp(record.networkAddress);
      emberAfPutInt8uInResp(record.endpointId);
      emberAfPutInt16uInResp(record.profileId);
      emberAfPutInt16uInResp(record.deviceId);
      emberAfPutInt8uInResp(record.version);
      (*count)++;
    }
  }

  emberAfSendResponse();
  return TRUE;
}
