// *******************************************************************
// * price-client.c
// *
// * The Price client plugin is responsible for keeping track of the current
// * and future prices.
// *
// * Copyright 2010 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/price-common/price-common.h"
#include "app/framework/plugin/price-common/price-common-time.h"
#include "price-client.h"

#define fieldLength(field) \
  (emberAfCurrentCommand()->bufLen - (field - emberAfCurrentCommand()->buffer))
#define VALID  BIT(1)

// keep track of last seen issuerEventId to discard old commands.
static int32u lastSeenIssuerEventId = 0x00;

// Used by the CLI and CppEventCallback to determine the default policy for non-forced CPP Events.
int8u emberAfPriceClusterDefaultCppEventAuthorization = EMBER_AF_PLUGIN_PRICE_CPP_AUTH_ACCEPTED;

static void initPrice(EmberAfPluginPriceClientPrice *price);
static void printPrice(EmberAfPluginPriceClientPrice *price);
static void scheduleTick(int8u endpoint);
static void emberAfPriceInitBlockPeriod( int8u endpoint );
static void emberAfPriceInitBlockThresholdsTable( int8u endpoint );
static void emberAfPriceInitConversionFactorTable( int8u endpoint );
static void emberAfPriceInitCalorificValueTable( int8u endpoint );
static void emberAfPriceClusterInitCpp( int8u endpoint );
static void emberAfPriceClusterInitCO2Table( int8u endpoint );
static void emAfPriceClientTierLabelsInit( int8u endpoint );
static void emberAfPriceClusterInitCreditPaymentTable( int8u endpoint );
static void emberAfPriceClusterInitCurrencyConversionTable( int8u endpoint );
static void emberAfPriceInitBillingPeriodTable( int8u endpoint );

static int8u emAfPriceCommonGetMatchingOrUnusedIndex( EmberAfPriceClientCommonInfo *pcommon, int8u numElements,
                                                         int32u newProviderId, int32u newIssuerEventId );
static int8u emAfPriceCommonGetMatchingIndex( EmberAfPriceClientCommonInfo *pcommon, int8u numElements, int32u issuerEventId );
static int8u emAfPriceCommonGetActiveIndex( EmberAfPriceClientCommonInfo *pcommon, int8u numElements );


static EmberAfPluginPriceClientPrice priceTable[EMBER_AF_PRICE_CLUSTER_CLIENT_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE];

void emAfPriceClearPriceTable(int8u endpoint)
{
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, 
                                                   ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    assert(FALSE);
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE; i++) {
    initPrice(&priceTable[ep][i]);
  }
}


void emberAfPriceClusterClientInitCallback(int8u endpoint)
{
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint,
                                                   ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE; i++) {
    initPrice(&priceTable[ep][i]);
  }
  emberAfPriceInitConsolidatedBillsTable( endpoint );
  emAfPriceClientTierLabelsInit( endpoint );
  emberAfPriceInitBlockPeriod( endpoint );
  emberAfPriceInitBlockThresholdsTable( endpoint );
  emberAfPriceClusterInitCreditPaymentTable( endpoint );
  emberAfPriceClusterInitCurrencyConversionTable( endpoint );
  emberAfPriceClusterInitCpp( endpoint );
  emberAfPriceClusterInitCO2Table( endpoint );
  emberAfPriceInitConversionFactorTable( endpoint );
  emberAfPriceInitCalorificValueTable( endpoint );
  emberAfPriceInitBillingPeriodTable( endpoint );
}

void emberAfPriceClusterClientTickCallback(int8u endpoint)
{
  scheduleTick(endpoint);
}

boolean emberAfPriceClusterPublishPriceCallback(int32u providerId,
                                                int8u* rateLabel,
                                                int32u issuerEventId,
                                                int32u currentTime,
                                                int8u unitOfMeasure,
                                                int16u currency,
                                                int8u priceTrailingDigitAndPriceTier,
                                                int8u numberOfPriceTiersAndRegisterTier,
                                                int32u startTime,
                                                int16u durationInMinutes,
                                                int32u prc,
                                                int8u priceRatio,
                                                int32u generationPrice,
                                                int8u generationPriceRatio,
                                                int32u alternateCostDelivered,
                                                int8u alternateCostUnit,
                                                int8u alternateCostTrailingDigit,
                                                int8u numberOfBlockThresholds,
                                                int8u priceControl,
                                                int8u numberOfGenerationTiers,
                                                int8u generationTier,
                                                int8u extendedNumberOfPriceTiers,
                                                int8u extendedPriceTier,
                                                int8u extendedRegisterTier)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint,
                                                   ZCL_PRICE_CLUSTER_ID);
  EmberAfPluginPriceClientPrice *price = NULL, *last;
  EmberAfStatus status;
  int32u endTime, now = emberAfGetCurrentTime();
  int8u i;

  if (ep == 0xFF) {
    return FALSE;
  }

  last = &priceTable[ep][0];

  emberAfPriceClusterPrint("RX: PublishPrice 0x%4x, \"", providerId);
  emberAfPriceClusterPrintString(rateLabel);
  emberAfPriceClusterPrint("\"");
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrint(", 0x%4x, 0x%4x, 0x%x, 0x%2x, 0x%x, 0x%x, 0x%4x",
                           issuerEventId,
                           currentTime,
                           unitOfMeasure,
                           currency,
                           priceTrailingDigitAndPriceTier,
                           numberOfPriceTiersAndRegisterTier,
                           startTime);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrint(", 0x%2x, 0x%4x, 0x%x, 0x%4x, 0x%x, 0x%4x, 0x%x",
                           durationInMinutes,
                           prc,
                           priceRatio,
                           generationPrice,
                           generationPriceRatio,
                           alternateCostDelivered,
                           alternateCostUnit);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln(", 0x%x, 0x%x, 0x%x",
                             alternateCostTrailingDigit,
                             numberOfBlockThresholds,
                             priceControl);
  emberAfPriceClusterFlush();

  if (startTime == ZCL_PRICE_CLUSTER_START_TIME_NOW) {
    startTime = now;
  }
  endTime = (durationInMinutes == ZCL_PRICE_CLUSTER_DURATION_UNTIL_CHANGED
             ? ZCL_PRICE_CLUSTER_END_TIME_NEVER
             : startTime + durationInMinutes * 60);

  // If the price has already expired, don't bother with it.
  if (endTime <= now) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto kickout;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE; i++) {
    // Ignore invalid prices, but remember the empty slot for later.
    if (!priceTable[ep][i].valid) {
      if (price == NULL) {
        price = &priceTable[ep][i];
      }
      continue;
    }

    // Reject duplicate prices based on the issuer event id.  This assumes that
    // issuer event ids are unique and that pricing information associated with
    // an issuer event id never changes.
    if (priceTable[ep][i].issuerEventId == issuerEventId) {
      status = EMBER_ZCL_STATUS_DUPLICATE_EXISTS;
      goto kickout;
    }

    // Nested and overlapping prices are not allowed.  Prices with the newer
    // issuer event ids takes priority over all nested and overlapping prices.
    // The only exception is when a price with a newer issuer event id overlaps
    // with the end of the current active price.  In this case, the duration of
    // the current active price is changed to "until changed" and it will expire
    // when the new price starts.
    if (priceTable[ep][i].startTime < endTime
        && priceTable[ep][i].endTime > startTime) {
      if (priceTable[ep][i].issuerEventId < issuerEventId) {
        if (priceTable[ep][i].active && now < startTime) {
          priceTable[ep][i].endTime = startTime;
          priceTable[ep][i].durationInMinutes = ZCL_PRICE_CLUSTER_DURATION_UNTIL_CHANGED;
        } else {
          if (priceTable[ep][i].active) {
            priceTable[ep][i].active = FALSE;
            emberAfPluginPriceClientPriceExpiredCallback(&priceTable[ep][i]);
          }
          initPrice(&priceTable[ep][i]);
        }
      } else {
        status = EMBER_ZCL_STATUS_FAILURE;
        goto kickout;
      }
    }

    // Along the way, search for an empty slot for this new price and find the
    // price in the table with the latest start time.  If there are no empty
    // slots, we will either have to drop this price or the last one, depending
    // on the start times.
    if (price == NULL) {
      if (!priceTable[ep][i].valid) {
        price = &priceTable[ep][i];
      } else if (last->startTime < priceTable[ep][i].startTime) {
        last = &priceTable[ep][i];
      }
    }
  }

  // If there were no empty slots and this price starts after all of the other
  // prices in the table, drop this price.  Otherwise, drop the price with the
  // latest start time and replace it with this one.
  if (price == NULL) {
    if (last->startTime < startTime) {
      status = EMBER_ZCL_STATUS_INSUFFICIENT_SPACE;
      goto kickout;
    } else {
      price = last;
    }
  }

  price->valid                             = TRUE;
  price->active                            = FALSE;
  price->clientEndpoint                    = endpoint;
  price->providerId                        = providerId;
  emberAfCopyString(price->rateLabel, rateLabel, ZCL_PRICE_CLUSTER_MAXIMUM_RATE_LABEL_LENGTH);
  price->issuerEventId                     = issuerEventId;
  price->currentTime                       = currentTime;
  price->unitOfMeasure                     = unitOfMeasure;
  price->currency                          = currency;
  price->priceTrailingDigitAndPriceTier    = priceTrailingDigitAndPriceTier;
  price->numberOfPriceTiersAndRegisterTier = numberOfPriceTiersAndRegisterTier;
  price->startTime                         = startTime;
  price->endTime                           = endTime;
  price->durationInMinutes                 = durationInMinutes;
  price->price                             = prc;
  price->priceRatio                        = priceRatio;
  price->generationPrice                   = generationPrice;
  price->generationPriceRatio              = generationPriceRatio;
  price->alternateCostDelivered            = alternateCostDelivered;
  price->alternateCostUnit                 = alternateCostUnit;
  price->alternateCostTrailingDigit        = alternateCostTrailingDigit;
  price->numberOfBlockThresholds           = numberOfBlockThresholds;
  price->priceControl                      = priceControl;

  // Now that we have saved the price in our table, we may have to reschedule
  // our tick to activate or expire prices.
  scheduleTick(endpoint);

  // If the acknowledgement is required, send it immediately.  Otherwise, a
  // default response is sufficient.
  if (priceControl & ZCL_PRICE_CLUSTER_PRICE_ACKNOWLEDGEMENT_MASK) {
    emberAfFillCommandPriceClusterPriceAcknowledgement(providerId,
                                                       issuerEventId,
                                                       now,
                                                       priceControl);
    emberAfSendResponse();
    return TRUE;
  } else {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }

