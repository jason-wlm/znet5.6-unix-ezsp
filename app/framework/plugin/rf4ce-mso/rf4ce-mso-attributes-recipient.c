// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#include "rf4ce-mso-attributes.h"

#if defined(EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)

static EmberAfRf4ceStatus getSetAttributeRequestStatus(int8u pairingIndex,
                                                       EmberAfRf4ceMsoAttributeId attributeId,
                                                       int8u index,
                                                       int8u valueLen,
                                                       boolean isGet)
{
  if (!emAfRf4ceMsoAttributeIsSupported(attributeId)) {
    return EMBER_AF_RF4CE_STATUS_UNSUPPORTED_ATTRIBUTE;
  }

  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE
      || (isGet && valueLen < emAfRf4ceMsoGetAttributeLength(attributeId))
      || (!isGet && emAfRf4ceMsoGetAttributeLength(attributeId) < valueLen)
      || (emAfRf4ceMsoAttributeIsArrayed(attributeId)
          && !emAfRf4ceMsoArrayedAttributeIndexIsValid(pairingIndex,
                                                       attributeId,
                                                       index,
                                                       isGet))) {
    return EMBER_AF_RF4CE_STATUS_INVALID_INDEX;
  }

  return EMBER_AF_RF4CE_STATUS_SUCCESS;
}

EmberAfRf4ceStatus emAfRf4ceMsoSetAttributeRequestCallback(int8u pairingIndex,
                                                           EmberAfRf4ceMsoAttributeId attributeId,
                                                           int8u index,
                                                           int8u valueLen,
                                                           const int8u *value)
{
  EmberAfRf4ceStatus status = getSetAttributeRequestStatus(pairingIndex,
                                                           attributeId,
                                                           index,
                                                           valueLen,
                                                           FALSE); // isGet
#if defined(EMBER_TEST)
  emberAfCorePrint("Set Attribute Request %d, 0x%x, %d, %d, {",
                   pairingIndex,
                   attributeId,
                   index,
                   valueLen);
  emberAfCorePrintBuffer(value, valueLen, TRUE);
  emberAfCorePrintln("}");
#endif // EMBER_TEST

  if (status == EMBER_AF_RF4CE_STATUS_SUCCESS) {
    if (!emAfRf4ceMsoAttributeIsRemotelyWritable(attributeId)) {
      return EMBER_AF_RF4CE_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    emAfRf4ceMsoWriteAttributeRecipient(pairingIndex,
                                        attributeId,
                                        index,
                                        valueLen,
                                        value);
  }

  return status;
}

EmberAfRf4ceStatus emAfRf4ceMsoGetAttributeRequestCallback(int8u pairingIndex,
                                                           EmberAfRf4ceMsoAttributeId attributeId,
                                                           int8u index,
                                                           int8u *valueLen,
                                                           int8u *value)
{
  EmberAfRf4ceStatus status = getSetAttributeRequestStatus(pairingIndex,
                                                           attributeId,
                                                           index,
                                                           *valueLen,
                                                           TRUE); // isGet
#if defined(EMBER_TEST)
  emberAfCorePrintln("Get Attribute Request %d, 0x%x, %d, %d",
                     pairingIndex,
                     attributeId,
                     index,
                     *valueLen);
#endif // EMBER_TEST

  if (status == EMBER_AF_RF4CE_STATUS_SUCCESS) {
    emAfRf4ceMsoReadAttributeRecipient(pairingIndex,
                                       attributeId,
                                       index,
                                       valueLen,
                                       value);
  }

  return status;
}

boolean emAfRf4ceMsoArrayedAttributeIndexIsValid(int8u pairingIndex,
                                                 EmberAfRf4ceMsoAttributeId attributeId,
                                                 int8u index,
                                                 boolean isGet)
{
  if (attributeId == EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS) {
    int8u entryIndex = emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex,
                                                                    index);
    if (isGet) {
      return (entryIndex != 0xFF);
    } else {
      return (entryIndex != 0xFF
              || emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(pairingIndex) != 0xFF);
    }
  } else if (attributeId == EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE) {
    return emberAfPluginRf4ceMsoHaveIrRfDatabaseAttributeCallback(pairingIndex, index);
  } else {
    return (index < emAfRf4ceMsoGetArrayedAttributeDimension(attributeId));
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
