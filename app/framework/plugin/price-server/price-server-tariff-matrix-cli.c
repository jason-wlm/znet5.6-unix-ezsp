// ****************************************************************************
// * price-server-tariff-matrix-cli.c
// *
// *
// * Copyright 2014 by Silicon Labs. All rights reserved.                  *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/util/serial/command-interpreter2.h"
#include "app/framework/plugin/price-server/price-server.h"

#if defined(EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_MATRIX_SUPPORT)

//=============================================================================
// Globals

static EmberAfPriceCommonInfo          scheduledTariffInfo;
static EmberAfScheduledTariff          scheduledTariff;
static EmberAfPriceCommonInfo          btInfo;
static EmberAfScheduledBlockThresholds bt;
static EmberAfPriceCommonInfo          pmInfo;
static EmberAfScheduledPriceMatrix     pm;

//=============================================================================
// Functions


// plugin price-server tclear <endpoint:1>
void emAfPriceServerCliTClear(void)
{
  emberAfPriceClearTariffTable(emberUnsignedCommandArgument(0));
}

// plugin price-server pmclear <endpoint:1>
void emAfPriceServerCliPmClear(void) 
{
  emberAfPriceClearPriceMatrixTable(emberUnsignedCommandArgument(0));
}

// plugin price-server btclear <endpoint:1>
void emAfPriceServerCliBtClear(void)
{
  emberAfPriceClearBlockThresholdsTable(emberUnsignedCommandArgument(0));
}

// plugin price-server twho <provId:4> <label:1-13> <eventId:4> <tariffId:4>
void emAfPriceServerCliTWho(void)
{
  int8u length;
  scheduledTariff.providerId = emberUnsignedCommandArgument(0);
  length = emberCopyStringArgument(1,
                                   scheduledTariff.tariffLabel + 1,
                                   ZCL_PRICE_CLUSTER_MAXIMUM_RATE_LABEL_LENGTH,
                                   FALSE);
  scheduledTariff.tariffLabel[0] = length;
  scheduledTariffInfo.issuerEventId = (int32u)emberUnsignedCommandArgument(2);
  scheduledTariff.issuerTariffId = (int32u)emberUnsignedCommandArgument(3);
}

// plugin price-server twhat <type:1> <unitOfMeas:1> <curr:2> <ptd:1> <prt:1> <btu:1> <blockMode:1>
void emAfPriceServerCliTWhat(void)
{
  scheduledTariff.tariffTypeChargingScheme = (int8u)emberUnsignedCommandArgument(0);
  scheduledTariff.unitOfMeasure = (int8u)emberUnsignedCommandArgument(1);
  scheduledTariff.currency = (int16u)emberUnsignedCommandArgument(2);
  scheduledTariff.priceTrailingDigit = (int8u)emberUnsignedCommandArgument(3);
  scheduledTariff.numberOfPriceTiersInUse = (int8u)emberUnsignedCommandArgument(4);
  scheduledTariff.numberOfBlockThresholdsInUse = (int8u)emberUnsignedCommandArgument(5);
  scheduledTariff.tierBlockMode = (int8u)emberUnsignedCommandArgument(6);
}

// plugin price-server twhen <startTime:4>
void emAfPriceServerCliTWhen(void)
{
  scheduledTariffInfo.startTime = emberUnsignedCommandArgument(0);
  scheduledTariffInfo.durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;
}

// plugin price-server tariff <standingCharge:4> <btm:4> <btd:4>
void emAfPriceServerCliTariff(void)
{
  scheduledTariff.standingCharge = emberUnsignedCommandArgument(0);
  scheduledTariff.blockThresholdMultiplier = emberUnsignedCommandArgument(1);
  scheduledTariff.blockThresholdDivisor = emberUnsignedCommandArgument(2);
}


// plugin price-server tadd <endpoint:1> <status:1>
void emAfPriceServerCliTAdd(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u status = (int8u)emberUnsignedCommandArgument(1);

  if (status == 0) {
    scheduledTariff.status |= CURRENT;
  } else {
    scheduledTariff.status |= FUTURE;
  }

  if (!emberAfPriceAddTariffTableEntry(endpoint,
                                       &scheduledTariffInfo,
                                       &scheduledTariff)) {
    emberAfPriceClusterPrintln("ERR:Failed to set table entry!");
  }
}

// plugin price-server tget <endpoint:1> <index:1>
void emAfPriceServerCliTGet(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u index = (int8u)emberUnsignedCommandArgument(1);
  if (!emberAfPriceGetTariffTableEntry(endpoint,
                                       index,
                                       &scheduledTariffInfo,
                                       &scheduledTariff)) {

    emberAfPriceClusterPrintln("tariff entry %d not present", index);
  }
}

