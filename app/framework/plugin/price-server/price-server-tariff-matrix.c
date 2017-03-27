// ****************************************************************************
// * price-server-tariff-matrix.c
// *
// *
// * Copyright 2014 by Silicon Labs. All rights reserved.                  *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "price-server.h"
#include "price-server-tick.h"

#if defined(EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_MATRIX_SUPPORT)

//=============================================================================
// Functions
void emberAfPriceClearTariffTable(int8u endpoint)
{
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    MEMSET(&info.scheduledTariffTable.commonInfos[ep][i], 0x00, sizeof(EmberAfPriceCommonInfo));
    MEMSET(&info.scheduledTariffTable.scheduledTariffs[ep][i], 0x00, sizeof(EmberAfScheduledTariff));
  }
}

void emberAfPriceClearPriceMatrixTable(int8u endpoint)
{
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    MEMSET(&info.scheduledPriceMatrixTable.commonInfos[ep][i], 0x00, sizeof(EmberAfPriceCommonInfo));
    MEMSET(&info.scheduledPriceMatrixTable.scheduledPriceMatrix[ep][i], 0x00, sizeof(EmberAfScheduledPriceMatrix));
  }
}

void emberAfPriceClearBlockThresholdsTable(int8u endpoint)
{
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    MEMSET(&info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][i], 0x00, sizeof(EmberAfScheduledBlockThresholds));
    MEMSET(&info.scheduledBlockThresholdsTable.commonInfos[ep][i], 0x00, sizeof(EmberAfPriceCommonInfo));
  }
}

// Retrieves the tariff at the index. Returns FALSE if the index is invalid.
boolean emberAfPriceGetTariffTableEntry(int8u endpoint,
                                        int8u index,
                                        EmberAfPriceCommonInfo *curInfo,
                                        EmberAfScheduledTariff *tariff)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF || index == 0xFF) {
    return FALSE;
  }

  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) {
    emberAfDebugPrintln("emberAfPriceGetTariffTableEntry: retrieving tariff at ep(%d), index(%d)", ep, index);
    MEMCOPY(curInfo, &info.scheduledTariffTable.commonInfos[ep][index], sizeof(EmberAfPriceCommonInfo));
    MEMCOPY(tariff, &info.scheduledTariffTable.scheduledTariffs[ep][index], sizeof(EmberAfScheduledTariff));
    return TRUE;
  }

  return FALSE;
}

// Retrieves the price matrix at the index. Returns FALSE if the index is invalid.
boolean emberAfPriceGetPriceMatrix(int8u endpoint,
                                   int8u index,
                                   EmberAfPriceCommonInfo *inf,
                                   EmberAfScheduledPriceMatrix *pm)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF || index == 0xFF) {
    return FALSE;
  }

  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) {
    emberAfDebugPrintln("emberAfPriceGetTariffTableEntry: retrieving tariff at ep(%d), index(%d)", ep, index);
    MEMCOPY(inf, &info.scheduledPriceMatrixTable.commonInfos[ep][index], sizeof(EmberAfPriceCommonInfo));
    MEMCOPY(pm, &info.scheduledPriceMatrixTable.scheduledPriceMatrix[ep][index], sizeof(EmberAfScheduledPriceMatrix));
    return TRUE;
  }

  return FALSE;
}

// Retrieves the block thresholds at the index. Returns FALSE if the index is invalid.
boolean emberAfPriceGetBlockThresholdsTableEntry(int8u endpoint,
                                                 int8u index,
                                                 EmberAfScheduledBlockThresholds *bt)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF || index == 0xFF) {
    return FALSE;
  }

  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) {
    emberAfDebugPrintln("emberAfPriceGetBlockThresholdsTableEntry: retrieving tariff at index %d, %d", ep, index);
    MEMCOPY(bt,
            &info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][index],
            sizeof(EmberAfScheduledBlockThresholds));
    return TRUE;
  }

  return FALSE;
}

// Retrieves the tariff with the corresponding issuer tariff ID. Returns FALSE
// if the tariff is not found or is invalid.
boolean emberAfPriceGetTariffByIssuerTariffId(int8u endpoint,
                                              int32u issuerTariffId,
                                              EmberAfPriceCommonInfo *_info,
                                              EmberAfScheduledTariff *tariff)
{
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return FALSE;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    EmberAfPriceCommonInfo *curInfo = &info.scheduledTariffTable.commonInfos[ep][i];
    EmberAfScheduledTariff *lookup = &info.scheduledTariffTable.scheduledTariffs[ep][i];

    if (lookup->status != 0 
        && lookup->issuerTariffId == issuerTariffId) {
      MEMCOPY(_info, curInfo, sizeof(EmberAfPriceCommonInfo));
      MEMCOPY(tariff, lookup, sizeof(EmberAfScheduledTariff));
      return TRUE;
    }
  }

  return FALSE;
}

// Retrieves the price matrix with the corresponding issuer tariff ID. Returns FALSE
// if the price matrix is not found or is invalid.
boolean emberAfPriceGetPriceMatrixByIssuerTariffId(int8u endpoint,
                                                   int32u issuerTariffId,
                                                   EmberAfPriceCommonInfo * inf,
                                                   EmberAfScheduledPriceMatrix *pm)
{
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return FALSE;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
	EmberAfPriceCommonInfo * lookup_info = &info.scheduledPriceMatrixTable.commonInfos[ep][i];
    EmberAfScheduledPriceMatrix *lookup_pm = &info.scheduledPriceMatrixTable.scheduledPriceMatrix[ep][i];
    if (lookup_info->valid
        && (lookup_pm->status != 0)
        && lookup_pm->issuerTariffId == issuerTariffId) {
      MEMCOPY(inf, lookup_info, sizeof(EmberAfPriceCommonInfo));
      MEMCOPY(pm, lookup_pm, sizeof(EmberAfScheduledPriceMatrix));
      return TRUE;
    }
  }

  return FALSE;
}

