// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"
#include "rf4ce-gdp-poll.h"

//------------------------------------------------------------------------------
// Global variables.

// TODO: For now we assume that the node is already up and running, thus we skip
// the 'dormant' status.
int16u emAfGdpState = EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND;

int8u emAfTemporaryPairingIndex = 0xFF;

int8u emAfCurrentProfileSpecificIndex = 0xFF;

EmberEventControl emberAfPluginRf4ceGdpPendingCommandEventControl;
EmberEventControl emberAfPluginRf4ceGdpBlackoutTimeEventControl;
EmberEventControl emberAfPluginRf4ceGdpValidationEventControl;

EmAfBindingInfo emAfGdpPeerInfo;

const int8u emAfRf4ceGdpApplicationSpecificUserString[USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH]
            = EMBER_AF_PLUGIN_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING;

//------------------------------------------------------------------------------
// Local variables.

static const int8u gdp1xProfiles[] = GDP_1_X_BASED_PROFILE_ID_LIST;
static const int8u gdp20Profiles[] = GDP_2_0_BASED_PROFILE_ID_LIST;
static int8u currentProfileSpecificIndex = 0xFF;

//------------------------------------------------------------------------------
// External declarations.

void emAfRf4ceGdpPairCompleteOriginatorCallback(EmberStatus status,
                                                int8u pairingIndex,
                                                const EmberRf4ceVendorInfo *vendorInfo,
                                                const EmberRf4ceApplicationInfo *appInfo);
void emAfRf4ceGdpBasedProfileConfigurationCompleteOriginatorCallback(boolean success);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
void emAfRf4ceGdpPairCompleteRecipientCallback(EmberStatus status,
                                               int8u pairingIndex,
                                               const EmberRf4ceVendorInfo *vendorInfo,
                                               const EmberRf4ceApplicationInfo *appInfo);
void emAfRf4ceGdpBasedProfileConfigurationCompleteRecipientCallback(boolean success);
#endif

// GDP messages dispatchers.
void emAfRf4ceGdpIncomingGenericResponseOriginatorCallback(EmberAfRf4ceGdpResponseCode status);
void emAfRf4ceGdpIncomingGenericResponsePollNegotiationCallback(EmberAfRf4ceGdpResponseCode responseCode);
void emAfRf4ceGdpIncomingGenericResponseIdentificationCallback(EmberAfRf4ceGdpResponseCode responseCode);
void emAfRf4ceGdpIncomingCheckValidationResponseOriginator(EmberAfRf4ceGdpCheckValidationStatus status);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
void emAfRf4ceGdpIncomingConfigurationCompleteRecipientCallback(EmberAfRf4ceGdpStatus status);
void emAfRf4ceGdpIncomingCheckValidationRequestRecipientCallback(int8u control);
void emAfRf4ceGdpCheckValidationResponseSentRecipient(EmberStatus status);
#endif

// Event handler dispatchers.
void emAfPendingCommandEventHandlerOriginator(void);
void emAfPendingCommandEventHandlerRecipient(void);
void emAfPendingCommandEventHandlerSecurity(void);
void emAfPendingCommandEventHandlerPollNegotiation(void);
void emAfPendingCommandEventHandlerIdentification(void);
void emAfPluginRf4ceGdpBlackoutTimeEventHandlerOriginator(void);
void emAfPluginRf4ceGdpBlackoutTimeEventHandlerRecipient(void);
void emAfPluginRf4ceGdpValidationEventHandlerOriginator(void);
void emAfPluginRf4ceGdpValidationEventHandlerRecipient(void);

//------------------------------------------------------------------------------
// Event handlers.

void emberAfPluginRf4ceGdpPendingCommandEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);

  if (isInternalStateBindingOriginator()) {
    emAfPendingCommandEventHandlerOriginator();
  } else if (isInternalStateBindingRecipient()) {
    emAfPendingCommandEventHandlerRecipient();
  } else if (isInternalStateSecurity()) {
    emAfPendingCommandEventHandlerSecurity();
  } else if (isInternalStatePollNegotiation()) {
    emAfPendingCommandEventHandlerPollNegotiation();
  } else if (isInternalStateIdentification()) {
    emAfPendingCommandEventHandlerIdentification();
  } else {
    assert(0);
  }
}

void emberAfPluginRf4ceGdpBlackoutTimeEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);

  if (isInternalStateBindingOriginator()) {
    emAfPluginRf4ceGdpBlackoutTimeEventHandlerOriginator();
  } else if (isInternalStateBindingRecipient()) {
    emAfPluginRf4ceGdpBlackoutTimeEventHandlerRecipient();
  } else {
    assert(0);
  }
}

