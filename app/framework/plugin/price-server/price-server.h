 // *******************************************************************
// * price-server.h
// *
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/plugin/price-common/price-common.h"

#ifndef EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE
  #define EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE (5)
#endif

#ifndef EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE
  #define EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE (2)
#endif

#ifndef EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE
  #define EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE (2)
#endif

#define ZCL_PRICE_CLUSTER_PRICE_ACKNOWLEDGEMENT_MASK 0x01
#define ZCL_PRICE_CLUSTER_RESERVED_MASK              0xFE
#define ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE (6)
#define ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUBPAYLOAD_BLOCK_SIZE     (5)

#define fieldLength(field) \
      (emberAfCurrentCommand()->bufLen - (field - emberAfCurrentCommand()->buffer));

#define ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS 15
#define ZCL_PRICE_CLUSTER_MAX_TOU_BLOCK_TIERS 15
#define ZCL_PRICE_CLUSTER_MAX_TOU_TIERS 48
#define ZCL_PRICE_CLUSTER_PRICE_MATRIX_SUB_PAYLOAD_ENTRY_SIZE 5

// To help keep track of the status of the tariffs in the table
// (also, corresponding price matrices).
#define CURRENT       BIT(1)
#define FUTURE        BIT(2)
#define PUBLISHED     BIT(3)

#define TARIFF_TYPE_MASK (0x0F)
#define CHARGING_SCHEME_MASK (0xF0)

#define tariffIsCurrent(tariff) ((tariff)->status & CURRENT)
#define tariffIsFuture(tariff)  ((tariff)->status & FUTURE)
#define tariffIsPublished(tariff)  ((tariff)->status & PUBLISHED)
#define priceMatrixIsCurrent(pm) ((pm)->status & CURRENT)
#define priceMatrixIsFuture(pm) ((pm)->status & FUTURE)
#define priceMatrixIsPublished(pm) ((pm)->status & PUBLISHED)
#define blockThresholdsIsCurrent(bt) ((bt)->status & CURRENT)
#define blockThresholdsIsFuture(bt) ((bt)>status & FUTURE)
#define blockThresholdsIsPublished(bt) ((bt)->status & PUBLISHED)

/**
 * @brief The price and metadata used by the Price server plugin.
 *
 * The application can get and set the prices used by the plugin by calling
 * ::emberAfPriceGetPriceTableEntry and
 * ::emberAfPriceSetPriceTableEntry.
 */

/**
 * @brief conversion factor infos by the Price server plugin.
 *
 */

typedef struct{
  int32u providerId;
  int32u rawBlockPeriodStartTime;
  int32u blockPeriodDuration;
  // The "thresholdMultiplier" and "threadholdDivisor" are included in this stucture
  // since these should be specified with the block period.
  // These values are stored as the "Threshold Multiplier" and "Threshold Divisor"
  // attributes in the Block Period (Delivered) attribute set (D.4.2.2.3).
  int32u thresholdMultiplier;
  int32u thresholdDivisor;
  int8u  blockPeriodControl;
  int8u  blockPeriodDurationType;
  int8u  tariffType;
  int8u  tariffResolutionPeriod;
} EmberAfPriceBlockPeriod;

typedef struct {
  int32u providerId;
  int32u rawBillingPeriodStartTime;
  int32u billingPeriodDuration;
  int8u billingPeriodDurationType;
  int8u tariffType;
} EmberAfPriceBillingPeriod;

typedef struct{
  int32u providerId;
  int32u durationInMinutes;
  int8u  tariffType;
  int8u  cppPriceTier;
  int8u  cppAuth;
} EmberAfPriceCppEvent;

typedef struct{
  int32u providerId;
  int32u rawStartTimeUtc;   // start time as received from caller, prior to any adjustments
  int32u billingPeriodDuration;
  int32u consolidatedBill;
  int16u currency;
  int8u  billingPeriodDurationType;
  int8u  tariffType;
  int8u  billTrailingDigit;
} EmberAfPriceConsolidatedBills;

#define CREDIT_PAYMENT_REF_STRING_LEN 20
typedef struct{
  int32u providerId;
  int32u creditPaymentDueDate;
  int32u creditPaymentAmountOverdue;
  int32u creditPayment;
  int32u creditPaymentDate;
  int8u  creditPaymentStatus;
  int8u  creditPaymentRef[ CREDIT_PAYMENT_REF_STRING_LEN + 1 ];
} EmberAfPriceCreditPayment;

typedef struct {
  int32u conversionFactor;
  int8u conversionFactorTrailingDigit;
} EmberAfPriceConversionFactor;

typedef struct {
  int32u calorificValue;
  int8u calorificValueUnit;
  int8u calorificValueTrailingDigit;
} EmberAfPriceCalorificValue;

typedef struct{
  int32u providerId;
  int32u issuerTariffId;
  int8u  tariffType;
  boolean valid;
} EmberAfPriceCancelTariff;

typedef struct {
  int32u providerId;
  int32u co2Value;
  int8u tariffType;
  int8u co2ValueUnit;
  int8u co2ValueTrailingDigit;
} EmberAfPriceCo2Value;

typedef struct{
  int32u providerId;
  int16u oldCurrency;
  int16u newCurrency;
  int32u conversionFactor;
  int8u  conversionFactorTrailingDigit;
  int32u currencyChangeControlFlags;
} EmberAfPriceCurrencyConversion;

