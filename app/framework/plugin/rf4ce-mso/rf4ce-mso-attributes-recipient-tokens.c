// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#include "rf4ce-mso-attributes.h"

#if defined(EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT)

void emAfRf4ceMsoInitAttributes(void)
{
  // Nothing to do here
}

int8u emAfRf4ceMsoAttributePeripheralIdEntryLookup(int8u pairingIndex, int8u deviceType)
{
  int8u i;
  tokMsoAttributePeripheralIds tok;

  halCommonGetIndexedToken(&tok,
                           TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_PERIPHERAL_IDS,
                           pairingIndex);

  for(i=0; i<EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES; i++) {
    if (tok[i].deviceType == deviceType) {
      return i;
    }
  }

  return 0xFF;
}

int8u emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(int8u pairingIndex)
{
  return emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex, 0xFF);
}

// These assume sanity check for the passed parameters has been performed.

void emAfRf4ceMsoWriteAttributeRecipient(int8u pairingIndex,
                                         EmberAfRf4ceMsoAttributeId attributeId,
                                         int8u index,
                                         int8u valueLen,
                                         const int8u *value)
{
  switch(attributeId) {
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS:
  {
    tokMsoAttributePeripheralIds tok;

    int8u entryIndex = emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex,
                                                                    index);
    if (entryIndex == 0xFF) {
      entryIndex = emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(pairingIndex);
    }

    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_PERIPHERAL_IDS,
                             pairingIndex);

    tok[entryIndex].deviceType = index;
    MEMCOPY(tok[entryIndex].peripheralId,
            value,
            valueLen);

    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_PERIPHERAL_IDS,
                             pairingIndex,
                             &tok);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS:
  {
    tokMsoAttributeRfStatistics tok;

    MEMCOPY(&tok, value, valueLen);
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_RF_STATISTICS,
                             pairingIndex,
                             &tok);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING:
  {
    tokMsoAttributeVersioning tok;

    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_VERSIONING,
                             pairingIndex);
    MEMCOPY(tok[index], value, valueLen);
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_VERSIONING,
                             pairingIndex,
                             &tok);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS:
  {
    tokMsoAttributeBatteryStatus tok;

    MEMCOPY(&tok, value, valueLen);
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_BATTERY_STATUS,
                             pairingIndex,
                             &tok);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD:
  {
    tokMsoAttributeShortRfRetryPeriod tok;

    MEMCOPY(&tok, value, valueLen);
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_SHOR_RF_RETRY_PERIOD,
                             pairingIndex,
                             &tok);
  }
    break;
  //case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE:
  // IR_RF database attributes can't be written remotely.
  //  break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION:
  {
    tokMsoAttributeValidationConfiguration tok;

    MEMCOPY(&tok, value, valueLen);
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_VALIDATION_CONFIGURATION,
                             pairingIndex,
                             &tok);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE:
  {
    tokMsoAttributeGeneralPurpose tok;

    MEMCOPY(&tok, value, valueLen);
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_GENERAL_PURPOSE,
                             pairingIndex,
                             &tok);
  }
    break;
  default:
    assert(0);
  }
}

void emAfRf4ceMsoReadAttributeRecipient(int8u pairingIndex,
                                        EmberAfRf4ceMsoAttributeId attributeId,
                                        int8u index,
                                        int8u *valueLen,
                                        int8u *value)
{
  // We know the length of all the attributes except the IR-RF Database, which
  // is managed externally.
  if (attributeId != EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE) {
    *valueLen = emAfRf4ceMsoGetAttributeLength(attributeId);
  }
  switch(attributeId) {
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS:
  {
    tokMsoAttributePeripheralIds tok;
    int8u entryIndex = emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex,
                                                                    index);
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_PERIPHERAL_IDS,
                             pairingIndex);
    MEMCOPY(value, tok[entryIndex].peripheralId, *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS:
  {
    tokMsoAttributeRfStatistics tok;
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_RF_STATISTICS,
                             pairingIndex);
    MEMCOPY(value, tok, *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING:
  {
    tokMsoAttributeVersioning tok;
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_VERSIONING,
                             pairingIndex);
    MEMCOPY(value, tok[index], *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS:
  {
    tokMsoAttributeBatteryStatus tok;
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_BATTERY_STATUS,
                             pairingIndex);
    MEMCOPY(value, tok, *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD:
  {
    tokMsoAttributeShortRfRetryPeriod tok;
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_SHOR_RF_RETRY_PERIOD,
                             pairingIndex);
    MEMCOPY(value, tok, *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE:
    emberAfPluginRf4ceMsoGetIrRfDatabaseAttributeCallback(pairingIndex,
                                                          index,
                                                          valueLen,
                                                          value);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION:
  {
    tokMsoAttributeValidationConfiguration tok;
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_VALIDATION_CONFIGURATION,
                             pairingIndex);
    MEMCOPY(value, tok, *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE:
  {
    tokMsoAttributeGeneralPurpose tok;
    halCommonGetIndexedToken(&tok,
                             TOKEN_PLUGIN_RF4CE_MSO_ATTRIBUTE_GENERAL_PURPOSE,
                             pairingIndex);
    MEMCOPY(value, tok, *valueLen);
  }
    break;
  default:
    assert(0);
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