void emberAfPluginRf4ceGdpValidationEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpValidationEventControl);

  if (isInternalStateBindingOriginator()) {
    emAfPluginRf4ceGdpValidationEventHandlerOriginator();
  } else if (isInternalStateBindingRecipient()) {
    emAfPluginRf4ceGdpValidationEventHandlerRecipient();
  } else {
    assert(0);
  }
}

//------------------------------------------------------------------------------
// Common callbacks.

// Init callback
void emberAfPluginRf4ceGdpInitCallback(void)
{
  int8u i;

  emAfRf4ceGdpAttributesInitCallback();

  // If we detect an entry in 'bound' status with the 'binding complete' not
  // set, we delete the corresponding pairing entry and set the status back to
  // 'not bound'. This would happen if a node is re-binding with a node it
  // already bound previously and it reboots after a temporary pairing is
  // established, before the validation process completes.
  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    int8u bindStatus = emAfRf4ceGdpGetPairingBindStatus(i);

    if (((bindStatus & PAIRING_ENTRY_BINDING_STATUS_MASK)
         << PAIRING_ENTRY_BINDING_STATUS_OFFSET)
        != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND
        && !(bindStatus & PAIRING_ENTRY_BINDING_COMPLETE_BIT)) {
      emberAfRf4ceSetPairingTableEntry(i, NULL);
      emAfRf4ceGdpSetPairingBindStatus(i, (PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND
                                           << PAIRING_ENTRY_BINDING_STATUS_OFFSET));
    }
  }
}

void emberAfPluginRf4ceGdpStackStatusCallback(EmberStatus status)
{
  // Add originator stack status function call here if needed

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpRecipientStackStatusCallback(status);
#endif

  emAfRf4ceGdpPollingStackStatusCallback(status);
}

void emberAfPluginRf4ceProfileGdpAutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                       const EmberEUI64 srcIeeeAddr,
                                                                       int8u nodeCapabilities,
                                                                       const EmberRf4ceVendorInfo *vendorInfo,
                                                                       const EmberRf4ceApplicationInfo *appInfo,
                                                                       int8u searchDevType)
{
}

void emberAfPluginRf4ceProfileGdpPairCompleteCallback(EmberStatus status,
                                                      int8u pairingIndex,
                                                      const EmberRf4ceVendorInfo *vendorInfo,
                                                      const EmberRf4ceApplicationInfo *appInfo)
{
  // The originator passes the function pointer for the pairComplete() callback
  // directly in the pair() API.

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpPairCompleteRecipientCallback(status,
                                            pairingIndex,
                                            vendorInfo,
                                            appInfo);
#endif
}

#if defined(EMBER_SCRIPTED_TEST)
boolean isZrcTest = FALSE;
#endif

void emberAfRf4ceGdpConfigurationProcedureComplete(boolean success)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (isZrcTest) {
    if (success) {
      debugScriptCheck("Configuration succeeded");
    } else {
      debugScriptCheck("Configuration failed");
    }
    return;
  }
#endif // EMBER_SCRIPTED_TEST

  if (isInternalStateBindingOriginator()) {
    emAfRf4ceGdpBasedProfileConfigurationCompleteOriginatorCallback(success);
  }
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  else if (isInternalStateBindingRecipient()) {
    emAfRf4ceGdpBasedProfileConfigurationCompleteRecipientCallback(success);
  }
#endif
  else {
    assert(0);
  }
}

//------------------------------------------------------------------------------
// Common internal APIs.

void emAfRf4ceGdpUpdatePublicStatus(boolean init)
{
  int8u i;
  boolean bound = FALSE;

  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    int8u bindStatus =
        (emAfRf4ceGdpGetPairingBindStatus(i) & PAIRING_ENTRY_BINDING_STATUS_MASK);

    if (bindStatus == PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT
        || bindStatus == PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR) {
      bound = TRUE;
      break;
    }
  }

  // Set the public state to "bound" if there is at least a pairing entry whose
  // status is "bound as recipient" or "bound as originator", otherwise set it
  // to "not bound".
  setPublicState(((bound)
                  ? EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND
                  : EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND),
                 init);
}

boolean emAfIsProfileGdpBased(int8u profileId, int8u gdpCheckVersion)
{
  int8u gdpVersion = GDP_VERSION_NONE;
  int8u i;

  for(i=0; i<GDP_1_X_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    if (gdp1xProfiles[i] == profileId) {
      gdpVersion = GDP_VERSION_1_X;
    }
  }

  for(i=0; i<GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    if (gdp20Profiles[i] == profileId) {
      gdpVersion = GDP_VERSION_2_0;
    }
  }

  return (gdpVersion == gdpCheckVersion);
}