// plugin price-server tset <endpoint:1> <index:1> <status:1>
void emAfPriceServerCliTSet(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u index = (int8u)emberUnsignedCommandArgument(1);
  int8u status = (int8u)emberUnsignedCommandArgument(2);

  scheduledTariffInfo.valid = TRUE;
  scheduledTariff.status = status;

  if (!emberAfPriceSetTariffTableEntry(endpoint,
                                       index,
                                       &scheduledTariffInfo,
                                       &scheduledTariff)) {
    emberAfPriceClusterPrintln("tariff entry %d not present", index);
  }
}


// plugin price-server tprint <endpoint:1>
void emAfPriceServerCliTPrint(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emberAfPricePrintTariffTable(endpoint);
}

// plugin price-server tsprint
void emAfPriceServerCliTSPrint(void)
{
  emberAfPricePrintTariff(&scheduledTariffInfo, &scheduledTariff);
}

// plugin price-server pmprint <endpoint:1>
void emAfPriceServerCliPmPrint(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emberAfPricePrintPriceMatrixTable(endpoint);
}

// plugin price-server pm set-metadata <endpoint:1> <providerId:4> <issuerEventId:4> 
//                                     <issuerTariffId:4> <startTime:4> <status:1>
void emAfPriceServerCliPmSetMetadata(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int32u tariffId = (int32u)emberUnsignedCommandArgument(3);
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo i;

  if (!(emberAfPriceGetTariffByIssuerTariffId(endpoint, tariffId, &i, &t)
        || scheduledTariff.issuerTariffId == tariffId)) {
    emberAfPriceClusterPrint("Invalid issuer tariff ID; no corresponding tariff found.\n");
    return;
  }

  pm.providerId = (int32u)emberUnsignedCommandArgument(1);
  pmInfo.issuerEventId = (int32u)emberUnsignedCommandArgument(2);
  pm.issuerTariffId = tariffId;
  pmInfo.startTime = (int32u)emberUnsignedCommandArgument(4);
  pm.status = (int8u)emberUnsignedCommandArgument(5);
}