kickout:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}

void emAfPluginPriceClientPrintInfo(int8u endpoint)
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  int8u i;                                                                                                              
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);                                     

  if (ep == 0xFF) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE; i++) {                                                       
    emberAfPriceClusterFlush();                                                                                         
    emberAfPriceClusterPrintln("= Price %d =", i);                                                                      
    printPrice(&priceTable[ep][i]);                                                                
    emberAfPriceClusterFlush();                                                                                         
  }  
#endif //defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emAfPluginPriceClientPrintByEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE; i++ ){
    if( priceTable[ep][i].issuerEventId == issuerEventId ){
      emberAfPriceClusterPrintln("Matching Price [%d]", i);
      printPrice( &priceTable[ep][i] );
      break;
    }
  }
  if( i >= EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Error: Event ID %d not in price table", issuerEventId);
  }
}


static void initPrice(EmberAfPluginPriceClientPrice *price)
{
  price->valid                             = FALSE;
  price->active                            = FALSE;
  price->clientEndpoint                    = 0xFF;
  price->providerId                        = 0x00000000UL;
  price->rateLabel[0]                      = 0;
  price->issuerEventId                     = 0x00000000UL;
  price->currentTime                       = 0x00000000UL;
  price->unitOfMeasure                     = 0x00;
  price->currency                          = 0x0000;
  price->priceTrailingDigitAndPriceTier    = 0x00;
  price->numberOfPriceTiersAndRegisterTier = 0x00;
  price->startTime                         = 0x00000000UL;
  price->endTime                           = 0x00000000UL;
  price->durationInMinutes                 = 0x0000;
  price->price                             = 0x00000000UL;
  price->priceRatio                        = ZCL_PRICE_CLUSTER_PRICE_RATIO_NOT_USED;
  price->generationPrice                   = ZCL_PRICE_CLUSTER_GENERATION_PRICE_NOT_USED;
  price->generationPriceRatio              = ZCL_PRICE_CLUSTER_GENERATION_PRICE_RATIO_NOT_USED;
  price->alternateCostDelivered            = ZCL_PRICE_CLUSTER_ALTERNATE_COST_DELIVERED_NOT_USED;
  price->alternateCostUnit                 = ZCL_PRICE_CLUSTER_ALTERNATE_COST_UNIT_NOT_USED;
  price->alternateCostTrailingDigit        = ZCL_PRICE_CLUSTER_ALTERNATE_COST_TRAILING_DIGIT_NOT_USED;
  price->numberOfBlockThresholds           = ZCL_PRICE_CLUSTER_NUMBER_OF_BLOCK_THRESHOLDS_NOT_USED;
  price->priceControl                      = ZCL_PRICE_CLUSTER_PRICE_CONTROL_NOT_USED;
}

static void printPrice(EmberAfPluginPriceClientPrice *price)
{
  emberAfPriceClusterPrintln("    vld: %p", (price->valid ? "YES" : "NO"));
  emberAfPriceClusterPrintln("    act: %p", (price->active ? "YES" : "NO"));
  emberAfPriceClusterPrintln("    pid: 0x%4x", price->providerId);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrint(  "     rl: \"");
  emberAfPriceClusterPrintString(price->rateLabel);
  emberAfPriceClusterPrintln("\"");
  emberAfPriceClusterPrintln("   ieid: 0x%4x", price->issuerEventId);
  emberAfPriceClusterPrintln("     ct: 0x%4x", price->currentTime);
  emberAfPriceClusterPrintln("    uom: 0x%x",  price->unitOfMeasure);
  emberAfPriceClusterPrintln("      c: 0x%2x", price->currency);
  emberAfPriceClusterPrintln(" ptdapt: 0x%x",  price->priceTrailingDigitAndPriceTier);
  emberAfPriceClusterPrintln("noptart: 0x%x",  price->numberOfPriceTiersAndRegisterTier);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("     st: 0x%4x", price->startTime);
  emberAfPriceClusterPrintln("     et: 0x%4x", price->endTime);
  emberAfPriceClusterPrintln("    dim: 0x%2x", price->durationInMinutes);
  emberAfPriceClusterPrintln("      p: 0x%4x", price->price);
  emberAfPriceClusterPrintln("     pr: 0x%x",  price->priceRatio);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("     gp: 0x%4x", price->generationPrice);
  emberAfPriceClusterPrintln("    gpr: 0x%x",  price->generationPriceRatio);
  emberAfPriceClusterPrintln("    acd: 0x%4x", price->alternateCostDelivered);
  emberAfPriceClusterPrintln("    acu: 0x%x",  price->alternateCostUnit);
  emberAfPriceClusterPrintln("   actd: 0x%x",  price->alternateCostTrailingDigit);
  emberAfPriceClusterPrintln("   nobt: 0x%x",  price->numberOfBlockThresholds);
  emberAfPriceClusterPrintln("     pc: 0x%x",  price->priceControl);
}