// Check that the passed "checkDeviceType" matches one of the device types in
// the passed "compareDeviceTypeList" and that at least one of the profile IDs
// stored in the passed "checkProfileIdList" matches one of the profile IDs
// stored in the passed "compareProfileIdList".
// Returns the number of profile IDs that matched and those IDs are stored in
// the passed "matchingProfileIdList".
int8u emAfCheckDeviceTypeAndProfileIdMatch(int8u checkDeviceType,
                                           int8u *compareDevTypeList,
                                           int8u compareDevTypeListLength,
                                           int8u *checkProfileIdList,
                                           int8u checkProfileIdListLength,
                                           int8u *compareProfileIdList,
                                           int8u compareProfileIdListLength,
                                           int8u *matchingProfileIdList)
{
  int8u i, j;
  boolean matchFound = FALSE;
  int8u matchingProfileIdListLength = 0;

  // Check that "at least one device type contained in the device type list
  // field matches the search device type supplied by the application".
  for(i=0; i<compareDevTypeListLength; i++) {
    if (checkDeviceType == 0xFF  // wildcard
        || checkDeviceType == compareDevTypeList[i]) {
      matchFound = TRUE;
      break;
    }
  }

  if (!matchFound) {
    return 0;
  }

  // Initialize the list of the matching profile IDs to all 0xFFs.
  MEMSET(matchingProfileIdList, 0xFF, EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH);

  // Check that "at least one profile identifier contained in the profile
  // identifier list matches at least one profile identifier from the discovery
  // profile identifier list supplied by the application.
  for(i=0; i<checkProfileIdListLength; i++) {
    for(j=0; j<compareProfileIdListLength; j++) {
      if (checkProfileIdList[i] == compareProfileIdList[j]) {
        matchingProfileIdList[matchingProfileIdListLength++] =
            checkProfileIdList[i];
      }
    }
  }

  return matchingProfileIdListLength;
}

boolean emAfRf4ceIsProfileSupported(int8u profileId,
                                    const int8u *profileIdList,
                                    int8u profileIdListLength)
{
  int8u i;

  for(i=0; i<profileIdListLength; i++) {
    if (profileId == profileIdList[i]) {
      return TRUE;
    }
  }

  return FALSE;
}

boolean emAfRf4ceGdpMaybeStartNextProfileSpecificConfigurationProcedure(boolean isOriginator,
                                                                        const int8u *remoteProfileIdList,
                                                                        int8u remoteProfileIdListLength)
{
  int8u i = ((currentProfileSpecificIndex == 0xFF)
             ? 0
             : currentProfileSpecificIndex + 1);

  for(; i<GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    // If we find a GDP 2.0 profile ID that is supported by both nodes, start
    // the corresponding configuration procedure.
    if (emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                    emAfRf4ceApplicationInfo.profileIdList,
                                    emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities))
        && emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                       remoteProfileIdList,
                                       remoteProfileIdListLength)) {
      boolean configurationProcedureStarted = FALSE;
      switch(gdp20Profiles[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        configurationProcedureStarted =
            emberAfPluginRf4ceGdpZrc20StartConfigurationCallback(isOriginator,
                                                                 emAfTemporaryPairingIndex);
        break;
      default:
        assert(0);
      }

      if (configurationProcedureStarted) {
        currentProfileSpecificIndex = i;
        return TRUE;
      }
    }
  }

  currentProfileSpecificIndex = 0xFF;
  return FALSE;
}

void emAfRf4ceGdpNotifyBindingCompleteToProfiles(EmberAfRf4ceGdpBindingStatus status,
                                                 int8u pairingIndex,
                                                 const int8u *remoteProfileIdList,
                                                 int8u remoteProfileIdListLength)
{
  int8u i;
  for(i=0; i<GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    // If we find a GDP 2.0 profile ID that is supported by both nodes, we
    // notify the profile.
    if (emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                    emAfRf4ceApplicationInfo.profileIdList,
                                    emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities))
        && emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                       remoteProfileIdList,
                                       remoteProfileIdListLength)) {
      switch(gdp20Profiles[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        emberAfPluginRf4ceGdpZrc20BindingCompleteCallback(status, pairingIndex);
        break;
      default:
        assert(0);
      }
    }
  }
}