// Retrieves the block thresholds with the corresponding issuer tariff ID. Returns FALSE
// if the block thresholds is not found or is invalid.
boolean emberAfPriceGetBlockThresholdsByIssuerTariffId(int8u endpoint,
                                                       int32u issuerTariffId,
                                                       EmberAfPriceCommonInfo * inf,
                                                       EmberAfScheduledBlockThresholds *bt)
{
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return FALSE;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    EmberAfPriceCommonInfo *lookup_info = &info.scheduledBlockThresholdsTable.commonInfos[ep][i];
    EmberAfScheduledBlockThresholds *lookup_bt = &info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][i];
    if ((lookup_info->valid)
        && (lookup_bt->status != 0)
        && lookup_bt->issuerTariffId == issuerTariffId) {

      if (inf != NULL){
        MEMCOPY(inf, lookup_info, sizeof(EmberAfPriceCommonInfo));
      }
      if (bt != NULL){
        MEMCOPY(bt, lookup_bt, sizeof(EmberAfScheduledBlockThresholds));
      }
      return TRUE;
    }
  }

  return FALSE;
}

// Query the tariff table for tariffs matching the selection requirements dictated
// by the GetTariffInformation command. Returns the number of matching tariffs found.
/*
static int8u findTariffs(int8u endpoint,
                         int32u earliestStartTime,
                         int32u minIssuerId,
                         int8u numTariffs,
                         EmberAfTariffType tariffType,
                         EmberAfPriceCommonInfo *dstInfos,
                         EmberAfScheduledTariff *dstTariffs)
{
  int8u ep;
  int8u i, tariffsFound = 0;
  EmberAfScheduledTariff *curTariff;
  EmberAfPriceCommonInfo *curInfo;

  emberAfDebugPrintln("findTariffs: selection criteria");
  emberAfDebugPrintln("             earliestStartTime: %4x", earliestStartTime);
  emberAfDebugPrintln("             minIssuerId: %4x", minIssuerId);
  emberAfDebugPrintln("             numberOfTariffsRequested: %d", numTariffs);
  emberAfDebugPrintln("             tariffType: %x", tariffType);

  ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    goto kickout;
  }

  while (numTariffs == 0 || tariffsFound < numTariffs) {
    int32u referenceStartTime = MAX_INT32U_VALUE;
    int8u indexToSend = 0xFF;

    // Find active or scheduled tariffs matching the filter fields in the
    // request that have not been sent out yet.  Of those, find the one that
    // starts the earliest.
    for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
      curTariff = &info.scheduledTariffTable.scheduledTariffs[ep][i];
      curInfo = &info.scheduledTariffTable.commonInfos[ep][i];

      if (!tariffIsPublished(curTariff)
          && (curInfo->startTime < referenceStartTime)
          && (minIssuerId == ZCL_PRICE_CLUSTER_WILDCARD_ISSUER_ID
              || curInfo->issuerEventId >= minIssuerId)
          && (curTariff->tariffTypeChargingScheme & TARIFF_TYPE_MASK) == (tariffType & TARIFF_TYPE_MASK)
          && (tariffIsCurrent(curTariff) || tariffIsFuture(tariff))) {
        referenceStartTime =*curTariff->startTime;
        indexToSend = i;
      }
    }

    // If no active or scheduled tariffs were found, it either means there are
    // no active or scheduled tariffs at the specified time or we've already
    // found all of them in previous iterations.  If we did find one, we send
    // it, mark it as sent, and move on.
    if (indexToSend == 0xFF) {
      break;
    } else {
      emberAfDebugPrintln("findTariffs: found matching tariff at index %d", indexToSend);
      MEMCOPY(&dstInfos[tariffsFound],
              &info.scheduledTariffTable.commonInfos[ep][indexToSend],
              sizeof(EmberAfPriceCommonInfo));
      MEMCOPY(&dstTariffs[tariffsFound],
              &info.scheduledTariffTable.scheduledTariffs[ep][indexToSend],
              sizeof(EmberAfScheduledTariff));
      tariffsFound++;
      emberAfDebugPrintln("findTariffs: %d tariffs found so far", tariffsFound);
      info.scheduledTariffTable.scheduledTariffs[ep][indexToSend].status |= PUBLISHED;
    }
  }

  // Roll through all the tariffs and clear the published bit. We only use it in
  // in this function to keep track of which tariffs have already been added to
  // the return tariffs array.
  if (tariffsFound > 0) {
    for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
      if (tariffIsPublished(&info.scheduledTariffTable.scheduledTariffs[ep][i])) {
        info.scheduledTariffTable.scheduledTariffs[ep][i].status &= ~PUBLISHED;
      }
    }
  } 
kickout:
  return tariffsFound;
}
*/

boolean emberAfPriceAddTariffTableEntry(int8u endpoint,
                                        EmberAfPriceCommonInfo *curInfo, 
                                        const EmberAfScheduledTariff *curTariff)
{
  int8u destinationIndex = ZCL_PRICE_INVALID_INDEX;
  int32u smallestEventId = MAX_INT32U_VALUE;
  int8u  smallestEventIdIndex = ZCL_PRICE_INVALID_INDEX;
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  int8u tableIndex = ZCL_PRICE_INVALID_INDEX;

  if (ep == 0xFF) {
    emberAfPriceClusterPrintln("ERR: Unable to find endpoint (%d)!", endpoint);
    return FALSE;
  }

  // init
  curInfo->valid = TRUE;
  curInfo->actionsPending = TRUE;

  // get table index for new entry
  tableIndex = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(info.scheduledTariffTable.commonInfos[ep],
                                                          EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                                          curInfo->issuerEventId,
                                                          curInfo->startTime,
                                                          FALSE);

  if (tableIndex == ZCL_PRICE_INVALID_INDEX){
    emberAfPriceClusterPrintln("ERR: Unable to add new tariff info entry(%d)!", endpoint);
    return FALSE;
  }

  return emberAfPriceSetTariffTableEntry(endpoint,
                                         tableIndex,
                                         curInfo, 
                                         curTariff);
}

