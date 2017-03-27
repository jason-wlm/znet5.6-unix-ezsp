// Copyright 2013 Silicon Laboratories, Inc.

#define NWKC_MIN_CONTROLLER_PAIRING_TABLE_SIZE 1
#define NWKC_MIN_TARGET_PAIRING_TABLE_SIZE     5

boolean emAfRf4ceDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                         int8u nodeCapabilities,
                                         const EmberRf4ceVendorInfo *vendorInfo,
                                         const EmberRf4ceApplicationInfo *appInfo,
                                         int8u searchDevType,
                                         int8u rxLinkQuality);

boolean emAfRf4ceDiscoveryResponseHandler(boolean atCapacity,
                                          int8u channel,
                                          EmberPanId panId,
                                          EmberEUI64 srcIeeeAddr,
                                          int8u nodeCapabilities,
                                          const EmberRf4ceVendorInfo *vendorInfo,
                                          const EmberRf4ceApplicationInfo *appInfo,
                                          int8u rxLinkQuality,
                                          int8u discRequestLqi);

void emAfRf4ceDiscoveryCompleteHandler(EmberStatus status);

void emAfRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   int8u nodeCapabilities,
                                                   const EmberRf4ceVendorInfo *vendorInfo,
                                                   const EmberRf4ceApplicationInfo *appInfo,
                                                   int8u searchDevType);

boolean emAfRf4cePairRequestHandler(EmberStatus status,
                                    int8u pairingIndex,
                                    EmberEUI64 srcIeeeAddr,
                                    int8u nodeCapabilities,
                                    const EmberRf4ceVendorInfo *vendorInfo,
                                    const EmberRf4ceApplicationInfo *appInfo,
                                    int8u keyExchangeTransferCount);

void emAfRf4cePairCompleteHandler(EmberStatus status,
                                  int8u pairingIndex,
                                  const EmberRf4ceVendorInfo *vendorInfo,
                                  const EmberRf4ceApplicationInfo *appInfo);

void emAfRf4ceMessageSentHandler(EmberStatus status,
                                 int8u pairingIndex,
                                 int8u txOptions,
                                 int8u profileId,
                                 int16u vendorId,
                                 int8u messageTag,
                                 int8u messageLength,
                                 const int8u *message);

void emAfRf4ceIncomingMessageHandler(int8u pairingIndex,
                                     int8u profileId,
                                     int16u vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     int8u messageLength,
                                     const int8u *message);

void emAfRf4ceUnpairHandler(int8u pairingIndex);

void emAfRf4ceUnpairCompleteHandler(int8u pairingIndex);

int8u emAfRf4ceGetBaseChannel(void);

#ifdef EZSP_HOST
  #define emAfRf4ceStart                         ezspRf4ceStart
  #define emAfRf4ceSetPowerSavingParameters      ezspRf4ceSetPowerSavingParameters
  #define emAfRf4ceSetFrequencyAgilityParameters ezspRf4ceSetFrequencyAgilityParameters
  #define emAfRf4ceSetDiscoveryLqiThreshold(threshold)                      \
    ezspSetValue(EZSP_VALUE_RF4CE_DISCOVERY_LQI_THRESHOLD, 1, &(threshold))
  // emAfRf4ceGetBaseChannel is a function defined in rf4ce-profile-host.c
  #define emAfRf4ceDiscovery                     ezspRf4ceDiscovery
  #define emAfRf4ceEnableAutoDiscoveryResponse   ezspRf4ceEnableAutoDiscoveryResponse
  #define emAfRf4cePair                          ezspRf4cePair
  #define emAfRf4ceSetPairingTableEntry(pairingIndex, entry)                   \
  (entry == NULL) ? ezspRf4ceDeletePairingTableEntry(pairingIndex)             \
                  : ezspRf4ceSetPairingTableEntry(pairingIndex, entry)
  #define emAfRf4ceGetPairingTableEntry          ezspRf4ceGetPairingTableEntry
  #define emAfRf4ceSetApplicationInfo            ezspRf4ceSetApplicationInfo
  #define emAfRf4ceGetApplicationInfo            ezspRf4ceGetApplicationInfo
  #define emAfRf4ceKeyUpdate                     ezspRf4ceKeyUpdate
  #define emAfRf4ceSend                          ezspRf4ceSend
  #define emAfRf4ceUnpair                        ezspRf4ceUnpair
  #define emAfRf4ceStop                          ezspRf4ceStop
  #define emAfRf4ceGetMaxPayload                 ezspRf4ceGetMaxPayload
#else
  #define emAfRf4ceStart                         emberRf4ceStart
  #define emAfRf4ceSetPowerSavingParameters      emberRf4ceSetPowerSavingParameters
  #define emAfRf4ceSetFrequencyAgilityParameters emberRf4ceSetFrequencyAgilityParameters
  #define emAfRf4ceSetDiscoveryLqiThreshold      emberRf4ceSetDiscoveryLqiThreshold
  #define emAfRf4ceGetBaseChannel                emberRf4ceGetBaseChannel
  #define emAfRf4ceDiscovery                     emberRf4ceDiscovery
  #define emAfRf4ceEnableAutoDiscoveryResponse   emberRf4ceEnableAutoDiscoveryResponse
  #define emAfRf4cePair                          emberRf4cePair
  #define emAfRf4ceSetPairingTableEntry          emberRf4ceSetPairingTableEntry
  #define emAfRf4ceGetPairingTableEntry          emberRf4ceGetPairingTableEntry
  #define emAfRf4ceSetApplicationInfo            emberRf4ceSetApplicationInfo
  #define emAfRf4ceGetApplicationInfo            emberRf4ceGetApplicationInfo
  #define emAfRf4ceKeyUpdate                     emberRf4ceKeyUpdate
  #define emAfRf4ceSend                          emberRf4ceSend
  #define emAfRf4ceUnpair                        emberRf4ceUnpair
  #define emAfRf4ceStop                          emberRf4ceStop
  #define emAfRf4ceGetMaxPayload                 emberRf4ceGetMaxPayload
#endif

typedef struct {
  int32u dutyCycleMs;
  int32u activePeriodMs;
} EmAfRf4cePowerSavingState;

extern PGM EmberAfRf4ceProfileId emAfRf4ceProfileIds[];
extern boolean emAfRf4ceRxOnWhenIdleProfileStates[];
extern EmAfRf4cePowerSavingState emAfRf4cePowerSavingState;
