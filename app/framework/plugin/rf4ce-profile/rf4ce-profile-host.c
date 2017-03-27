// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile.h"
#include "rf4ce-profile-internal.h"

void emberAfPluginRf4ceProfileNcpInitCallback(boolean memoryAllocation)
{
  if (!memoryAllocation) {
    // Enable built-in support for supported profiles.
    ezspSetPolicy(EZSP_RF4CE_DISCOVERY_AND_PAIRING_PROFILE_BEHAVIOR_POLICY,
                  EZSP_RF4CE_DISCOVERY_AND_PAIRING_PROFILE_BEHAVIOR_ENABLED);

    // Push the list of supported device types to the NCP.
    ezspSetValue(EZSP_VALUE_RF4CE_SUPPORTED_DEVICE_TYPES_LIST,
                 emberAfRf4ceDeviceTypeListLength(emAfRf4ceApplicationInfo.capabilities),
                 emAfRf4ceApplicationInfo.deviceTypeList);

    // Push the list of supported profiles to the NCP.
    ezspSetValue(EZSP_VALUE_RF4CE_SUPPORTED_PROFILES_LIST,
                 emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities),
                 emAfRf4ceApplicationInfo.profileIdList);
  }
}

int8u emAfRf4ceGetBaseChannel(void)
{
  int8u value;
  int8u valueLength;

  ezspGetValue(EZSP_VALUE_RF4CE_BASE_CHANNEL, &valueLength, &value);

  return value;
}

void ezspRf4ceDiscoveryRequestHandler(EmberEUI64 ieeeAddr,
                                      int8u nodeCapabilities,
                                      EmberRf4ceVendorInfo *vendorInfo,
                                      EmberRf4ceApplicationInfo *appInfo,
                                      int8u searchDevType,
                                      int8u rxLinkQuality)
{
  emAfRf4ceDiscoveryRequestHandler(ieeeAddr,
                                   nodeCapabilities,
                                   vendorInfo,
                                   appInfo,
                                   searchDevType,
                                   rxLinkQuality);
}

void ezspRf4ceDiscoveryResponseHandler(boolean atCapacity,
                                       int8u channel,
                                       EmberPanId panId,
                                       EmberEUI64 ieeeAddr,
                                       int8u nodeCapabilities,
                                       EmberRf4ceVendorInfo *vendorInfo,
                                       EmberRf4ceApplicationInfo *appInfo,
                                       int8u rxLinkQuality,
                                       int8u discRequestLqi)
{
  emAfRf4ceDiscoveryResponseHandler(atCapacity,
                                    channel,
                                    panId,
                                    ieeeAddr,
                                    nodeCapabilities,
                                    vendorInfo,
                                    appInfo,
                                    rxLinkQuality,
                                    discRequestLqi);
}

void ezspRf4ceDiscoveryCompleteHandler(EmberStatus status)
{
  emAfRf4ceDiscoveryCompleteHandler(status);
}

void ezspRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   int8u nodeCapabilities,
                                                   EmberRf4ceVendorInfo *vendorInfo,
                                                   EmberRf4ceApplicationInfo *appInfo,
                                                   int8u searchDevType)
{
  emAfRf4ceAutoDiscoveryResponseCompleteHandler(status,
                                                srcIeeeAddr,
                                                nodeCapabilities,
                                                vendorInfo,
                                                appInfo,
                                                searchDevType);
}

void ezspRf4cePairRequestHandler(EmberStatus status,
                                 int8u pairingIndex,
                                 EmberEUI64 srcIeeeAddr,
                                 int8u nodeCapabilities,
                                 EmberRf4ceVendorInfo *vendorInfo,
                                 EmberRf4ceApplicationInfo *appInfo,
                                 int8u keyExchangeTransferCount)
{
  emAfRf4cePairRequestHandler(status,
                              pairingIndex,
                              srcIeeeAddr,
                              nodeCapabilities,
                              vendorInfo,
                              appInfo,
                              keyExchangeTransferCount);
}

void ezspRf4cePairCompleteHandler(EmberStatus status,
                                  int8u pairingIndex,
                                  EmberRf4ceVendorInfo *vendorInfo,
                                  EmberRf4ceApplicationInfo *appInfo)
{
  emAfRf4cePairCompleteHandler(status, pairingIndex, vendorInfo, appInfo);
}

void ezspRf4ceMessageSentHandler(EmberStatus status,
                                 int8u pairingIndex,
                                 int8u txOptions,
                                 int8u profileId,
                                 int16u vendorId,
                                 int8u messageTag,
                                 int8u messageLength,
                                 int8u *message)
{
  emAfRf4ceMessageSentHandler(status,
                              pairingIndex,
                              txOptions,
                              profileId,
                              vendorId,
                              messageTag,
                              messageLength,
                              message);
}

void ezspRf4ceIncomingMessageHandler(int8u pairingIndex,
                                     int8u profileId,
                                     int16u vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     int8u messageLength,
                                     int8u *message)

{
  emAfRf4ceIncomingMessageHandler(pairingIndex,
                                  profileId,
                                  vendorId,
                                  txOptions,
                                  messageLength,
                                  message);
}

void ezspRf4ceUnpairHandler(int8u pairingIndex)
{
  emAfRf4ceUnpairHandler(pairingIndex);
}

void ezspRf4ceUnpairCompleteHandler(int8u pairingIndex)
{
  emAfRf4ceUnpairCompleteHandler(pairingIndex);
}
