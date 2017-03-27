#ifndef _PRICE_COMMON_H_
#define _PRICE_COMMON_H_

#include "price-common-time.h"

#define EVENT_ID_UNSPECIFIED                          (0xFFFFFFFF)
#define TARIFF_TYPE_UNSPECIFIED                       (0xFF)
#define ZCL_PRICE_CLUSTER_DURATION16_UNTIL_CHANGED    (0xFFFF)
#define ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED  (0xFFFFFFFFUL)
#define ZCL_PRICE_CLUSTER_END_TIME_NEVER              (0xFFFFFFFFUL)
#define ZCL_PRICE_CLUSTER_NUMBER_OF_EVENTS_ALL        (0x00)
#define ZCL_PRICE_CLUSTER_START_TIME_NOW              (0x00000000UL)
#define ZCL_PRICE_CLUSTER_WILDCARD_ISSUER_ID          (0xFFFFFFFFUL)
#define ZCL_PRICE_INVALID_ENDPOINT_INDEX              (0xFF)
#define ZCL_PRICE_INVALID_INDEX                       (0xFF)

typedef struct {
  int32u startTime;

  /* Using int32u since int24u doesn't exist.  MEASURED IN SECONDS.
  * FOREVER or UNTIL_CHANGED should set this to ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED.
  * Some commands might not use this field.
  */
  int32u durationSec; 

  int32u issuerEventId;
  boolean valid;

  /* flag showing if actions are required by the current entry, such
   * as updating attributes and etc, are still pending. Usage of this flag is
   * optional for the moment.
   */
  boolean actionsPending;
} EmberAfPriceCommonInfo;

/**
 * @brief Return the best matching or other index to use for inserting new data
 * 
 * @param commonInfos An array of EmberAfPriceCommonInfo structures whose data is used to find the best available index.
 * @param numberOfEntries The number of entries in the EmberAfPriceCommonInfo array.
 * @param newIssuerEventId The issuerEventId of the new data.  This is used to see if a match is present.
 * @param newStartTime The startTime of the new data.
 * @param expireTimedOut Treats any timed-out entries as invalid if set to TRUE.
 * @return The best index - either a matching index, if found, or an invalid or timed out index.
 * 
 **/
int8u emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(EmberAfPriceCommonInfo *commonInfos,
                                                 int8u  numberOfEntries,
                                                 int32u newIssuerEventId,
                                                 int32u newStartTime,
                                                 boolean expireTimedOut );

/**
 * @brief Sort Price related data structures
 *
 * This semi-generic sorting function can be used to sort all structures that
 * utilizes the EmberAfPriceCommonInfo data type.
 *
 * @param commonInfos The destination address to which the command should be sent.
 * @param dataArray The source endpoint used in the transmission.
 * @param dataArrayBlockSizeInByte The source endpoint used in the transmission.
 * @param dataArraySize The source endpoint used in the transmission.
 *
 **/
void emberAfPluginPriceCommonSort(EmberAfPriceCommonInfo * commonInfos,
                                  int8u * dataArray,
                                  int8u dataArrayBlockSizeInByte,
                                  int8u dataArraySize );

/**
 * @brief Update durations to avoid overlapping next event
 *
 * @param commonInfos An array of EmberAfPriceCommonInfo structures that will be evaluated.
 * @param numberOfEntries The number of entries in the EmberAfPriceCommonInfo array.
 *
 **/
void emberAfPluginPriceCommonUpdateDurationForOverlappingEvents(EmberAfPriceCommonInfo *commonInfos,
                                                          int8u numberOfEntries);

/**
 * @brief Determine time until the next index becomes active.
 *
 * This function assumes the commonInfos[] array is already sorted by startTime from earliest to latest.
 *
 * @param commonInfos An array of EmberAfPriceCommonInfo structures that will be evaluated.
 * @param numberOfEntries The number of entries in the EmberAfPriceCommonInfo array.
 *
 **/
int32u emberAfPluginPriceCommonSecondsUntilSecondIndexActive(EmberAfPriceCommonInfo *commonInfos,
                                                             int8u numberOfEntries );

/**
 * @brief Find valid entries in the EmberAfPriceCommonInfo structure array.
 *
 * @param validEntries An array of the same size as the EmberAfPriceCommonInfo array that will store the valid flag for each entry (TRUE or FALSE).
 * @param numberOfEntries The number of entries in the validEntries array and the commonInfos array.
 * @param commonInfos The EmberAfPriceCommonInfo array that will be searched for valid entries.
 * @param earliestStartTime A minimum start time such that all valid entries have a start time greater than or equal to this value.
 * @minIssuerEventId A minimum event ID such that all valid entries have an issuerEventId greater than or equal to this.
 * @numberOfRequestedCommands The maximum number of valid entries to be returned.
 * @return The number of valid commands found in the commonInfos array.
 *
 **/
int8u emberAfPluginPriceCommonFindValidEntries(int8u* validEntries,
                                               int8u numberOfEntries,
                                               EmberAfPriceCommonInfo* commonInfos,
                                               int32u earliestStartTime,
                                               int32u minIssuerEventId,
                                               int8u numberOfRequestedCommands );

/**
 * @brief Returns the index of the active entry in the EmberAfPriceCommonInfo array.
 *
 * Search through array for the most recent active entry. "Issuer Event Id" has higher
 * priority than "start time".
 *
 * @param commonInfos The EmberAfPriceCommonInfo array that will be searched for an active entry.
 * @param numberOfEntries The number of entries in the commonInfo array.
 * @return The index of the active entry, or 0xFF if an active entry is not found.
 *
 **/
int8u emberAfPluginPriceCommonServerGetActiveIndex(EmberAfPriceCommonInfo *commonInfos,
                                                   int8u numberOfEntries );

/**
 * @brief Returns the index to the most recent entry that will become active in the future.
 *
 * @param commonInfos The EmberAfPriceCommonInfo array that will be searched for the entry.
 * @numberOfEntries The number of entries in the commonInfo array.
 * @secUntilFutureEvent The output pointer to the number of seconds until the next
 *                      active entry.
 *
 * @return The index of the next-active entry, or 0xFF if an active entry is not found.
 *
 **/
int8u emberAfPluginPriceCommonServerGetFutureIndex( EmberAfPriceCommonInfo *commonInfos,
                                        int8u numberOfEntries,
                                        int32u * secUntilFutureEvent);
#endif  // #ifndef _PRICE_COMMON_H_



