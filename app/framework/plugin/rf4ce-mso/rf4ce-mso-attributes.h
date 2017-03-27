// Copyright 2014 Silicon Laboratories, Inc.

#ifndef _RF4CE_MSO_ATTRIBUTES_H_
#define _RF4CE_MSO_ATTRIBUTES_H_

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"

#define MSO_RIB_ATTRIBUTE_PERIPHERAL_IDS_LENGTH                   4
#define MSO_RIB_ATTRIBUTE_RF_STATISTICS_LENGTH                    16
#define MSO_RIB_ATTRIBUTE_VERSIONING_LENGTH                       2
#define MSO_RIB_ATTRIBUTE_BATTERY_STATUS_LENGTH                   11
#define MSO_RIB_ATTRIBUTE_SHORT_RF_RETRY_PERIOD_LENGTH            4
// IR-RF database length has variable length
#define MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH         4
#define MSO_RIB_ATTRIBUTE_GENERAL_PURPOSE_LENGTH                  16

// Bitmask field definitions.
#define MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT                 0x01
#define MSO_ATTRIBUTE_IS_ARRAYED_BIT                              0x02

#define MSO_ATTRIBUTES_COUNT                                      8

// Versioning attribute
#define MSO_ATTRIBUTE_VERSIONING_ENTRIES                          3
#define MSO_ATTRIBUTE_VERSIONING_INDEX_SOFTWARE                   0x00
#define MSO_ATTRIBUTE_VERSIONING_INDEX_HARDWARE                   0x01
#define MSO_ATTRIBUTE_VERSIONING_INDEX_IR_DATABASE                0x02

typedef struct {
  EmberAfRf4ceMsoAttributeId id;
  int8u size;
  int8u bitmask;
  int8u dimension; // only for arrayed attributes
} EmAfRf4ceMsoAttributeDescriptor;

typedef struct {
  int8u deviceType;
  int8u peripheralId[MSO_RIB_ATTRIBUTE_PERIPHERAL_IDS_LENGTH];
} EmAfRf4ceMsoPeripheralIdEntry;

typedef struct {
  EmAfRf4ceMsoPeripheralIdEntry peripheralIds[EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES];
  int8u rfStatistics[MSO_RIB_ATTRIBUTE_RF_STATISTICS_LENGTH];
  int8u versioning[MSO_ATTRIBUTE_VERSIONING_ENTRIES][MSO_RIB_ATTRIBUTE_VERSIONING_LENGTH];
  int8u batteryStatus[MSO_RIB_ATTRIBUTE_BATTERY_STATUS_LENGTH];
  int8u shortRfRetryPeriod[MSO_RIB_ATTRIBUTE_SHORT_RF_RETRY_PERIOD_LENGTH];
  // IR-RF database stored by the application
  int8u validationConfiguration[MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH];
  int8u generalPurpose[EMBER_AF_PLUGIN_RF4CE_MSO_GENERAL_PURPOSE_ENTRIES][MSO_RIB_ATTRIBUTE_GENERAL_PURPOSE_LENGTH];
} EmAfRf4ceMsoRibAttributes;

// Originator local attributes.
extern EmAfRf4ceMsoRibAttributes emAfRf4ceMsoLocalRibAttributes;

void emAfRf4ceMsoInitAttributes(void);

EmberAfRf4ceStatus emAfRf4ceMsoSetAttributeRequestCallback(int8u pairingIndex,
                                                           EmberAfRf4ceMsoAttributeId attributeId,
                                                           int8u index,
                                                           int8u valueLen,
                                                           const int8u *value);

EmberAfRf4ceStatus emAfRf4ceMsoGetAttributeRequestCallback(int8u pairingIndex,
                                                           EmberAfRf4ceMsoAttributeId attributeId,
                                                           int8u index,
                                                           int8u *valueLen,
                                                           int8u *value);

void emAfRf4ceMsoSetAttributeResponseCallback(EmberAfRf4ceMsoAttributeId attributeId,
                                              int8u index,
                                              EmberAfRf4ceStatus status);

void emAfRf4ceMsoGetAttributeResponseCallback(EmberAfRf4ceMsoAttributeId attributeId,
                                              int8u index,
                                              EmberAfRf4ceStatus status,
                                              int8u valueLen,
                                              const int8u *value);

boolean emAfRf4ceMsoAttributeIsSupported(EmberAfRf4ceMsoAttributeId attrId);
boolean emAfRf4ceMsoAttributeIsArrayed(EmberAfRf4ceMsoAttributeId attrId);
boolean emAfRf4ceMsoAttributeIsRemotelyWritable(EmberAfRf4ceMsoAttributeId attrId);
int8u emAfRf4ceMsoGetAttributeLength(EmberAfRf4ceMsoAttributeId attrId);
int8u emAfRf4ceMsoGetArrayedAttributeDimension(EmberAfRf4ceMsoAttributeId attrId);
boolean emAfRf4ceMsoArrayedAttributeIndexIsValid(int8u pairingIndex,
                                                 EmberAfRf4ceMsoAttributeId attributeId,
                                                 int8u index,
                                                 boolean isGet);

void emAfRf4ceMsoWriteAttributeRecipient(int8u pairingIndex,
                                         EmberAfRf4ceMsoAttributeId attributeId,
                                         int8u index,
                                         int8u valueLen,
                                         const int8u *value);

void emAfRf4ceMsoReadAttributeRecipient(int8u pairingIndex,
                                        EmberAfRf4ceMsoAttributeId attributeId,
                                        int8u index,
                                        int8u *valueLen,
                                        int8u *value);

int8u emAfRf4ceMsoAttributePeripheralIdEntryLookup(int8u pairingIndex,
                                                   int8u deviceType);
int8u emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(int8u pairingIndex);

#endif //_RF4CE_MSO_ATTRIBUTES_H_
