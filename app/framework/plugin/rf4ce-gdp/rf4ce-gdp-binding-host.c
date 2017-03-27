// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-gdp-internal.h"

void emberAfPluginRf4ceGdpNcpInitCallback(boolean memoryAllocation)
{
  if (!memoryAllocation) {
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT)
    // Push the recipient binding parameters to the NCP.
    int8u bindingParams[GDP_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH];

    bindingParams[0] = ((EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_NUMBER
                         << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                        | (EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_DUPLICATE_HANDLING
                           << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    bindingParams[1] = ((EMBER_AF_PLUGIN_RF4CE_GDP_SECONDARY_CLASS_NUMBER
                         << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                        | (EMBER_AF_PLUGIN_RF4CE_GDP_SECONDARY_CLASS_DUPLICATE_HANDLING
                           << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    bindingParams[2] = ((EMBER_AF_PLUGIN_RF4CE_GDP_TERTIARY_CLASS_NUMBER
                         << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                        | (EMBER_AF_PLUGIN_RF4CE_GDP_TERTIARY_CLASS_DUPLICATE_HANDLING
                           << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    bindingParams[3] = EMBER_AF_PLUGIN_RF4CE_GDP_DISCOVERY_RESPONSE_LQI_THRESHOLD;

    ezspSetValue(EZSP_VALUE_RF4CE_GDP_BINDING_RECIPIENT_PARAMETERS,
                 GDP_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH,
                 bindingParams);
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT

    // Push the application specific user string to the NCP.
    ezspSetValue(EZSP_VALUE_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING,
                 USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH,
                 (int8u *)emAfRf4ceGdpApplicationSpecificUserString);
  }
}

void emAfRf4ceGdpSetPushButtonPendingReceivedFlag(boolean set)
{
  ezspSetValue(EZSP_VALUE_RF4CE_GDP_PUSH_BUTTON_STIMULUS_RECEIVED_PENDING_FLAG,
               GDP_SET_VALUE_FLAG_LENGTH,
               &set);
}

void emAfRf4ceGdpSetProxyBindingFlag(boolean set)
{
  ezspSetValue(EZSP_VALUE_RF4CE_GDP_BINDING_PROXY_FLAG,
               GDP_SET_VALUE_FLAG_LENGTH,
               &set);
}

EmberStatus emAfRf4ceGdpSetDiscoveryResponseAppInfo(boolean pushButton,
                                                    int8u gdpVersion)
{
  // Nothing to do at the HOST, the application info is set at the NCP.

  return EMBER_SUCCESS;
}

EmberStatus emAfRf4ceGdpSetPairResponseAppInfo(const EmberRf4ceApplicationInfo *pairRequestAppInfo)
{
  // Nothing to do at the HOST, the application info is set at the NCP.

  return EMBER_SUCCESS;
}