// Sets the tariff at the index. Returns FALSE if the index is invalid.
boolean emberAfPriceSetTariffTableEntry(int8u endpoint,
                                        int8u index,
                                        EmberAfPriceCommonInfo *newInfo, 
                                        const EmberAfScheduledTariff *newTariff)
{
  boolean update = FALSE;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    emberAfPriceClusterPrintln("ERR: Unable to find endpoint (%d)!", endpoint);
    update = FALSE;
    goto kickout;
  }
  
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) {
    emberAfPriceClusterPrintln("ERR: Index out of bound! (%d)!", index);
    update = FALSE;
    goto kickout;
  }

  if ((newTariff == NULL)
      || (newInfo == NULL)) {
    info.scheduledTariffTable.commonInfos[ep][index].valid = 0;
    info.scheduledTariffTable.scheduledTariffs[ep][index].status = 0;
    update = TRUE;
    goto kickout;
  }

  // this command doesn't use a corresponding duration time, so we set the duration as never ending.
  newInfo->durationSec = ZCL_PRICE_CLUSTER_END_TIME_NEVER;

  MEMCOPY(&info.scheduledTariffTable.commonInfos[ep][index], newInfo, sizeof(EmberAfPriceCommonInfo));
  MEMCOPY(&info.scheduledTariffTable.scheduledTariffs[ep][index], newTariff, sizeof(EmberAfScheduledTariff));

  emberAfPriceClusterPrintln("Info: Updated TariffInfo(index=%d) with tariff infos below.", index);
  emberAfPricePrintTariff(&info.scheduledTariffTable.commonInfos[ep][index],
                          &info.scheduledTariffTable.scheduledTariffs[ep][index]);

  if (info.scheduledTariffTable.scheduledTariffs[ep][index].status == 0) {
    info.scheduledTariffTable.scheduledTariffs[ep][index].status |= FUTURE;
  }
  update = TRUE;

kickout:
  if (update) {
    emberAfPluginPriceCommonSort((EmberAfPriceCommonInfo *)&info.scheduledTariffTable.commonInfos[ep],
                                 (int8u *)&info.scheduledTariffTable.scheduledTariffs[ep],
                                 sizeof(EmberAfScheduledTariff),
                                 EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);
    emberAfPriceClusterScheduleTickCallback(endpoint,
                                            EMBER_AF_PRICE_SERVER_CHANGE_TARIFF_INFORMATION_EVENT_MASK);
  }
  return update;
}

boolean emberAfPriceAddPriceMatrix(int8u endpoint,
                                   EmberAfPriceCommonInfo * inf,
                                   EmberAfScheduledPriceMatrix * pm){
  int8u ep;
  int8u tableIndex;
  ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    emberAfPriceClusterPrintln("ERR: Unable to find endpoint (%d)!", endpoint);
    return FALSE;
  }

  tableIndex = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(info.scheduledPriceMatrixTable.commonInfos[ep],
                                                          EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                                          inf->issuerEventId,
                                                          inf->startTime,
                                                          FALSE);
  if (tableIndex == ZCL_PRICE_INVALID_INDEX){
    emberAfPriceClusterPrintln("ERR: Unable to add block threshold entry!");
    return FALSE;
  }

  inf->valid = TRUE;
  inf->durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;

  return emberAfPriceSetPriceMatrix(endpoint,
                                    tableIndex,
                                    inf,
                                    pm);
}
                                            
                                             
boolean emberAfPriceAddPriceMatrixRaw(int8u endpoint,
                                      int32u providerId,
                                      int32u issuerEventId,
                                      int32u startTime,
                                      int32u issuerTariffId,
                                      int8u commandIndex,
                                      int8u numberOfCommands,
                                      int8u subPayloadControl,
                                      int8u* payload)
{
  EmberAfScheduledPriceMatrix pm;
  EmberAfPriceCommonInfo pmInfo;
  int16u payloadIndex = 0;
  int16u payloadLength = fieldLength(payload);

  pmInfo.valid = TRUE;
  pmInfo.startTime = startTime;
  pmInfo.issuerEventId = issuerEventId;
  pmInfo.durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;
  pm.issuerTariffId = issuerTariffId;
  pm.providerId = providerId;

  while (payloadIndex + ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUBPAYLOAD_BLOCK_SIZE <= payloadLength){
    int32u price;
    if ((subPayloadControl & 0x01) == 0x01) {
      int8u tier = payload[payloadIndex];
      MEMCOPY(&price, &payload[payloadIndex+1], 4);
      pm.matrix.tier[tier] = price;
      emberAfPriceClusterPrintln("Info: Updating PriceMatrix tier[%d] = 0x%4X",
                                 tier,
                                 price);
    } else if((subPayloadControl & 0x01) == 0x00) {
      int8u blockNumber = payload[payloadIndex] & 0x0F;
      int8u tier = payload[payloadIndex] & 0xF0;
      MEMCOPY(&price, &payload[payloadIndex+1], 4);
      pm.matrix.blockAndTier[tier][blockNumber] = price;
      emberAfPriceClusterPrintln("Info: Updating PriceMatrix blockAndTier[%d][%d] = 0x%4X",
                                  tier,
                                  blockNumber, 
                                  price);
    }

    payloadIndex += 5;
  }

  return emberAfPriceAddPriceMatrix(endpoint, &pmInfo, &pm);
}

