/** @file ezsp-callbacks.c
 * @brief Convenience stubs for little-used EZSP callbacks. 
 *
 * <!--Copyright 2006 by Ember Corporation. All rights reserved.         *80*-->
 */

#include PLATFORM_HEADER 
#include "stack/include/ember-types.h"
#include "stack/include/error.h"

// *****************************************
// Convenience Stubs
// *****************************************

#ifndef EZSP_APPLICATION_HAS_WAITING_FOR_RESPONSE
void ezspWaitingForResponse(void)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_NO_CALLBACKS
void ezspNoCallbacks(void)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_TIMER_HANDLER
void ezspTimerHandler(int8u timerId)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_DEBUG_HANDLER
void ezspDebugHandler(int8u messageLength, 
                      int8u *messageContents)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_CHILD_JOIN_HANDLER
void ezspChildJoinHandler(int8u index,
                          boolean joining,
                          EmberNodeId childId,
                          EmberEUI64 childEui64,
                          EmberNodeType childType)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_TRUST_CENTER_JOIN_HANDLER
void ezspTrustCenterJoinHandler(EmberNodeId newNodeId,
                                EmberEUI64 newNodeEui64,
                                EmberDeviceUpdate status,
                                EmberJoinDecision policyDecision,
                                EmberNodeId parentOfNewNode)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_ZIGBEE_KEY_ESTABLISHMENT_HANDLER
void ezspZigbeeKeyEstablishmentHandler(EmberEUI64 partner, EmberKeyStatus status)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_SWITCH_NETWORK_KEY_HANDLER
void ezspSwitchNetworkKeyHandler(int8u sequenceNumber)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_REMOTE_BINDING_HANDLER
void ezspRemoteSetBindingHandler(EmberBindingTableEntry *entry,
                                 int8u index,
                                 EmberStatus policyDecision)
{}

void ezspRemoteDeleteBindingHandler(int8u index,
                                    EmberStatus policyDecision)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_POLL_COMPLETE_HANDLER
void ezspPollCompleteHandler(EmberStatus status) {}
#endif

#ifndef EZSP_APPLICATION_HAS_POLL_HANDLER
void ezspPollHandler(EmberNodeId childId) {}
#endif

#ifndef EZSP_APPLICATION_HAS_ENERGY_SCAN_RESULT_HANDLER
void ezspEnergyScanResultHandler(int8u channel, int8s maxRssiValue)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_ROUTE_RECORD_HANDLER
void ezspIncomingRouteRecordHandler(EmberNodeId source,
                                    EmberEUI64 sourceEui,
                                    int8u lastHopLqi,
                                    int8s lastHopRssi,
                                    int8u relayCount,
                                    int8u *relayList)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_BUTTON_HANDLER
void halButtonIsr(int8u button, int8u state)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_INCOMING_SENDER_EUI64_HANDLER
void ezspIncomingSenderEui64Handler(EmberEUI64 senderEui64)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_ID_CONFLICT_HANDLER
void ezspIdConflictHandler(EmberNodeId id)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_INCOMING_MANY_TO_ONE_ROUTE_REQUEST_HANDLER
void ezspIncomingManyToOneRouteRequestHandler(EmberNodeId source,
                                              EmberEUI64 longId,
                                              int8u cost)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_INCOMING_ROUTE_ERROR_HANDLER
void ezspIncomingRouteErrorHandler(EmberStatus status, EmberNodeId target)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_BOOTLOADER_HANDLER
void ezspIncomingBootloadMessageHandler(EmberEUI64 longId,
                                        int8u lastHopLqi,
                                        int8s lastHopRssi,
                                        int8u messageLength,
                                        int8u *messageContents)
{}

void ezspBootloadTransmitCompleteHandler(EmberStatus status,
                                         int8u messageLength,
                                         int8u *messageContents)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_MAC_PASSTHROUGH_HANDLER
void ezspMacPassthroughMessageHandler(int8u messageType,
                                      int8u lastHopLqi,
                                      int8s lastHopRssi,
                                      int8u messageLength,
                                      int8u *messageContents)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_MAC_FILTER_MATCH_HANDLER
void ezspMacFilterMatchMessageHandler(int8u filterIndexMatch,
                                      int8u legacyPassthroughType,
                                      int8u lastHopLqi,
                                      int8s lastHopRssi,
                                      int8u messageLength,
                                      int8u *messageContents)
{
  ezspMacPassthroughMessageHandler(legacyPassthroughType,
                                   lastHopLqi,
                                   lastHopRssi,
                                   messageLength,
                                   messageContents);
}
#endif

#ifndef EZSP_APPLICATION_HAS_MFGLIB_HANDLER
void ezspMfglibRxHandler(
      int8u linkQuality,
      int8s rssi,
      int8u packetLength,
      int8u *packetContents)
{}
#endif

#ifndef EZSP_APPLICATION_HAS_RAW_HANDLER
void ezspRawTransmitCompleteHandler(EmberStatus status)
{}
#endif

// Certificate Based Key Exchange (CBKE)
#ifndef EZSP_APPLICATION_HAS_GENERATE_CBKE_KEYS_HANDLER
void ezspGenerateCbkeKeysHandler(EmberStatus status,
                                 EmberPublicKeyData* ephemeralPublicKey)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_CALCULATE_SMACS_HANDLER
