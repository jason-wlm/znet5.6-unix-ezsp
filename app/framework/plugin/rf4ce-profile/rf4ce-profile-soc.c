// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile-internal.h"

void emberAfPluginRf4ceProfileNcpInitCallback(boolean memoryAllocation)
{
}

boolean emberRf4ceDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                          int8u nodeCapabilities,
                                          EmberRf4ceVendorInfo *vendorInfo,
                                          EmberRf4ceApplicationInfo *appInfo,
                                          int8u searchDevType,
                                          int8u rxLinkQuality)
{
  return emAfRf4ceDiscoveryRequestHandler(srcIeeeAddr,
                                          nodeCapabilities,
                                          vendorInfo,
                                          appInfo,
                                          searchDevType,
                                          rxLinkQuality);
}

boolean emberRf4ceDiscoveryResponseHandler(boolean atCapacity,
                                           int8u channel,
                                           EmberPanId panId,
                                           EmberEUI64 srcIeeeAddr,
                                           int8u nodeCapabilities,
                                           EmberRf4ceVendorInfo *vendorInfo,
                                           EmberRf4ceApplicationInfo *appInfo,
                                           int8u rxLinkQuality,
                                           int8u discRequestLqi)
{
  return emAfRf4ceDiscoveryResponseHandler(atCapacity,
                                           channel,
                                           panId,
                                           srcIeeeAddr,
                                           nodeCapabilities,
                                           vendorInfo,
                                           appInfo,
                                           rxLinkQuality,
                                           discRequestLqi);
}

void emberRf4ceDiscoveryCompleteHandler(EmberStatus status)
{
  emAfRf4ceDiscoveryCompleteHandler(status);
}

void emberRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
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

boolean emberRf4cePairRequestHandler(EmberStatus status,
                                     int8u pairingIndex,
                                     EmberEUI64 srcIeeeAddr,
                                     int8u nodeCapabilities,
                                     EmberRf4ceVendorInfo *vendorInfo,
                                     EmberRf4ceApplicationInfo *appInfo,
                                     int8u keyExchangeTransferCount)
{
  return emAfRf4cePairRequestHandler(status,
                                     pairingIndex,
                                     srcIeeeAddr,
                                     nodeCapabilities,
                                     vendorInfo,
                                     appInfo,
                                     keyExchangeTransferCount);
}

void emberRf4cePairCompleteHandler(EmberStatus status,
                                   int8u pairingIndex,
                                   EmberRf4ceVendorInfo *vendorInfo,
                                   EmberRf4ceApplicationInfo *appInfo)
{
  emAfRf4cePairCompleteHandler(status, pairingIndex, vendorInfo, appInfo);
}

void emberRf4ceMessageSentHandler(EmberStatus status,
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

void emberRf4ceIncomingMessageHandler(int8u pairingIndex,
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

void emberRf4ceUnpairHandler(int8u pairingIndex)
{
  emAfRf4ceUnpairHandler(pairingIndex);
}

void emberRf4ceUnpairCompleteHandler(int8u pairingIndex)
{
  emAfRf4ceUnpairCompleteHandler(pairingIndex);
}