// Sets the price matrix at the index. Returns FALSE if the index is invalid.
boolean emberAfPriceSetPriceMatrix(int8u endpoint,
                                   int8u index,
                                   EmberAfPriceCommonInfo * inf,
                                   const EmberAfScheduledPriceMatrix *pm)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ) {
    emberAfPriceClusterPrintln("ERR: Unable to find endpoint (%d)!", endpoint);
    return FALSE;
  }
  
  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) {
	EmberAfPriceCommonInfo * curInfo = &info.scheduledPriceMatrixTable.commonInfos[ep][index];
	EmberAfScheduledPriceMatrix * curPM = &info.scheduledPriceMatrixTable.scheduledPriceMatrix[ep][index];

    if (pm == NULL || inf == NULL) {
      curInfo->valid = FALSE;
      curPM->status = 0;
      return TRUE;
    }

    MEMCOPY(curInfo, inf, sizeof(EmberAfPriceCommonInfo));
    MEMCOPY(curPM, pm, sizeof(EmberAfScheduledPriceMatrix));

    if (curPM->status == 0) {
      curPM->status |= FUTURE;
    }

    return TRUE;
  }

  return FALSE;
}

boolean emberAfPriceAddBlockThresholdsTableEntry(int8u endpoint,
                                                 int32u providerId,
                                                 int32u issuerEventId,
                                                 int32u startTime,
                                                 int32u issuerTariffId,
                                                 int8u commandIndex,
                                                 int8u numberOfCommands,
                                                 int8u subpayloadControl,
                                                 int8u* payload)
{
  EmberAfPriceCommonInfo inf;
  EmberAfScheduledBlockThresholds bt;
  int16u payloadIndex = 0;
  boolean status = TRUE;
  int8u tableIndex;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  int16u payloadLength = fieldLength(payload);

  if (ep == 0xFF) {
    return FALSE;
  }

  tableIndex = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(info.scheduledTariffTable.commonInfos[ep],
                                                          EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                                          issuerEventId,
                                                          startTime,
                                                          FALSE);
  if (tableIndex == ZCL_PRICE_INVALID_INDEX){
    emberAfPriceClusterPrintln("ERR: Unable to add block threshold entry!");
    return FALSE;
  }

  inf.startTime = startTime;
  inf.issuerEventId = issuerEventId;
  bt.providerId = providerId;
  bt.issuerTariffId = issuerTariffId;

  if ((subpayloadControl & 0x01) == 0x01){
    // tier/numberofblockthresholds
    // least sig nibble - number of thresholds
    // most  sig nibble - not used
    // number of thresholds will apply to all tier
    // we assume payload will contain 1 8bit bit/numberOfBlockThresholds
    // followed by block thresholds
    int8u i;
    int8u tier = (payload[payloadIndex] & 0xF0) >> 4;
    int8u numberOfBt = (payload[payloadIndex] & 0x0F);

    payloadIndex++;
    if (payloadIndex + numberOfBt * ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE 
        > payloadLength){
        emberAfPriceClusterPrintln("ERR: Invalid number of thresholds(%d) within BlockThresholds payload!",
                                   numberOfBt);
        status = FALSE;
        goto kickout;
    }

    if (numberOfBt > ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS-1){
      emberAfPriceClusterPrintln("ERR: The number(%d) of block threshold is larger than the max (%d)!",
                                  numberOfBt,
                                  ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS-1);
      status = FALSE;
      goto kickout;
    }

    for (i=0; i<numberOfBt; i++){
      MEMCOPY(&bt.thresholds.block[i],
              &payload[payloadIndex + i*ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE],
              ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE);
      emberAfPriceClusterPrint("Info: Updating block[%d] = 0x",
                               i);
       emberAfPriceClusterPrintBuffer(&payload[payloadIndex + i*ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE],
                                      ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE,
                                      FALSE);
       emberAfPriceClusterPrintln("");
    }

  } else if ((subpayloadControl & 0x01) == 0x00){
    // tier/numberofblockthresholds
    // least sig nibble - number of thresholds
    // most  sig nibble - tier
    while (payloadIndex < payloadLength){
      int8u tier = (payload[payloadIndex] & 0xF0) >> 4;
      int8u numberOfBt = (payload[payloadIndex] & 0x0F);
      int8u i;
      payloadIndex++;

      // do the block thresholds data exist in the payloads?
      if (payloadIndex +
          numberOfBt * ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE
          > payloadLength){
        emberAfPriceClusterPrintln("ERR: Invalid number of thresholds(%d) within BlockThresholds payload!",
                                   numberOfBt);
        status = FALSE;
        goto kickout;
      }

      if (tier > ZCL_PRICE_CLUSTER_MAX_TOU_BLOCK_TIERS){
        emberAfPriceClusterPrintln("ERR: Invalid tier id within BlockThresholds payload!");
        status = FALSE;
        goto kickout;
      }
      
      if (numberOfBt > ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS-1){
        emberAfPriceClusterPrintln("ERR: The number(%d) of block threshold is larger than the max (%d)!",
                                   numberOfBt,
                                   ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS-1);
        status = FALSE;
        goto kickout;
      }
      
      for (i=0; i<numberOfBt; i++){
        MEMCOPY(&bt.thresholds.blockAndTier[tier][i],
                &payload[payloadIndex + i*ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE],
                ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE);
        emberAfPriceClusterPrint("Info: Updating blockAndTier[%d][%d] = 0x",
                                   tier,
                                   i);
        emberAfPriceClusterPrintBuffer(&payload[payloadIndex + i*ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE],
                                       ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE,
                                       FALSE);
        emberAfPriceClusterPrintln("");
      }
      payloadIndex += numberOfBt * ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE;
    }
  }

kickout:
  if (status){
    return emberAfPriceSetBlockThresholdsTableEntry(endpoint,
                                                    tableIndex,
                                                    &inf,
                                                    &bt);
  } else {
    return FALSE;
  }

}