#define TIER_LABEL_SIZE  12
typedef struct{
  int32u providerId;
  int32u issuerEventId;
  int32u issuerTariffId;
  int8u  valid;
  int8u  numberOfTiers;
  int8u  tierIds[EMBER_AF_PLUGIN_PRICE_SERVER_MAX_TIERS_PER_TARIFF];
  int8u  tierLabels[EMBER_AF_PLUGIN_PRICE_SERVER_MAX_TIERS_PER_TARIFF][TIER_LABEL_SIZE+1];
} EmberAfPriceTierLabelValue;

typedef struct {
  EmberAfPriceTierLabelValue entry[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
//  int8u valid[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
//  int32u providerId[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
//  int32u issuerEventId[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
//  int32u issuerTariffId[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
//  int8u tierId[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
//  int8u tierLabel[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE][13];
} EmberAfPriceTierLabelTable ;

typedef struct{
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE];
  EmberAfPriceBlockPeriod blockPeriods[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE];
} EmberAfPriceBlockPeriodTable;

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE];
  EmberAfPriceBillingPeriod billingPeriods[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE];
} EmberAfPriceBillingPeriodTable;


typedef struct{
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT];
  EmberAfPriceCppEvent cppEvent[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT];
} EmberAfPriceCppTable;
  

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE];
  EmberAfPriceConsolidatedBills consolidatedBills[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE];
} EmberAfPriceConsolidatedBillsTable;

typedef struct{
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE];
  EmberAfPriceCreditPayment creditPayment[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE];
} EmberAfPriceCreditPaymentTable;

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE];
  EmberAfPriceConversionFactor priceConversionFactors[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE];
} EmberAfPriceConversionFactorTable;

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE];
  EmberAfPriceCalorificValue calorificValues[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE];
} EmberAfPriceCalorificValueTable;

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE];
  EmberAfPriceCo2Value co2Values[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE];
} EmberAfPriceCO2Table;

typedef struct{
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT];
  EmberAfPriceCurrencyConversion currencyConversion[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT];
} EmberAfPriceCurrencyConversionTable;

typedef struct{
  EmberAfPriceCancelTariff cancelTariff[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT];
} EmberAfPriceCancelTariffTable;

typedef struct {
  int8u   rateLabel[ZCL_PRICE_CLUSTER_MAXIMUM_RATE_LABEL_LENGTH + 1];
  int32u  providerId;
  int32u  issuerEventID;
  int32u  startTime;
  int32u  price;
  int32u  generationPrice;
  int32u  alternateCostDelivered;
  int16u  currency;
  int16u  duration; // in minutes
  int8u   unitOfMeasure;
  int8u   priceTrailingDigitAndTier;
  int8u   numberOfPriceTiersAndTier; // added later in errata
  int8u   priceRatio;
  int8u   generationPriceRatio;
  int8u   alternateCostUnit;
  int8u   alternateCostTrailingDigit;
  int8u   numberOfBlockThresholds;
  int8u   priceControl;
} EmberAfScheduledPrice;

typedef int8u emAfPriceBlockThreshold[ZCL_PRICE_CLUSTER_BLOCK_THRESHOLDS_PAYLOAD_SIZE];
typedef struct {
  union {
    emAfPriceBlockThreshold blockAndTier[ZCL_PRICE_CLUSTER_MAX_TOU_BLOCK_TIERS][ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS-1];
    emAfPriceBlockThreshold block[ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS-1];
  } thresholds;
  int32u providerId;
  int32u issuerTariffId;
  int8u status;
} EmberAfScheduledBlockThresholds;

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
  EmberAfScheduledBlockThresholds scheduledBlockThresholds[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
} EmberAfScheduledBlockThresholdsTable;

typedef struct {
  int32u providerId;
  int32u issuerTariffId;
  int8u status;
  int8u tariffTypeChargingScheme;

  // below fields have corresponding zcl attributes.
  int8u  tariffLabel[ZCL_PRICE_CLUSTER_MAXIMUM_RATE_LABEL_LENGTH + 1]; 
  int8u  numberOfPriceTiersInUse;
  int8u  numberOfBlockThresholdsInUse;
  int8u  tierBlockMode;
  int8u  unitOfMeasure;
  int16u currency;
  int8u  priceTrailingDigit;
  int32u standingCharge;
  int32u blockThresholdMultiplier;
  int32u blockThresholdDivisor;
} EmberAfScheduledTariff;


typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
  EmberAfScheduledTariff scheduledTariffs[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
} EmberAfScheduledTariffTable;

typedef struct {
  union {
    int32u blockAndTier[ZCL_PRICE_CLUSTER_MAX_TOU_BLOCK_TIERS][ZCL_PRICE_CLUSTER_MAX_TOU_BLOCKS];
    int32u tier[ZCL_PRICE_CLUSTER_MAX_TOU_TIERS];
  } matrix;
  int32u providerId;
  int32u issuerTariffId;
  int8u status;
} EmberAfScheduledPriceMatrix;