void emAfRf4ceGdpNoteProfileSpecificConfigurationStart(void)
{
  currentProfileSpecificIndex = 0xFF;
}

// It adds to the profile ID list of destAppInfo the profile IDs in srcAppInfo
// that are based on the passed GDP version (it never includes the GDP profile
// 0x00).
void emAfGdpAddToProfileIdList(int8u *srcProfileIdList,
                               int8u srcProfileIdListLength,
                               EmberRf4ceApplicationInfo *destAppInfo,
                               int8u gdpVersion)
{
  int8u i;
  int8u profileIdCount =
      emberAfRf4ceProfileIdListLength(destAppInfo->capabilities);

  for(i=0; i<srcProfileIdListLength; i++) {
    // Never include the GDP 2.0 profile ID.
    if (srcProfileIdList[i] == EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE) {
      continue;
    }

    if (emAfIsProfileGdpBased(srcProfileIdList[i], gdpVersion)) {
      destAppInfo->profileIdList[profileIdCount++] = srcProfileIdList[i];
    }
  }

  // Set the number of supported profiles in the appInfo capabilities byte.
  destAppInfo->capabilities &=
      ~EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK;
  destAppInfo->capabilities |=
      (profileIdCount << EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_OFFSET);
}

// - If the profile ID list contains at least a GDP 2.0 profile ID, it returns
//   GDP 2.0.
// - If the profile ID list contains at least a GDP 1.x profile ID, it returns
//   GDP 1.x.
// - Otherwise returns nonGDP.
int8u emAfRf4ceGdpGetGdpVersion(const int8u *profileIdList,
                                int8u profileIdListLength)
{
  int8u i;

  // Check if there is at least one profile ID that is GDP 2.0 based first.
 for(i=0; i<profileIdListLength; i++) {
   // At least one of the matching profile IDs is GDP 2.0 based
   if (emAfIsProfileGdpBased(profileIdList[i], GDP_VERSION_2_0)) {
     return GDP_VERSION_2_0;
   }
 }

 // No GDP 2.0 based matching profile ID. Check whether at least one of the
 // matching profile IDs is GDP 1.x based.
 for(i=0; i<profileIdListLength; i++) {
   // At least one of the matching profile IDs is GDP 1.x based
   if (emAfIsProfileGdpBased(profileIdList[i], GDP_VERSION_1_X)) {
     return GDP_VERSION_1_X;
   }
 }

 return GDP_VERSION_NONE;
}

void emAfGdpStartBlackoutTimer(int8u state)
{
  setInternalState(state);
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpBlackoutTimeEventControl,
                              isInternalStateBindingOriginator()
                              ? BLACKOUT_TIME_ORIGINATOR_MS
                              : BLACKOUT_TIME_RECIPIENT_MS);
}

void emAfGdpStartCommandPendingTimer(int8u state, int16u timeMs)
{
  setInternalState(state);
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPendingCommandEventControl,
                              timeMs);
}

//------------------------------------------------------------------------------
// Commands dispatchers (originator/recipient).

void emAfRf4ceGdpIncomingGenericResponse(EmberAfRf4ceGdpResponseCode responseCode)
{
  if (isInternalStatePollNegotiation()) {
    emAfRf4ceGdpIncomingGenericResponsePollNegotiationCallback(responseCode);
  } else if (isInternalStateIdentification()) {
    emAfRf4ceGdpIncomingGenericResponseIdentificationCallback(responseCode);
  } else {
    emAfRf4ceGdpIncomingGenericResponseOriginatorCallback(responseCode);
  }
}

void emAfRf4ceGdpIncomingConfigurationComplete(EmberAfRf4ceGdpStatus status)
{
  // Incoming configuration complete messages are always dispatched to the
  // recipient code.
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpIncomingConfigurationCompleteRecipientCallback(status);
#endif
}

void emAfRf4ceGdpIncomingCheckValidationRequest(int8u control)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpIncomingCheckValidationRequestRecipientCallback(control);
#endif
}

void emAfRf4ceGdpIncomingCheckValidationResponse(EmberAfRf4ceGdpCheckValidationStatus status)
{
  emAfRf4ceGdpIncomingCheckValidationResponseOriginator(status);
}

void emAfRf4ceGdpCheckValidationResponseSent(EmberStatus status)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpCheckValidationResponseSentRecipient(status);
#endif
}

#if defined(EMBER_SCRIPTED_TEST)
void setBindOriginatorState(int8u state)
{
  emAfGdpState = state;
  if (state == EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND) {
    emAfTemporaryPairingIndex = 0xFF;
  }
}
#endif