static void scheduleTick(int8u endpoint)
{
  int32u next = ZCL_PRICE_CLUSTER_END_TIME_NEVER;
  int32u now = emberAfGetCurrentTime();
  boolean active = FALSE;
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint,
                                                   ZCL_PRICE_CLUSTER_ID);

  if (ep == 0xFF) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_CLIENT_TABLE_SIZE; i++) {
    if (!priceTable[ep][i].valid) {
      continue;
    }

    // Remove old prices from the table.  This may result in the active price
    // being expired, which requires notifying the application.
    if (priceTable[ep][i].endTime <= now) {
      if (priceTable[ep][i].active) {
        priceTable[ep][i].active = FALSE;
        emberAfPluginPriceClientPriceExpiredCallback(&priceTable[ep][i]);
      }
      initPrice(&priceTable[ep][i]);
      continue;
    }

    // If we don't have a price that should be active right now, we will need to
    // schedule the tick to wake us up when the next active price should start,
    // so keep track of the price with the start time soonest after the current
    // time.
    if (!active && priceTable[ep][i].startTime < next) {
      next = priceTable[ep][i].startTime;
    }

    // If we have a price that should be active now, a tick is scheduled for the
    // time remaining in the duration to wake us up and expire the price.  If
    // the price is transitioning from inactive to active for the first time, we
    // also need to notify the application the application.
    if (priceTable[ep][i].startTime <= now) {
      if (!priceTable[ep][i].active) {
        priceTable[ep][i].active = TRUE;
        emberAfPluginPriceClientPriceStartedCallback(&priceTable[ep][i]);
      }
      active = TRUE;
      next = priceTable[ep][i].endTime;
    }
  }

  // We need to wake up again to activate a new price or expire the current
  // price.  Otherwise, we don't have to do anything until we receive a new
  // price from the server.
  if (next != ZCL_PRICE_CLUSTER_END_TIME_NEVER) {
    emberAfScheduleClientTick(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              (next - now) * MILLISECOND_TICKS_PER_SECOND);
  } else {
    emberAfDeactivateClientTick(endpoint, ZCL_PRICE_CLUSTER_ID);
  }
}

static EmberAfPriceClientInfo info;

// Get the index of the EmberAfPriceClientCommonInfo structure whose event ID matches.
static int8u emAfPriceCommonGetMatchingIndex( EmberAfPriceClientCommonInfo *pcommon, int8u numElements, int32u issuerEventId ){
  int8u i;
  for( i=0; i<numElements; i++ ){
    if( pcommon[i].valid && (pcommon[i].issuerEventId == issuerEventId) ){
      break;
    }
  }
  return i;
}

static int8u emAfPriceCommonGetActiveIndex( EmberAfPriceClientCommonInfo *pcommon, int8u numElements ){
  int8u i;
  int32u largestEventId = 0;
  int8u  largestEventIdIndex = 0xFF;
  int32u timeNow = emberAfGetCurrentTime();

  for( i=0; i<numElements; i++ ){
    if( pcommon[i].valid &&
        ((pcommon[i].startTime <= timeNow) ||
         (pcommon[i].startTime == 0)) &&
         (pcommon[i].issuerEventId > largestEventId) ){
        //(PriceClientCo2Table[i].startTime > nearestTime) ){
      // Found entry that is closer to timeNow
      //nearestTime = PriceClientCo2Table[i].startTime;
      largestEventId = pcommon[i].issuerEventId;
      largestEventIdIndex = i;
    }
  }
  return largestEventIdIndex;
}

// Parses the EmberAfPriceClientCommonInfo structure to find a matching, unused, or oldest (smallest eventId)
// element that can be overwritten with new data.
static int8u emAfPriceCommonGetMatchingOrUnusedIndex( EmberAfPriceClientCommonInfo *pcommon, int8u numElements,
                                                         int32u newProviderId, int32u newIssuerEventId ){
  int32u smallestEventId = 0xFFFFFFFF;
  int8u  smallestEventIdIndex;
  int8u  i;

  for( i=0; i<numElements; i++ ){
    if( !pcommon[i].valid ){
      // Use the first invalid entry unless a matching entry is found.
      if( smallestEventId > 0 ){
        smallestEventId = 0;
        smallestEventIdIndex = i;
      }
    }
    else{
      if( (pcommon[i].providerId == newProviderId) && (pcommon[i].issuerEventId == newIssuerEventId) ){
        // Match found
        smallestEventIdIndex = i;
        smallestEventId = pcommon[i].issuerEventId;
        break;
      }
      else if( pcommon[i].issuerEventId < smallestEventId ){
        smallestEventId = pcommon[i].issuerEventId;
        smallestEventIdIndex = i;
      }
    }
  }
  // Do quick sanity check here to validate the index.
  // Expect the indexed entry to either:
  //  1. Be invalid, or
  //  2. Have an issuerEventId <= newIssuerEventId
  if( (smallestEventIdIndex < numElements) &&
      pcommon[smallestEventIdIndex].valid && 
      (smallestEventId > newIssuerEventId) ){
    // FAIL above conditions - return invalid index
    smallestEventIdIndex = 0xFF;
  }
  return smallestEventIdIndex;
}

static void emberAfPriceInitBlockPeriod( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_PERIOD_TABLE_SIZE; i++ ){
    info.blockPeriodTable.commonInfos[ep][i].valid = FALSE;
  }
}


boolean emberAfPriceClusterPublishBlockPeriodCallback(int32u providerId,
                                                      int32u issuerEventId,
                                                      int32u blockPeriodStartTime,
                                                      int32u blockPeriodDuration, // int24u
                                                      int8u  blockPeriodControl,
                                                      int8u  blockPeriodDurationType,
                                                      int8u  tariffType,
                                                      int8u  tariffResolutionPeriod){
 
  int8u i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }
   
  emberAfPriceClusterPrintln("RX: PublishBlockPeriod, 0x%4x, 0x%4x, 0x%4x, 0x%4X, 0x%X, 0x%X, 0x%X, 0x%X",
                             providerId,
                             issuerEventId,
                             blockPeriodStartTime,
                             blockPeriodDuration, // int24u
                             blockPeriodControl,
                             blockPeriodDurationType,
                             tariffType,
                             tariffResolutionPeriod);

  if( blockPeriodStartTime == 0 ){
    blockPeriodStartTime = emberAfGetCurrentTime();
  }

  // Find the entry to update
  // We will update any invalid entry, or the oldest event ID.
  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.blockPeriodTable.commonInfos[ep], 
                                                  EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_PERIOD_TABLE_SIZE,
                                                  providerId, issuerEventId );
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_PERIOD_TABLE_SIZE ){
    info.blockPeriodTable.commonInfos[ep][i].valid = TRUE;
    info.blockPeriodTable.commonInfos[ep][i].providerId = providerId;
    info.blockPeriodTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.blockPeriodTable.commonInfos[ep][i].startTime = emberAfPluginPriceCommonClusterGetAdjustedStartTime( blockPeriodStartTime,
                                                                                              blockPeriodDurationType );
    info.blockPeriodTable.commonInfos[ep][i].durationSec = 
                    emberAfPluginPriceCommonClusterConvertDurationToSeconds( blockPeriodStartTime,
                                                                 blockPeriodDuration,
                                                                 blockPeriodDurationType );
    info.blockPeriodTable.blockPeriod[ep][i].blockPeriodStartTime = blockPeriodStartTime;
    info.blockPeriodTable.blockPeriod[ep][i].blockPeriodDuration = blockPeriodDuration;
    info.blockPeriodTable.blockPeriod[ep][i].blockPeriodControl = blockPeriodControl;
    info.blockPeriodTable.blockPeriod[ep][i].blockPeriodDurationType = blockPeriodDurationType;
    info.blockPeriodTable.blockPeriod[ep][i].tariffType = tariffType;
    info.blockPeriodTable.blockPeriod[ep][i].tariffResolutionPeriod = tariffResolutionPeriod;
  }

  return TRUE;
}

int8u emAfPriceGetBlockPeriodTableIndexByEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_PERIOD_TABLE_SIZE; i++ ){
    if( info.blockPeriodTable.commonInfos[ep][i].valid &&
        (info.blockPeriodTable.commonInfos[ep][i].issuerEventId == issuerEventId) ){
      break;
    }
  }
  return i;
}