// Sets the block thresholds at the index. Returns FALSE if the index is invalid.
boolean emberAfPriceSetBlockThresholdsTableEntry(int8u endpoint,
                                                 int8u index,
                                                 const EmberAfPriceCommonInfo *inf,
                                                 const EmberAfScheduledBlockThresholds *bt)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return FALSE;
  }

  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) {
    if (bt == NULL || inf == NULL) {
      info.scheduledBlockThresholdsTable.commonInfos[ep][index].valid = FALSE;
      info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][index].status = 0;
      return TRUE;
    }

    MEMCOPY(&info.scheduledBlockThresholdsTable.commonInfos[ep][index],
            inf, 
            sizeof(EmberAfPriceCommonInfo));
    MEMCOPY(&info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][index],
            bt,
            sizeof(EmberAfScheduledBlockThresholds));

    info.scheduledBlockThresholdsTable.commonInfos[ep][index].valid = TRUE;
    info.scheduledBlockThresholdsTable.commonInfos[ep][index].durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;

    if (info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][index].status == 0) {
      info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][index].status |= FUTURE;
    }

    return TRUE;
  }
  return FALSE;
}

void emberAfPricePrintTariff(const EmberAfPriceCommonInfo *info, 
                             const EmberAfScheduledTariff *tariff)
{
  emberAfPriceClusterPrintln(" valid: %d", info->valid);
  emberAfPriceClusterPrint(" label: ");
  emberAfPriceClusterPrintString(tariff->tariffLabel);
  if (emberAfStringLength(tariff->tariffLabel) > 0){
    emberAfPriceClusterPrint("(0x%X)", emberAfStringLength(tariff->tariffLabel));
  }
  emberAfPriceClusterPrint("\n uom/cur: 0x%X/0x%2X"
                           "\r\n pid/eid/etid: 0x%4X/0x%4X/0x%4X"
                           "\r\n st/tt: 0x%4X/0x%X",
                           tariff->unitOfMeasure,
                           tariff->currency,
                           tariff->providerId,
                           info->issuerEventId,
                           tariff->issuerTariffId,
                           info->startTime,
                           tariff->tariffTypeChargingScheme);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrint("\r\n ptu/btu: 0x%X/0x%X"
                           "\r\n ptd/sc/tbm: 0x%X/0x%4X/0x%X"
                           "\r\n btm/btd: 0x%4X/0x%4X",
                           tariff->numberOfPriceTiersInUse,
                           tariff->numberOfBlockThresholdsInUse,
                           tariff->priceTrailingDigit,
                           tariff->standingCharge,
                           tariff->tierBlockMode,
                           tariff->blockThresholdMultiplier,
                           tariff->blockThresholdDivisor);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrint("\n tariff is ");
  if (tariffIsCurrent(tariff)) {
    emberAfPriceClusterPrintln("current");
  } else if (tariffIsFuture(tariff)) {
    emberAfPriceClusterPrintln("future");
  } else {
    emberAfPriceClusterPrintln("invalid");
  }
  emberAfPriceClusterFlush();
}
    
void emberAfPricePrintPriceMatrix(int8u endpoint,
		                          const EmberAfPriceCommonInfo *inf,
                                  const EmberAfScheduledPriceMatrix *pm)
{
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo tariffInfo;
  boolean found;
  int8u i, j, chargingScheme;

  found = emberAfPriceGetTariffByIssuerTariffId(endpoint,
                                                pm->issuerTariffId,
                                                &tariffInfo,
                                                &t);

  if (!found) {
    emberAfPriceClusterPrint("  No corresponding valid tariff found; price matrix cannot be printed.\n");
    emberAfPriceClusterPrint("  (NOTE: If printing from command line, be sure the tariff has been pushed to the tariff table.)\n");
    return;
  }

  chargingScheme = t.tariffTypeChargingScheme;

  emberAfPriceClusterPrint("  provider id: %4x\r\n", pm->providerId);
  emberAfPriceClusterPrint("  issuer event id: %4x\r\n", inf->issuerEventId);
  emberAfPriceClusterPrint("  issuer tariff id: %4x\r\n", pm->issuerTariffId);
  emberAfPriceClusterPrint("  start time: %4x\r\n", inf->startTime);

  emberAfPriceClusterFlush();

  emberAfPriceClusterPrint("  == matrix contents == \r\n");
  switch (chargingScheme >> 4) {
    case 0: // TOU only
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        emberAfPriceClusterPrint("  tier %d: %4x\r\n", i, pm->matrix.tier[i]);
      }
      break;
    case 1: // Block only
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        emberAfPriceClusterPrint("  tier %d (block 1): %4x\r\n", i, pm->matrix.blockAndTier[i][0]);
      }
      break;
    case 2: // TOU and Block
    case 3:
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        for (j = 0; j < t.numberOfBlockThresholdsInUse + 1; j++) {
          emberAfPriceClusterPrint("  tier %d block %d: %4x\r\n", i, j + 1, pm->matrix.blockAndTier[i][j]);
        }
      }
      break;
    default:
      emberAfPriceClusterPrint("  Invalid pricing scheme; no contents. \r\n");
      break;
  }

  emberAfPriceClusterPrint("  == end matrix contents == \r\n");
  emberAfPriceClusterFlush();
}