// plugin price-server pm set-provider <providerId:4>
void emAfPriceServerCliPmSetProvider(void)
{
  pm.providerId = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server pm set-event <issuerEventId:4>
void emAfPriceServerCliPmSetEvent(void)
{
  pmInfo.issuerEventId = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server pm set-tariff <issuerTariffId:4>
void emAfPriceServerCliPmSetTariff(void)
{
  pm.issuerTariffId = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server pm set-time <startTime:4>
void emAfPriceServerCliPmSetTime(void)
{
  pmInfo.startTime = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server pm set-status <status:1>
void emAfPriceServerCliPmSetStatus(void)
{
  pm.status = (int8u)emberUnsignedCommandArgument(0);
}

// plugin price-server pm get <endpoint:1> <index:1>
void emAfPriceServerCliPmGet(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u index = (int8u)emberUnsignedCommandArgument(1);
  emberAfPriceGetPriceMatrix(endpoint, index, &pmInfo, &pm);
}

// plugin price-server pm set <endpoint:1>
void emAfPriceServerCliPmSet(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u index = (int8u)emberUnsignedCommandArgument(1);
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo i;

  if (!(emberAfPriceGetTariffByIssuerTariffId(endpoint, pm.issuerTariffId, &i, &t)
        || scheduledTariff.issuerTariffId == pm.issuerTariffId)) {
    emberAfPriceClusterPrint("Invalid issuer tariff ID; no corresponding tariff found.\n");
    return;
  }

  emberAfPriceAddPriceMatrix(endpoint, &pmInfo, &pm);
}

// plugin price-server pm put <endpoint:1> <tier:1> <block:1> <price:4>
void emAfPriceServerCliPmPut(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u tier = (int8u)emberUnsignedCommandArgument(1);
  int8u block = (int8u)emberUnsignedCommandArgument(2);
  int32u price = (int32u)emberUnsignedCommandArgument(3);
  int8u chargingScheme;
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo i;

  if (pm.issuerTariffId == scheduledTariff.issuerTariffId) {
    t = scheduledTariff;
    chargingScheme = scheduledTariff.tariffTypeChargingScheme;
  } else {
    boolean found = emberAfPriceGetTariffByIssuerTariffId(endpoint, pm.issuerTariffId, &i, &t);
    if (!found) {
      emberAfPriceClusterPrint("Invalid issuer tariff ID in price matrix; no corresponding tariff found.\n"); 
      return;
    } else {
      chargingScheme = t.tariffTypeChargingScheme;
    }
  }

  if (tier >= t.numberOfPriceTiersInUse 
      || block > t.numberOfBlockThresholdsInUse) {
    emberAfPriceClusterPrint("Invalid index into price matrix. Value not set.\n");
    return;
  }

  switch (chargingScheme >> 4) {
    case 0: // TOU only
      pm.matrix.tier[tier] = price;
      break;
    case 1: // Block only
      pm.matrix.blockAndTier[tier][0] = price;
      break;
    case 2:
    case 3: // TOU and Block
      pm.matrix.blockAndTier[tier][block] = price;
      break;
    default:
      emberAfDebugPrintln("Invalid tariff type / charging scheme.");
      break;
  }
}

// plugin price-server pm fill-tier <endpoint:1> <tier:1> <price:4>
void emAfPriceServerCliPmFillTier(void)
{
  int8u chargingScheme, i;
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo inf;
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u tier = (int8u)emberUnsignedCommandArgument(1);
  int32u price = (int32u)emberUnsignedCommandArgument(2);

  if (pm.issuerTariffId == scheduledTariff.issuerTariffId) {
    t = scheduledTariff;
    chargingScheme = scheduledTariff.tariffTypeChargingScheme;
  } else {
    boolean found = emberAfPriceGetTariffByIssuerTariffId(endpoint, pm.issuerTariffId, &inf, &t);
    if (!found) {
      emberAfPriceClusterPrint("Invalid issuer tariff ID in price matrix; no corresponding tariff found.\n"); 
      return;
    } else {
      chargingScheme = t.tariffTypeChargingScheme;
    }
  }

  if (tier >= t.numberOfPriceTiersInUse) {
    emberAfPriceClusterPrint("Tier exceeds number of price tiers in use for this tariff. Values not set.\n");
    return;
  }

  switch (chargingScheme >> 4) {
    case 0: // TOU only
      pm.matrix.tier[tier] = price;
      break;
    case 1: // Block only
      pm.matrix.blockAndTier[tier][0] = price;
      break;
    case 2:
    case 3: // TOU and Block
      for (i = 0; i < t.numberOfBlockThresholdsInUse + 1; i++) {
        pm.matrix.blockAndTier[tier][i] = price;
      }
      break;
    default:
      emberAfDebugPrintln("Invalid tariff type / charging scheme.");
      break;
  }
}

// plugin price-server pm fill-tier <endpoint:1> <block:1> <price:4>
void emAfPriceServerCliPmFillBlock(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u block = (int8u)emberUnsignedCommandArgument(1);
  int32u price = (int32u)emberUnsignedCommandArgument(2);
  int8u chargingScheme, i;
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo inf;

  if (pm.issuerTariffId == scheduledTariff.issuerTariffId) {
    t = scheduledTariff;
    chargingScheme = scheduledTariff.tariffTypeChargingScheme;
  } else {
    boolean found = emberAfPriceGetTariffByIssuerTariffId(endpoint, pm.issuerTariffId, &inf, &t);
    if (!found) {
      emberAfPriceClusterPrint("Invalid issuer tariff ID in price matrix; no corresponding tariff found.\n"); 
      return;
    } else {
      chargingScheme = t.tariffTypeChargingScheme;
    }
  }

  if ( block > t.numberOfBlockThresholdsInUse) {
    emberAfPriceClusterPrint("Block exceeds number of block thresholds in use for this tariff. Values not set.\n");
    return;
  }

  switch (chargingScheme >> 4) {
    case 0: // TOU only
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        pm.matrix.tier[i] = price;
      }
      break;
    case 1: // Block only
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        pm.matrix.blockAndTier[i][0] = price;
      }
      break;
    case 2:
    case 3: // TOU and Block
      for (i = 0; i < t.numberOfPriceTiersInUse; i++) {
        pm.matrix.blockAndTier[i][block] = price;
      }
      break;
    default:
      emberAfDebugPrintln("Invalid tariff type / charging scheme.");
      break;
  }
}

// plugin price-server btprint <endpoint:1>
void emAfPriceServerCliBtPrint(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emberAfPricePrintBlockThresholdsTable(endpoint);
}

// plugin price-server bt set-metadata <endpoint:1> <providerId:4> <issuerEventId:4>
//                                     <issuerTariffId:4> <startTime:4> <status:1>
void emAfPriceServerCliBtSetMetadata(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int32u tariffId = (int32u)emberUnsignedCommandArgument(3);
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo i;

  if (!(emberAfPriceGetTariffByIssuerTariffId(endpoint, tariffId, &i, &t)
        || scheduledTariff.issuerTariffId == tariffId)) {
    emberAfPriceClusterPrint("Invalid issuer tariff ID; no corresponding tariff found.\n");
    return;
  }

  btInfo.issuerEventId = (int32u)emberUnsignedCommandArgument(2);
  btInfo.startTime = (int32u)emberUnsignedCommandArgument(4);
  bt.issuerTariffId = tariffId;
  bt.providerId = (int32u)emberUnsignedCommandArgument(1);
  bt.issuerTariffId = tariffId;
  bt.status = (int8u)emberUnsignedCommandArgument(5);
}

// plugin price-server bt set-provider <providerId:4>
void emAfPriceServerCliBtSetProvider(void)
{
  bt.providerId = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server bt set-event <issuerEventId:4>
void emAfPriceServerCliBtSetEvent(void)
{
  btInfo.issuerEventId = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server bt set-tariff <issuerTariffId:4>
void emAfPriceServerCliBtSetTariff(void)
{
  bt.issuerTariffId = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server bt set-time <startTime:4>
void emAfPriceServerCliBtSetTime(void)
{
  btInfo.startTime = (int32u)emberUnsignedCommandArgument(0);
}

// plugin price-server bt set-status <status:1>
void emAfPriceServerCliBtSetStatus(void)
{
  bt.status = (int8u)emberUnsignedCommandArgument(0);
}

// plugin price-server bt get <endpoint:1> <index:1>
void emAfPriceServerCliBtGet(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u index = (int8u)emberUnsignedCommandArgument(1);

  emberAfPriceGetBlockThresholdsTableEntry(endpoint, index, &bt);
}

// plugin price-server bt set <endpoint:1> <index:1>
void emAfPriceServerCliBtSet(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u index = (int8u)emberUnsignedCommandArgument(1);
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo i;

  if (!(emberAfPriceGetTariffByIssuerTariffId(endpoint, bt.issuerTariffId, &i, &t)
        || scheduledTariff.issuerTariffId == bt.issuerTariffId)) {
    emberAfPriceClusterPrint("Invalid issuer tariff ID; no corresponding tariff found.\n");
    return;
  }

  emberAfPriceSetBlockThresholdsTableEntry(endpoint, index, &btInfo, &bt);
}

// plugin price-server bt put <endpoint:1> <tier:1> <block:1> <threshold:6>
void emAfPriceServerCliBtPut(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u tier = (int8u)emberUnsignedCommandArgument(1);
  int8u block = (int8u)emberUnsignedCommandArgument(2);
  emAfPriceBlockThreshold threshold;
  int8u tierBlockMode;
  EmberAfScheduledTariff t;
  EmberAfPriceCommonInfo i;
  int8u *dst = NULL;
  emberCopyStringArgument(3, (int8u *)&threshold, sizeof(emAfPriceBlockThreshold), TRUE);

  if (bt.issuerTariffId == scheduledTariff.issuerTariffId) {
    t = scheduledTariff;
    tierBlockMode = scheduledTariff.tierBlockMode;
  } else {
    boolean found = emberAfPriceGetTariffByIssuerTariffId(endpoint, bt.issuerTariffId, &i, &t);
    if (!found) {
      emberAfPriceClusterPrintln("Invalid issuer tariff ID in block thresholds; no corresponding tariff found.");
      return;
    } else {
      tierBlockMode = t.tierBlockMode;
    }
  }

  if (tier >= t.numberOfPriceTiersInUse
      || block > t.numberOfBlockThresholdsInUse) {
    emberAfPriceClusterPrintln("Invalid index into block thresholds. Value not set.");
    return;
  }

  switch (tierBlockMode) {
    case 0: // ActiveBlock
    case 1: // ActiveBlockPriceTier
      dst = (int8u *)&bt.thresholds.block[block];
      break;
    case 2: // ActiveBlockPriceTierThreshold
      dst = (int8u *)&bt.thresholds.blockAndTier[tier][block];
      break;
    default:
      emberAfDebugPrintln("Invalid tier block mode.");
      break;
  }

  if (dst != NULL) {
#if BIGENDIAN_CPU
    MEMCOPY(dst, &threshold, sizeof(emAfPriceBlockThreshold));
#else
    int8u i;
    for (i=0; i<sizeof(emAfPriceBlockThreshold); i++) {
      dst[i] = threshold[sizeof(emAfPriceBlockThreshold) - 1 - i];
    }
#endif
  }
}

#else // !defined(EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_MATRIX_SUPPORT)

#endif // defined(EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_MATRIX_SUPPORT)