void emAfPricePrintBlockPeriodTableIndex( int8u endpoint, int8u index ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( index < EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_PERIOD_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Print Block Period [%d]", index );
    emberAfPriceClusterPrintln("  valid=%d", info.blockPeriodTable.commonInfos[ep][index].valid );
    emberAfPriceClusterPrintln("  providerId=%d", info.blockPeriodTable.commonInfos[ep][index].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.blockPeriodTable.commonInfos[ep][index].issuerEventId );
    emberAfPriceClusterPrintln("  startTime=0x%4x", info.blockPeriodTable.commonInfos[ep][index].startTime );
    emberAfPriceClusterPrintln("  duration=%d", info.blockPeriodTable.commonInfos[ep][index].durationSec );
    emberAfPriceClusterPrintln("  rawStartTime=0x%4x", info.blockPeriodTable.blockPeriod[ep][index].blockPeriodStartTime );
    emberAfPriceClusterPrintln("  rawDuration=%d", info.blockPeriodTable.blockPeriod[ep][index].blockPeriodDuration );
    emberAfPriceClusterPrintln("  durationType=%d", info.blockPeriodTable.blockPeriod[ep][index].blockPeriodDurationType );
    emberAfPriceClusterPrintln("  blockPeriodControl=%d", info.blockPeriodTable.blockPeriod[ep][index].blockPeriodControl );
    emberAfPriceClusterPrintln("  tariffType=%d", info.blockPeriodTable.blockPeriod[ep][index].tariffType );
    emberAfPriceClusterPrintln("  tariffResolutionPeriod=%d", info.blockPeriodTable.blockPeriod[ep][index].tariffResolutionPeriod );
  }
  else{
    emberAfPriceClusterPrintln("Error: Block Period NOT FOUND");
  }
}

// ==  BLOCK THRESHOLDS COMMAND  ==

static void emberAfPriceInitBlockThresholdsTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_THRESHOLD_TABLE_SIZE; i++ ){
    info.blockThresholdTable.commonInfos[ep][i].valid = FALSE;
  }
}

#define BLOCK_THRESHOLD_SUB_PAYLOAD_ALL_TOU_TIERS  (1 << 0)
//#define ALL_TIERS  0xFF

boolean emberAfPriceClusterPublishBlockThresholdsCallback(int32u providerId,
                                                          int32u issuerEventId,
                                                          int32u startTime,
                                                          int32u issuerTariffId,
                                                          int8u commandIndex,
                                                          int8u numberOfCommands,
                                                          int8u subPayloadControl,
                                                          int8u* payload){

  int8u tierNumber;
  int8u numThresholds;
  int8u i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }
  emberAfPriceClusterPrintln("RX: PublishBlockThresholds 0x%4x, 0x%4x, 0x%4x, 0x%4x, 0x%x, 0x%x, 0x%x",
                           providerId, 
                           issuerEventId,
                           startTime, 
                           issuerTariffId,
                           commandIndex,
                           numberOfCommands,
                           subPayloadControl);

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.blockThresholdTable.commonInfos[ep],
                                               numberOfCommands, providerId, issuerEventId );


  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_BLOCK_THRESHOLD_TABLE_SIZE ){
    // Only update the entry if the new eventID is greater than this one.
    tierNumber = payload[0] >> 4;
    numThresholds = payload[0] & 0x0F;
    if( (numThresholds > EMBER_AF_PLUGIN_PRICE_CLIENT_MAX_NUMBER_BLOCK_THRESHOLDS) ||
        (tierNumber > EMBER_AF_PLUGIN_PRICE_CLIENT_MAX_NUMBER_TIERS) ){
      // Out of range
      goto kickout;
    }
    info.blockThresholdTable.commonInfos[ep][i].valid = TRUE;
    info.blockThresholdTable.commonInfos[ep][i].providerId = providerId;
    info.blockThresholdTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.blockThresholdTable.commonInfos[ep][i].startTime = startTime;
    info.blockThresholdTable.commonInfos[ep][i].durationSec = UNSPECIFIED_DURATION;
    info.blockThresholdTable.blockThreshold[ep][i].issuerTariffId = issuerTariffId;
    info.blockThresholdTable.blockThreshold[ep][i].subPayloadControl = subPayloadControl;
    if( subPayloadControl & BLOCK_THRESHOLD_SUB_PAYLOAD_ALL_TOU_TIERS ){
      info.blockThresholdTable.blockThreshold[ep][i].tierNumberOfBlockThresholds[0] = payload[0];
      MEMCOPY( info.blockThresholdTable.blockThreshold[ep][i].blockThreshold[0], &payload[1], (numThresholds*6) );
    }
    else{
      // TODO:   Note that multiple tier/NumberOfBlockThresholds fields could be present in this case.
      // eg. The payload could specify 5 blocks for tier 1, 2 blocks for tier 2, etc.
      // However, we don't know how many bytes are in "payload", so we can't read them all.  =(
      // For now, read the first tiers worth of data.
      //x = 0;
      info.blockThresholdTable.blockThreshold[ep][i].tierNumberOfBlockThresholds[tierNumber] = payload[0];
      MEMCOPY( info.blockThresholdTable.blockThreshold[ep][i].blockThreshold[tierNumber], &payload[1], (numThresholds*6) );
    }
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
kickout:
  return TRUE;
}

// == CONVERSION FACTOR COMMAND  ==

static void emberAfPriceInitConversionFactorTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CONVERSION_FACTOR_TABLE_SIZE; i++ ){
    info.conversionFactorTable.commonInfos[ep][i].providerId = UNSPECIFIED_PROVIDER_ID;
    info.conversionFactorTable.commonInfos[ep][i].valid = FALSE;
  }
}


boolean emberAfPriceClusterPublishConversionFactorCallback(int32u issuerEventId,
                                                           int32u startTime,
                                                           int32u conversionFactor,
                                                           int8u conversionFactorTrailingDigit){

  int8u  i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  emberAfPriceClusterPrintln("RX: PublishConversionFactor 0x%4X, 0x%4X, 0x%4X, 0x%X",
                             issuerEventId,
                             startTime,
                             conversionFactor,
                             conversionFactorTrailingDigit);

  if (ep == 0xFF) {
    return FALSE;
  }

  if( startTime == 0 ){
    startTime = emberAfGetCurrentTime();
  }

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.conversionFactorTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_CLIENT_CONVERSION_FACTOR_TABLE_SIZE,
                                                  UNSPECIFIED_PROVIDER_ID,
                                                  issuerEventId );
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CONVERSION_FACTOR_TABLE_SIZE ){
    // Only update the entry if the new eventID is greater than this one.
    info.conversionFactorTable.commonInfos[ep][i].valid = TRUE;
    info.conversionFactorTable.commonInfos[ep][i].providerId = UNSPECIFIED_PROVIDER_ID;
    info.conversionFactorTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.conversionFactorTable.commonInfos[ep][i].startTime = startTime;
    info.conversionFactorTable.commonInfos[ep][i].durationSec = UNSPECIFIED_DURATION;
    info.conversionFactorTable.conversionFactor[ep][i].conversionFactor = conversionFactor;
    info.conversionFactorTable.conversionFactor[ep][i].conversionFactorTrailingDigit = conversionFactorTrailingDigit;
  }
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}


int8u emAfPriceGetConversionFactorIndexByEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }

  i = emAfPriceCommonGetMatchingIndex( info.conversionFactorTable.commonInfos[ep],
                                       EMBER_AF_PLUGIN_PRICE_CLIENT_CONVERSION_FACTOR_TABLE_SIZE,
                                       issuerEventId );
  return i;
}


void emAfPricePrintConversionFactorEntryIndex( int8u endpoint, int8u i ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CONVERSION_FACTOR_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Print Conversion Factor [%d]", i );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.conversionFactorTable.commonInfos[ep][i].issuerEventId );
    emberAfPriceClusterPrintln("  startTime=%d", info.conversionFactorTable.commonInfos[ep][i].startTime );
    emberAfPriceClusterPrintln("  conversionFactor=%d", info.conversionFactorTable.conversionFactor[ep][i].conversionFactor );
    emberAfPriceClusterPrintln("  conversionFactorTrailingDigit=%d", info.conversionFactorTable.conversionFactor[ep][i].conversionFactorTrailingDigit );
  }
  else{
    emberAfPriceClusterPrintln("Conversion Factor NOT FOUND");
  }
}


///  CALORIFIC VALUE

static void emberAfPriceInitCalorificValueTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CALORIFIC_VALUE_TABLE_SIZE; i++ ){
    info.calorificValueTable.commonInfos[ep][i].valid = FALSE;
    info.calorificValueTable.commonInfos[ep][i].providerId = UNSPECIFIED_PROVIDER_ID;
  }
}

boolean emberAfPriceClusterPublishCalorificValueCallback(int32u issuerEventId,
                                                         int32u startTime,
                                                         int32u calorificValue,
                                                         int8u calorificValueUnit,
                                                         int8u calorificValueTrailingDigit){
  int8u  i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  emberAfPriceClusterPrintln("RX: PublishCalorificValue 0x%4X, 0x%4X, 0x%4X, 0x%X, 0x%X",
                             issuerEventId,
                             startTime,
                             calorificValue,
                             calorificValueUnit,
                             calorificValueTrailingDigit);

  if (ep == 0xFF) {
    return FALSE;
  }

  if( startTime == 0 ){
    startTime = emberAfGetCurrentTime();
  }

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.calorificValueTable.commonInfos[ep], EMBER_AF_PLUGIN_PRICE_CLIENT_CALORIFIC_VALUE_TABLE_SIZE,
                                               UNSPECIFIED_PROVIDER_ID, issuerEventId );

  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CALORIFIC_VALUE_TABLE_SIZE ){
    // Only update the entry if the new eventID is greater than this one.
    info.calorificValueTable.commonInfos[ep][i].valid = TRUE;
    info.calorificValueTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.calorificValueTable.commonInfos[ep][i].startTime = startTime;
    info.calorificValueTable.calorificValue[ep][i].calorificValue = calorificValue;
    info.calorificValueTable.calorificValue[ep][i].calorificValueUnit = calorificValueUnit;
    info.calorificValueTable.calorificValue[ep][i].calorificValueTrailingDigit = calorificValueTrailingDigit;
  }
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}


int8u emAfPriceGetCalorificValueIndexByEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }
  i = emAfPriceCommonGetMatchingIndex( info.calorificValueTable.commonInfos[ep], 
                                       EMBER_AF_PLUGIN_PRICE_CLIENT_CALORIFIC_VALUE_TABLE_SIZE,
                                       issuerEventId );
  return i;
}


void emAfPricePrintCalorificValueEntryIndex( int8u endpoint, int8u i ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CALORIFIC_VALUE_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Print Calorific Value [%d]", i );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.calorificValueTable.commonInfos[ep][i].issuerEventId );
    emberAfPriceClusterPrintln("  startTime=%d", info.calorificValueTable.commonInfos[ep][i].startTime );
    emberAfPriceClusterPrintln("  calorificValue=%d", info.calorificValueTable.calorificValue[ep][i].calorificValue );
    emberAfPriceClusterPrintln("  calorificValueUnit=%d", info.calorificValueTable.calorificValue[ep][i].calorificValueUnit );
    emberAfPriceClusterPrintln("  calorificValueTrailingDigit=%d", info.calorificValueTable.calorificValue[ep][i].calorificValueTrailingDigit );
  }
  else{
    emberAfPriceClusterPrintln("Calorific Value NOT FOUND");
  }
}

/// TARIFF INFORMATION

boolean emberAfPriceClusterPublishTariffInformationCallback(int32u providerId,
                                                            int32u issuerEventId,
                                                            int32u issuerTariffId,
                                                            int32u startTime,
                                                            int8u tariffTypeChargingScheme,
                                                            int8u* tariffLabel,
                                                            int8u numberOfPriceTiersInUse,
                                                            int8u numberOfBlockThresholdsInUse,
                                                            int8u unitOfMeasure,
                                                            int16u currency,
                                                            int8u priceTrailingDigit,
                                                            int32u standingCharge,
                                                            int8u tierBlockMode,
                                                            int32u blockThresholdMultiplier,
                                                            int32u blockThresholdDivisor)
{
  emberAfPriceClusterPrint("RX: PublishTariffInformation 0x%4x, 0x%4x, 0x%4x, 0x%4x, 0x%x, \"",
                           providerId,
                           issuerEventId,
                           issuerTariffId,
                           startTime,
                           tariffTypeChargingScheme);

  emberAfPriceClusterPrintString(tariffLabel);
  emberAfPriceClusterPrint("\"");
  emberAfPriceClusterPrint(", 0x%x, 0x%x, 0x%x, 0x%2x, 0x%x",
                           numberOfPriceTiersInUse,
                           numberOfBlockThresholdsInUse,
                           unitOfMeasure,
                           currency,
                           priceTrailingDigit);
  emberAfPriceClusterPrintln(", 0x%4x, 0x%x, 0x%4x, 0x%4x",
                             standingCharge,
                             tierBlockMode,
                             blockThresholdMultiplier,
                             blockThresholdDivisor);
  emberAfPriceClusterFlush();

  if (lastSeenIssuerEventId >= issuerEventId){
    // reject old command.
    emberAfPriceClusterPrintln("Rejected command due to old issuer event id (0x%4X)!", 
                               issuerEventId);
  } else {
    // accept command.
    // optional attributes are not updated for now.
    lastSeenIssuerEventId = issuerEventId;
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}


/// CO2 VALUE

static void emberAfPriceClusterInitCO2Table( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CO2_TABLE_SIZE; i++ ){
    info.co2ValueTable.commonInfos[ep][i].valid = FALSE;
  }
}

#define CANCELLATION_START_TIME  0xFFFFFFFF

boolean emberAfPriceClusterPublishCO2ValueCallback(int32u providerId,
                                                   int32u issuerEventId,
                                                   int32u startTime,
                                                   int8u tariffType,
                                                   int32u cO2Value,
                                                   int8u cO2ValueUnit,
                                                   int8u cO2ValueTrailingDigit){

  int8u  i;
  int32u timeNow = emberAfGetCurrentTime();
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }

  emberAfPriceClusterPrintln("RX: Publish CO2 Value");

  if( startTime == 0 ){
    startTime = emberAfGetCurrentTime();
  }
  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.co2ValueTable.commonInfos[ep], EMBER_AF_PLUGIN_PRICE_CLIENT_CO2_TABLE_SIZE,
                                               providerId, issuerEventId );

  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CO2_TABLE_SIZE ){
    // Do NOT overwrite data if the entry is valid and the new
    // data has a smaller (or equal) event ID.
    info.co2ValueTable.commonInfos[ep][i].providerId = providerId;
    info.co2ValueTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.co2ValueTable.commonInfos[ep][i].startTime = startTime;
    info.co2ValueTable.co2Value[ep][i].tariffType = tariffType;
    info.co2ValueTable.commonInfos[ep][i].valid = TRUE;
    info.co2ValueTable.co2Value[ep][i].cO2Value = cO2Value;
    info.co2ValueTable.co2Value[ep][i].cO2ValueUnit = cO2ValueUnit;
    info.co2ValueTable.co2Value[ep][i].cO2ValueTrailingDigit = cO2ValueTrailingDigit;
  }
  return TRUE;
}

int8u emberAfPriceClusterGetActiveCo2ValueIndex( int8u endpoint ){
  int8u i;

  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }
  i = emAfPriceCommonGetActiveIndex( info.co2ValueTable.commonInfos[ep], 
                                     EMBER_AF_PLUGIN_PRICE_CLIENT_CO2_TABLE_SIZE );
  return i;
}

void emAfPricePrintCo2ValueTablePrintIndex( int8u endpoint, int8u i ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CO2_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Print CO2 Value [%d]", i);
    emberAfPriceClusterPrintln("  isValid=%d", info.co2ValueTable.commonInfos[ep][i].valid );
    emberAfPriceClusterPrintln("  providerId=0x%4X", info.co2ValueTable.commonInfos[ep][i].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=0x%4X", info.co2ValueTable.commonInfos[ep][i].issuerEventId);
    emberAfPriceClusterPrint("  startTime= ");
    emberAfPrintTime(info.co2ValueTable.commonInfos[ep][i].startTime);
    emberAfPriceClusterPrintln("  tariffType=0x%X", info.co2ValueTable.co2Value[ep][i].tariffType);
    emberAfPriceClusterPrintln("  cO2Value=0x%4X", info.co2ValueTable.co2Value[ep][i].cO2Value);
    emberAfPriceClusterPrintln("  cO2ValueUnit=0x%X", info.co2ValueTable.co2Value[ep][i].cO2ValueUnit);
    emberAfPriceClusterPrintln("  cO2ValueTrailingDigit=0x%X", info.co2ValueTable.co2Value[ep][i].cO2ValueTrailingDigit);
  }
}