typedef struct {
  EmberAfPriceCommonInfo commonInfos[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
  EmberAfScheduledPriceMatrix scheduledPriceMatrix[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE];
} EmberAfScheduledPriceMatrixTable;


typedef struct {
  EmberAfPriceBlockPeriodTable blockPeriodTable;
  EmberAfPriceConversionFactorTable conversionFactorTable;
  EmberAfPriceCalorificValueTable calorificValueTable;
  EmberAfPriceCO2Table co2ValueTable;
  EmberAfPriceTierLabelTable tierLabelTable;
  EmberAfPriceBillingPeriodTable billingPeriodTable;
  EmberAfPriceConsolidatedBillsTable consolidatedBillsTable;
  EmberAfPriceCppTable cppTable;
  EmberAfPriceCreditPaymentTable creditPaymentTable;
  EmberAfPriceCurrencyConversionTable currencyConversionTable;
  EmberAfPriceCancelTariffTable cancelTariffTable;
  EmberAfScheduledTariffTable scheduledTariffTable;
  EmberAfScheduledBlockThresholdsTable scheduledBlockThresholdsTable;
  EmberAfScheduledPriceMatrixTable  scheduledPriceMatrixTable;
} EmberAfPriceServerInfo;

extern EmberAfPriceServerInfo info;

/**
 * @brief Called to send the next Get Scheduled Prices command.
 *
 * @param endpoint The endpoint in question.
 **/
void emberAfPriceServerSendGetScheduledPrices(int8u endpoint);

/**
 * @brief Return the number of seconds until the next Get Scheduled Prices command should be sent.
 * 
 * @return The number of seconds until the next Get Scheduled Prices command should be sent.
 **/
int32u emberAfPriceServerSecondsUntilGetScheduledPricesEvent(void);



/** 
 * @brief Clear all prices in the price table. 
 * 
 * @param endpoint The endpoint in question
 **/
void emberAfPriceClearPriceTable(int8u endpoint);

/**
 * @brief Clear all tariffs in the tariff table.
 *
 * @param endpoint The endpoint in question
 */
void emberAfPriceClearTariffTable(int8u endpoint);

/**
 * @brief Clear all price matrices in the price matrix table.
 *
 * @param endpoint The endpoint in question
 */
void emberAfPriceClearPriceMatrixTable(int8u endpoint);

/**
 * @brief Clear all block thresholds in the block thresholds table.
 *
 * @param endpoint The endpoint in question
 */
void emberAfPriceClearBlockThresholdsTable(int8u endpoint);

/**
 * @brief Get a price used by the Price server plugin.
 *
 * This function can be used to get a price and metadata that the plugin will
 * send to clients.  For "start now" prices that are current or scheduled, the
 * duration is adjusted to reflect how many minutes remain for the price.
 * Otherwise, the start time and duration of "start now" prices reflect the
 * actual start and the original duration.
 *
 * @param endpoint The relevant endpoint.
 * @param index The index in the price table.
 * @param price The ::EmberAfScheduledPrice structure describing the price.
 * @return TRUE if the price was found or FALSE is the index is invalid.
 */
boolean emberAfPriceGetPriceTableEntry(int8u endpoint, 
                                       int8u index,
                                       EmberAfScheduledPrice *price);

/**
 * @brief Sets values in the Block Period table.
 *
 * @param endpoint The relevant endpoint.
 * @param providerId A unique identifier for the commodity provider.
 * @param issuerEventId The event ID of the block period data.
 * @param blockPeriodStartTime Time at which the block period data is valid.
 * @param blockPeriodDuration The block period duration.  Units are specified by the blockPeriodDurationType.
 * @param blockPeriodControl Identifies additional control options for the block period command.
 * @param blockPeriodDurationType A bitmap that indicates the units used in the block period.
 * @param tariffType Bitmap identifying the type of tariff published in this command.
 * @param tariffResolutionPeriod The resolution period for the block tariff.
 *
 **/
void emberAfPluginPriceServerBlockPeriodAdd( int8u endpoint, int32u providerId, int32u issuerEventId,
                                             int32u blockPeriodStartTime, int32u blockPeriodDuration,
                                             int8u  blockPeriodControl, int8u blockPeriodDurationType,
                                             int32u thresholdMultiplier, int32u thresholdDivisor,
                                             int8u  tariffType, int8u tariffResolutionPeriod );


/**
 * @brief Sends a Publish Block Period command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param index The index of the table whose data will be used in the command.
 *
 **/
void emberAfPluginPriceServerBlockPeriodPub( int16u nodeId, int8u srcEndpoint, int8u dstEndpoint, int8u index );


/**
 * @brief Prints the data in the specified index of the Block Period table.
 *
 * @param endpoint The relevant endpoint.
 * @param index The index of the table whose index will be printed.
 *
 **/
void emberAfPluginPriceServerBlockPeriodPrint( int8u endpoint, int8u index );


/**
 * @brief Returns the number of seconds until the next block period event will occur.
 *
 * @param endpoint The relevant endpoint.
 * @return Returns the number of seconds until the next block period event.
 *
 **/
int32u emberAfPriceServerSecondsUntilBlockPeriodEvent( int8u endpoint );


/**
 * @brief Updates block period attributes to match the current block period.
 *
 * @param endpoint The relevant endpoint.
 *
 **/
void emberAfPriceServerRefreshBlockPeriod( int8u endpoint );



/**
 * @brief Get a tariff used by the Price server plugin.
 *
 * This function can be used to get a tariff and associated metadata that
 * the plugin will send to clients.
 *
 * @param endpoint The relevant endpoint.
 * @param index    The index in the tariff table.
 * @param info   The ::EmberAfPriceCommonInfo structure describing the tariff.
 * @param tariff   The ::EmberAfScheduledTariff structure describing the tariff.
 * @return         TRUE if the tariff was found
 */
boolean emberAfPriceGetTariffTableEntry(int8u endpoint,
                                        int8u index,
                                        EmberAfPriceCommonInfo *info,
                                        EmberAfScheduledTariff *tariff);

/**
 * @brief Retrieve a price matrix entry by index
 *
 * This function can be used to get a price matrix and associated metadata that
 * the plugin will send to clients.
 *
 * @param endpoint The relevant endpoint.
 * @param index    The index in the price matrix table.
 * @param pm       The ::EmberAfScheduledPriceMatrix structure describing the price matrix.
 * @return         TRUE if the price matrix was found
 */
boolean emberAfPriceGetPriceMatrix(int8u endpoint,
                                   int8u index,
                                   EmberAfPriceCommonInfo * inf,
                                   EmberAfScheduledPriceMatrix *pm);

/**
 * @brief Get the block thresholds used by the Price server plugin.
 *
 * This function can be used to get the block thresholds and associated metadata that
 * the plugin will send to clients.
 *
 * @param endpoint The relevant endpoint.
 * @param index    The index in the block thresholds table.
 * @param bt       The ::EmberAfScheduledBlockThresholds structure describing the block thresholds.
 * @return         TRUE if the block thresholds was found
 */
boolean emberAfPriceGetBlockThresholdsTableEntry(int8u endpoint,
                                                 int8u index,
                                                 EmberAfScheduledBlockThresholds *bt);

/**
 * @brief Get a tariff by issuer tariff ID and endpoint
 *
 * @param endpoint        The relevant endpoint.
 * @param issuerTariffId  The issuer tariff ID.
 * @param info            The ::EmberAfPriceCommonInfo structure describing the tariff.
 * @param tariff          The ::EmberAfScheduledTariff structure describing the tariff.
 * @return                TRUE if the tariff was found
 */
boolean emberAfPriceGetTariffByIssuerTariffId(int8u endpoint,
                                              int32u issuerTariffId,
                                              EmberAfPriceCommonInfo *info,
                                              EmberAfScheduledTariff *tariff);

/**
 * @brief Get a price matrix by issuer tariff ID and endpoint
 *
 * @param endpoint The relevant endpoint.
 * @param issuerTariffId  The issuer tariff ID.
 * @param pm       The ::EmberAfScheduledPriceMatrix structure describing the price matrix.
 * @return         TRUE if the price matrix was found
 */
boolean emberAfPriceGetPriceMatrixByIssuerTariffId(int8u endpoint,
                                                   int32u issuerTariffId,
                                                   EmberAfPriceCommonInfo * inf,
                                                   EmberAfScheduledPriceMatrix *pm);

/**
 * @brief Get the block thresholds by issuer tariff ID and endpoint
 *
 * @param endpoint The relevant endpoint.
 * @param issuerTariffId  The issuer tariff ID.
 * @param bt       The ::EmberAfScheduledBlockThresholds structure describing the block thresholds.
 * @return         TRUE if the block thresholds was found
 */
boolean emberAfPriceGetBlockThresholdsByIssuerTariffId(int8u endpoint,
                                                       int32u issuerTariffId,
                                                       EmberAfPriceCommonInfo *inf,
                                                       EmberAfScheduledBlockThresholds *bt);
/**
 * @brief Set a price used by the Price server plugin.
 *
 * This function can be used to set a price and metadata that the plugin will
 * send to clients.  Setting the start time to zero instructs clients to start
 * the price now.  For "start now" prices, the plugin will automatically adjust
 * the duration reported to clients based on the original start time of the
 * price.
 *
 * @param endpoint The relevant endpoint
 * @param index The index in the price table.
 * @param price The ::EmberAfScheduledPrice structure describing the price.  If
 * NULL, the price is removed from the server.
 * @return TRUE if the price was set or removed or FALSE is the index is
 * invalid.
 */
boolean emberAfPriceSetPriceTableEntry(int8u endpoint, 
                                       int8u index,
                                       const EmberAfScheduledPrice *price);

/**
 * @brief Set a tariff used by the Price server plugin.
 *
 * This function can be used to set a tariff and metadata that the plugin
 * will send to clients.
 *
 * @param endpoint The relevant endpoint.
 * @param index    The index in the tariff table.
 * @param tariff   The ::EmberAfScheduledTariff structure describing the tariff.
 *                 If NULL, the tariff is removed from the server.
 * @return         TRUE if the tariff was set or removed, or FALSE if the
 *                 index is invalid.
 */
boolean emberAfPriceSetTariffTableEntry(int8u endpoint,
                                        int8u index,
                                        EmberAfPriceCommonInfo * info,
                                        const EmberAfScheduledTariff *tariff);

/**
 * @brief Set a price matrix entry by index.
 *
 * This function can be used to set a price matrix and metadata that the plugin
 * will send to clients.
 *
 * @param endpoint The relevant endpoint.
 * @param index    The index in the price matrix table.
 * @param pm       The ::EmberAfScheduledPriceMatrix structure describing the
 *                 price matrix. If NULL, the price matrix is removed from the 
 *                 server.
 * @return         TRUE if the price matrix was set or removed, or FALSE if the
 *                 index is invalid.
 */
boolean emberAfPriceSetPriceMatrix(int8u endpoint,
                                   int8u index,
                                   EmberAfPriceCommonInfo * inf,
                                   const EmberAfScheduledPriceMatrix *pm);

/**
 * @brief Set the block thresholds used by the Price server plugin.
 *
 * This function can be used to set the block thresholds and metadata that the plugin
 * will send to clients.
 *
 * @param endpoint The relevant endpoint.
 * @param index    The index in the block thresholds table.
 * @param bt       The ::EmberAfScheduledBlockThresholds structure describing the
 *                 block thresholds. If NULL, the block thresholds entry is removed
 *                 from the table.
 * @return         TRUE if the block thresholds was set or removed, or FALSE if the
 *                 index is invalid.
 */
boolean emberAfPriceSetBlockThresholdsTableEntry(int8u endpoint,
                                                 int8u index,
                                                 const EmberAfPriceCommonInfo *inf,
                                                 const EmberAfScheduledBlockThresholds *bt);

/**
 * @brief Get the current price used by the Price server plugin.
 *
 * This function can be used to get the current price and metadata that the
 * plugin will send to clients.  For "start now" prices, the duration is
 * adjusted to reflect how many minutes remain for the price.  Otherwise, the
 * start time and duration reflect the actual start and the original duration.
 *
 * @param endpoint The relevant endpoint
 * @param price The ::EmberAfScheduledPrice structure describing the price.
 * @return TRUE if the current price was found or FALSE is there is no current
 * price.
 */
boolean emberAfGetCurrentPrice(int8u endpoint, EmberAfScheduledPrice *price);

/**
 * @brief Find the first free index in the price table
 *
 * This function looks through the price table and determines whether
 * the entry is in-use or scheduled to be in use; if not, it's 
 * considered "free" for the purposes of the user adding a new price
 * entry to the server's table, and the index is returned.
 *
 * @param endpoint The relevant endpoint
 * @return The index of the first free (unused/unscheduled) entry in
 * the requested endpoint's price table, or ZCL_PRICE_INVALID_INDEX
 * if no available entry could be found.
 */
int8u emberAfPriceFindFreePriceIndex(int8u endpoint);
 
void emberAfPricePrint(const EmberAfScheduledPrice *price);
void emberAfPricePrintPriceTable(int8u endpoint);
void emberAfPricePrintTariff(const EmberAfPriceCommonInfo *info,
                             const EmberAfScheduledTariff *tariff);
void emberAfPricePrintTariffTable(int8u endpoint);
void emberAfPricePrintPriceMatrix(int8u endpoint,
		                          const EmberAfPriceCommonInfo *inf,
                                  const EmberAfScheduledPriceMatrix *pm);
void emberAfPricePrintPriceMatrixTable(int8u endpoint);
void emberAfPricePrintBlockThresholds(int8u endpoint,
                                      const EmberAfPriceCommonInfo * inf,
                                      const EmberAfScheduledBlockThresholds *bt);
void emberAfPricePrintBlockThresholdsTable(int8u endpoint);
void emberAfPluginPriceServerPublishPriceMessage(EmberNodeId nodeId,
                                                 int8u srcEndpoint,
                                                 int8u dstEndpoint,
                                                 int8u priceIndex);

/**
 * @brief Sets parameters in the conversion factors table.
 *
 * @param endpoint The endpoint in question
 * @param issuerEventId The event ID of the conversion factor data.
 * @param startTime The time when the conversion factor data is valid.
 * @param conversionFactor Accounts for changes in the volume of gas based on temperature and pressure.
 * @param conversionFactorTrailingDigit Determines where the decimal point is located in the conversion factor.
 **/
EmberAfStatus emberAfPluginPriceServerConversionFactorAdd(int8u endpoint,
                                                          int32u issuerEventId,
                                                          int32u startTime,
                                                          int32u conversionFactor,
                                                          int8u conversionFactorTrailingDigit);
/**
 * @brief Clears the conversion factors table and invalidates all entries.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPluginPriceServerConversionFactorClear( int8u endpoint );


/**
 * @brief Sends a Publish Conversion Factor command using the data at the specified table index.
 *
 * @param tableIndex The index of the conversion factor table whose data should be used in the publish conversion factor command.
 * @param dstAddr The destination address to which the command should be sent.
 * @param srcEp The source endpoint used in the transmission.
 * @param dstEp The destination endpoint used in the transmission.
 *
 **/
void emberAfPluginPriceServerConversionFactorPub(int8u tableIndex,
                                                 EmberNodeId dstAddr,
                                                 int8u srcEndpoint,
                                                 int8u dstEndpoint);


/**
 * @brief Returns the number of seconds until the next conversion factor event will become active.
 *
 * @param endpoint The endpoint in question
 * @return The number of seconds until the next conversion factor event becomes active.
 *
 **/ 
int32u emberAfPriceServerSecondsUntilConversionFactorEvent( int8u endpoint );


/**
 * @brief Refreshes conversion factor information if necessary.
 * If the second conversion factor event is active, the first is inactivated and the array is re-sorted.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPriceServerRefreshConversionFactor( int8u endpoint );


/**
 * @brief Sets values in the Calorific Value table.
 *
 * @param endpoint The endpoint in question
 * @issuerEventId The event ID of the calorific value data.
 * @startTime The time at which the calorific value data is valid.
 * @calorificValue The amount of heat generated when a given mass of fuel is burned.
 * @calorificValueTrailingDigit Determines where the decimal point is located in the calorific value.
 *
 **/
EmberAfStatus emberAfPluginPriceServerCalorificValueAdd(int8u endpoint, 
                                                        int32u issuerEventId,
                                                        int32u startTime,
                                                        int32u calorificValue,
                                                        int8u calorificValueUnit,
                                                        int8u calorificValueTrailingDigit);

/**
 * @brief Returns the number of seconds until the next calorific value event will become active.
 *
 * @param endpoint The endpoint in question
 * @return The number of seconds until the next calorific value event becomes active.
 *
 **/ 
int32u emberAfPriceServerSecondsUntilCalorificValueEvent( int8u endpoint );


/**
 * @brief Refreshes caloric value information if necessary.
 * If the second calorific value event is active, the first is inactivated and the array is re-sorted.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPriceServerRefreshCalorificValue( int8u endpoint );


/**
 * @brief Clears the calorific value table and invalidates all entries.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPluginPriceServerCalorificValueClear( int8u endpoint );

/**
 * @brief Sends a Publish Tariff Information command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param tariffIndex The index of the tariff table whose data will be used in the Publish Tariff Information command.
 *
 **/
void emberAfPluginPriceServerPublishTariffMessage(EmberNodeId nodeId,
                                                  int8u srcEndpoint,
                                                  int8u dstEndpoint,
                                                  int8u tariffIndex);

/**
 * @brief Prints the data in the conversion factor table.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPrintConversionTable( int8u endpoint );

/**
 * @brief Prints the data in the calorific values table.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPrintCalorificValuesTable( int8u endpoint );

/**
 * @brief Returns the number of seconds until the next CO2 value event will become active.
 *
 * @param endpoint The endpoint in question
 * @return The number of seconds until the next CO2 value event becomes active.
 *
 **/
int32u emberAfPriceServerSecondsUntilCO2ValueEvent( int8u endpoint );


/**
 * @brief Refreshes CO2 value information if necessary.
 * If the second CO2 value event is active, the first is inactivated and the array is re-sorted.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPriceServerRefreshCO2Value( int8u endpoint );


/**
 * @brief Sets values in the Co2 Value table.
 *
 * @param endpoint The endpoint in question
 * @param issuerEventId The event ID of the Co2 value table data.
 * @param startTime The time at which the Co2 value data is valid.
 * @param providerId A unique identifier for the commodity provider.
 * @param tariffType Bitmap identifying the type of tariff published in this command
 * @param co2Value Used to calculate the amount of carbon dioxide produced from energy use.
 * @param co2ValueUnit Enum which defines the unit of the co2Value attribute.
 * @param co2ValueTrailingDigit Determines where the decimal point is located in the co2Value.
 *
 **/
void emberAfPluginPriceServerCo2ValueAdd(int8u endpoint,
                                         int32u issuerEventId,
                                         int32u startTime,
                                         int32u providerId,
                                         int8u tariffType,
                                         int32u co2Value,
                                         int8u co2ValueUnit,
                                         int8u co2ValueTrailingDigit);
 
/**
 * @brief Clears the Co2 value table and invalidates all entries
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPluginPriceServerCo2ValueClear( int8u endpoint );

/**
 * @brief Prints the data in the CO2 values table
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPrintCo2ValuesTable( int8u endpoint );

/**
 * @brief Sends a Publish CO2 Value command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param index The index of the CO2 values table whose data will be used in the command.
 *
 **/
void emberAfPluginPriceServerCo2LabelPub(int16u nodeId,
                                          int8u srcEndpoint,
                                          int8u dstEndpoint,
                                          int8u index);

/**
 * @brief Sets values in the Tier Label table.
 *
 * @param endpoint The endpoint in question
 * @param index The index of the billing period table whose data will be modified.
 * @param valid Indicates if the data at this index is valid or not.
 * @param providerId A unique identifier for the commodity provider.
 * @param issuerEventId The event ID of the tier labels table data.
 * @param issuerTariffId Unique identifier that identifies which tariff the labels apply to.
 * @param tierId The tier number that associated tier label applies to.
 * @param tierLabel Character string descriptor for this tier.
 *
 **/
void emberAfPluginPriceServerTierLabelSet(int8u  endpoint,
                                          int8u  index,
                                          int8u  valid,
                                          int32u providerId,
                                          int32u issuerEventId,
                                          int32u issuerTariffId,
                                          int8u tierId,
                                          int8u* tierLabel);
void emberAfPluginPriceServerTierLabelPub(int16u nodeId,
                                          int8u srcEp,
                                          int8u dstEp,
                                          int8u index);
void emberAfPrintPrintTierLabelsTable(void);

/**
 * @brief Adds a tier label to the specified tier label table.
 *
 * @param endpoint The endpoint in question
 * @param issuerTariffId Unique identifier that identifies which tariff the labels apply to.
 * @param tierId The tier number that associated tier label applies to.
 * @param tierLabel Character string descriptor for this tier.
 *
 **/
void emberAfPluginPriceServerTierLabelAddLabel( int8u endpoint,
                                                int32u issuerTariffId, 
                                                int8u tierId, 
                                                int8u *tierLabel );

/**
 * @brief Prints the tier labels table.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPrintTierLabelsTable( int8u endpoint );

/**
 * @brief Sends a Publish Tier Labels command
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param index The index of the tier labels table whose data will be used in the Publish Tier Labels command.
 *
 **/
void emberAfPluginPriceServerTierLabelPub(int16u nodeId, int8u srcEndpoint, int8u dstEndpoint, int8u index );


/**
 * @brief Returns the number of seconds until the next billing period event will become active.
 *
 * @param endpoint The endpoint in question
 * @return The number of seconds until the next billing period event becomes active.
 *
 **/
int32u emberAfPriceServerSecondsUntilBillingPeriodEvent( int8u endpoint );


/**
 * @brief Refreshes billing period information if necessary.
 * If the second billing period event is active, the first is inactivated and the array is re-sorted.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPriceServerRefreshBillingPeriod( int8u endpoint );



/**
 * @brief Sets values in the billing period table.
 *
 * @param endpoint The endpoint in question
 * @param startTime The time at which the billing period data is valid.
 * @param issuerEventId The event ID of the billing period data.
 * @param providerId A unique identifier for the commodity provider.
 * @param billingPeriodDuration The billing period duration.  Units are specified by the billingPeriodDurationType.
 * @param billingPeriodDurationType A bitmap that indicates the units used in the billing period.
 * @param tariffType Bitmap identifying the type of tariff published in this command.
 *
 **/
EmberStatus emberAfPluginPriceServerBillingPeriodAdd(int8u endpoint,
                                                     int32u startTime,
                                                     int32u issuerEventId,
                                                     int32u providerId,
                                                     int32u billingPeriodDuration,
                                                     int8u billingPeriodDurationType,
                                                     int8u tariffType);
/**
 * @brief Semds a Publish Billing Period command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param index The index of the table whose data will be used in the command.
 *
 **/
void emberAfPluginPriceServerBillingPeriodPub(int16u nodeId,  int8u srcEndpoint,
                                              int8u dstEndpoint, int8u index);

/**
 * @brief Prints the data in the billing period table for the specified endpoint.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPrintBillingPeriodTable( int8u endpoint );


/**
 * @brief Prints the data in the consolidated bills table at the specified index.
 *
 * @param endpoint The endpoint in question
 * @param index The index of the consolidated bills table whose data should be printed.
 *
 **/
void emberAfPrintConsolidatedBillTableEntry( int8u endpoint, int8u index );


/**
 * @brief Sets values in the consolidated bills table.
 *
 * @param endpoint The endpoint in question
 * @param startTime The time at which the consolidated bills data is valid.
 * @param issuerEventId The event ID of the consolidated bills data.
 * @param providerId A unique identifier for the commodity provider.
 * @param billingPeriodDuration The billing period duration.  Units are specified by the billingPeriodDurationType.
 * @param billingPeriodDurationType A bitmap that indicates the units used in the billing period.
 * @param tariffType Bitmap identifying the type of tariff published in this command.
 * @param consolidatedBill The conosolidated bill value for the specified billing period.
 * @param currency The currency used in the consolidatedBill field.
 * @param billTrailingDigit Determines where the decimal point is located in the consolidatedBill field.
 *
 **/
void emberAfPluginPriceServerConsolidatedBillAdd( int8u endpoint, int32u startTime,
                                                  int32u issuerEventId, int32u providerId,
                                                  int32u billingPeriodDuration, int8u billingPeriodDurationType,
                                                  int8u tariffType, int32u consolidatedBill,
                                                  int16u currency, int8u billTrailingDigit );

/**
 * @brief Sends a Publish Consolidated Bill command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param index The index of the table whose data will be used in the command.
 *
 **/
void emberAfPluginPriceServerConsolidatedBillPub( int16u nodeId, int8u srcEndpoint, int8u dstEndpoint, int8u index );


/**
 * @brief Sets values of the Cpp Event.
 *
 * @param endpoint The endpoint in question.
 * @param valid Indicates if the Cpp Event data is valid or not.
 * @param providerId A unique identifier for the commodity provider.
 * @param issuerEventId The event ID of the Cpp Event.
 * @param startTime The time at which the Cpp Event data is valid.
 * @param durationInMinutes Defines the duration of the Cpp Event.
 * @param tariffType Bitmap identifying the type of tariff published in this command.
 * @param cppPriceTier Indicates which CPP price tier should be used for the event.
 * @param cppAuth The status of the CPP event.
 *
 **/
void emberAfPluginPriceServerCppEventSet( int8u endpoint, int8u valid, int32u providerId, int32u issuerEventId, int32u startTime,
                                          int16u durationInMinutes, int8u tariffType, int8u cppPriceTier, int8u cppAuth );

/**
 * @brief Sends a Publish CPP Event command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 *
 **/
void emberAfPluginPriceServerCppEventPub( int16u nodeId, int8u srcEndpoint, int8u dstEndpoint );

/**
 * @brief Prints the data in the CPP Event.
 *
 * @param endpoint The endpoint in question
 *
 **/
void emberAfPluginPriceServerCppEventPrint( int8u endpoint );

/**
 * @brief Sends a Publish Credit Payment command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 * @param index The index of the table whose data will be used in the command.
 *
 **/
void emberAfPluginPriceServerCreditPaymentPub(int16u nodeId, int8u srcEndpoint, int8u dstEndpoint, int8u index);

/**
 * @brief Sets values in the credit payment table.
 *
 * @param endpoint The endpoint in question
 * @param index The index of the credit payment table whose data will be modified.
 * @param valid Indicates if the data at this index is valid or not.
 * @param providerId A unique identifier for the commodity provider.
 * @param issuerEventId The event ID of the credit payment data.
 * @param creditPaymentDueDate The time the next credit payment is due.
 * @param creditPaymentOverdueAmount The current amount that is overdue from the customer.
 * @param creditPaymentStatus Indicates the current credit payment status.
 * @param creditPayment The amount of the last credit payment.
 * @param creditPaymentDate The time at which the last credit payment was made.
 * @param creditPaymentRef A string used to denote the last credit payment reference used by the energy supplier.
 *
 **/
void emberAfPluginPriceServerCreditPaymentSet( int8u endpoint, int8u index, int8u valid, 
                                               int32u providerId, int32u issuerEventId,
                                               int32u creditPaymentDueDate, int32u creditPaymentOverdueAmount,
                                               int8u creditPaymentStatus, int32u creditPayment, 
                                               int32u creditPaymentDate, int8u *creditPaymentRef );

//void emberAfPluginPriceServerCreditPaymentPrint( void );

/**
 * @brief Sends a Publish Currency Conversion command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 *
 **/
void emberAfPluginPriceServerCurrencyConversionPub( int16u nodeId, int8u srcEndpoint, int8u dstEndpoint );

/**
 * @brief Sets values for the Currency Conversion command.
 *
 * @param endpoint The endpoint in question
 * @param valid Indicates if the currency conversion data is valid or not.
 * @param providerId A unique identifier for the commodity provider.
 * @param issuerEventId The event ID of the currency conversion data.
 * @param startTime The time at which the currency conversion data is valid.
 * @param oldCurrency Information about the old unit of currency.
 * @param newCurrency Information about the new unit of currency.
 * @param conversionFactor Accounts for changes in the volume of gas based on temperature and pressure.
 * @param conversionFactorTrailingDigit Determines where the decimal point is located in the conversion factor.
 * @param currencyChangeControlFlags Denotes functions that are required to be carried out by the client.
 *
 **/
void emberAfPluginPriceServerCurrencyConversionSet( int8u endpoint, int8u valid, 
                                                    int32u providerId, int32u issuerEventId,
                                                    int32u startTime, int16u oldCurrency, int16u newCurrency,
                                                    int32u conversionFactor, int8u conversionFactorTrailingDigit,
                                                    int32u currencyChangeControlFlags );

/**
 * @brief Sets values in the tariff cancellation command.
 *
 * @param endpoint The endpoint in question
 * @param valid Indicates if the tariff cancellation command is valid or not.
 * @param providerId A unique identifier for the commodity provider.
 * @param issuerTariffId Unique identifier that identifies which tariff should be cancelled.
 * @param tariffType Bitmap identifying the type of tariff to be cancelled.
 *
 **/
void emberAfPluginPriceServerTariffCancellationSet( int8u endpoint, int8u valid, int32u providerId, 
                                                    int32u issuerTariffId, int8u tariffType );

/**
 * @brief Sends a Cancel Tariff command.
 *
 * @param nodeId The destination address to which the command should be sent.
 * @param srcEndpoint The source endpoint used in the transmission.
 * @param dstEndpoint The destination endpoint used in the transmission.
 *
 **/
void emberAfPluginPriceServerTariffCancellationPub( int16u nodeId, int8u srcEndpoint, int8u dstEndpoint );

int32u emberAfPriceServerSecondsUntilTariffInfoEvent(int8u endpoint);

void emberAfPriceServerRefreshTariffInformation(int8u endpoint);

boolean emberAfPriceAddTariffTableEntry(int8u endpoint,
                                        EmberAfPriceCommonInfo *curInfo, 
                                        const EmberAfScheduledTariff *curTariff);


boolean emberAfPriceAddPriceMatrixRaw(int8u endpoint,
                                      int32u providerId,
                                      int32u issuerEventId,
                                      int32u startTime,
                                      int32u issuerTariffId,
                                      int8u commandIndex,
                                      int8u numberOfCommands,
                                      int8u subPayloadControl,
                                      int8u* payload);

boolean emberAfPriceAddPriceMatrix(int8u endpoint,
                                   EmberAfPriceCommonInfo * inf,
                                   EmberAfScheduledPriceMatrix * pm);

boolean emberAfPriceAddBlockThresholdsTableEntry(int8u endpoint,
                                                 int32u providerId,
                                                 int32u issuerEventId,
                                                 int32u startTime,
                                                 int32u issuerTariffId,
                                                 int8u commandIndex,
                                                 int8u numberOfCommands,
                                                 int8u subpayloadControl,
                                                 int8u* payload);

void emberAfPriceClearBlockPeriodTable(int8u endpoint);
void sendValidCmdEntries(int8u cmdId,
                         int8u ep,
                         int8u* validEntries,
                         int8u validEntryCount);
void emberAfPluginPriceServerPriceUpdateBindings(void);