void emberAfPricePrintBlockThresholds(int8u endpoint,
                                      const EmberAfPriceCommonInfo * inf,
                                      const EmberAfScheduledBlockThresholds *bt)
{
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo info;
  boolean found;
  int8u i, j, tierBlockMode;

  found = emberAfPriceGetTariffByIssuerTariffId(endpoint,
                                                bt->issuerTariffId,
                                                &info, 
                                                &t);

  if (!found) {
    emberAfPriceClusterPrint("  No corresponding valid tariff found; block thresholds cannot be printed.\n");
    emberAfPriceClusterPrint("  (NOTE: If printing from command line, be sure the tariff has been pushed to the tariff table.)\n");
    return;
  }

  tierBlockMode = t.tierBlockMode;

  emberAfPriceClusterPrint("  provider id: %4x\r\n", bt->providerId);
  emberAfPriceClusterPrint("  issuer event id: %4x\r\n", inf->issuerEventId);
  emberAfPriceClusterPrint("  issuer tariff id: %4x\r\n", bt->issuerTariffId);
  emberAfPriceClusterPrint("  start time: %4x\r\n", inf->startTime);

  emberAfPriceClusterFlush();

  emberAfPriceClusterPrint("  == thresholds contents == \r\n");
  switch (tierBlockMode) {
    case 0: // ActiveBlock
    case 1: // ActiveBlockPriceTier
      for (j = 0; j < t.numberOfBlockThresholdsInUse; j++) {
        emberAfPriceClusterPrint("  block threshold %d: 0x%x%x%x%x%x%x\r\n", j,
                                 bt->thresholds.block[j][0],
                                 bt->thresholds.block[j][1],
                                 bt->thresholds.block[j][2],
                                 bt->thresholds.block[j][3],
                                 bt->thresholds.block[j][4],
                                 bt->thresholds.block[j][5]);
      }
      break;
    case 2: // ActiveBlockPriceTierThreshold
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        for (j = 0; j < t.numberOfBlockThresholdsInUse; j++) {
          emberAfPriceClusterPrint("  tier %d block threshold %d: 0x%x%x%x%x%x%x\r\n", i, j,
                                   bt->thresholds.blockAndTier[i][j][0],
                                   bt->thresholds.blockAndTier[i][j][1],
                                   bt->thresholds.blockAndTier[i][j][2],
                                   bt->thresholds.blockAndTier[i][j][3],
                                   bt->thresholds.blockAndTier[i][j][4],
                                   bt->thresholds.blockAndTier[i][j][5]);
        }
      }
      break;
    default:
      emberAfPriceClusterPrint("  Invalid tier block mode; no contents. \r\n");
      break;
  }

  emberAfPriceClusterPrint("  == end thresholds contents == \r\n");
  emberAfPriceClusterFlush();
}

void emberAfPricePrintTariffTable(int8u endpoint) 
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  
  if (ep == 0xFF) {
    return;
  }

  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("Tariff Table Contents: ");
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("  Note: ALL values given in HEX\r\n");
  emberAfPriceClusterFlush();
  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    emberAfPriceClusterPrintln("= TARIFF %d =",
                               i);
    emberAfPricePrintTariff(&info.scheduledTariffTable.commonInfos[ep][i],
                            &info.scheduledTariffTable.scheduledTariffs[ep][i]);
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPricePrintPriceMatrixTable(int8u endpoint) 
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  
  if (ep == 0xFF) {
    return;
  }

  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("Price Matrix Table Contents: ");
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("  Note: ALL values given in HEX (except indices)\r\n");
  emberAfPriceClusterFlush();
  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    emberAfPriceClusterPrintln("=PRICE MATRIX %x =",
                               i);
    emberAfPricePrintPriceMatrix(endpoint,
                                 &info.scheduledPriceMatrixTable.commonInfos[ep][i],
                                 &info.scheduledPriceMatrixTable.scheduledPriceMatrix[ep][i]);
  }

#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPricePrintBlockThresholdsTable(int8u endpoint)
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  int8u i;
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return;
  }

  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("Block Thresholds Table Contents: ");
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("  Note: ALL values given in HEX (except indices)\r\n");
  emberAfPriceClusterFlush();
  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE; i++) {
    emberAfPriceClusterPrintln("=BLOCK THRESHOLDS %x =",
                               i);
    emberAfPricePrintBlockThresholds(endpoint,
                                     &info.scheduledBlockThresholdsTable.commonInfos[ep][i],
                                     &info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][i]);
  }

#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