/// TIER LABELS

static void emAfPriceClientTierLabelsInit( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_TIER_LABELS_TABLE_SIZE; i++ ){
    info.tierLabelsTable.commonInfos[ep][i].valid = FALSE;
  }
}

static void emAfPriceClientAddTierLabel( int8u endpoint, int32u providerId, int32u issuerEventId, int32u issuerTariffId,
                                         int8u numberOfLabels, int8u *tierLabelsPayload ){

  int8u i,j,x;
  int8u tierLabelLen;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.tierLabelsTable.commonInfos[ep],
                                               EMBER_AF_PLUGIN_PRICE_CLIENT_TIER_LABELS_TABLE_SIZE,
                                               providerId, issuerEventId );
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_TIER_LABELS_TABLE_SIZE ){
    info.tierLabelsTable.commonInfos[ep][i].providerId = providerId;
    info.tierLabelsTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.tierLabelsTable.tierLabels[ep][i].issuerTariffId = issuerTariffId;
    info.tierLabelsTable.commonInfos[ep][i].valid = TRUE;
    if( numberOfLabels > EMBER_AF_PLUGIN_PRICE_CLIENT_MAX_TIERS_PER_TARIFF ){
      numberOfLabels = EMBER_AF_PLUGIN_PRICE_CLIENT_MAX_TIERS_PER_TARIFF;
    }
    info.tierLabelsTable.tierLabels[ep][i].numberOfLabels = numberOfLabels;
    x = 0;
    for( j=0; j<numberOfLabels; j++ ){
      info.tierLabelsTable.tierLabels[ep][i].tierIds[j] = tierLabelsPayload[x];
      tierLabelLen = tierLabelsPayload[x+1];
      if( tierLabelLen > 12 ){
        tierLabelLen = 12;
        tierLabelsPayload[x+1] = tierLabelLen;
      }
      MEMCOPY( info.tierLabelsTable.tierLabels[ep][i].tierLabels[j], &tierLabelsPayload[x+1], tierLabelLen+1 );
      x += (tierLabelLen + 2);
    }
  }
}


void emAfPricePrintTierLabelTableEntryIndex( int8u endpoint, int8u i ){
  int8u j;
  int8u numLabels;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }

  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_TIER_LABELS_TABLE_SIZE ){
    emberAfPriceClusterPrintln("= CLIENT TIER LABEL TABLE [%d] =", i);
    emberAfPriceClusterPrintln("  providerId=%d", info.tierLabelsTable.commonInfos[ep][i].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.tierLabelsTable.commonInfos[ep][i].issuerEventId );
    emberAfPriceClusterPrintln("  issuerTariffId=%d", info.tierLabelsTable.tierLabels[ep][i].issuerTariffId );
    emberAfPriceClusterPrintln("  numberOfLabels=%d", info.tierLabelsTable.tierLabels[ep][i].numberOfLabels );
    numLabels = info.tierLabelsTable.tierLabels[ep][i].numberOfLabels;
    if( numLabels > EMBER_AF_PLUGIN_PRICE_CLIENT_MAX_TIERS_PER_TARIFF ){
      numLabels = EMBER_AF_PLUGIN_PRICE_CLIENT_MAX_TIERS_PER_TARIFF;
    }
    for( j=0; j<numLabels; j++ ){
      emberAfPriceClusterPrintln("  tierId[%d]=%d", j, info.tierLabelsTable.tierLabels[ep][i].tierIds[j] );
    }
  }
  else{
    emberAfPriceClusterPrintln("Error:  Tier Label index %d not valid", i );
  }
}


boolean emberAfPriceClusterPublishTierLabelsCallback(int32u providerId,
                                                     int32u issuerEventId,
                                                     int32u issuerTariffId,
                                                     int8u commandIndex,
                                                     int8u numberOfCommands,
                                                     int8u numberOfLabels,
                                                     int8u* tierLabelsPayload){
  int8u endpoint = emberAfCurrentEndpoint();
  emberAfPriceClusterPrintln("RX: PublishTierLabels");
  emAfPriceClientAddTierLabel( endpoint, providerId, issuerEventId, issuerTariffId,
                               numberOfLabels, tierLabelsPayload );

  return TRUE;
}

int8u emAfPriceGetActiveTierLabelTableIndexByTariffId( int8u endpoint, int32u tariffId ){
  int32u largestEventId = 0;
  int8u  largestEventIdIndex = 0xFF;
  int8u  i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }

  emberAfPriceClusterPrintln("===========   TIER LABEL TABLE CHECK, tariff=%d", tariffId);
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_TIER_LABELS_TABLE_SIZE; i++ ){
    emberAfPriceClusterPrintln("  i=%d, val=%d, tariffId=%d, eventId=%d", i,
        info.tierLabelsTable.commonInfos[ep][i].valid, info.tierLabelsTable.tierLabels[ep][i].issuerTariffId,
        info.tierLabelsTable.commonInfos[ep][i].issuerEventId );

    if( info.tierLabelsTable.commonInfos[ep][i].valid &&
        (info.tierLabelsTable.tierLabels[ep][i].issuerTariffId == tariffId) &&
        (info.tierLabelsTable.commonInfos[ep][i].issuerEventId > largestEventId) ){
      largestEventId = info.tierLabelsTable.commonInfos[ep][i].issuerEventId;
      largestEventIdIndex = i;
      emberAfPriceClusterPrintln("   ___ UPDATING i=%d", i );
    }
  }
  emberAfPriceClusterPrintln("- RETURN %d", largestEventIdIndex );
  return largestEventIdIndex;
}

/// BILLING PERIOD
static void emberAfPriceInitBillingPeriodTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_BILLING_PERIOD_TABLE_SIZE; i++ ){
    info.billingPeriodTable.commonInfos[ep][i].valid = FALSE;
  }
}


boolean emberAfPriceClusterPublishBillingPeriodCallback(int32u providerId,
                                                        int32u issuerEventId,
                                                        int32u billingPeriodStartTime,
                                                        int32u billingPeriodDuration,
                                                        int8u billingPeriodDurationType,
                                                        int8u tariffType)
{
  int8u  i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }

  emberAfPriceClusterPrintln("RX: PublishBillingPeriod, issuerId=%d,  start=0x%4x", issuerEventId, billingPeriodStartTime );

  if( billingPeriodStartTime == 0 ){
    billingPeriodStartTime = emberAfGetCurrentTime();
  }

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.billingPeriodTable.commonInfos[ep],
                                               EMBER_AF_PLUGIN_PRICE_CLIENT_BILLING_PERIOD_TABLE_SIZE,
                                               providerId, issuerEventId );

  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_BILLING_PERIOD_TABLE_SIZE ){
    if( billingPeriodStartTime == CANCELLATION_START_TIME ){
      info.billingPeriodTable.commonInfos[ep][i].valid = FALSE;
      emberAfPriceClusterPrintln("Canceling eventId=%d", issuerEventId );
    }
    else{
      info.billingPeriodTable.commonInfos[ep][i].valid = TRUE;
      info.billingPeriodTable.commonInfos[ep][i].providerId = providerId;
      info.billingPeriodTable.commonInfos[ep][i].issuerEventId = issuerEventId;
      info.billingPeriodTable.commonInfos[ep][i].startTime = 
                emberAfPluginPriceCommonClusterGetAdjustedStartTime( billingPeriodStartTime,
                                                         billingPeriodDurationType );
      info.billingPeriodTable.commonInfos[ep][i].durationSec = 
                emberAfPluginPriceCommonClusterConvertDurationToSeconds( billingPeriodStartTime,
                                                             billingPeriodDuration,
                                                             billingPeriodDurationType );


      info.billingPeriodTable.billingPeriod[ep][i].billingPeriodStartTime = billingPeriodStartTime;
      info.billingPeriodTable.billingPeriod[ep][i].billingPeriodDuration = billingPeriodDuration;
      info.billingPeriodTable.billingPeriod[ep][i].billingPeriodDurationType = billingPeriodDurationType;
      info.billingPeriodTable.billingPeriod[ep][i].tariffType = tariffType;
    }
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;

}