void ezspCalculateSmacsHandler(EmberStatus status,
                               EmberSmacData* initiatorSmac,
                               EmberSmacData* responderSmac)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_GENERATE_CBKE_KEYS_HANDLER_283K1
void ezspGenerateCbkeKeysHandler283k1(EmberStatus status,
                                      EmberPublicKey283k1Data* ephemeralPublicKey)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_CALCULATE_SMACS_HANDLER_283K1
void ezspCalculateSmacsHandler283k1(EmberStatus status,
                                    EmberSmacData* initiatorSmac,
                                    EmberSmacData* responderSmac)
{
}
#endif

// Elliptical Cryptography Digital Signature Algorithm (ECDSA)
#ifndef EZSP_APPLICATION_HAS_DSA_SIGN_HANDLER
void ezspDsaSignHandler(EmberStatus status,
                        int8u messageLength,
                        int8u* messageContents)
{
}
#endif

// Elliptical Cryptography Digital Signature Verification
#ifndef EZSP_APPLICATION_HAS_DSA_VERIFY_HANDLER
void ezspDsaVerifyHandler(EmberStatus status)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_FRAGMENT_SOURCE_ROUTE_HANDLER
EmberStatus ezspFragmentSourceRouteHandler(void)
{
  return EMBER_SUCCESS;
}
#endif

#ifndef EZSP_APPLICATION_HAS_CUSTOM_FRAME_HANDLER
void ezspCustomFrameHandler(int8u payloadLength, int8u* payload)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_STACK_TOKEN_CHANGED_HANDLER
void ezspStackTokenChangedHandler(int16u tokenAddress)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_ZLL_NETWORK_FOUND_HANDLER
void ezspZllNetworkFoundHandler(const EmberZllNetwork* networkInfo,
                                const EmberZllDeviceInfoRecord* deviceInfo)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_ZLL_SCAN_COMPLETE_HANDLER
void ezspZllScanCompleteHandler(EmberStatus status)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_ZLL_ADDRESS_ASSIGNMENT_HANDLER
void ezspZllAddressAssignmentHandler(const EmberZllAddressAssignment* addressInfo)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_ZLL_TOUCH_LINK_TARGET_HANDLER
void ezspZllTouchLinkTargetHandler(const EmberZllNetwork* networkInfo)
{
}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_DISCOVERY_REQUEST_HANDLER
void ezspRf4ceDiscoveryRequestHandler(EmberEUI64 ieeeAddr,
                                      int8u nodeCapabilities,
                                      EmberRf4ceVendorInfo *vendorInfo,
                                      EmberRf4ceApplicationInfo *appInfo,
                                      int8u searchDevType,
                                      int8u rxLinkQuality) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_DISCOVERY_RESPONSE_HANDLER
void ezspRf4ceDiscoveryResponseHandler(boolean atCapacity,
                                       int8u channel,
                                       EmberPanId panId,
                                       EmberEUI64 ieeeAddr,
                                       int8u nodeCapabilities,
                                       EmberRf4ceVendorInfo *vendorInfo,
                                       EmberRf4ceApplicationInfo *appInfo,
                                       int8u rxLinkQuality,
                                       int8u discRequestLqi) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_AUTO_DISCOVERY_RESPONSE_COMPLETE_HANDLER
void ezspRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   int8u nodeCapabilities,
                                                   EmberRf4ceVendorInfo *vendorInfo,
                                                   EmberRf4ceApplicationInfo *appInfo,
                                                   int8u searchDevType) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_DISCOVERY_COMPLETE_HANDLER
void ezspRf4ceDiscoveryCompleteHandler(EmberStatus status) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_INCOMING_MESSAGE_HANDLER
void ezspRf4ceIncomingMessageHandler(int8u pairingIndex,
                                     int8u profileId,
                                     int16u vendorId,
                                     int8u messageLength,
                                     int8u *message) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_MESSAGE_SENT_HANDLER
void ezspRf4ceMessageSentHandler(EmberStatus status,
                                 int8u pairingIndex,
                                 int8u txOptions,
                                 int8u profileId,
                                 int16u vendorId,
                                 int8u messageLength,
                                 int8u *message) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_PAIR_REQUEST_HANDLER
void ezspRf4cePairRequestHandler(EmberStatus status,
                                 int8u pairingIndex,
                                 EmberEUI64 srcIeeeAddr,
                                 int8u nodeCapabilities,
                                 EmberRf4ceVendorInfo *vendorInfo,
                                 EmberRf4ceApplicationInfo *appInfo,
                                 int8u keyExchangeTransferCount) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_PAIR_COMPLETE_HANDLER
void ezspRf4cePairCompleteHandler(EmberStatus status,
                                  int8u pairingIndex) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_UNPAIR_HANDLER
void ezspRf4ceUnpairHandler(int8u pairingIndex) {}
#endif

#ifndef EZSP_APPLICATION_HAS_RF4CE_UNPAIR_COMPLETE_HANDLER
void ezspRf4ceUnpairCompleteHandler(int8u pairingIndex) {}
#endif

#ifndef EZSP_APPLICATION_HAS_COUNTER_ROLLOVER_HANDLER
void ezspCounterRolloverHandler(EmberCounterType type) {}
#endif