boolean emberAfPriceClusterGetTariffInformationCallback(int32u earliestStartTime,
                                                        int32u minIssuerEventId,
                                                        int8u numberOfCommands,
                                                        int8u tariffType) 
{
  int8u validCmds[EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
  EmberAfScheduledTariff tariffs[EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
  EmberAfPriceCommonInfo infos[EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
  int8u ep = emberAfFindClusterServerEndpointIndex(emberAfCurrentEndpoint(), ZCL_PRICE_CLUSTER_ID);
  int8u entriesCount;
  int8u validEntriesCount;
  int8u i;

  emberAfDebugPrintln("RX: GetTariffInformation, 0x%4X, 0x%4X, 0x%X, 0x%X", 
                      earliestStartTime,
                      minIssuerEventId, 
                      numberOfCommands, 
                      tariffType);

  if (ep == 0xFF) {
    emberAfPriceClusterPrintln("ERR: Unable to find endpoint (%d)!", ep);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_VALUE);
    return TRUE;
  }

  entriesCount = emberAfPluginPriceCommonFindValidEntries(validCmds, 
                                              EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                              (EmberAfPriceCommonInfo *)info.scheduledTariffTable.commonInfos[ep],
                                              earliestStartTime, 
                                              minIssuerEventId,
                                              numberOfCommands);
  validEntriesCount = entriesCount;

  // eliminate commands with mismatching tariffType
  // upper nibble is reserved. we'll ignore them.
  {
    int8u i;
    for (i=0; i<entriesCount; i++){
      int8u index = validCmds[i];
      if ((info.scheduledTariffTable.scheduledTariffs[ep][index].tariffTypeChargingScheme & TARIFF_TYPE_MASK) != (tariffType & TARIFF_TYPE_MASK)){
        validCmds[i] = ZCL_PRICE_INVALID_INDEX;
        validEntriesCount--;
      }
    }
  }

  emberAfDebugPrintln("Tariffs found: %d", validEntriesCount);
  sendValidCmdEntries(ZCL_PUBLISH_TARIFF_INFORMATION_COMMAND_ID,
                      ep,
                      validCmds,
                      entriesCount);

  return TRUE;
}

boolean emberAfPriceClusterGetPriceMatrixCallback(int32u issuerTariffId)
{
  EmberAfScheduledTariff tariff;
  EmberAfPriceCommonInfo tariffInfo;
  EmberAfScheduledPriceMatrix pm;
  EmberAfPriceCommonInfo pmInfo;
  boolean found;
  int8u endpoint = emberAfCurrentEndpoint(), i, j, payloadControl;
  int16u size = 0;
  // Allocate for the largest possible size, unfortunately
  int8u subPayload[ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS 
                   * ZCL_PRICE_CLUSTER_MAX_TOU_BLOCK_TIERS
                   * ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE];

  // A price matrix must have an associated tariff, otherwise it is meaningless
  found = emberAfPriceGetTariffByIssuerTariffId(endpoint,
                                                issuerTariffId,
                                                &tariffInfo,
                                                &tariff);

  if (!found) {
    emberAfDebugPrintln("GetPriceMatrix: no corresponding tariff for id 0x%4x found", 
                        issuerTariffId);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  // Grab the actual price matrix
  found = emberAfPriceGetPriceMatrixByIssuerTariffId(endpoint,
                                                     issuerTariffId,
                                                     &pmInfo,
                                                     &pm);

  if (!found) {
    emberAfDebugPrintln("GetPriceMatrix: no corresponding price matrix for id 0x%4x found", 
                        issuerTariffId);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  // The structure of the price matrix will vary depending on the type of the tariff
  switch (tariff.tariffTypeChargingScheme >> 4) {
    case 0: // TOU only
      payloadControl = 1;
      for (i = 0; i < tariff.numberOfPriceTiersInUse; i++) {
        subPayload[i * ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE] = i;
        emberAfCopyInt32u(subPayload,
                          i * ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE + 1,
                          pm.matrix.tier[i]);
        size += ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE;
      }
      break;
    case 1: // Block only
      payloadControl = 0;
      for (i = 0; i < tariff.numberOfPriceTiersInUse; i++) {
        subPayload[i * ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE] = i << 4;
        emberAfCopyInt32u(subPayload,
                          i * ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE + 1,
                          pm.matrix.blockAndTier[i][0]);
        size += ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE;
      }
      break;
    case 2:
    case 3: // TOU / Block combined
      payloadControl = 0;
      for (i = 0; i < tariff.numberOfPriceTiersInUse; i++) {
        for (j = 0; j < tariff.numberOfBlockThresholdsInUse + 1; j++) { 
          subPayload[size] = (i << 4) | j;
          emberAfCopyInt32u(subPayload,
                            size + 1,
                            pm.matrix.blockAndTier[i][j]);
          size += ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE;
        }
      }
      break;
    default:
      emberAfDebugPrintln("GetPriceMatrix: invalid tariff type / charging scheme");
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_VALUE);
      return TRUE;
  }

  // Populate and send the PublishPriceMatrix command
  emberAfDebugPrintln("GetPriceMatrix: subpayload size 0x%2x", size);
  emberAfFillCommandPriceClusterPublishPriceMatrix(pm.providerId,
                                                   pmInfo.issuerEventId,
                                                   pmInfo.startTime,
                                                   pm.issuerTariffId,
                                                   0,
                                                   1,
                                                   payloadControl,
                                                   subPayload,
                                                   size);
  emberAfSendResponse();

  return TRUE;
}

static void emberAfPutPriceBlockThresholdInResp(emAfPriceBlockThreshold *threshold)
{
  int16u length = sizeof(emAfPriceBlockThreshold);

#if BIGENDIAN_CPU
  int8s loc  = length - 1;
  int8s end  = -1;
  int8s incr = -1;
#else
  int8s loc  = 0;
  int8s end  = length;
  int8s incr = 1;
#endif

  while ( loc != end ) {
    emberAfPutInt8uInResp(((int8u *)threshold)[loc]);
    loc += incr;
  }
}

boolean emberAfPriceClusterGetBlockThresholdsCallback(int32u issuerTariffId)
{
  EmberAfPriceCommonInfo tariffInfo;
  EmberAfScheduledTariff tariff;
  EmberAfPriceCommonInfo btInfo;
  EmberAfScheduledBlockThresholds bt;
  boolean found;
  int8u endpoint = emberAfCurrentEndpoint(), i, j;
  int16u size = 0;

  // Block thresholds must have an associated tariff, otherwise it is meaningless
  found = emberAfPriceGetTariffByIssuerTariffId(endpoint,
                                                issuerTariffId,
                                                &tariffInfo,
                                                &tariff);

  if (!found) {
    emberAfDebugPrintln("GetBlockThresholds: no corresponding tariff for id 0x%4x found",
                        issuerTariffId);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  // Grab the actual block thresholds
  found = emberAfPriceGetBlockThresholdsByIssuerTariffId(endpoint,
                                                         issuerTariffId,
                                                         &btInfo,
                                                         &bt);

  if (!found) {
    emberAfDebugPrintln("GetBlockThresholds: no corresponding block thresholds for id 0x%4x found",
                        issuerTariffId);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  // Populate and send the PublishBlockThresholds command
  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                             | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                            ZCL_PRICE_CLUSTER_ID,
                            ZCL_PUBLISH_BLOCK_THRESHOLDS_COMMAND_ID,
                            "wwwwuu",
                            bt.providerId,
                            btInfo.issuerEventId,
                            btInfo.startTime,
                            bt.issuerTariffId,
                            0,
                            1);

  // The structure of the block thresholds subpayload will vary depending on the tier block mode
  switch (tariff.tierBlockMode) {
    case 0: // ActiveBlock
    case 1: // ActiveBlockPriceTier
      emberAfPutInt8uInResp(1); // payload control
      emberAfPutInt8uInResp(tariff.numberOfBlockThresholdsInUse);
      size += 1;
      for (j = 0; j < tariff.numberOfBlockThresholdsInUse; j++) {
        emberAfPutPriceBlockThresholdInResp(&bt.thresholds.block[j]);
        size += 6;
      }
      break;
    case 2: // ActiveBlockPriceTierThreshold
      emberAfPutInt8uInResp(0); // payload control
      for (i = 0; i < tariff.numberOfPriceTiersInUse; i++) {
        emberAfPutInt8uInResp((i << 4) | tariff.numberOfBlockThresholdsInUse);
        size += 1;
        for (j = 0; j < tariff.numberOfBlockThresholdsInUse; j++) {
          emberAfPutPriceBlockThresholdInResp(&bt.thresholds.blockAndTier[i][j]);
          size += 6;
        }
      }
      break;
    default:
      emberAfDebugPrintln("GetBlockThresholds: invalid tier block mode");
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_VALUE);
      return TRUE;
  }

  emberAfDebugPrintln("GetBlockThresholds: subpayload size 0x%2x", size);
  emberAfSendResponse();

  return TRUE;
}
#else // !defined(EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_MATRIX_SUPPORT)

void emberAfPriceClearTariffTable(int8u endpoint)
{
}

void emberAfPriceClearPriceMatrixTable(int8u endpoint)
{
}

void emberAfPriceClearBlockThresholdsTable(int8u endpoint)
{
}

boolean emberAfPriceSetTariffTableEntry(int8u endpoint,
                                        int8u index,
                                        EmberAfPriceCommonInfo *info, 
                                        const EmberAfScheduledTariff *tariff)
{
  return FALSE;
}

boolean emberAfPriceClusterGetTariffInformationCallback(int32u earliestStartTime,
                                                        int32u minIssuerEventId,
                                                        int8u numberOfCommands,
                                                        int8u tariffType) 
{
  return FALSE;
}

boolean emberAfPriceClusterGetPriceMatrixCallback(int32u issuerTariffId)
{
  return FALSE;
}

boolean emberAfPriceClusterGetBlockThresholdsCallback(int32u issuerTariffId)
{
  return FALSE;
}

boolean emberAfPriceGetTariffTableEntry(int8u endpoint,
                                        int8u index,
                                        EmberAfPriceCommonInfo *info,
                                        EmberAfScheduledTariff *tariff)
{
  return FALSE;
}

boolean emberAfPriceAddTariffTableEntry(int8u endpoint,
                                        EmberAfPriceCommonInfo *curInfo, 
                                        const EmberAfScheduledTariff *curTariff)
{
  return FALSE;
}

boolean emberAfPriceGetPriceMatrix(int8u endpoint,
                                   int8u index,
                                   EmberAfPriceCommonInfo *inf,
                                   EmberAfScheduledPriceMatrix *pm)
{ 
  return FALSE;
}

boolean emberAfPriceGetBlockThresholdsTableEntry(int8u endpoint,
                                                 int8u index,
                                                 EmberAfScheduledBlockThresholds *bt) {
  return FALSE;
}

boolean emberAfPriceGetTariffByIssuerTariffId(int8u endpoint,
                                              int32u issuerTariffId,
                                              EmberAfPriceCommonInfo *_info,
                                              EmberAfScheduledTariff *tariff) {
  return FALSE;
}

boolean emberAfPriceGetPriceMatrixByIssuerTariffId(int8u endpoint,
                                                   int32u issuerTariffId,
                                                   EmberAfPriceCommonInfo * inf,
                                                   EmberAfScheduledPriceMatrix *pm)
{
  return FALSE;
}

boolean emberAfPriceGetBlockThresholdsByIssuerTariffId(int8u endpoint,
                                                       int32u issuerTariffId,
                                                       EmberAfPriceCommonInfo *inf,
                                                       EmberAfScheduledBlockThresholds *bt)
{
  return FALSE;
}

boolean emberAfPriceSetPriceMatrix(int8u endpoint,
                                   int8u index,
                                   EmberAfPriceCommonInfo * inf,
                                   const EmberAfScheduledPriceMatrix *pm)
{

  return FALSE;
}

boolean emberAfPriceSetBlockThresholdsTableEntry(int8u endpoint,
                                                 int8u index,
                                                 const EmberAfPriceCommonInfo *inf,
                                                 const EmberAfScheduledBlockThresholds *bt) {
  return FALSE;
}

void emberAfPricePrintTariff(const EmberAfPriceCommonInfo *info, 
                             const EmberAfScheduledTariff *tariff) {}
void emberAfPricePrintPriceMatrix(int8u endpoint,
	                              const EmberAfPriceCommonInfo * inf,
                                  const EmberAfScheduledPriceMatrix *pm){}
void emberAfPricePrintBlockThresholds(int8u endpoint,
                                      const EmberAfPriceCommonInfo * inf,
                                      const EmberAfScheduledBlockThresholds *bt){}
void emberAfPricePrintTariffTable(int8u endpoint) {}
void emberAfPricePrintPriceMatrixTable(int8u endpoint) {}
void emberAfPricePrintBlockThresholdsTable(int8u endpoint) {}
boolean emberAfPriceAddBlockThresholdsTableEntry(int8u endpoint,
                                                 int32u providerId,
                                                 int32u issuerEventId,
                                                 int32u startTime,
                                                 int32u issuerTariffId,
                                                 int8u commandIndex,
                                                 int8u numberOfCommands,
                                                 int8u subpayloadControl,
                                                 int8u* payload ) {
  return FALSE;
}

boolean emberAfPriceAddPriceMatrixRaw(int8u endpoint,
                                      int32u providerId,
                                      int32u issuerEventId,
                                      int32u startTime,
                                      int32u issuerTariffId,
                                      int8u commandIndex,
                                      int8u numberOfCommands,
                                      int8u subPayloadControl,
                                      int8u* payload) {
  return FALSE;
}

#endif // EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_MATRIX_SUPPORT