int8u emAfPriceGetActiveBillingPeriodIndex( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }
  // Get the event that started before current time, with largest event Id.
  i = emAfPriceCommonGetActiveIndex( info.billingPeriodTable.commonInfos[ep],
                                     EMBER_AF_PLUGIN_PRICE_CLIENT_BILLING_PERIOD_TABLE_SIZE );

  return i;
}


void emAfPricePrintBillingPeriodTableEntryIndex( int8u endpoint, int8u i ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_BILLING_PERIOD_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Print Billing Period Table [%d]", i );
    emberAfPriceClusterPrintln("  providerId=%d", info.billingPeriodTable.commonInfos[ep][i].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.billingPeriodTable.commonInfos[ep][i].issuerEventId );
    emberAfPriceClusterPrintln("  startTime=%d", info.billingPeriodTable.billingPeriod[ep][i].billingPeriodStartTime );
    emberAfPriceClusterPrintln("  duration=%d", info.billingPeriodTable.billingPeriod[ep][i].billingPeriodDuration );
    emberAfPriceClusterPrintln("  durationType=%d", info.billingPeriodTable.billingPeriod[ep][i].billingPeriodDurationType );
    emberAfPriceClusterPrintln("  tariffType=%d", info.billingPeriodTable.billingPeriod[ep][i].tariffType );
  }
  else{
    emberAfPriceClusterPrintln("Billing Period Entry NOT FOUND");
  }
}


///  CPP EVENT
static void emberAfPriceClusterInitCpp( int8u endpoint ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  info.cppEventTable.commonInfos[ep].valid = FALSE;
  info.cppEventTable.commonInfos[ep].issuerEventId = 0;
}

void emberAfPricePrintCppEvent( int8u endpoint ){
  int32u timeNow;
  int32u cppEndTime;
  boolean cppActive;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }

  timeNow = emberAfGetCurrentTime();
  cppEndTime = info.cppEventTable.commonInfos[ep].startTime + info.cppEventTable.commonInfos[ep].durationSec;

  cppActive = ( info.cppEventTable.commonInfos[ep].valid &&
                (info.cppEventTable.commonInfos[ep].startTime <= timeNow) &&
                (cppEndTime >= timeNow) );

  emberAfPriceClusterPrintln("  == startTime=0x%4x, endTime=0x%4x,  timeNow=0x%4x",
      info.cppEventTable.commonInfos[ep].startTime, cppEndTime, info.cppEventTable.commonInfos[ep].durationSec );

  emberAfPriceClusterPrintln("= CPP Event =");
  emberAfPriceClusterPrintln("  active=%d", cppActive);
  emberAfPriceClusterPrintln("  valid=%d", info.cppEventTable.commonInfos[ep].valid);
  emberAfPriceClusterPrintln("  providerId=%d", info.cppEventTable.commonInfos[ep].providerId);
  emberAfPriceClusterPrintln("  issuerEventId=%d", info.cppEventTable.commonInfos[ep].issuerEventId);
  emberAfPriceClusterPrintln("  startTime=0x%4x", info.cppEventTable.commonInfos[ep].startTime);
  emberAfPriceClusterPrintln("  durationInMinutes=%d", info.cppEventTable.cppEvent[ep].durationInMinutes);
  emberAfPriceClusterPrintln("  tariffType=%d", info.cppEventTable.cppEvent[ep].tariffType);
  emberAfPriceClusterPrintln("  cppPriceTier=%d", info.cppEventTable.cppEvent[ep].cppPriceTier);
  emberAfPriceClusterPrintln("  cppAuth=%d", info.cppEventTable.cppEvent[ep].cppAuth);
}

boolean emberAfPriceClusterPublishCppEventCallback(int32u providerId,
                                                   int32u issuerEventId,
                                                   int32u startTime,
                                                   int16u durationInMinutes,
                                                   int8u tariffType,
                                                   int8u cppPriceTier,
                                                   int8u cppAuth){

  int8u responseCppAuth;
  EmberAfStatus status;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }

  emberAfPriceClusterPrintln("RX: PublishCppEvent:");
  if( (cppPriceTier > 1) || (cppAuth >= EMBER_AF_PLUGIN_PRICE_CPP_AUTH_RESERVED) ){
    emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_INVALID_FIELD );
  }
  else{
    if( startTime == 0x00000000 ){
      startTime = emberAfGetCurrentTime();
    }
    else if( startTime == CANCELLATION_START_TIME ){
      // Cancellation attempt.
      if( (info.cppEventTable.commonInfos[ep].providerId == providerId) &&
          (info.cppEventTable.commonInfos[ep].issuerEventId == issuerEventId) ){
        emberAfPriceClusterPrintln("CPP Event Cancelled");
        info.cppEventTable.commonInfos[ep].valid = FALSE;
        status = EMBER_ZCL_STATUS_SUCCESS;
      }
      else{
        status = EMBER_ZCL_STATUS_NOT_FOUND;
      }
      emberAfSendImmediateDefaultResponse( status );
      goto kickout;
    }

    // If the cppAuth is PENDING, the client may decide to accept or reject the CPP event.
    if( cppAuth == EMBER_AF_PLUGIN_PRICE_CPP_AUTH_PENDING ){
      responseCppAuth = emberAfPluginPriceClientPendingCppEventCallback( cppAuth );
      emberAfPriceClusterPrintln( "  Pending CPP Event, status=%d", responseCppAuth );
      emberAfFillCommandPriceClusterCppEventResponse( issuerEventId, responseCppAuth );
      emberAfSendResponse();
    }
    else{
      if( (cppAuth == EMBER_AF_PLUGIN_PRICE_CPP_AUTH_ACCEPTED) ||
          (cppAuth == EMBER_AF_PLUGIN_PRICE_CPP_AUTH_FORCED) ){
        // Apply the CPP
        emberAfPriceClusterPrintln("CPP Event Accepted");
        info.cppEventTable.commonInfos[ep].providerId = providerId;
        info.cppEventTable.commonInfos[ep].issuerEventId = issuerEventId;
        info.cppEventTable.commonInfos[ep].startTime = startTime;
        info.cppEventTable.commonInfos[ep].durationSec = (durationInMinutes * 60);
        info.cppEventTable.commonInfos[ep].valid = TRUE;
        info.cppEventTable.cppEvent[ep].durationInMinutes = durationInMinutes;
        info.cppEventTable.cppEvent[ep].tariffType = tariffType;
        info.cppEventTable.cppEvent[ep].cppPriceTier = cppPriceTier;
        info.cppEventTable.cppEvent[ep].cppAuth = cppAuth;
      }
      else if( cppAuth == EMBER_AF_PLUGIN_PRICE_CPP_AUTH_REJECTED ){
        emberAfPriceClusterPrintln("CPP Event Rejected");
      }
      emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
    }
  }
kickout:
  return TRUE;
}

// CREDIT PAYMENT

static void emberAfPriceClusterInitCreditPaymentTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CREDIT_PAYMENT_TABLE_SIZE; i++ ){
    info.creditPaymentTable.commonInfos[ep][i].valid = FALSE;
  }
}

boolean emberAfPriceClusterPublishCreditPaymentCallback(int32u providerId, int32u issuerEventId,
                                                        int32u creditPaymentDueDate, int32u creditPaymentOverDueAmount,
                                                        int8u creditPaymentStatus, int32u creditPayment,
                                                        int32u creditPaymentDate, int8u* creditPaymentRef){
  int8u i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.creditPaymentTable.commonInfos[ep],
                                               EMBER_AF_PLUGIN_PRICE_CLIENT_CREDIT_PAYMENT_TABLE_SIZE,
                                               providerId, issuerEventId );

  emberAfPriceClusterPrintln("RX: PublishCreditPayment [%d], issuerEventId=%d", i, issuerEventId );
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CREDIT_PAYMENT_TABLE_SIZE ){
    info.creditPaymentTable.commonInfos[ep][i].valid = TRUE;
    info.creditPaymentTable.commonInfos[ep][i].providerId = providerId;
    info.creditPaymentTable.commonInfos[ep][i].issuerEventId = issuerEventId;
    info.creditPaymentTable.creditPayment[ep][i].creditPaymentDueDate = creditPaymentDueDate;
    info.creditPaymentTable.creditPayment[ep][i].creditPaymentOverDueAmount = creditPaymentOverDueAmount;
    info.creditPaymentTable.creditPayment[ep][i].creditPaymentStatus = creditPaymentStatus;
    info.creditPaymentTable.creditPayment[ep][i].creditPayment = creditPayment;
    info.creditPaymentTable.creditPayment[ep][i].creditPaymentDate = creditPaymentDate;
    MEMCOPY( info.creditPaymentTable.creditPayment[ep][i].creditPaymentRef, creditPaymentRef, EMBER_AF_PLUGIN_PRICE_CLUSTER_MAX_CREDIT_PAYMENT_REF_LENGTH+1 );
  }

  emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  return TRUE;
}

int8u emAfPriceCreditPaymentTableGetIndexWithEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }

  i = emAfPriceCommonGetMatchingIndex( info.creditPaymentTable.commonInfos[ep],
                                       EMBER_AF_PLUGIN_PRICE_CLIENT_CREDIT_PAYMENT_TABLE_SIZE,
                                       issuerEventId );
  return i;
}


void emAfPricePrintCreditPaymentTableIndex( int8u endpoint, int8u index ){
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( index >= EMBER_AF_PLUGIN_PRICE_CLIENT_CREDIT_PAYMENT_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Error: Print index %d out of bounds.", index );
  }
  else if( info.creditPaymentTable.commonInfos[ep][index].valid == FALSE ){
    emberAfPriceClusterPrintln("Error: Entry %d invalid", index );
  }
  else{
    emberAfPriceClusterPrintln("Print Credit Payment [%d]", index );
    emberAfPriceClusterPrintln("  valid=%d", info.creditPaymentTable.commonInfos[ep][index].valid );
    emberAfPriceClusterPrintln("  providerId=%d", info.creditPaymentTable.commonInfos[ep][index].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.creditPaymentTable.commonInfos[ep][index].issuerEventId );
    emberAfPriceClusterPrintln("  dueDate=%d", info.creditPaymentTable.creditPayment[ep][index].creditPaymentDueDate );
    emberAfPriceClusterPrintln("  overDueAmount=%d", info.creditPaymentTable.creditPayment[ep][index].creditPaymentOverDueAmount );
    emberAfPriceClusterPrintln("  status=%d", info.creditPaymentTable.creditPayment[ep][index].creditPaymentStatus );
    emberAfPriceClusterPrintln("  payment=%d", info.creditPaymentTable.creditPayment[ep][index].creditPayment );
    emberAfPriceClusterPrintln("  paymentDate=%d", info.creditPaymentTable.creditPayment[ep][index].creditPaymentDate );
    emberAfPriceClusterPrintln("  paymentRef=%d", info.creditPaymentTable.creditPayment[ep][index].creditPaymentRef );
  }
}



// CURRENCY CONVERSION

static void emberAfPriceClusterInitCurrencyConversionTable( int8u endpoint ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE; i++ ){
    info.currencyConversionTable.commonInfos[ep][i].valid = FALSE;
  }
}


boolean emberAfPriceClusterPublishCurrencyConversionCallback(int32u providerId,
                                                             int32u issuerEventId,
                                                             int32u startTime,
                                                             int16u oldCurrency,
                                                             int16u newCurrency,
                                                             int32u conversionFactor,
                                                             int8u conversionFactorTrailingDigit,
                                                             int32u currencyChangeControlFlags){

  int8u i;
  int8u endpoint = emberAfCurrentEndpoint();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return FALSE;
  }

  emberAfPriceClusterPrintln("RX: Currency Conversion, start=%d, pid=%d, eid=%d", startTime, providerId, issuerEventId );

  if( startTime == 0 ){
    startTime = emberAfGetCurrentTime();
  }

  i = emAfPriceCommonGetMatchingOrUnusedIndex( info.currencyConversionTable.commonInfos[ep],
                                               EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE,
                                               providerId, issuerEventId );

  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE ){
    if( startTime == CANCELLATION_START_TIME ){
      info.currencyConversionTable.commonInfos[ep][i].valid = FALSE;
    }
    else{
      info.currencyConversionTable.commonInfos[ep][i].valid = TRUE;
      info.currencyConversionTable.commonInfos[ep][i].providerId = providerId;
      info.currencyConversionTable.commonInfos[ep][i].issuerEventId = issuerEventId;
      info.currencyConversionTable.commonInfos[ep][i].startTime = startTime;
      info.currencyConversionTable.currencyConversion[ep][i].newCurrency = newCurrency;
      info.currencyConversionTable.currencyConversion[ep][i].conversionFactor = conversionFactor;
      info.currencyConversionTable.currencyConversion[ep][i].conversionFactorTrailingDigit = conversionFactorTrailingDigit;
      info.currencyConversionTable.currencyConversion[ep][i].currencyChangeControlFlags = currencyChangeControlFlags;
    }
    emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  }
  return TRUE;
}

int8u emberAfPriceClusterGetActiveCurrencyIndex( int8u endpoint ){
  int8u i;
  int32u timeNow = emberAfGetCurrentTime();
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }

  i = emAfPriceCommonGetActiveIndex( info.currencyConversionTable.commonInfos[ep],
                                     EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE );

  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE ){
    emberAfPriceClusterPrintln("GET ACTIVE:  i=%d, startTime=%d,  currTime=%d",
      i, info.currencyConversionTable.commonInfos[ep][i].startTime, timeNow );
  }

  return i;
}



int8u emberAfPriceClusterCurrencyConversionTableGetIndexByEventId( int8u endpoint, int32u issuerEventId ){
  int8u i;
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return 0xFF;
  }
  i = emAfPriceCommonGetMatchingIndex( info.currencyConversionTable.commonInfos[ep],
                                       EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE,
                                       issuerEventId );
  return i;
}

void emAfPricePrintCurrencyConversionTableIndex( int8u endpoint, int8u i ){  
  int8u ep = emberAfFindClusterClientEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == 0xFF) {
    return;
  }
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE ){
    emberAfPriceClusterPrintln("= Print Currency Conversion Table [%d]", i );
    emberAfPriceClusterPrintln("  providerId=%d", info.currencyConversionTable.commonInfos[ep][i].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=%d", info.currencyConversionTable.commonInfos[ep][i].issuerEventId );
    emberAfPriceClusterPrintln("  startTime=%d", info.currencyConversionTable.commonInfos[ep][i].startTime );
    emberAfPriceClusterPrintln("  newCurrency=%d", info.currencyConversionTable.currencyConversion[ep][i].newCurrency );
    emberAfPriceClusterPrintln("  conversionFactor=%d", info.currencyConversionTable.currencyConversion[ep][i].conversionFactor );
    emberAfPriceClusterPrintln("  conversionFactorTrailingDigit=%d", info.currencyConversionTable.currencyConversion[ep][i].conversionFactorTrailingDigit );
    emberAfPriceClusterPrintln("  currencyChangeControlFlags=%d", info.currencyConversionTable.currencyConversion[ep][i].currencyChangeControlFlags );
  }
}

boolean emberAfPriceClusterCancelTariffCallback(int32u providerId, int32u issuerTariffId, int8u tariffType){

  emberAfPriceClusterPrintln("RX: Cancel Tariff, providerId=%d, issuerTariffId=%d, tariffType=%d", 
                              providerId, issuerTariffId, tariffType );

  emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  return TRUE;
}



