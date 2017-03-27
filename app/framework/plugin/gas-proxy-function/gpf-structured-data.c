// *******************************************************************
// * gpf-structured-data.c
// *
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/gbz-message-controller/gbz-message-controller.h"
#include "app/framework/plugin/gas-proxy-function/gas-proxy-function.h"
#include "app/framework/plugin/events-server/events-server.h"

/**
 * This file adds support for the GPF Structured Data Items as defined in the
 * GBCS v0.8.1 section 10.4.
 *
 * Section 10.4.1: There are GPF requirements to store structured data items
 * which do not have a direct one to one mapping in ZSE, or the interpretation
 * may be uncertain.  These structured data items have to be constructed by
 * the GPF.
 */

#define GPF_INVALID_LOG_INDEX                       0xFF
#define GPF_UNUSED_ENDPOINT_ID                      0xFF
#define GPF_INVALID_ENTRY_INDEX                     0xFFFF

#define GPF_DAILY_CONSUMPTION_LOG_SAMPLE_ID         0x0000
#define GPF_PROFILE_DATA_LOG_SAMPLE_ID              0x0001

#define GPF_SNAPSHOT_CAUSE_GENERAL                  0x00000001
#define GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD    0x00000002
#define GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF         0x00000008
#define GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER       0x00002000
#define GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE   0x00004000

/*
 * The two types of snapshot payload's that we support as defined by GBCS are
 *
 * - EMBER_ZCL_SNAPSHOT_PAYLOAD_TYPE_TOU_INFORMATION_SET_DELIVERED_REGISTERS
 *
 *   The format of this payload consists of a set of fixed lengths fields
 *   totaling 24 bytes and the tier summation registers, each of which is 6
 *   bytes in length. The tier summation registers represent the "Tariff TOU
 *   Register Matrix" which is defined by SMETS as a 1 x 4 matrix. Therefore,
 *   the total expected length of snapshots with this payload type is
 *   24 + (4 * 6) or 48 bytes.
 *
 * - EMBER_ZCL_SNAPSHOT_PAYLOAD_TYPE_BLOCK_TIER_INFORMATION_SET_DELIVERED
 *
 *   The format of this payload consists of a set of fixed length fields
 *   totaling 25 bytes and two variable lengths fields that will include
 *   the tier summation registers and the tier block summation registers. The
 *   length of these registers are 6 bytes. There are 4 tier summation registers
 *   as defined above and there will also be 4 tier block summation registers.
 *   The tier block summation registers represent the "Tariff Block Counter Matrix"
 *   which is defined as a 4 x 1 matrix. Therefore, the total expected length
 *   of snapshots with this payload type is 25 + (4 * 6) + (4 * 6) or 73 bytes.
 *
 * The types were obtained by looking at the response in the use case
 * description for GCS16a then also looking at the description of the billing
 * data log in GBCS v0.8.1 section 10.4.2.4 and comparing it to the description
 * of the daily read log in the SMETS v1.58 section 4.4.94.
 *
 * The following define is the max of the above payload lengths.
 */
#define GPF_MAX_SNAPSHOT_PAYLOAD_SIZE               73

// There is only one prepay snapshot payload type defined by the SE spec.
//
// - EMBER_ZCL_PREPAY_SNAPSHOT_PAYLOAD_TYPE_DEBT_CREDIT_STATUS
//
//   The format of this payload consists of a set of fixed lengths fields
//   totaling 24 bytes.
#define GPF_MAX_PREPAY_SNAPSHOT_PAYLOAD_SIZE        24

#define fieldLength(field) (emberAfCurrentCommand()->bufLen - (field - emberAfCurrentCommand()->buffer))
#define getFirstIndex(log) ((log->numberOfEntries < log->maxEntries) ? 0 : log->nextEntry)
#define getNextIndex(log, index) ((index == (log->maxEntries-1)) ? 0 : (index+1))
#define getPrevIndex(log, index) ((index == 0) ? (log->maxEntries-1) : (index-1))
#define getLastIndex(log) ((log->numberOfEntries == 0) ? 0 : getPrevIndex(log, log->nextEntry))
#define incNumberOfEntries(log) if (log->numberOfEntries < log->maxEntries) { log->numberOfEntries++; }

#define PREV_MIDNIGHT(utc) (((utc) / SECONDS_IN_DAY) * SECONDS_IN_DAY)
#define NEXT_MIDNIGHT(utc) PREV_MIDNIGHT(utc + SECONDS_IN_DAY)
#define SECONDS_IN_HALF_HOUR 1800
#define PREV_HALF_HOUR(utc) (((utc) / SECONDS_IN_HALF_HOUR) * SECONDS_IN_HALF_HOUR)
#define NEXT_HALF_HOUR(utc) PREV_HALF_HOUR(utc + SECONDS_IN_HALF_HOUR)

enum {
  GPF_FLAGS_SENT = 0x01,
};

typedef struct {
  int32u time;
  int32u sample;
} GpfSampleEntry;

typedef struct {
  int32u time;
  int32u snapshotId;
  int32u snapshotCause;
  int8u  snapshotPayloadType;
  int16u snapshotPayloadSize;
  int8u snapshotPayload[GPF_MAX_SNAPSHOT_PAYLOAD_SIZE];
} GpfSnapshotEntry;

typedef struct {
  int32u time;
  int32u snapshotId;
  int32u snapshotCause;
  int8u  snapshotPayloadType;
  int16u snapshotPayloadSize;
  int8u snapshotPayload[GPF_MAX_PREPAY_SNAPSHOT_PAYLOAD_SIZE];
} GpfPrepaySnapshotEntry;

typedef struct {
  int8u flags;
  int8u code[26];
  int32s amount;
  int32u time;
} GpfTopUpEntry;

typedef struct {
  int8u flags;
  int32u collectionTime;
  int32u amountCollected;
  int32u outstandingDebt;
  int8u  debtType;
} GpfDebtEntry;

typedef struct {
  boolean catchup;
  int8u catchupSequenceNumber;
  int16u nextEntry;
  int16u numberOfEntries;
  int16u maxEntries;
  GpfSampleEntry *entries;
  int16u sampleId;
  int16u sampleInterval;
  int32u startTime;
  int32u prevSummation;

} GpfSampleLog;

typedef struct {
  boolean catchup;
  int8u catchupSequenceNumber;
  int8u catchupSnapshotOffset;
  int32u catchupSnapshotCause;
  int16u nextEntry;
  int16u numberOfEntries;
  int16u maxEntries;
  GpfSnapshotEntry *entries;
} GpfSnapshotLog;

typedef struct {
  boolean catchup;
  int8u catchupSequenceNumber;
  int8u catchupSnapshotOffset;
  int32u catchupSnapshotCause;
  int16u nextEntry;
  int16u numberOfEntries;
  int16u maxEntries;
  GpfPrepaySnapshotEntry *entries;
} GpfPrepaySnapshotLog;

typedef struct {
  boolean catchup;
  int8u catchupSequenceNumber;
  int16u nextEntry;
  int16u numberOfEntries;
  int16u maxEntries;
  GpfTopUpEntry *entries;
} GpfTopUpLog;

typedef struct {
  boolean catchup;
  int8u catchupSequenceNumber;
  int16u nextEntry;
  int16u numberOfEntries;
  int16u maxEntries;
  GpfDebtEntry *entries;
} GpfDebtLog;

typedef struct {
  GpfSnapshotLog snapshot;
  GpfTopUpLog topUp;
  GpfDebtLog debt;
  GpfPrepaySnapshotLog prepaySnapshot;
} GpfBillingDataLog;

typedef struct {
  boolean catchup;
  int32u prevCurrentDayAlternativeConsumption;
  int32u prevCurrentDayAlternativeConsumptionTime;
} GpfAlternativeHistoricalConsumption;

typedef struct {
  boolean catchup;
  int32u prevCurrentDayCostConsumption;
  int32u prevCurrentDayCostConsumptionTime;
} GpfHistoricalCostConsumption;

typedef struct {
  EmberEUI64 deviceIeeeAddress;
  int8u endpoint;
  GpfSnapshotLog dailyReadLog;
  GpfPrepaySnapshotLog prepayDailyReadLog;
  GpfBillingDataLog billingDataLog;
  GpfSampleLog profileDataLog;
  GpfSampleLog dailyConsumptionLog;
  int8u remoteEndpoint;
  EmberNodeId remoteNodeId;
  int32u functionalNotificationFlags;
  int32u notificationFlags4;
  int32u lastAttributeReportTime;
  GpfAlternativeHistoricalConsumption alternativeHistoricalConsumption;
  GpfHistoricalCostConsumption historicalCostConsumption;
} GpfStructuredData;

static GpfStructuredData structuredData[EMBER_AF_PLUGIN_METER_MIRROR_MAX_MIRRORS];

// Keep track of the logs currently in "catchup" mode.  This is only used for
// printing when catchup starts and when its ends.  Its really just a debugging
// aid and can be removed if desired.
static int8u logCatchupsInProgress = 0;

// Event used to handle work when GetNotifiedMessage is received in response
// to notification flags being set when catchup is started on any of the logs.
EmberEventControl emberAfPluginGasProxyFunctionCatchupEventControl;

//------------------------------------------------------------------------------
// Storage for the various logs making up the structured data.

/**
 * GBCS v0.8.1 section 10.4.2.1
 *
 * The GSME shall record the Daily Read Log data items at midnight UTC as
 * defined in SMETS.  In ZSE terms, the GSME shall take a snapshot of the
 * relevant items.  Note that the format and data of the snapshot taken is
 * dependent upon the operating tariff.  For example if the GSME tariff is
 * ‘TOU only’, the snapshot shall not capture the block values.
 *
 * The GSME shall use the snapshot cause ‘General’ (0x0001) for the snapshot
 * taken.
 *
 * The GSME shall push the snapshot to the GPF using the PublishSnapshot
 * command. It is not necessary for the GSME to report any attributes which
 * duplicate those contained in the snapshot.
 *
 * The GPF shall populate the relevant attributes upon receipt of the
 * PublishSnapshot command, providing the command is received between midnight
 * (UTC) and the next scheduled wake of the GSME.
 *
 * The GPF shall store the data contained in the PublishSnapshot command in
 * the GPF copy of the GSME Daily Read Log.
 *
 * In the event of a communications outage, the GPF shall retrieve missing
 * snapshots using the GetSnapshot command, with the UTC start time field
 * populated based on the last received snapshot timestamp, if one has been
 * received.
 */
static GpfSnapshotEntry dailyReadLogEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_DAILY_READ_LOG_ENTRIES];

/**
 * GBCS v0.8.1 section 10.4.2.2
 *
 * If the GSME is operating in prepayment mode it shall record the Prepayment
 * Daily Read Log data items at midnight UTC.  In ZSE terms, the GSME shall
 * take a prepayment snapshot of the relevant items.  The format and data of
 * the prepayment snapshot taken is defined in ZSE.
 *
 * The GSME shall use the snapshot cause ‘General’ (0x0001) for the prepayment
 * snapshot taken.
 *
 * The GSME shall push the prepayment snapshot to the GPF using the Publish
 * Prepay Snapshot command.
 *
 * The GPF shall populate the relevant attributes upon receipt of the Publish
 * Prepay Snapshot command, providing the command is received between midnight
 * (UTC) and the next scheduled wake of the GSME.
 *
 * The GPF shall store the data contained in the Publish Prepay Snapshot
 * command in the GPF copy of the GSME Prepayment Daily Read Log.
 *
 * In the event of a communications outage, the GPF shall retrieve missing
 * prepayment snapshots using the GetPrepaySnapshot command (and
 * GetPrepaySnapshot notification flag) with the UTC start time field
 * populated based on the last received prepayment snapshot timestamp, if one
 * has been received.
 */
static GpfPrepaySnapshotEntry prepayDailyReadLogEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_PREPAYMENT_DAILY_READ_LOG_ENTRIES];

/**
 * GBCS v0.8.1 section 10.4.2.3
 *
 * SMETS defines Billing Data Log as ‘a log capable of storing the following UTC
 * date and time stamped entries:
 *   - twelve entries comprising Tariff TOU Register Matrix,
 *     the Consumption Register and Tariff Block Counter Matrix;
 *   - five entries comprising the value of prepayment credits;
 *   - ten entries comprising the value of payment-based debt payments; and
 *   - twelve entries comprising Meter Balance, Emergency Credit Balance,
 *     Accumulated Debt Register, Payment Debt Register and
 *     Time Debt Registers [1 … 2].
 */
#define EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_SNAPSHOT_ENTRIES 12
#define EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_TOP_UP_ENTRIES 5
#define EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_DEBT_ENTRIES 10
#define EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_PREPAY_SNAPSHOT_ENTRIES 12

/**
 * GBCS v0.8.1 section 10.4.2.4
 *
 * The GSME shall capture this snapshot at the following trigger points:
 *   - End of Billing Cycle (snapshot cause “End of Billing Period” );
 *   - Change of Payment Mode (snapshot cause “Change of Meter Mode”);
 *   - Change of Tariff (snapshot cause ‘Change of Tariff Information’); and
 *   - as specified in Section 13.3.5.10 (snapshot cause ‘Change of Supplier’).
 *
 * When it next turns on its HAN Interface, the GSME shall push this snapshot
 * to the GPF using the PublishSnapshot Command.
 *
 * The GPF shall store the data contained in the PublishSnapshot command in
 * the GPF copy of the GSME Billing data Log.
 *
 * In the event of a communications outage, the GPF shall retrieve missing
 * snapshots using the GetSnapshot command (and the relevant notification
 * flag) with the UTC start time field populated based on the last received
 * snapshot timestamp, if one has been received, or 0x0000 otherwise.
 */
static GpfSnapshotEntry billingDataLogSnapshotEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_SNAPSHOT_ENTRIES];

/**
 * GBCS v0.8.1 section 10.4.2.5
 *
 * Upon completion of processing of a valid prepayment top-up, the GSME shall
 * push the latest five prepayment top-ups to the GPF using the PublishTopUpLog
 * command.
 *
 * The GPF shall store the data contained in the PublishTopUpLog command in the
 * GPF copy of the GSME Billing data Log.
 *
 * If there has been a communications outage, the GPF shall use the GetTopUp
 * Log command to retrieve all prepayment top-ups that may have been processed
 * during the communications outage.  The GSME shall set the Date/Time field of
 * the GetTopUp Log command to the current UTC time.
 */
static GpfTopUpEntry billingDataLogTopUpEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_TOP_UP_ENTRIES];

/**
 * GBCS v0.8.1 section 10.4.2.6
 *
 * Upon completion of processing of a valid prepayment top-up where the GSME has
 * made a debt payment using part of that top-up, the GSME shall push details of
 * that debt payment only to the GPF using the PublishDebtLog command.
 *
 * The GPF shall record the details provided in the GPF copy of the GSME Billing
 * Data Log.
 *
 * In cases of communications outages, the GPF shall request any outstanding
 * payment-based debt payments by use of the GetDebtRepaymentLog command (and
 * GetDebtRepaymentLog notification flag) with the Debt Type field set to
 * 0x02 (Debt 3).
 */
static GpfDebtEntry billingDataLogDebtEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_DEBT_ENTRIES];

/**
 * GBCS v0.8.1 section 10.4.2.7
 *
 * The GSME shall capture this snapshot at the following trigger points:
 *   - End of Billing Cycle (snapshot cause bit 1 set: ‘End of Billing Period’,
 *     as per PublishSnapshot command);
 *   - Change of Payment Mode (snapshot cause bit 14 set: ‘Change of Meter Mode’);
 *   - Change of Tariff (snapshot cause at least one of the bits set: bit 3
 *     ‘Change of Tariff Information’ and / or bit 4 'Change of Price Matrix'
 *     and / or bit 5 'Change of Block Thresholds'); and
 *   - as specified in Section 13.3.5.10 (snapshot cause ‘Change of Supplier’).
 *
 * When it next turns on its HAN Interface, the GSME shall push this snapshot
 * to the GPF using the Publish Prepay Snapshot command.
 *
 * The GPF shall store the data contained in the Publish Prepay Snapshot command
 * in the GPF copy of the GSME Billing Data Log.
 *
 * In the event of a communications outage, the GPF shall retrieve missing
 * snapshots using the GetPrepaySnapshot command (and GetPrepaySnapshot
 * notification flag) with the UTC start time field populated based on the last
 * received snapshot timestamp, if one has been received.
 */
static GpfPrepaySnapshotEntry billingDataLogPrepaySnapshotEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_PREPAY_SNAPSHOT_ENTRIES];

/**
  * GBCS v0.8.1 section 10.4.2.8
  *
  * The GPF shall create the GPF Profile Data Log from the consumption
  * information pushed by the GSME each half hour.
  *
  * The GSME shall, on each half hour, record the following information and
  * push to the GPF:
  *
  *    - the CurrentSummationDelivered attribute containing total consumption
  *      value (with units of m3);
  *    - the CurrentDayAlternative ConsumptionDelivered attribute containing
  *      total consumption today (with units of kWh); and
  *    - the CurrentDayCostConsumptionDelivered attribute containing total cost
  *      of consumption today (with units of Currency Unit);
  *
  * Upon receipt of the pushed data, the GPF shall calculate the consumption
  * with units of m3 over the previous half hour by subtracting its previously
  * recorded total consumption value from the total consumption value now sent.
  *
  * The resulting value shall be stored in the GPF Profile Data Log.
  *
  * In the event that there are missing values in the GPF Profile Data Log,
  * the GPF shall interrogate the GSME Profile Data Log using the
  * GetSampledData (SampleID 0x0000) command and the GetSampledData
  * notification flag to retrieve missing values.
  */
static GpfSampleEntry profileDataLogEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_PROFILE_DATA_LOG_ENTRIES];

/**
  * GBCS v0.8.1 section 10.4.2.9
 *
 * The GPF shall create the GPF Daily Gas Consumption Log based on the values
 * pushed from the GSME.  The difference between last total consumption value
 * pushed from the GSME each UTC day and the last value pushed in the prior
 * UTC day shall be time stamped and stored in the GPF Daily Gas Consumption
 * Log, so that the values in the log represent consumption in that UTC day.
 *
 * In the event of communications outages resulting in the final daily value
 * being missed, the GPF shall retrieve the values from the GSME Profile Data
 * Log using the GetSampledData (SampleID 0x0000) command and GetSampledData
 * notification flag.
 */
static GpfSampleEntry dailyConsumptionLogEntries[EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_DAILY_CONSUMPTION_LOG_ENTRIES];

/*
 * GBCS v0.8.1 section 10.4.2.10
 *
 * As per Section 10.4.2.8, the GSME shall, on each half hour, record the
 * following information and push to the GPF:
 *   - total consumption value (with units of m3);
 *   - total consumption today (with units of kWh); and
 *   - total cost of consumption today (with units of Currency Unit);
 * Using the "total consumption today" value, the GPF shall update the
 * attributes of the mirrored Alternative Historical Consumption attribute set.
 */
static int16u altConsumptionDayAttrIds[] = {
  ZCL_CURRENT_ALTERNATIVE_DAY_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY2_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY3_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY4_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY5_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY6_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY7_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY8_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
};
#define NUM_ALT_CONSUMPTION_DAY_ATTRS (sizeof(altConsumptionDayAttrIds) / sizeof(altConsumptionDayAttrIds[0]))

static int16u altConsumptionWeekAttrIds[] = {
  ZCL_CURRENT_WEEK_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK2_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK3_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK4_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK5_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
};
#define NUM_ALT_CONSUMPTION_WEEK_ATTRS (sizeof(altConsumptionWeekAttrIds) / sizeof(altConsumptionWeekAttrIds[0]))

static int16u altConsumptionMonthAttrIds[] = {
  ZCL_CURRENT_MONTH_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH2_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH3_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH4_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH5_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH6_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH6_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH8_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH9_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH10_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH11_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH12_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH13_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
};
#define NUM_ALT_CONSUMPTION_MONTH_ATTRS (sizeof(altConsumptionMonthAttrIds) / sizeof(altConsumptionMonthAttrIds[0]))

/*
 * GBCS v0.8.1 section 10.4.2.10
 *
 * As per Section 10.4.2.8, the GSME shall, on each half hour, record the
 * following information and push to the GPF:
 *   - total consumption value (with units of m3);
 *   - total consumption today (with units of kWh); and
 *   - total cost of consumption today (with units of Currency Unit);
 * Using the ‘total cost of consumption today’ value, the GPF shall update the
 * attributes of the mirrored Historical Cost Consumption Information
 * attribute set.
 */
static int16u costConsumptionDayAttrIds[] = {
  ZCL_CURRENT_DAY_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_2_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_3_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_4_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_5_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_6_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_7_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_DAY_8_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
};
#define NUM_COST_CONSUMPTION_DAY_ATTRS (sizeof(costConsumptionDayAttrIds) / sizeof(costConsumptionDayAttrIds[0]))

static int16u costConsumptionWeekAttrIds[] = {
  ZCL_CURRENT_WEEK_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK_2_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK_3_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK_4_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_WEEK_5_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
};
#define NUM_COST_CONSUMPTION_WEEK_ATTRS (sizeof(costConsumptionWeekAttrIds) / sizeof(costConsumptionWeekAttrIds[0]))

static int16u costConsumptionMonthAttrIds[] = {
  ZCL_CURRENT_MONTH_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_2_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_3_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_4_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_5_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_6_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_7_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_8_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_9_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_10_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_11_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_12_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
  ZCL_PREVIOUS_MONTH_13_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
};
#define NUM_COST_CONSUMPTION_MONTH_ATTRS (sizeof(costConsumptionMonthAttrIds) / sizeof(costConsumptionMonthAttrIds[0]))

//------------------------------------------------------------------------------
// Internal Functions
static int8u findStructuredDataByEUI64(const EmberEUI64 deviceIeeeAddress)
{
  int8u i;

  for (i=0; i<EMBER_AF_PLUGIN_METER_MIRROR_MAX_MIRRORS; i++) {
    if (MEMCOMPARE(deviceIeeeAddress,structuredData[i].deviceIeeeAddress, EUI64_SIZE) == 0) {
      return i;
    }
  }

  return GPF_INVALID_LOG_INDEX;
}

static int8u findStructuredData(int8u endpoint)
{
  int8u i;

  for (i=0; i<EMBER_AF_PLUGIN_METER_MIRROR_MAX_MIRRORS; i++) {
    if (structuredData[i].endpoint == endpoint) {
      return i;
    }
  }

  return GPF_INVALID_LOG_INDEX;
}

static void setNotificationFlag(int8u endpoint, int16u attrId, int32u flag)
{
  EmberAfStatus status;
  int32u notificationFlags;

  status = emberAfReadClientAttribute(endpoint,
                                      ZCL_SIMPLE_METERING_CLUSTER_ID,
                                      attrId,
                                      (int8u *)&notificationFlags,
                                      4);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("Unable to read notification flags attribute 0x%2x: status 0x%x", attrId, status);
    return;
  }

  notificationFlags |= flag;
  status = emberAfWriteClientAttribute(endpoint,
                                       ZCL_SIMPLE_METERING_CLUSTER_ID,
                                       attrId,
                                       (int8u *)&notificationFlags,
                                       ZCL_BITMAP32_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("Unable to write notification flags attribute 0x%2x: status 0x%x", attrId, status);
    return;
  }
}

static void clearNotificationFlag(int8u endpoint, int16u attrId, int32u flag)
{
  EmberAfStatus status;
  int32u notificationFlags;

  status = emberAfReadClientAttribute(endpoint,
                                      ZCL_SIMPLE_METERING_CLUSTER_ID,
                                      attrId,
                                      (int8u *)&notificationFlags,
                                      4);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("Unable to read notification flags attribute 0x%2x: status 0x%x", attrId, status);
    return;
  }

  notificationFlags &= ~flag;
  status = emberAfWriteClientAttribute(endpoint,
                                       ZCL_SIMPLE_METERING_CLUSTER_ID,
                                       attrId,
                                       (int8u *)&notificationFlags,
                                       ZCL_BITMAP32_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("Unable to write notification flags attribute 0x%2x: status 0x%x", attrId, status);
    return;
  }
}

static void sendSampleData(int32u earliestSampleTime,
                           int16u numberOfSamples,
                           GpfSampleLog *sampleLog)
{
  int16u i;
  int16u entryIndex;
  int16u totalSamples = 0;
  int16u includedSamples = 0;

  // First run through the samples and calculate the total number of samples
  // matching the given criteria.
  for (i = 0, entryIndex = getFirstIndex(sampleLog);
       i < sampleLog->numberOfEntries && totalSamples < numberOfSamples;
       i++, entryIndex = getNextIndex(sampleLog, entryIndex)) {
    if (sampleLog->entries[entryIndex].time >= earliestSampleTime) {
      totalSamples++;
    }
  }

  // loop through all the entries looking for the entries that are greater than
  // or equal to the the given earliest start time.
  for (i = 0, entryIndex = getFirstIndex(sampleLog);
       i < sampleLog->numberOfEntries && includedSamples < totalSamples;
       i++, entryIndex = getNextIndex(sampleLog, entryIndex)) {
    if (sampleLog->entries[entryIndex].time < earliestSampleTime) {
      continue;
    }

    if (includedSamples == 0) {
      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                           | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                          ZCL_SIMPLE_METERING_CLUSTER_ID,
                          ZCL_GET_SAMPLED_DATA_RESPONSE_COMMAND_ID,
                          "vwuvv",
                          sampleLog->sampleId,
                          sampleLog->entries[entryIndex].time,
                          EMBER_ZCL_SAMPLE_TYPE_CONSUMPTION_DELIVERED,
                          sampleLog->sampleInterval,
                          totalSamples);
    }

    emberAfPutInt24uInResp(sampleLog->entries[entryIndex].sample);
    includedSamples++;
  }

  if (includedSamples != 0) {
    emberAfSendResponse();
    return;
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
}

static void sendSnapshot(int32u earliestTime,
                         int32u latestTime,
                         int8u snapshotOffset,
                         int32u snapshotCause,
                         GpfSnapshotLog *snapshotLog)
{
  int16u i;
  int16u entryIndex;
  int8u totalSnapshots = 0;
  int16u skippedSnapshots = 0;

  // First run the the snapshot and calculate the total number of snapshots
  // matching the given criteria.
  for (i = 0, entryIndex = getFirstIndex(snapshotLog);
       i < snapshotLog->numberOfEntries;
       i++, entryIndex = getNextIndex(snapshotLog, entryIndex)) {
    if (((snapshotLog->entries[entryIndex].snapshotCause & snapshotCause) != 0)
        && snapshotLog->entries[entryIndex].time >= earliestTime
        && snapshotLog->entries[entryIndex].time < latestTime) {
      totalSnapshots++;
    }
  }

  // loop through all the entries looking for the entries that are greater than
  // or equal to the the given earliest start time and less than the latest end time.
  for (i = 0, entryIndex = getFirstIndex(snapshotLog);
       i < snapshotLog->numberOfEntries;
       i++, entryIndex = getNextIndex(snapshotLog, entryIndex)) {
     if (((snapshotLog->entries[entryIndex].snapshotCause & snapshotCause) == 0)
        || snapshotLog->entries[entryIndex].time < earliestTime
        || snapshotLog->entries[entryIndex].time >= latestTime) {
      continue;
    }

    if (snapshotOffset != skippedSnapshots) {
      skippedSnapshots++;
      continue;
    }

    emberAfFillCommandSimpleMeteringClusterPublishSnapshot(snapshotLog->entries[entryIndex].snapshotId,
                                                           snapshotLog->entries[entryIndex].time,
                                                           totalSnapshots,
                                                           0,
                                                           1,
                                                           snapshotLog->entries[entryIndex].snapshotCause,
                                                           snapshotLog->entries[entryIndex].snapshotPayloadType,
                                                           snapshotLog->entries[entryIndex].snapshotPayload,
                                                           snapshotLog->entries[entryIndex].snapshotPayloadSize);
    emberAfSendResponse();
    return;
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
}

static void sendPrepaySnapshot(int32u earliestTime,
                               int32u latestTime,
                               int8u snapshotOffset,
                               int32u snapshotCause,
                               GpfPrepaySnapshotLog *prepaySnapshotLog)
{
  int16u i;
  int16u entryIndex;
  int8u totalSnapshots = 0;
  int16u skippedSnapshots = 0;

  // First run the the snapshot and calculate the total number of snapshots
  // matching the given criteria.
  for (i = 0, entryIndex = getFirstIndex(prepaySnapshotLog);
       i < prepaySnapshotLog->numberOfEntries;
       i++, entryIndex = getNextIndex(prepaySnapshotLog, entryIndex)) {
    if (((prepaySnapshotLog->entries[entryIndex].snapshotCause & snapshotCause) != 0)
        && prepaySnapshotLog->entries[entryIndex].time >= earliestTime
        && prepaySnapshotLog->entries[entryIndex].time < latestTime) {
      totalSnapshots++;
    }
  }

  // loop through all the entries looking for the entries that are greater than
  // or equal to the the given earliest start time and less than the latest end time.
  for (i = 0, entryIndex = getFirstIndex(prepaySnapshotLog);
       i < prepaySnapshotLog->numberOfEntries;
       i++, entryIndex = getNextIndex(prepaySnapshotLog, entryIndex)) {
    if (((prepaySnapshotLog->entries[entryIndex].snapshotCause & snapshotCause) == 0)
        || prepaySnapshotLog->entries[entryIndex].time < earliestTime
        || prepaySnapshotLog->entries[entryIndex].time >= latestTime) {
      continue;
    }

    if (snapshotOffset != skippedSnapshots) {
      skippedSnapshots++;
      continue;
    }

    emberAfFillCommandPrepaymentClusterPublishPrepaySnapshot(prepaySnapshotLog->entries[entryIndex].snapshotId,
                                                             prepaySnapshotLog->entries[entryIndex].time,
                                                             totalSnapshots,
                                                             0,
                                                             1,
                                                             prepaySnapshotLog->entries[entryIndex].snapshotCause,
                                                             prepaySnapshotLog->entries[entryIndex].snapshotPayloadType,
                                                             prepaySnapshotLog->entries[entryIndex].snapshotPayload,
                                                             prepaySnapshotLog->entries[entryIndex].snapshotPayloadSize);
    emberAfSendResponse();
    return;
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
}

static void sendTopUp(int32u earliestStartTime,
                      int32u latestEndTime,
                      int8u numberOfRecords,
                      GpfTopUpLog *topUpLog)
{
  int16u i;
  int16u entryIndex;
  int16u includedRecords = 0;

  // loop through all the entries looking for the entries that are greater than
  // or equal to the the given earliest start time. The SE spec says, "The
  // first returned Top Up record shall be the most recent record with its TopUp
  // Time equal to or older than the Latest End Time provided." This means we
  // need to to keep iterating through the topUpLog looking for the correct
  // entry to send next.
  while (numberOfRecords == 0 || includedRecords < numberOfRecords) {
    int32u referenceUtc = 0;
    int16u indexToSend = 0xFF;

    for (i = 0, entryIndex = getFirstIndex(topUpLog);
         i < topUpLog->numberOfEntries;
         i++, entryIndex = getNextIndex(topUpLog, entryIndex)) {
      if (!READBITS(topUpLog->entries[entryIndex].flags, GPF_FLAGS_SENT)
          && topUpLog->entries[entryIndex].time > referenceUtc
          && topUpLog->entries[entryIndex].time >= earliestStartTime
          && topUpLog->entries[entryIndex].time <= latestEndTime) {
        referenceUtc = topUpLog->entries[entryIndex].time;
        indexToSend = entryIndex;
      }
    }

    // If no top up entries were found, it either means there are
    // no top up entries at the specified time or we've already
    // found all of them in previous iterations.
    if (indexToSend == 0xFF) {
      break;
    }

    if (includedRecords == 0) {
      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                          | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                          ZCL_PREPAYMENT_CLUSTER_ID,
                          ZCL_PUBLISH_TOP_UP_LOG_COMMAND_ID,
                          "uu",
                          0,
                          1);
    }

    emberAfPutStringInResp(topUpLog->entries[indexToSend].code);
    emberAfPutInt32uInResp(topUpLog->entries[indexToSend].amount);
    emberAfPutInt32uInResp(topUpLog->entries[indexToSend].time);
    SETBITS(topUpLog->entries[indexToSend].flags, GPF_FLAGS_SENT);
    includedRecords++;
  }

  if (includedRecords != 0) {
    for (i = 0, entryIndex = getFirstIndex(topUpLog);
         i < topUpLog->numberOfEntries;
         i++, entryIndex = getNextIndex(topUpLog, entryIndex)) {
      if (READBITS(topUpLog->entries[entryIndex].flags, GPF_FLAGS_SENT)) {
        CLEARBITS(topUpLog->entries[entryIndex].flags, GPF_FLAGS_SENT);
      }
    }
    emberAfSendResponse();
    return;
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
}

static void sendDebt(int32u earliestStartTime,
                     int32u latestEndTime,
                     int8u numberOfDebts,
                     int8u debtType,
                     GpfDebtLog *debtLog)
{
  int16u i;
  int16u entryIndex;
  int16u includedDebts = 0;

  // loop through all the entries looking for the entries that are greater than
  // or equal to the the given earliest start time. The SE spec says, "The
  // first returned debt repayment record shall be the the most recent record
  // with its Collection Time equal to or older than the Latest End Time provided."
  // This means we need to to keep iterating through the debtLog looking for the
  // correct entry to send next.
  while (numberOfDebts == 0 || includedDebts < numberOfDebts) {
    int32u referenceUtc = 0;
    int16u indexToSend = 0xFF;

    for (i = 0, entryIndex = getFirstIndex(debtLog);
         i < debtLog->numberOfEntries;
         i++, entryIndex = getNextIndex(debtLog, entryIndex)) {
      if (!READBITS(debtLog->entries[entryIndex].flags, GPF_FLAGS_SENT)
          && debtLog->entries[entryIndex].collectionTime > referenceUtc
          && debtLog->entries[entryIndex].collectionTime >= earliestStartTime
          && debtLog->entries[entryIndex].collectionTime <= latestEndTime
          && (debtType == 0xFF || debtLog->entries[entryIndex].debtType == debtType)) {
        referenceUtc = debtLog->entries[entryIndex].collectionTime;
        indexToSend = entryIndex;
      }
    }

    // If no debt entries were found, it either means there are
    // no debt entries at the specified time or we've already
    // found all of them in previous iterations.
    if (indexToSend == 0xFF) {
      break;
    }

    if (includedDebts == 0) {
      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                          | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                          ZCL_PREPAYMENT_CLUSTER_ID,
                          ZCL_PUBLISH_DEBT_LOG_COMMAND_ID,
                          "uu",
                          0,
                          1);
    }

    emberAfPutInt32uInResp(debtLog->entries[indexToSend].collectionTime);
    emberAfPutInt32uInResp(debtLog->entries[indexToSend].amountCollected);
    emberAfPutInt8uInResp(debtLog->entries[indexToSend].debtType);
    emberAfPutInt32uInResp(debtLog->entries[indexToSend].outstandingDebt);
    SETBITS(debtLog->entries[indexToSend].flags, GPF_FLAGS_SENT);
    includedDebts++;
  }

  if (includedDebts != 0) {
    for (i = 0, entryIndex = getFirstIndex(debtLog);
         i < debtLog->numberOfEntries;
         i++, entryIndex = getNextIndex(debtLog, entryIndex)) {
      if (READBITS(debtLog->entries[entryIndex].flags, GPF_FLAGS_SENT)) {
        CLEARBITS(debtLog->entries[entryIndex].flags, GPF_FLAGS_SENT);
      }
    }
    emberAfSendResponse();
    return;
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
}

static void receiveSampleData(int32u time,
                              int32u sample,
                              GpfSampleLog *sampleLog)
{
  sampleLog->entries[sampleLog->nextEntry].time = time;
  sampleLog->entries[sampleLog->nextEntry].sample = sample;
  incNumberOfEntries(sampleLog);
  sampleLog->nextEntry = getNextIndex(sampleLog, sampleLog->nextEntry);
}

static void receiveSnapshot(int32u snapshotId,
                            int32u snapshotTime,
                            int32u snapshotCause,
                            int8u snapshotPayloadType,
                            int8u *snapshotPayload,
                            GpfSnapshotLog *snapshotLog)
{
  int16u snapshotPayloadLength = fieldLength(snapshotPayload);

  if (snapshotPayloadLength > GPF_MAX_SNAPSHOT_PAYLOAD_SIZE) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: received a PublishSnapshot command that is longer than the max length expected, truncating entry stored in log: received len=0x%2x, max expected len=0x%2x",
                                         snapshotPayloadLength, GPF_MAX_SNAPSHOT_PAYLOAD_SIZE);
    snapshotPayloadLength = GPF_MAX_SNAPSHOT_PAYLOAD_SIZE;
  }

  snapshotLog->entries[snapshotLog->nextEntry].snapshotId = snapshotId;
  snapshotLog->entries[snapshotLog->nextEntry].time = snapshotTime;
  snapshotLog->entries[snapshotLog->nextEntry].snapshotCause = snapshotCause;
  snapshotLog->entries[snapshotLog->nextEntry].snapshotPayloadType = snapshotPayloadType;
  snapshotLog->entries[snapshotLog->nextEntry].snapshotPayloadSize = snapshotPayloadLength;
  MEMCOPY(snapshotLog->entries[snapshotLog->nextEntry].snapshotPayload,
          snapshotPayload,
          snapshotPayloadLength);
  incNumberOfEntries(snapshotLog);
  snapshotLog->nextEntry = getNextIndex(snapshotLog, snapshotLog->nextEntry);
}

static void receivePrepaySnapshot(int32u snapshotId,
                                  int32u snapshotTime,
                                  int32u snapshotCause,
                                  int8u snapshotPayloadType,
                                  int8u *snapshotPayload,
                                  GpfPrepaySnapshotLog *prepaySnapshotLog)
{
  int16u snapshotPayloadLength = fieldLength(snapshotPayload);

  if (snapshotPayloadLength > GPF_MAX_PREPAY_SNAPSHOT_PAYLOAD_SIZE) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: received a PublishPrepaySnapshot command that is longer than the max length expected, truncating entry stored in log: received len=0x%2x, max expected len=0x%2x",
                                         snapshotPayloadLength, GPF_MAX_PREPAY_SNAPSHOT_PAYLOAD_SIZE);
    snapshotPayloadLength = GPF_MAX_PREPAY_SNAPSHOT_PAYLOAD_SIZE;
  }

  prepaySnapshotLog->entries[prepaySnapshotLog->nextEntry].snapshotId = snapshotId;
  prepaySnapshotLog->entries[prepaySnapshotLog->nextEntry].time = snapshotTime;
  prepaySnapshotLog->entries[prepaySnapshotLog->nextEntry].snapshotCause = snapshotCause;
  prepaySnapshotLog->entries[prepaySnapshotLog->nextEntry].snapshotPayloadType = snapshotPayloadType;
  prepaySnapshotLog->entries[prepaySnapshotLog->nextEntry].snapshotPayloadSize = snapshotPayloadLength;
  MEMCOPY(prepaySnapshotLog->entries[prepaySnapshotLog->nextEntry].snapshotPayload,
          snapshotPayload,
          snapshotPayloadLength);
  incNumberOfEntries(prepaySnapshotLog);
  prepaySnapshotLog->nextEntry = getNextIndex(prepaySnapshotLog, prepaySnapshotLog->nextEntry);
}

static void receiveTopUp(int8u *topUpPayloadCode,
                         int32u topUpPayloadAmount,
                         int32u topUpPayloadTime,
                         GpfTopUpLog *topUpLog)
{
  topUpLog->entries[topUpLog->nextEntry].flags = 0;
  emberAfCopyString(topUpLog->entries[topUpLog->nextEntry].code, topUpPayloadCode, 25);
  topUpLog->entries[topUpLog->nextEntry].amount = topUpPayloadAmount;
  topUpLog->entries[topUpLog->nextEntry].time = topUpPayloadTime;
  incNumberOfEntries(topUpLog);
  topUpLog->nextEntry = getNextIndex(topUpLog, topUpLog->nextEntry);
}

static void receiveDebt(int32u collectionTime,
                        int32u amountCollected,
                        int32u outstandingDebt,
                        int8u  debtType,
                        GpfDebtLog *debtLog)
{
  debtLog->entries[debtLog->nextEntry].flags = 0;
  debtLog->entries[debtLog->nextEntry].collectionTime = collectionTime;
  debtLog->entries[debtLog->nextEntry].amountCollected = amountCollected;
  debtLog->entries[debtLog->nextEntry].outstandingDebt = outstandingDebt;
  debtLog->entries[debtLog->nextEntry].debtType = debtType;
  incNumberOfEntries(debtLog);
  debtLog->nextEntry = getNextIndex(debtLog, debtLog->nextEntry);
}

static void getSampleData(int8u srcEndpoint,
                          int8u dstEndpoint,
                          EmberNodeId dstNodeId,
                          GpfSampleLog *sampleLog)
{
  /**
   * GBCS v0.8.1 Section 10.4.2.8
   *
   * In the event that there are missing values in the GPF Profile Data Log,
   * the GPF shall interrogate the GSME Profile Data Log using the
   * GetSampledData (SampleID 0x0000) command and the GetSampledData notification
   * flag to retrieve missing values.
   *
   * GBCS v0.8.1 Section 10.4.2.9
   *
   * In the event of communications outages resulting in the final daily value
   * being missed, the GPF shall retrieve the values from the GSME Profile Data
   * Log using the GetSampledData (SampleID 0x0000) command and GetSampledData
   * notification flag.
   */
  emberAfFillCommandSimpleMeteringClusterGetSampledData(sampleLog->sampleId,
                                                        0,
                                                        EMBER_ZCL_SAMPLE_TYPE_CONSUMPTION_DELIVERED,
                                                        sampleLog->maxEntries);

  // Save the sequence number so we can match up the response with this request
  sampleLog->catchupSequenceNumber = appResponseData[1];
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstNodeId);
}

static void getSnapshot(int8u srcEndpoint,
                        int8u dstEndpoint,
                        EmberNodeId dstNodeId,
                        GpfSnapshotLog *snapshotLog)
{
  /**
   * GBCS v0.8.1 Section 10.4.2.1
   *
   * In the event of a communications outage, the GPF shall retrieve missing
   * snapshots using the GetSnapshot command, with the UTC start time field
   * populated based on the last received snapshot timestamp, if one has been
   * received.
   *
   * GBCS v0.8.1 Section 10.4.2.4
   *
   * In the event of a communications outage, the GPF shall retrieve missing
   * snapshots using the GetSnapshot command (and the relevant notification f
   * lag) with the UTC start time field populated based on the last received
   * snapshot timestamp, if one has been received, or 0x0000 otherwise.
   */
  emberAfFillCommandSimpleMeteringClusterGetSnapshot(0,
                                                     0xFFFFFFFF,
                                                     snapshotLog->catchupSnapshotOffset,
                                                     snapshotLog->catchupSnapshotCause);

  // Save the sequence number so we can match up the response with this request
  snapshotLog->catchupSequenceNumber = appResponseData[1];
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstNodeId);
}

static void getPrepaySnapshot(int8u srcEndpoint,
                              int8u dstEndpoint,
                              EmberNodeId dstNodeId,
                              GpfPrepaySnapshotLog *prepaySnapshotLog)
{
  /**
   * GBCS v0.8.1 Section 10.4.2.2
   *
   * In the event of a communications outage, the GPF shall retrieve missing
   * prepayment snapshots using the GetPrepaySnapshot command (and
   * GetPrepaySnapshot notification flag) with the UTC start time field
   * populated based on the last received prepayment snapshot timestamp, if one
   * has been received.
   *
   * GBCS v0.8.1 Section 10.4.2.7
   *
   * In the event of a communications outage, the GPF shall retrieve missing
   * snapshots using the GetPrepaySnapshot command (and GetPrepaySnapshot
   * notification flag) with the UTC start time field populated based on the
   * last received snapshot timestamp, if one has been received.
   */
  emberAfFillCommandPrepaymentClusterGetPrepaySnapshot(0,
                                                       0xFFFFFFFF,
                                                       prepaySnapshotLog->catchupSnapshotOffset,
                                                       prepaySnapshotLog->catchupSnapshotCause);

  // Save the sequence number so we can match up the response with this request
  prepaySnapshotLog->catchupSequenceNumber = appResponseData[1];
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstNodeId);
}

static void getTopUp(int8u srcEndpoint,
                     int8u dstEndpoint,
                     EmberNodeId dstNodeId,
                     GpfTopUpLog *topUpLog)
{
  /**
   * GBCS v0.8.1 Section 10.4.2.5
   *
   * If there has been a communications outage, the GPF shall use the Get Top Up
   * Log command to retrieve all prepayment top-ups that may have been processed
   * during the communications outage.  The GSME shall set the Date / Time field
   * of the Get Top Up Log command to the current UTC time.
   */
  emberAfFillCommandPrepaymentClusterGetTopUpLog(0xFFFFFFFF,
                                                 topUpLog->maxEntries);

  // Save the sequence number so we can match up the response with this request
  topUpLog->catchupSequenceNumber = appResponseData[1];
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstNodeId);
}

static void getDebt(int8u srcEndpoint,
                    int8u dstEndpoint,
                    EmberNodeId dstNodeId,
                    GpfDebtLog *debtLog)
{
  /**
   * GBCS v0.8.1 Section 10.4.2.6
   *
   * In cases of communications outages, the GPF shall request any outstanding
   * payment-based debt payments by use of the GetDebtRepaymentLog command
   * (and GetDebtRepaymentLog notification flag) with the Debt Type field set
   * to 0x02 (Debt 3).
   */
  emberAfFillCommandPrepaymentClusterGetDebtRepaymentLog(0xFFFFFFFF,
                                                         debtLog->maxEntries,
                                                         EMBER_ZCL_REPAYMENT_DEBT_TYPE_DEBT3);

  // Save the sequence number so we can match up the response with this request
  debtLog->catchupSequenceNumber = appResponseData[1];
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstNodeId);
}

static void incrementLogCatchupsInProgress(void)
{
  if (++logCatchupsInProgress == 1) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Log catch-up started");
  }
}

static void decrementLogCatchupsInProgress(void)
{
  if (--logCatchupsInProgress == 0) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Log catch-up complete");
  }
}

void emAfPrintLogCatchupsInProgress(void)
{
  emberAfPluginGasProxyFunctionPrintln("GPF: Log catch-ups %d", logCatchupsInProgress);
}

static void resetAlternativeHistoricalConsumption(int8u endpoint,
                                                  GpfAlternativeHistoricalConsumption *altConsumption)
{
  int8u i;
  EmberAfStatus status;
  int8u consumption[] = {0, 0, 0, 0};

  for (i=0; i<NUM_ALT_CONSUMPTION_DAY_ATTRS; i++) {
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_SIMPLE_METERING_CLUSTER_ID,
                                         altConsumptionDayAttrIds[i],
                                         consumption,
                                         ZCL_INT24U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPluginGasProxyFunctionPrintln("GPF: ERR: writing metering server attribute 0x%2x - status 0x%x",
                                           altConsumptionDayAttrIds[i], status);
    }
  }

  for (i=0; i<NUM_ALT_CONSUMPTION_WEEK_ATTRS; i++) {
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_SIMPLE_METERING_CLUSTER_ID,
                                         altConsumptionWeekAttrIds[i],
                                         consumption,
                                         ZCL_INT24U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPluginGasProxyFunctionPrintln("GPF: ERR: writing metering server attribute 0x%2x - status 0x%x",
                                           altConsumptionWeekAttrIds[i], status);
    }
  }

  for (i=0; i<NUM_ALT_CONSUMPTION_MONTH_ATTRS; i++) {
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_SIMPLE_METERING_CLUSTER_ID,
                                         altConsumptionMonthAttrIds[i],
                                         consumption,
                                         ZCL_INT32U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPluginGasProxyFunctionPrintln("GPF: ERR: writing metering server attribute 0x%2x - status 0x%x",
                                           altConsumptionMonthAttrIds[i], status);
    }
  }

  altConsumption->catchup = TRUE;
  altConsumption->prevCurrentDayAlternativeConsumption = 0;
  altConsumption->prevCurrentDayAlternativeConsumptionTime = emberAfGetCurrentTime();

  setNotificationFlag(endpoint,
                      ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                      EMBER_AF_METERING_FNF_PUSH_HISTORICAL_METERING_DATA_ATTRIBUTE_SET);
}

static void addToByteArray(int8u *data,
                           int8u len,
                           int32u toAdd)
{
#if (BIGENDIAN_CPU)
  int8s loc  = len - 1;
  int8s end  = -1;
  int8s incr = -1;
#else
  int8s loc  = 0;
  int8s end  = len;
  int8s incr = 1;
#endif
  int16u sum = 0;

  while ( loc != end ) {
    int8u t, s;
    t = data[loc];
    s = toAdd & 0xff;
    sum += t + s;
    data[loc] = sum & 0xff;
    sum >>= 8;
    toAdd >>= 8;
    loc += incr;
  }
}

/*
 * GBCS v0.8.1 section 10.4.2.10
 *
 * As per Section 10.4.2.8, the GSME shall, on each half hour, record the
 * following information and push to the GPF:
 *   - total consumption value (with units of m3);
 *   - total consumption today (with units of kWh); and
 *   - total cost of consumption today (with units of Currency Unit);
 * Using the ‘total consumption today’ value, the GPF shall update the
 * attributes of the mirrored Alternative Historical Consumption attribute set.
 */
static void updateAlternativeHistoricalConsumption(int8u endpoint,
                                                   int32u currentConsumptionTime,
                                                   int32u currentConsumption,
                                                   GpfAlternativeHistoricalConsumption *altConsumption)
{
  boolean newDay = FALSE;
  int8u i;
  int8u consumption24[] = {0, 0, 0};
  int8u consumption32[] = {0, 0, 0, 0};
  EmberAfTimeStruct previousConsumptionTimeStruct;
  EmberAfTimeStruct currentConsumptionTimeStruct;
  int8u previousConsumptionDayOfWeek;
  int8u currentConsumptionDayOfWeek;
  int32u consumption;

  emberAfFillTimeStructFromUtc(altConsumption->prevCurrentDayAlternativeConsumptionTime, &previousConsumptionTimeStruct);
  emberAfFillTimeStructFromUtc(currentConsumptionTime, &currentConsumptionTimeStruct);
  previousConsumptionDayOfWeek = emberAfGetWeekdayFromUtc(altConsumption->prevCurrentDayAlternativeConsumptionTime);
  currentConsumptionDayOfWeek = emberAfGetWeekdayFromUtc(currentConsumptionTime);
  consumption = currentConsumption - altConsumption->prevCurrentDayAlternativeConsumption;

  // If it's a new day then roll all the day attributes
  if (previousConsumptionTimeStruct.day != currentConsumptionTimeStruct.day) {
    newDay = TRUE;
    for (i=NUM_ALT_CONSUMPTION_DAY_ATTRS-1; i>0; i--) {
      emberAfReadServerAttribute(endpoint,
                                 ZCL_SIMPLE_METERING_CLUSTER_ID,
                                 altConsumptionDayAttrIds[i-1],
                                 consumption24,
                                 3);
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_SIMPLE_METERING_CLUSTER_ID,
                                  altConsumptionDayAttrIds[i],
                                  consumption24,
                                  ZCL_INT24U_ATTRIBUTE_TYPE);
    }
    MEMSET(consumption24, 0, 3);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_SIMPLE_METERING_CLUSTER_ID,
                                altConsumptionDayAttrIds[0],
                                consumption24,
                                ZCL_INT24U_ATTRIBUTE_TYPE);
  }

  //Update the current week consumption attribute
  emberAfReadServerAttribute(endpoint,
                             ZCL_SIMPLE_METERING_CLUSTER_ID,
                             ZCL_CURRENT_WEEK_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                             consumption24,
                             3);
  addToByteArray(consumption24, 3, consumption);
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_WEEK_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                              consumption24,
                              ZCL_INT24U_ATTRIBUTE_TYPE);

  // If it's a new week then roll all the week attributes
  if (previousConsumptionDayOfWeek != currentConsumptionDayOfWeek && currentConsumptionDayOfWeek == 0) {
    for (i=NUM_ALT_CONSUMPTION_WEEK_ATTRS-1; i>0; i--) {
      emberAfReadServerAttribute(endpoint,
                                 ZCL_SIMPLE_METERING_CLUSTER_ID,
                                 altConsumptionWeekAttrIds[i-1],
                                 consumption24,
                                 3);
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_SIMPLE_METERING_CLUSTER_ID,
                                  altConsumptionWeekAttrIds[i],
                                  consumption24,
                                  ZCL_INT24U_ATTRIBUTE_TYPE);
    }
    MEMSET(consumption24, 0, 3);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_SIMPLE_METERING_CLUSTER_ID,
                                altConsumptionWeekAttrIds[0],
                                consumption24,
                                ZCL_INT24U_ATTRIBUTE_TYPE);
  }

  //Update the current month consumption attribute
  emberAfReadServerAttribute(endpoint,
                             ZCL_SIMPLE_METERING_CLUSTER_ID,
                             ZCL_CURRENT_MONTH_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                             consumption32,
                             4);
  addToByteArray(consumption32, 4, consumption);
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_MONTH_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                              consumption32,
                              ZCL_INT32U_ATTRIBUTE_TYPE);

  // If it's a new month then roll all the month attributes
  if (previousConsumptionTimeStruct.month != currentConsumptionTimeStruct.month) {
    for (i=NUM_ALT_CONSUMPTION_MONTH_ATTRS-1; i>0; i--) {
      emberAfReadServerAttribute(endpoint,
                                 ZCL_SIMPLE_METERING_CLUSTER_ID,
                                 altConsumptionMonthAttrIds[i-1],
                                 consumption32,
                                 4);
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_SIMPLE_METERING_CLUSTER_ID,
                                  altConsumptionMonthAttrIds[i],
                                  consumption32,
                                  ZCL_INT32U_ATTRIBUTE_TYPE);
    }
    MEMSET(consumption32, 0, 4);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_SIMPLE_METERING_CLUSTER_ID,
                                altConsumptionMonthAttrIds[0],
                                consumption32,
                                ZCL_INT32U_ATTRIBUTE_TYPE);
  }

  altConsumption->prevCurrentDayAlternativeConsumptionTime = currentConsumptionTime;
  altConsumption->prevCurrentDayAlternativeConsumption = (newDay) ? 0 : currentConsumption;
}

static void resetHistoricalCostConsumptionInformation(int8u endpoint,
                                                      GpfHistoricalCostConsumption *costConsumption)
{
  int8u i;
  EmberAfStatus status;
  int8u cost[] = { 0, 0, 0, 0, 0, 0 };

  for (i=0; i<NUM_COST_CONSUMPTION_DAY_ATTRS; i++) {
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_PREPAYMENT_CLUSTER_ID,
                                         costConsumptionDayAttrIds[i],
                                         cost,
                                         ZCL_INT48U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPluginGasProxyFunctionPrintln("GPF: ERR: writing prepayment server attribute 0x%2x - status 0x%x",
                                           costConsumptionDayAttrIds[i], status);
    }
  }

  for (i=0; i<NUM_COST_CONSUMPTION_WEEK_ATTRS; i++) {
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_PREPAYMENT_CLUSTER_ID,
                                         costConsumptionWeekAttrIds[i],
                                         cost,
                                         ZCL_INT48U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPluginGasProxyFunctionPrintln("GPF: ERR: writing prepayment server attribute 0x%2x - status 0x%x",
                                           costConsumptionWeekAttrIds[i], status);
    }
  }

  for (i=0; i<NUM_COST_CONSUMPTION_MONTH_ATTRS; i++) {
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_PREPAYMENT_CLUSTER_ID,
                                         costConsumptionMonthAttrIds[i],
                                         cost,
                                         ZCL_INT48U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPluginGasProxyFunctionPrintln("GPF: ERR: writing prepayment server attribute 0x%2x - status 0x%x",
                                           costConsumptionMonthAttrIds[i], status);
    }
  }

  costConsumption->catchup = TRUE;
  costConsumption->prevCurrentDayCostConsumption = 0;
  costConsumption->prevCurrentDayCostConsumptionTime = emberAfGetCurrentTime();

  setNotificationFlag(endpoint,
                      ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                      EMBER_AF_METERING_FNF_PUSH_HISTORICAL_PREPAYMENT_DATA_ATTRIBUTE_SET);
}

/*
 * GBCS v0.8.1 section 10.4.2.10
 *
 * As per Section 10.4.2.8, the GSME shall, on each half hour, record the
 * following information and push to the GPF:
 *   - total consumption value (with units of m3);
 *   - total consumption today (with units of kWh); and
 *   - total cost of consumption today (with units of Currency Unit);
 * Using the ‘total cost of consumption today’ value, the GPF shall update the
 * attributes of the mirrored Historical Cost Consumption Information
 * attribute set.
*/
static void updateHistoricalCostConsumptionInformation(int8u endpoint,
                                                          int32u currentCostTime,
                                                          int32u currentCost,
                                                          GpfHistoricalCostConsumption *costConsumption)
{
  boolean newDay = FALSE;
  int8u i;
  int8u cost48[] = { 0, 0, 0, 0, 0, 0 };
  EmberAfTimeStruct previousCostTimeStruct;
  EmberAfTimeStruct currentCostTimeStruct;
  int8u previousCostDayOfWeek;
  int8u currentCostDayOfWeek;
  int32u cost;

  emberAfFillTimeStructFromUtc(costConsumption->prevCurrentDayCostConsumptionTime, &previousCostTimeStruct);
  emberAfFillTimeStructFromUtc(currentCostTime, &currentCostTimeStruct);
  previousCostDayOfWeek = emberAfGetWeekdayFromUtc(costConsumption->prevCurrentDayCostConsumptionTime);
  currentCostDayOfWeek = emberAfGetWeekdayFromUtc(currentCostTime);
  cost = currentCost - costConsumption->prevCurrentDayCostConsumption;

  // If it's a new day then roll all the day attributes
  if (previousCostTimeStruct.day != currentCostTimeStruct.day) {
    newDay = TRUE;
    for (i=NUM_COST_CONSUMPTION_DAY_ATTRS-1; i>0; i--) {
      emberAfReadServerAttribute(endpoint,
                                 ZCL_PREPAYMENT_CLUSTER_ID,
                                 costConsumptionDayAttrIds[i-1],
                                 cost48,
                                 6);
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_PREPAYMENT_CLUSTER_ID,
                                  costConsumptionDayAttrIds[i],
                                  cost48,
                                  ZCL_INT48U_ATTRIBUTE_TYPE);
    }
    MEMSET(cost48, 0, 6);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_PREPAYMENT_CLUSTER_ID,
                                costConsumptionDayAttrIds[0],
                                cost48,
                                ZCL_INT48U_ATTRIBUTE_TYPE);
  }

  //Update the current week consumption attribute
  emberAfReadServerAttribute(endpoint,
                             ZCL_PREPAYMENT_CLUSTER_ID,
                             ZCL_CURRENT_WEEK_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                             cost48,
                             6);
  addToByteArray(cost48, 6, cost);
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_CURRENT_WEEK_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                              cost48,
                              ZCL_INT48U_ATTRIBUTE_TYPE);

  // If it's a new week then roll all the week attributes
  if (previousCostDayOfWeek != currentCostDayOfWeek && currentCostDayOfWeek == 0) {
    for (i=NUM_COST_CONSUMPTION_WEEK_ATTRS-1; i>0; i--) {
      emberAfReadServerAttribute(endpoint,
                                 ZCL_PREPAYMENT_CLUSTER_ID,
                                 costConsumptionWeekAttrIds[i-1],
                                 cost48,
                                 6);
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_PREPAYMENT_CLUSTER_ID,
                                  costConsumptionWeekAttrIds[i],
                                  cost48,
                                  ZCL_INT48U_ATTRIBUTE_TYPE);
    }
    MEMSET(cost48, 0, 6);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_PREPAYMENT_CLUSTER_ID,
                                costConsumptionWeekAttrIds[0],
                                cost48,
                                ZCL_INT48U_ATTRIBUTE_TYPE);
  }

  //Update the current month consumption attribute
  emberAfReadServerAttribute(endpoint,
                             ZCL_PREPAYMENT_CLUSTER_ID,
                             ZCL_CURRENT_MONTH_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                             cost48,
                             6);
  addToByteArray(cost48, 6, cost);
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_CURRENT_MONTH_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                              cost48,
                              ZCL_INT48U_ATTRIBUTE_TYPE);

  // If it's a new month then roll all the month attributes
  if (previousCostTimeStruct.month != currentCostTimeStruct.month) {
    for (i=NUM_COST_CONSUMPTION_MONTH_ATTRS-1; i>0; i--) {
      emberAfReadServerAttribute(endpoint,
                                 ZCL_PREPAYMENT_CLUSTER_ID,
                                 costConsumptionMonthAttrIds[i-1],
                                 cost48,
                                 6);
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_PREPAYMENT_CLUSTER_ID,
                                  costConsumptionMonthAttrIds[i],
                                  cost48,
                                  ZCL_INT48U_ATTRIBUTE_TYPE);
    }
    MEMSET(cost48, 0, 6);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_PREPAYMENT_CLUSTER_ID,
                                costConsumptionMonthAttrIds[0],
                                cost48,
                                ZCL_INT48U_ATTRIBUTE_TYPE);
  }

  costConsumption->prevCurrentDayCostConsumptionTime = currentCostTime;
  costConsumption->prevCurrentDayCostConsumption = (newDay) ? 0 : currentCost;
}

static void startSampleLogCatchup(int8u endpoint, GpfSampleLog *sampleLog)
{
  sampleLog->catchup = TRUE;
  sampleLog->catchupSequenceNumber = 0xFF;
  sampleLog->nextEntry = 0;
  sampleLog->numberOfEntries = 0;
  sampleLog->startTime = 0;
  sampleLog->prevSummation = 0;

  setNotificationFlag(endpoint,
                      ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                      EMBER_AF_METERING_FNF_GET_SAMPLED_DATA);
  incrementLogCatchupsInProgress();
}

static void stopSampleLogCatchup(int8u endpoint, GpfSampleLog *sampleLog, GpfSampleLog *otherSampleLog)
{
  sampleLog->catchup = FALSE;
  sampleLog->catchupSequenceNumber = 0xFF;
  if (!otherSampleLog->catchup) {
    clearNotificationFlag(endpoint,
                          ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                          EMBER_AF_METERING_FNF_GET_SAMPLED_DATA);
  }
  decrementLogCatchupsInProgress();
}

static void startSnapshotLogCatchup(int8u endpoint, GpfSnapshotLog *snapshotLog)
{
  snapshotLog->catchup = TRUE;
  snapshotLog->catchupSequenceNumber = 0xFF;
  snapshotLog->catchupSnapshotOffset = 0;
  snapshotLog->nextEntry = 0;
  snapshotLog->numberOfEntries = 0;
  setNotificationFlag(endpoint,
                      ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                      EMBER_AF_METERING_FNF_GET_SNAPSHOT);
  incrementLogCatchupsInProgress();
}

static void stopSnapshotLogCatchup(int8u endpoint, GpfSnapshotLog *snapshotLog, GpfSnapshotLog *otherSnapshotLog)
{
  snapshotLog->catchup = FALSE;
  snapshotLog->catchupSequenceNumber = 0xFF;
  if (!otherSnapshotLog->catchup) {
    clearNotificationFlag(endpoint,
                          ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                          EMBER_AF_METERING_FNF_GET_SNAPSHOT);
  }
  decrementLogCatchupsInProgress();
}

static void startPrepaySnapshotLogCatchup(int8u endpoint, GpfPrepaySnapshotLog *prepaySnapshotLog)
{
  prepaySnapshotLog->catchup = TRUE;
  prepaySnapshotLog->catchupSequenceNumber = 0xFF;
  prepaySnapshotLog->catchupSnapshotOffset = 0;
  prepaySnapshotLog->nextEntry = 0;
  prepaySnapshotLog->numberOfEntries = 0;
  setNotificationFlag(endpoint,
                      ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID,
                      EMBER_AF_METERING_NF4_GET_PREPAY_SNAPSHOT);
  incrementLogCatchupsInProgress();
}

static void stopPrepaySnapshotLogCatchup(int8u endpoint, GpfPrepaySnapshotLog *prepaySnapshotLog, GpfPrepaySnapshotLog *otherPrepaySnapshotLog)
{
  prepaySnapshotLog->catchup = FALSE;
  prepaySnapshotLog->catchupSequenceNumber = 0xFF;
  if (!otherPrepaySnapshotLog->catchup) {
    clearNotificationFlag(endpoint,
                          ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID,
                          EMBER_AF_METERING_NF4_GET_PREPAY_SNAPSHOT);
  }
  decrementLogCatchupsInProgress();
}

static void startTopUpLogCatchup(int8u endpoint, GpfTopUpLog *topUpLog)
{
  topUpLog->catchup = TRUE;
  topUpLog->catchupSequenceNumber = 0xFF;
  topUpLog->nextEntry = 0;
  topUpLog->numberOfEntries = 0;
  setNotificationFlag(endpoint,
                      ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID,
                      EMBER_AF_METERING_NF4_GET_TOP_UP_LOG);
  incrementLogCatchupsInProgress();
}

static void stopTopUpLogCatchup(int8u endpoint, GpfTopUpLog *topUpLog)
{
  topUpLog->catchup = FALSE;
  topUpLog->catchupSequenceNumber = 0xFF;
  clearNotificationFlag(endpoint,
                        ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID,
                        EMBER_AF_METERING_NF4_GET_TOP_UP_LOG);
  decrementLogCatchupsInProgress();
}

static void startDebtLogCatchup(int8u endpoint, GpfDebtLog *debtLog)
{
  debtLog->catchup = TRUE;
  debtLog->catchupSequenceNumber = 0xFF;
  debtLog->nextEntry = 0;
  debtLog->numberOfEntries = 0;
  setNotificationFlag(endpoint,
                      ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID,
                      EMBER_AF_METERING_NF4_GET_DEBT_REPAYMENT_LOG);
  incrementLogCatchupsInProgress();
}

static void stopDebtLogCatchup(int8u endpoint, GpfDebtLog *debtLog)
{
  debtLog->catchup = FALSE;
  debtLog->catchupSequenceNumber = 0xFF;
  clearNotificationFlag(endpoint,
                        ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID,
                        EMBER_AF_METERING_NF4_GET_DEBT_REPAYMENT_LOG);
  decrementLogCatchupsInProgress();
}

/*
 *  Set the daily consumption log's previous summation value using the values
 *  from the profile log to subtract from the last summation recorded in the
 *  profile log back to the beginning of the day.
 */
static void setDailyConsumptionLogPrevSummation(GpfSampleLog *profileLog,
                                                GpfSampleLog *dailyLog)
{
  int32u dailyLogLastUpdateTime;
  int32u profileLogLastUpdateTime;
  int32u now = emberAfGetCurrentTime();
  int32u dailyLogSummation;
  int32u dailyLogSummationTime;
  int16u i;
  int16u entryIndex;

  // We only set the previous summation if it has not yet been set.
  if (dailyLog->prevSummation != 0) {
    return;
  }

  // Determine the time when the last summation was or should have
  // been set within the daily consumption log.
  if (dailyLog->numberOfEntries > 0) {
    // Last summation time is the last entry in the daily log
    dailyLogLastUpdateTime = dailyLog->entries[getLastIndex(dailyLog)].time;
  } else if (dailyLog->startTime) {
    // Last summation time is when the log was started
    dailyLogLastUpdateTime = dailyLog->startTime;
  } else {
    // Last summation time is 12:00 AM this morning
    dailyLogLastUpdateTime = PREV_MIDNIGHT(now);
  }

  profileLogLastUpdateTime = (0 == profileLog->numberOfEntries) ? profileLog->startTime :
      profileLog->entries[getLastIndex(profileLog)].time;
  if (profileLogLastUpdateTime < dailyLogLastUpdateTime) {
    // Nothing we can do.  The last entry in the profile log is older then the
    // last entry in the daily consumption log.
    return;
  }

  // Start with the last summation value stored in the profile log
  dailyLogSummation = profileLog->prevSummation;
  dailyLogSummationTime = profileLogLastUpdateTime;

  // Now loop backward through the profile log subtracting the sample value
  // of each entry from the summation.  We do this until we reach the time
  // of the last entry in the daily consumption log.
  for (i = 0, entryIndex = getLastIndex(profileLog);
       i < profileLog->numberOfEntries;
       i++, entryIndex = getPrevIndex(profileLog, entryIndex)) {

    // startTime should always be one interval earlier of the time of the first entry
    int32u prevTime = ((i+1) == profileLog->numberOfEntries) ? profileLog->startTime :
        profileLog->entries[getPrevIndex(profileLog, entryIndex)].time;

    // If the next sample going backwards in the log is in the previous day
    // then break out of the loop.
    if (prevTime < dailyLogLastUpdateTime) {
      break;
    }

    dailyLogSummation -= profileLog->entries[entryIndex].sample;
    dailyLogSummationTime = profileLog->entries[entryIndex].time;
  }

  if (dailyLog->startTime == 0) {
    dailyLog->startTime = dailyLogSummationTime;
  }
  dailyLog->prevSummation = dailyLogSummation;
}

int8u getInt48u(int8u* message, int16u currentIndex, int16u msgLen, int8u *destination)
{
  if ((currentIndex + 6) > msgLen) {
    emberAfPluginGasProxyFunctionPrintln("GetInt48u, %x bytes short", 6);
    return 0;
  }
#if (BIGENDIAN_CPU)
  emberReverseMemCopy(destination, message + currentIndex, 6);
#else
  MEMCOPY(destination, message + currentIndex, 6);
#endif
  return 6;
}

static void updateSnapshotAttributes(int8u endpoint,
                                     int8u snapshotPayloadType,
                                     int8u *snapshotPayload)
{
  int16u snapshotPayloadLength = fieldLength(snapshotPayload);
  int16u snapshotPayloadIndex = 0;
  int8u summationDelivered[6];
  int32u billToDateDelivered;
  int32u billToDateTimeStampDelivered;
  int32u projectedBillDelivered;
  int32u projectedBillTimeStampDelivered;
  int8u billDeliveredTrailingDigit;
  int8u numberOfTiersInUse;
  int8u numberOfTiersAndBlockThresholdsInUse;
  int8u i;

  getInt48u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength, summationDelivered);
  snapshotPayloadIndex += 6;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              summationDelivered,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
  billToDateDelivered = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_BILL_TO_DATE_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&billToDateDelivered,
                              ZCL_INT32U_ATTRIBUTE_TYPE);
  billToDateTimeStampDelivered = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_BILL_TO_DATE_TIME_STAMP_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&billToDateTimeStampDelivered,
                              ZCL_UTC_TIME_ATTRIBUTE_TYPE);
  projectedBillDelivered = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_PROJECTED_BILL_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&projectedBillDelivered,
                              ZCL_INT32U_ATTRIBUTE_TYPE);
  projectedBillTimeStampDelivered = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_PROJECTED_BILL_TIME_STAMP_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&projectedBillTimeStampDelivered,
                              ZCL_UTC_TIME_ATTRIBUTE_TYPE);
  billDeliveredTrailingDigit = emberAfGetInt8u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 1;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_BILL_DELIVERED_TRAILING_DIGIT_ATTRIBUTE_ID,
                              (int8u *)&billDeliveredTrailingDigit,
                              ZCL_BITMAP8_ATTRIBUTE_TYPE);
  numberOfTiersInUse = emberAfGetInt8u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 1;
  for (i=0; i<numberOfTiersInUse; i++) {
    getInt48u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength, summationDelivered);
    snapshotPayloadIndex += 6;
    emberAfWriteServerAttribute(endpoint,
                                ZCL_SIMPLE_METERING_CLUSTER_ID,
                                ZCL_CURRENT_TIER1_SUMMATION_DELIVERED_ATTRIBUTE_ID+(i*2),
                                summationDelivered,
                                ZCL_INT48U_ATTRIBUTE_TYPE);
  }

  if (snapshotPayloadType == EMBER_ZCL_SNAPSHOT_PAYLOAD_TYPE_BLOCK_TIER_INFORMATION_SET_DELIVERED) {
    numberOfTiersAndBlockThresholdsInUse = emberAfGetInt8u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
    snapshotPayloadIndex += 1;
    for (i=0; i<numberOfTiersAndBlockThresholdsInUse; i++) {
      getInt48u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength, summationDelivered);
      snapshotPayloadIndex += 6;
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_SIMPLE_METERING_CLUSTER_ID,
                                  ZCL_CURRENT_NO_TIER_BLOCK1_SUMMATION_DELIVERED_ATTRIBUTE_ID+(i),
                                  summationDelivered,
                                  ZCL_INT48U_ATTRIBUTE_TYPE);
    }
  }
}

static void updatePrepaySnapshotAttributes(int8u endpoint,
                                           int8u snapshotPayloadType,
                                           int8u *snapshotPayload)
{
  int16u snapshotPayloadLength = fieldLength(snapshotPayload);
  int16u snapshotPayloadIndex = 0;
  int32u accumulatedDebt;
  int32u type1DebtRemaining;
  int32u type2DebtRemaining;
  int32u type3DebtRemaining;
  int32u emergencyCreditRemaining;
  int32u creditRemaining;

  accumulatedDebt = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_ACCUMULATED_DEBT_ATTRIBUTE_ID,
                              (int8u *)&accumulatedDebt,
                              ZCL_INT32S_ATTRIBUTE_TYPE);
  type1DebtRemaining = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_DEBT_AMOUNT_1_ATTRIBUTE_ID,
                              (int8u *)&type1DebtRemaining,
                              ZCL_INT32U_ATTRIBUTE_TYPE);
  type2DebtRemaining = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_DEBT_AMOUNT_2_ATTRIBUTE_ID,
                              (int8u *)&type2DebtRemaining,
                              ZCL_INT32U_ATTRIBUTE_TYPE);
  type3DebtRemaining = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_DEBT_AMOUNT_3_ATTRIBUTE_ID,
                              (int8u *)&type3DebtRemaining,
                              ZCL_INT32U_ATTRIBUTE_TYPE);
  emergencyCreditRemaining = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_EMERGENCY_CREDIT_REMAINING_ATTRIBUTE_ID,
                              (int8u *)&emergencyCreditRemaining,
                              ZCL_INT32S_ATTRIBUTE_TYPE);
  creditRemaining = emberAfGetInt32u(snapshotPayload, snapshotPayloadIndex, snapshotPayloadLength);
  snapshotPayloadIndex += 4;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PREPAYMENT_CLUSTER_ID,
                              ZCL_CREDIT_REMAINING_ATTRIBUTE_ID,
                              (int8u *)&creditRemaining,
                              ZCL_INT32S_ATTRIBUTE_TYPE);
}

//------------------------------------------------------------------------------
// API Functions
void emberAfPluginGasProxyFunctionInitStructuredData(void)
{
  int8u i;

  for (i=0; i<EMBER_AF_PLUGIN_METER_MIRROR_MAX_MIRRORS; i++) {
    MEMSET(structuredData[i].deviceIeeeAddress, 0, EUI64_SIZE);
    structuredData[i].endpoint = GPF_UNUSED_ENDPOINT_ID;

    // Initialize the constant data within each log.  The transient data is
    // initialize when the mirror is added.

    structuredData[i].dailyConsumptionLog.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_DAILY_CONSUMPTION_LOG_ENTRIES;
    structuredData[i].dailyConsumptionLog.entries = dailyConsumptionLogEntries;
    structuredData[i].dailyConsumptionLog.sampleId = GPF_DAILY_CONSUMPTION_LOG_SAMPLE_ID;
    // GBCS v0.8.1 section 10.4.2.11, "The SampleRequestInterval field shall
    // contain 0xFFFF whenever the SampleID field is 0x0001."
    structuredData[i].dailyConsumptionLog.sampleInterval = 0xFFFF;

    structuredData[i].profileDataLog.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_PROFILE_DATA_LOG_ENTRIES;
    structuredData[i].profileDataLog.entries = profileDataLogEntries;
    structuredData[i].profileDataLog.sampleId = GPF_PROFILE_DATA_LOG_SAMPLE_ID;
    structuredData[i].profileDataLog.sampleInterval = 1800;

    structuredData[i].dailyReadLog.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_DAILY_READ_LOG_ENTRIES;
    structuredData[i].dailyReadLog.entries = dailyReadLogEntries;
    structuredData[i].dailyReadLog.catchupSnapshotCause = GPF_SNAPSHOT_CAUSE_GENERAL;

    structuredData[i].prepayDailyReadLog.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_PREPAYMENT_DAILY_READ_LOG_ENTRIES;
    structuredData[i].prepayDailyReadLog.entries = prepayDailyReadLogEntries;
    structuredData[i].prepayDailyReadLog.catchupSnapshotCause = GPF_SNAPSHOT_CAUSE_GENERAL;

    structuredData[i].billingDataLog.snapshot.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_SNAPSHOT_ENTRIES;
    structuredData[i].billingDataLog.snapshot.entries = billingDataLogSnapshotEntries;
    structuredData[i].billingDataLog.snapshot.catchupSnapshotCause = (GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD
                                                                      | GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF
                                                                      | GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER
                                                                      | GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE);

    structuredData[i].billingDataLog.topUp.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_TOP_UP_ENTRIES;
    structuredData[i].billingDataLog.topUp.entries = billingDataLogTopUpEntries;

    structuredData[i].billingDataLog.debt.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_DEBT_ENTRIES;
    structuredData[i].billingDataLog.debt.entries = billingDataLogDebtEntries;

    structuredData[i].billingDataLog.prepaySnapshot.maxEntries = EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MAX_BILLING_DATA_LOG_PREPAY_SNAPSHOT_ENTRIES;
    structuredData[i].billingDataLog.prepaySnapshot.entries = billingDataLogPrepaySnapshotEntries;
    structuredData[i].billingDataLog.prepaySnapshot.catchupSnapshotCause = (GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD
                                                                            | GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF
                                                                            | GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER
                                                                            | GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE);
  }
}

//------------------------------------------------------------------------------
// Callback  Functions

/** @brief Mirror Added
 *
 * This function is called by the Meter Mirror plugin whenever a RequestMirror
 * command is successfully processed.
 *
 * @param requestingDeviceIeeeAddress   Ver.: always
 * @param endpoint   Ver.: always
 */
void emberAfPluginMeterMirrorMirrorAddedCallback(const EmberEUI64 requestingDeviceIeeeAddress,
                                                 int8u endpoint)
{
  int8u i;

  // If we already have a structured data entry for the given device just keep
  // using it but make sure the endpoint is updated as it may have changed
  i = findStructuredDataByEUI64(requestingDeviceIeeeAddress);
  if (i != GPF_INVALID_LOG_INDEX) {
    structuredData[i].endpoint = endpoint;
    return;
  }

  // Find an available slot in the array of structured data logs and assign it to
  // this endpoint;
  i = findStructuredData(GPF_UNUSED_ENDPOINT_ID);
  if (i == GPF_INVALID_LOG_INDEX) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Mirror Added - Structured data not available on endpoint 0x%x", endpoint);
    return;
  }

  MEMCOPY(structuredData[i].deviceIeeeAddress, requestingDeviceIeeeAddress, EUI64_SIZE);
  structuredData[i].endpoint = endpoint;

  // Reset the transient data within each log and kick off the catchup functionality
  startSampleLogCatchup(endpoint, &structuredData[i].dailyConsumptionLog);
  startSampleLogCatchup(endpoint, &structuredData[i].profileDataLog);
  startSnapshotLogCatchup(endpoint, &structuredData[i].dailyReadLog);
  startPrepaySnapshotLogCatchup(endpoint, &structuredData[i].prepayDailyReadLog);
  startSnapshotLogCatchup(endpoint, &structuredData[i].billingDataLog.snapshot);
  startTopUpLogCatchup(endpoint, &structuredData[i].billingDataLog.topUp);
  startDebtLogCatchup(endpoint, &structuredData[i].billingDataLog.debt);
  startPrepaySnapshotLogCatchup(endpoint, &structuredData[i].billingDataLog.prepaySnapshot);

  resetAlternativeHistoricalConsumption(endpoint, &structuredData[i].alternativeHistoricalConsumption);
  resetHistoricalCostConsumptionInformation(endpoint, &structuredData[i].historicalCostConsumption);

  structuredData[i].remoteEndpoint = GPF_UNUSED_ENDPOINT_ID;
  structuredData[i].remoteNodeId = EMBER_UNKNOWN_NODE_ID;
  structuredData[i].functionalNotificationFlags = 0;
  structuredData[i].notificationFlags4 = 0;
  structuredData[i].lastAttributeReportTime = 0;

  emberAfPluginGasProxyFunctionPrintln("GPF: Structured Data initialized on endpoint 0x%x", endpoint);
}

/** @brief Mirror Removed
 *
 * This function is called by the Meter Mirror plugin whenever a RemoveMirror
 * command is successfully processed.
 *
 * @param requestingDeviceIeeeAddress   Ver.: always
 * @param endpoint   Ver.: always
 */
void emberAfPluginMeterMirrorMirrorRemovedCallback(const EmberEUI64 requestingDeviceIeeeAddress,
                                                   int8u endpoint)
{
  int8u i;

  // Find the structured data for the given endpoint
  i = findStructuredData(endpoint);
  if (i == GPF_INVALID_LOG_INDEX) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Mirror Removed - Structured data not available on endpoint 0x%x", endpoint);
    return;
  }

  // mark the structured data log unused
  structuredData[i].endpoint = GPF_UNUSED_ENDPOINT_ID;
  emberAfPluginGasProxyFunctionPrintln("GPF: Structured Data removed on endpoint 0x%x", endpoint);
}

/** @brief Reporting Complete
 *
 * This function is called by the Meter Mirror plugin after processing an
 * AttributeReportingStatus attribute set to ReportingComplete. If the
 * application needs to do any post attribute reporting processing it can do it
 * from within this callback.
 *
 * @param endpoint   Ver.: always
 *
 */
void emberAfPluginMeterMirrorReportingCompleteCallback(int8u endpoint)
{
  int8u i;
  EmberAfStatus status;
  int8u currentSummationDelivered[] = {0,0,0,0,0,0};
  int8u currentDayAlternativeConsumptionDelivered[] = {0,0,0};
  int8u currentDayCostConsumptionDelivered[] = {0,0,0,0,0,0};
  int32u currentSummation;
  int32u currentDayAlternativeConsumption;
  int32u currentDayCostConsumption;
  int32u now = emberAfGetCurrentTime();

  // Find the structured data for the given endpoint
  i = findStructuredData(endpoint);
  if (i == GPF_INVALID_LOG_INDEX) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Attribute Reporting Complete - Structured data not available on endpoint 0x%x", endpoint);
    return;
  }

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_SIMPLE_METERING_CLUSTER_ID,
                                      ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                                      currentSummationDelivered,
                                      6);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: can't read CurrentSummationDelivered attribute: status 0x%x", status);
  }

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_SIMPLE_METERING_CLUSTER_ID,
                                      ZCL_CURRENT_ALTERNATIVE_DAY_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                                      currentDayAlternativeConsumptionDelivered,
                                      3);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: can't read CurrentDayAlternativeConsumptionDelivered attribute: status 0x%x", status);
  }

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_PREPAYMENT_CLUSTER_ID,
                                      ZCL_CURRENT_DAY_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID,
                                      currentDayCostConsumptionDelivered,
                                      6);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: can't read CurrentDayCostConsumptionDelivered attribute: status 0x%x", status);
  }

  // We only care about the least significant 32 bits as the the various delivered
  // amounts should not change by more than a 32 bit value between attribute reports.
#if (BIGENDIAN_CPU)
  MEMCOPY((int8u *)&currentSummation, &currentSummationDelivered[2], 4);
  MEMCOPY(&((int8u *)&currentDayAlternativeConsumption)[1], &currentDayAlternativeConsumptionDelivered[0], 3);
  MEMCOPY((int8u *)&currentDayCostConsumption, &currentDayCostConsumptionDelivered[2], 4);
#else
  MEMCOPY((int8u *)&currentSummation, &currentSummationDelivered[0], 4);
  MEMCOPY(&((int8u *)&currentDayAlternativeConsumption)[0], &currentDayAlternativeConsumptionDelivered[0], 3);
  MEMCOPY((int8u *)&currentDayCostConsumption, &currentDayCostConsumptionDelivered[0], 4);
#endif

  // If we are "catching up" then don't process the reported attributes.  We wait
  // until we are completely caught up before we start processing these reports again.
  if (!structuredData[i].profileDataLog.catchup) {
    GpfSampleLog *sampleLog = &structuredData[i].profileDataLog;
    if (sampleLog->startTime != 0) {
      int32u prevTime = (0 == sampleLog->numberOfEntries) ? sampleLog->startTime :
          sampleLog->entries[getLastIndex(sampleLog)].time;
      int32u nextTime = NEXT_HALF_HOUR(prevTime);
      if (nextTime <= now) {
        int32u sample = currentSummation - sampleLog->prevSummation;
        receiveSampleData(now, sample, sampleLog);
        sampleLog->prevSummation = currentSummation;
      }
    } else {
      sampleLog->startTime = now;
      sampleLog->prevSummation = currentSummation;
    }
  }

  if (!structuredData[i].dailyConsumptionLog.catchup) {
    GpfSampleLog *sampleLog = &structuredData[i].dailyConsumptionLog;
    if (sampleLog->startTime != 0) {
      int32u prevTime = (0 == sampleLog->numberOfEntries) ? sampleLog->startTime :
          sampleLog->entries[getLastIndex(sampleLog)].time;
      int32u nextTime = NEXT_MIDNIGHT(prevTime);
      if (nextTime <= now) {
        int32u sample = currentSummation - sampleLog->prevSummation;
        receiveSampleData(now, sample, sampleLog);
        sampleLog->prevSummation = currentSummation;
      }
    } else {
      sampleLog->startTime = now;
      sampleLog->prevSummation = currentSummation;
    }
  }

  updateAlternativeHistoricalConsumption(endpoint,
                                         now,
                                         currentDayAlternativeConsumption,
                                         &structuredData[i].alternativeHistoricalConsumption);

  updateHistoricalCostConsumptionInformation(endpoint,
                                             now,
                                             currentDayCostConsumption,
                                             &structuredData[i].historicalCostConsumption);

  structuredData[i].lastAttributeReportTime = now;
}

/** @brief Simple Metering Cluster Get Sampled Data Response
 *
 * @param sampleId   Ver.: always
 * @param sampleStartTime   Ver.: always
 * @param sampleType   Ver.: always
 * @param sampleRequestInterval   Ver.: always
 * @param numberOfSamples   Ver.: always
 * @param samples   Ver.: always
 */
boolean emberAfSimpleMeteringClusterGetSampledDataResponseCallback(int16u sampleId,
                                                                   int32u sampleStartTime,
                                                                   int8u sampleType,
                                                                   int16u sampleRequestInterval,
                                                                   int16u numberOfSamples,
                                                                   int8u* samples)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfSampleLog *sampleLog;
  GpfSampleLog *otherSampleLog;
  int16u samplesLength = fieldLength(samples);
  int16u samplesIndex = 0;
  int32u sample;
  int32u sampleTime = sampleStartTime;
  EmberAfStatus status;
  int8u currentSummationDelivered[] = {0,0,0,0,0,0};
  int32u currentSummation;

  emberAfPluginGasProxyFunctionPrintln("GPF: GetSampledDataResponse 0x%2x 0x%4x 0x%x 0x%2x 0x%2x 0x%2x",
                                       sampleId,
                                       sampleStartTime,
                                       sampleType,
                                       sampleRequestInterval,
                                       numberOfSamples,
                                       samplesLength);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  if (sampleType != EMBER_ZCL_SAMPLE_TYPE_CONSUMPTION_DELIVERED) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: GetSampledDataResponse command received with invalid sampleType: 0x%x", sampleType);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_FIELD);
    return TRUE;
  }

  if (sampleId == GPF_DAILY_CONSUMPTION_LOG_SAMPLE_ID) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Receive Daily Consumption Log");
    sampleLog = &structuredData[i].dailyConsumptionLog;
    otherSampleLog = &structuredData[i].profileDataLog;
  } else if (sampleId == GPF_PROFILE_DATA_LOG_SAMPLE_ID) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Receive Profile Data Log");
    sampleLog = &structuredData[i].profileDataLog;
    otherSampleLog = &structuredData[i].dailyConsumptionLog;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: GetSampledDataResponse command received with invalid sampleId: 0x%2x", sampleId);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_FIELD);
    return TRUE;
  }

  // The only time we should receive a GetSampledDataResponse is after a node
  // restart where we send a GetSampledData request to obtain any data that
  // may have been missed while this node was out of service (aka "catchup").
  // As such we will ignore any commands that we were not expecting.
  if (!sampleLog->catchup || emberAfCurrentCommand()->seqNum != sampleLog->catchupSequenceNumber) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: ignoring unexpected GetSampledDataResponse command");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return TRUE;
  }

  sampleLog->startTime = sampleTime;
  sampleLog->sampleInterval = sampleRequestInterval;
  while (samplesIndex < samplesLength) {
    sample = emberAfGetInt24u(samples, samplesIndex, samplesLength);
    samplesIndex += 3;
    sampleTime = ((sampleId == GPF_PROFILE_DATA_LOG_SAMPLE_ID) ?
                  NEXT_HALF_HOUR(sampleTime) : NEXT_MIDNIGHT(sampleTime));
    receiveSampleData(sampleTime, sample, sampleLog);
  }

  // now that we are caught up we need to set the prev summation value so that
  // the next time the device reports consumption we can calculate the sample
  // correctly.  For the profile log it is easy, we just use the current
  // summation attribute which represents the last time it was reported. For
  // the daily consumption log it is a little more difficult. To set the prev
  // summation we need to start with the current summation then using the profile
  // log subtract the incremental values back to the beginning of the day.
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_SIMPLE_METERING_CLUSTER_ID,
                                      ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                                      currentSummationDelivered,
                                      6);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: can't read CurrentSummationDelivered attribute: status 0x%x",status);
  }
  // We only care about the least significant 32 bits as the the summation delivered
  // should not change by more than a 32 bit value between attribute reports.
#if (BIGENDIAN_CPU)
  MEMCOPY((int8u *)&currentSummation, &currentSummationDelivered[2], 4);
#else
  MEMCOPY((int8u *)&currentSummation, &currentSummationDelivered[0], 4);
#endif
  if (sampleId == GPF_PROFILE_DATA_LOG_SAMPLE_ID) {
    sampleLog->prevSummation = currentSummation;
    // in this case sampleLog is the profile data log and otherSamleLog is
    // the daily consumption log
    setDailyConsumptionLogPrevSummation(sampleLog, otherSampleLog);
  } else if (!otherSampleLog->catchup) {
    // in this case sampleLog is the daily consumption log and otherSampleLog is
    // the profile data log.
    setDailyConsumptionLogPrevSummation(otherSampleLog, sampleLog);
  }

  stopSampleLogCatchup(endpoint, sampleLog, otherSampleLog);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

/** @brief Simple Metering Cluster Get Sampled Data
 *
 * @param sampleId   Ver.: always
 * @param earliestSampleTime   Ver.: always
 * @param sampleType   Ver.: always
 * @param numberOfSamples   Ver.: always
 */
boolean emberAfSimpleMeteringClusterGetSampledDataCallback(int16u sampleId,
                                                           int32u earliestSampleTime,
                                                           int8u sampleType,
                                                           int16u numberOfSamples)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfSampleLog *sampleLog;

  emberAfPluginGasProxyFunctionPrintln("GPF: GetSampledData 0x%2x 0x%4x 0x%x 0x%2x",
                                       sampleId,
                                       earliestSampleTime,
                                       sampleType,
                                       numberOfSamples);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  if (sampleType != EMBER_ZCL_SAMPLE_TYPE_CONSUMPTION_DELIVERED) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: GetSampledData command received with invalid sampleType: 0x%x", sampleType);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  if (sampleId == GPF_DAILY_CONSUMPTION_LOG_SAMPLE_ID) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Publish Daily Consumption Log");
    sampleLog = &structuredData[i].dailyConsumptionLog;
  } else if (sampleId == GPF_PROFILE_DATA_LOG_SAMPLE_ID) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Publish Profile Data Log");
    sampleLog = &structuredData[i].profileDataLog;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: GetSampledData command received with invalid sampleId: 0x%2x", sampleId);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  sendSampleData(earliestSampleTime, numberOfSamples, sampleLog);
  return TRUE;
}

/** @brief Simple Metering Cluster Publish Snapshot
 *
 * @param snapshotId   Ver.: always
 * @param snapshotTime   Ver.: always
 * @param totalSnapshotsFound   Ver.: always
 * @param commandIndex   Ver.: always
 * @param totalCommands   Ver.: always
 * @param snapshotCause   Ver.: always
 * @param snapshotPayloadType   Ver.: always
 * @param snapshotPayload   Ver.: always
 */
boolean emberAfSimpleMeteringClusterPublishSnapshotCallback(int32u snapshotId,
                                                            int32u snapshotTime,
                                                            int8u totalSnapshotsFound,
                                                            int8u commandIndex,
                                                            int8u totalNumberOfCommands,
                                                            int32u snapshotCause,
                                                            int8u snapshotPayloadType,
                                                            int8u* snapshotPayload)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfSnapshotLog *snapshotLog;
  GpfSnapshotLog *otherSnapshotLog;
  int32u now = emberAfGetCurrentTime();

  emberAfPluginGasProxyFunctionPrintln("GPF: PublishSnapshot 0x%4x 0x%4x 0x%x 0x%x 0x%x 0x%4x 0x%x",
                                       snapshotId,
                                       snapshotTime,
                                       totalSnapshotsFound,
                                       commandIndex,
                                       totalNumberOfCommands,
                                       snapshotCause,
                                       snapshotPayloadType);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  // Both the Daily Read Log and Billing Data Log PublishSnapshot commands use
  // the same snapshotPayloadTypes (referenced below). The types were obtained
  // by looking at the response in the use case description for GCS16a then
  // also looking at the description of the billing data log in GBCS v0.8.1
  // section 10.4.2.4 and comparing it to the description of the daily read log
  // in the SMETS v1.58 section 4.4.94.
  if (snapshotPayloadType != EMBER_ZCL_SNAPSHOT_PAYLOAD_TYPE_TOU_INFORMATION_SET_DELIVERED_REGISTERS
      && snapshotPayloadType != EMBER_ZCL_SNAPSHOT_PAYLOAD_TYPE_BLOCK_TIER_INFORMATION_SET_DELIVERED) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: PublishSnapshot command received with unsupported payloadType: 0x%x", snapshotPayloadType);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_FIELD);
    return TRUE;
  }

  if (snapshotCause == GPF_SNAPSHOT_CAUSE_GENERAL) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Receive Daily Read Log");
    snapshotLog = &structuredData[i].dailyReadLog;
    otherSnapshotLog = &structuredData[i].billingDataLog.snapshot;
  } else if (snapshotCause
             & (GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE)) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Receive Billing Data Log - Tariff TOU Register Matrix, the Consumption Register and Tariff Block Counter Matrix");
    snapshotLog = &structuredData[i].billingDataLog.snapshot;
    otherSnapshotLog = &structuredData[i].dailyReadLog;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: PublishSnapshot command received with unsupported cause: 0x%4x", snapshotCause);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_FIELD);
    return TRUE;
  }

  // Ignore any commands that we were not expecting.
  if (snapshotLog->catchup && emberAfCurrentCommand()->seqNum != snapshotLog->catchupSequenceNumber) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: ignoring unexpected PublishSnapshot command");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return TRUE;
  }

  receiveSnapshot(snapshotId, snapshotTime, snapshotCause, snapshotPayloadType, snapshotPayload, snapshotLog);

  if (snapshotLog->catchup) {
    snapshotLog->catchupSnapshotOffset++;
    if (snapshotLog->catchupSnapshotOffset >= totalSnapshotsFound) {
      stopSnapshotLogCatchup(endpoint, snapshotLog, otherSnapshotLog);
    } else {
      EmberAfClusterCommand *cmd = emberAfCurrentCommand();
      getSnapshot(endpoint, cmd->apsFrame->sourceEndpoint, cmd->source, snapshotLog);
    }
  } else {
    if (snapshotCause == GPF_SNAPSHOT_CAUSE_GENERAL) {
      /*
       * GBCS v0.8.1 Section 10.4.2.1
       *
       * The GPF shall populate the relevant attributes upon receipt of the
       * PublishSnapshot command, providing the command is received between
       * midnight (UTC) and the next scheduled wake of the GSME.
       */
      if (snapshotTime >= PREV_MIDNIGHT(now)
          && structuredData[i].lastAttributeReportTime < PREV_MIDNIGHT(now)) {
        updateSnapshotAttributes(endpoint, snapshotPayloadType, snapshotPayload);
      }
    } else {
      /*
       * CHTS v1.46 Section 4.5.2
       *
       * Where changes have been made to the GSME Billing Data Log in accordance
       * with the timetable set-out in the GSME Billing Calendar, the GPF shall be
       * capable of generating and sending an Alert containing the most recent
       * entries of the GSME Tariff TOU Register Matrix, the GSME Tariff Block
       * Counter Matrix and the GSME Consumption Register in the GSME Billing Data
       * Log.
       */
      emAfGasProxyFunctionAlert(GBCS_ALERT_BILLING_DATA_LOG_UPDATED,
                                emberAfCurrentCommand(), 
                                GCS53_MESSAGE_CODE);
    }
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

/** @brief Simple Metering Cluster Get Snapshot
 *
 * @param earliestStartTime   Ver.: always
 * @param latestEndTime   Ver.: always
 * @param snapshotOffset   Ver.: always
 * @param snapshotCause   Ver.: always
 */
boolean emberAfSimpleMeteringClusterGetSnapshotCallback(int32u earliestStartTime,
                                                        int32u latestEndTime,
                                                        int8u snapshotOffset,
                                                        int32u snapshotCause)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfSnapshotLog *snapshotLog;

  emberAfPluginGasProxyFunctionPrintln("GPF: GetSnapshot 0x%4x 0x%4x 0x%x 0x%4x",
                                       earliestStartTime,
                                       latestEndTime,
                                       snapshotOffset,
                                       snapshotCause);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  if (snapshotCause == GPF_SNAPSHOT_CAUSE_GENERAL) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Publish Daily Read Log");
    snapshotLog = &structuredData[i].dailyReadLog;
  } else if (snapshotCause
             & (GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE)) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Publish Billing Data Log - Tariff TOU Register Matrix, the Consumption Register and Tariff Block Counter Matrix");
    snapshotLog = &structuredData[i].billingDataLog.snapshot;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: GetSnapshot command received with unsupported cause: 0x%4x", snapshotCause);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  sendSnapshot(earliestStartTime, latestEndTime, snapshotOffset, snapshotCause, snapshotLog);
  return TRUE;
}

/** @brief Prepayment Cluster Publish Prepay Snapshot
 *
 * @param snapshotId   Ver.: always
 * @param snapshotTime   Ver.: always
 * @param totalSnapshotsFound   Ver.: always
 * @param commandIndex   Ver.: always
 * @param totalNumberOfCommands   Ver.: always
 * @param snapshotCause   Ver.: always
 * @param snapshotPayloadType   Ver.: always
 * @param snapshotPayload   Ver.: always
 */
boolean emberAfPrepaymentClusterPublishPrepaySnapshotCallback(int32u snapshotId,
                                                              int32u snapshotTime,
                                                              int8u totalSnapshotsFound,
                                                              int8u commandIndex,
                                                              int8u totalNumberOfCommands,
                                                              int32u snapshotCause,
                                                              int8u snapshotPayloadType,
                                                              int8u* snapshotPayload)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfPrepaySnapshotLog *prepaySnapshotLog;
  GpfPrepaySnapshotLog *otherPrepaySnapshotLog;
  int32u now = emberAfGetCurrentTime();

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishPrepaySnapshot, 0x%4x, 0x%4x, 0x%x, 0x%x, 0x%x, 0x%4x, 0x%x",
                                       snapshotId,
                                       snapshotTime,
                                       totalSnapshotsFound,
                                       commandIndex,
                                       totalNumberOfCommands,
                                       snapshotCause,
                                       snapshotPayloadType);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  if (snapshotPayloadType != EMBER_ZCL_PREPAY_SNAPSHOT_PAYLOAD_TYPE_DEBT_CREDIT_STATUS) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: PublishPrepaySnapshot command received with unsupported payloadType: 0x%x", snapshotPayloadType);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_FIELD);
    return TRUE;
  }

  if (snapshotCause == GPF_SNAPSHOT_CAUSE_GENERAL) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Receive Prepay Daily Read Log");
    prepaySnapshotLog = &structuredData[i].prepayDailyReadLog;
    otherPrepaySnapshotLog = &structuredData[i].billingDataLog.prepaySnapshot;
  } else if (snapshotCause
             & (GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE)) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Receive Billing Data Log - Meter Balance, Emergency Credit Balance, Accumulated Debt Register, Payment Debt Register and Time Debt Registers");
    prepaySnapshotLog = &structuredData[i].billingDataLog.prepaySnapshot;
    otherPrepaySnapshotLog = &structuredData[i].prepayDailyReadLog;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: PublishPrepaySnapshot command received with unsupported cause: 0x%4x", snapshotCause);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_FIELD);
    return TRUE;
  }

  // Ignore any commands that we were not expecting.
  if (prepaySnapshotLog->catchup && emberAfCurrentCommand()->seqNum != prepaySnapshotLog->catchupSequenceNumber) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: ignoring unexpected PublishPrepaySnapshot command");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return TRUE;
  }

  receivePrepaySnapshot(snapshotId, snapshotTime, snapshotCause, snapshotPayloadType, snapshotPayload, prepaySnapshotLog);

  if (prepaySnapshotLog->catchup) {
    prepaySnapshotLog->catchupSnapshotOffset++;
    if (prepaySnapshotLog->catchupSnapshotOffset >= totalSnapshotsFound) {
      stopPrepaySnapshotLogCatchup(endpoint, prepaySnapshotLog, otherPrepaySnapshotLog);
    } else {
      EmberAfClusterCommand *cmd = emberAfCurrentCommand();
      getPrepaySnapshot(endpoint, cmd->apsFrame->sourceEndpoint, cmd->source, prepaySnapshotLog);
    }
  } else {
    if (snapshotCause == GPF_SNAPSHOT_CAUSE_GENERAL) {
      /*
       * GBCS v0.8.1 Section 10.4.2.2
       *
       * The GPF shall populate the relevant attributes upon receipt of the
       * Publish Prepay Snapshot command, providing the command is received
       * between midnight (UTC) and the next scheduled wake of the GSME.
       */
      if (snapshotTime >= PREV_MIDNIGHT(now)
          && structuredData[i].lastAttributeReportTime < PREV_MIDNIGHT(now)) {
        updatePrepaySnapshotAttributes(endpoint, snapshotPayloadType, snapshotPayload);
      }
    }
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

/** @brief Prepayment Cluster Get Prepay Snapshot
 *
 * @param earliestStartTime   Ver.: always
 * @param latestEndTime   Ver.: always
 * @param snapshotOffset   Ver.: always
 * @param snapshotCause   Ver.: always
 */
boolean emberAfPrepaymentClusterGetPrepaySnapshotCallback(int32u earliestStartTime,
                                                          int32u latestEndTime,
                                                          int8u snapshotOffset,
                                                          int32u snapshotCause)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfPrepaySnapshotLog *prepaySnapshotLog;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: GetPrepaySnapshot 0x%4x 0x%4x 0x%x 0x%4x",
                                       earliestStartTime,
                                       latestEndTime,
                                       snapshotOffset,
                                       snapshotCause);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  if (snapshotCause == GPF_SNAPSHOT_CAUSE_GENERAL) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Publish Prepay Daily Read Log");
    prepaySnapshotLog = &structuredData[i].prepayDailyReadLog;
  } else if (snapshotCause
             & (GPF_SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_TARIFF
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER
                | GPF_SNAPSHOT_CAUSE_CHANGE_OF_PAYMENT_MODE)) {
    emberAfPluginGasProxyFunctionPrintln("GPF: Publish Billing Data Log - Meter Balance, Emergency Credit Balance, Accumulated Debt Register, Payment Debt Register and Time Debt Registers");
    prepaySnapshotLog = &structuredData[i].billingDataLog.prepaySnapshot;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: GetPrepaySnapshot command received with unsupported cause: 0x%4x", snapshotCause);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
    return TRUE;
  }

  sendPrepaySnapshot(earliestStartTime, latestEndTime, snapshotOffset, snapshotCause, prepaySnapshotLog);
  return TRUE;
}

/** @brief Prepayment Cluster Publish Top Up Log
 *
 * @param commandIndex   Ver.: always
 * @param totalNumberOfCommands   Ver.: always
 * @param topUpPayload   Ver.: always
 */
boolean emberAfPrepaymentClusterPublishTopUpLogCallback(int8u commandIndex,
                                                        int8u totalNumberOfCommands,
                                                        int8u* topUpPayload)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfTopUpLog *topUpLog;
  int16u topUpPayloadLength = fieldLength(topUpPayload);
  int16u topUpPayloadIndex = 0;
  int8u *topUpPayloadCode;
  int32u topUpPayloadAmount;
  int32u topUpPayloadTime;

  emberAfPluginGasProxyFunctionPrintln("GPF: PublishTopUpLog 0x%x 0x%x 0x%2x",
                                       commandIndex,
                                       totalNumberOfCommands,
                                       topUpPayloadLength);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  emberAfPluginGasProxyFunctionPrintln("GPF: Receive Billing Data Log - value of prepayment credits");
  topUpLog = &structuredData[i].billingDataLog.topUp;

  // Ignore any commands that we were not expecting.
  if (topUpLog->catchup && emberAfCurrentCommand()->seqNum != topUpLog->catchupSequenceNumber) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: ignoring unexpected PublishTopUpLog command");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return TRUE;
  }

  while (topUpPayloadIndex < topUpPayloadLength) {
    topUpPayloadCode = emberAfGetString(topUpPayload, topUpPayloadIndex, topUpPayloadLength);
    topUpPayloadIndex += emberAfStringLength(topUpPayloadCode) + 1;
    topUpPayloadAmount = emberAfGetInt32u(topUpPayload, topUpPayloadIndex, topUpPayloadLength);
    topUpPayloadIndex += 4;
    topUpPayloadTime = emberAfGetInt32u(topUpPayload, topUpPayloadIndex, topUpPayloadLength);
    topUpPayloadIndex += 4;
    receiveTopUp(topUpPayloadCode, topUpPayloadAmount, topUpPayloadTime, topUpLog);
  }

  if (topUpLog->catchup) {
    stopTopUpLogCatchup(endpoint, topUpLog);
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

/** @brief Prepayment Cluster Get Top Up Log
 *
 * @param latestEndTime   Ver.: always
 * @param numberOfRecords   Ver.: always
 */
boolean emberAfPrepaymentClusterGetTopUpLogCallback(int32u latestEndTime,
                                                    int8u numberOfRecords)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfTopUpLog *topUpLog;
  int32u earliestStartTime;

  emberAfPluginGasProxyFunctionPrintln("GPF: GetTopUpLog 0x%4x 0x%x",
                                       latestEndTime,
                                       numberOfRecords);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  emberAfPluginGasProxyFunctionPrintln("GPF: Publish Billing Data Log - value of prepayment credits");
  topUpLog = &structuredData[i].billingDataLog.topUp;

  // GBCS adds an additional filter criteria so if this is use case GCS15e,
  // indicated by this being a loopback command, then grab the start time
  // from the GBZ parser.
  earliestStartTime = (emberAfCurrentCommand()->source == emberGetNodeId()) ?
                      emAfGasProxyFunctionGetGbzStartTime() : 0;
  sendTopUp(earliestStartTime, latestEndTime, numberOfRecords, topUpLog);
  return TRUE;
}

/** @brief Prepayment Cluster Publish Debt Log
 *
 * @param commandIndex   Ver.: always
 * @param totalNumberOfCommands   Ver.: always
 * @param debtPayload   Ver.: always
 */
boolean emberAfPrepaymentClusterPublishDebtLogCallback(int8u commandIndex,
                                                       int8u totalNumberOfCommands,
                                                       int8u* debtPayload)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfDebtLog *debtLog;
  int16u debtPayloadLength = fieldLength(debtPayload);
  int16u debtPayloadIndex = 0;
  int32u collectionTime;
  int32u amountCollected;
  int32u outstandingDebt;
  int8u  debtType;

  emberAfPluginGasProxyFunctionPrintln("GPF: PublishDebtLog 0x%x 0x%x 0x%2x",
                                       commandIndex,
                                       totalNumberOfCommands,
                                       debtPayloadLength);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  emberAfPluginGasProxyFunctionPrintln("GPF: Receive Billing Data Log - payment-based debt payments");
  debtLog = &structuredData[i].billingDataLog.debt;

  // Ignore any commands that we were not expecting.
  if (debtLog->catchup && emberAfCurrentCommand()->seqNum != debtLog->catchupSequenceNumber) {
    emberAfPluginGasProxyFunctionPrintln("GPF: WARN: ignoring unexpected PublishDebtLog command");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return TRUE;
  }

  while (debtPayloadIndex < debtPayloadLength) {
    collectionTime = emberAfGetInt32u(debtPayload, debtPayloadIndex, debtPayloadLength);
    debtPayloadIndex += 4;
    amountCollected = emberAfGetInt32u(debtPayload, debtPayloadIndex, debtPayloadLength);
    debtPayloadIndex += 4;
    debtType = emberAfGetInt8u(debtPayload, debtPayloadIndex, debtPayloadLength);
    debtPayloadIndex += 1;
    outstandingDebt = emberAfGetInt32u(debtPayload, debtPayloadIndex, debtPayloadLength);
    debtPayloadIndex += 4;
    receiveDebt(collectionTime, amountCollected, outstandingDebt, debtType, debtLog);
  }

  if (debtLog->catchup) {
    stopDebtLogCatchup(endpoint, debtLog);
  }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

/** @brief Prepayment Cluster Get Debt Repayment Log
 *
 * @param latestEndTime   Ver.: always
 * @param numberOfDebts   Ver.: always
 * @param debtType   Ver.: always
 */
boolean emberAfPrepaymentClusterGetDebtRepaymentLogCallback(int32u latestEndTime,
                                                            int8u numberOfDebts,
                                                            int8u debtType)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);
  GpfDebtLog *debtLog;
  int32u earliestStartTime;

  emberAfPluginGasProxyFunctionPrintln("GPF: GetDebtRepaymentLog 0x%4x 0x%x 0x%x",
                                       latestEndTime,
                                       numberOfDebts,
                                       debtType);

  if (i == GPF_INVALID_LOG_INDEX) {
    return FALSE;
  }

  emberAfPluginGasProxyFunctionPrintln("GPF: Publish Billing Data Log - payment-based debt payments");
  debtLog = &structuredData[i].billingDataLog.debt;

  // GBCS adds an additional filter criteria so if this is use case GCS15d,
  // indicated by this being a loopback command, then grab the start time
  // from the GBZ parser.
  earliestStartTime = (emberAfCurrentCommand()->source == emberGetNodeId()) ?
                      emAfGasProxyFunctionGetGbzStartTime() : 0;
  sendDebt(earliestStartTime, latestEndTime, numberOfDebts, debtType, debtLog);
  return TRUE;
}

// Catchup event handler used to retry previously attempted Get requests on
// the various logs.
void emberAfPluginGasProxyFunctionCatchupEventHandler(void)
{
  int8u i;
  int8u localEndpoint, remoteEndpoint;
  EmberNodeId remoteNodeId;

  emberEventControlSetInactive(emberAfPluginGasProxyFunctionCatchupEventControl);

  for (i=0; i<EMBER_AF_PLUGIN_METER_MIRROR_MAX_MIRRORS; i++) {
    if (structuredData[i].endpoint == GPF_UNUSED_ENDPOINT_ID) {
      continue;
    }

    localEndpoint = structuredData[i].endpoint;
    remoteEndpoint = structuredData[i].remoteEndpoint;
    remoteNodeId = structuredData[i].remoteNodeId;

    if (structuredData[i].functionalNotificationFlags & EMBER_AF_METERING_FNF_GET_SNAPSHOT) {
      GpfSnapshotLog *snapshotLog = &structuredData[i].dailyReadLog;
      if (snapshotLog->catchup) {
        snapshotLog->catchupSnapshotOffset = 0;
        getSnapshot(localEndpoint, remoteEndpoint, remoteNodeId, snapshotLog);
      }
      snapshotLog = &structuredData[i].billingDataLog.snapshot;
      if (snapshotLog->catchup) {
        snapshotLog->catchupSnapshotOffset = 0;
        getSnapshot(localEndpoint, remoteEndpoint, remoteNodeId, snapshotLog);
      }
      structuredData[i].functionalNotificationFlags &= ~EMBER_AF_METERING_FNF_GET_SNAPSHOT;
      emberEventControlSetDelayMS(emberAfPluginGasProxyFunctionCatchupEventControl,
                                  MILLISECOND_TICKS_PER_SECOND);
      continue;
    }

    if (structuredData[i].functionalNotificationFlags & EMBER_AF_METERING_FNF_GET_SAMPLED_DATA) {
      GpfSampleLog *sampleLog = &structuredData[i].dailyConsumptionLog;
      if (sampleLog->catchup) {
        getSampleData(localEndpoint, remoteEndpoint, remoteNodeId, sampleLog);
      }
      sampleLog = &structuredData[i].profileDataLog;
      if (sampleLog->catchup) {
        getSampleData(localEndpoint, remoteEndpoint, remoteNodeId, sampleLog);
      }
      structuredData[i].functionalNotificationFlags &= ~EMBER_AF_METERING_FNF_GET_SAMPLED_DATA;
      emberEventControlSetDelayMS(emberAfPluginGasProxyFunctionCatchupEventControl,
                                  MILLISECOND_TICKS_PER_SECOND);
      continue;
    }

    if (structuredData[i].notificationFlags4 & EMBER_AF_METERING_NF4_GET_PREPAY_SNAPSHOT) {
      GpfPrepaySnapshotLog *prepaySnapshotLog = &structuredData[i].prepayDailyReadLog;
      if (prepaySnapshotLog->catchup) {
        prepaySnapshotLog->catchupSnapshotOffset = 0;
        getPrepaySnapshot(localEndpoint, remoteEndpoint, remoteNodeId, prepaySnapshotLog);
      }
      prepaySnapshotLog = &structuredData[i].billingDataLog.prepaySnapshot;
      if (prepaySnapshotLog->catchup) {
        prepaySnapshotLog->catchupSnapshotOffset = 0;
        getPrepaySnapshot(localEndpoint, remoteEndpoint, remoteNodeId, prepaySnapshotLog);
      }
      structuredData[i].notificationFlags4 &= ~EMBER_AF_METERING_NF4_GET_PREPAY_SNAPSHOT;
      emberEventControlSetDelayMS(emberAfPluginGasProxyFunctionCatchupEventControl,
                                  MILLISECOND_TICKS_PER_SECOND);
      continue;
    }

    if (structuredData[i].notificationFlags4 & EMBER_AF_METERING_NF4_GET_TOP_UP_LOG) {
      GpfTopUpLog *topUpLog = &structuredData[i].billingDataLog.topUp;
      if (topUpLog->catchup) {
        getTopUp(localEndpoint, remoteEndpoint, remoteNodeId, topUpLog);
      }
      structuredData[i].notificationFlags4 &= ~EMBER_AF_METERING_NF4_GET_TOP_UP_LOG;
      emberEventControlSetDelayMS(emberAfPluginGasProxyFunctionCatchupEventControl,
                                  MILLISECOND_TICKS_PER_SECOND);
      continue;
    }

    if (structuredData[i].notificationFlags4 & EMBER_AF_METERING_NF4_GET_DEBT_REPAYMENT_LOG) {
      GpfDebtLog *debtLog = &structuredData[i].billingDataLog.debt;
      if (debtLog->catchup) {
        getDebt(localEndpoint, remoteEndpoint, remoteNodeId, debtLog);
      }
      structuredData[i].notificationFlags4 &= ~EMBER_AF_METERING_NF4_GET_DEBT_REPAYMENT_LOG;
      emberEventControlSetDelayMS(emberAfPluginGasProxyFunctionCatchupEventControl,
                                  MILLISECOND_TICKS_PER_SECOND);
      continue;
    }
  }
}

/** @brief Simple Metering Cluster Get Notified Message
 *
 * @param notificationScheme   Ver.: always
 * @param notificationFlagAttributeId   Ver.: always
 * @param notificationFlagsN   Ver.: always
 */
boolean emberAfSimpleMeteringClusterGetNotifiedMessageCallback(int8u notificationScheme,
                                                               int16u notificationFlagAttributeId,
                                                               int32u notificationFlagsN)
{
  EmberAfClusterCommand *cmd = emberAfCurrentCommand();
  int8u endpoint = emberAfCurrentEndpoint();
  int8u i = findStructuredData(endpoint);

  /*
   * From GBCS
   *
   * For clarity, the GSME:
   *
   *  - shall not action ZSE / ZCL commands received from the GPF in relation
   *    to any of the flags within NotificationFlags2, NotificationFlags3 and
   *    NotificationFlags5;
   *
   *  - for NotificationFlags4, shall only action ZSE / ZCL commands received
   *    from the GPF in relation to the flags specified below.
   *
   *      Bit Number    Waiting Command
   *      6             Get Prepay Snapshot
   *      7             Get Top Up Log
   *      9             Get Debt Repayment Log
   *
   *  - for FunctionalNotificationFlags, shall only action ZSE / ZCL commands
   *    received from the GPF in relation to the flags specified below
   *
   *      Bit Number   Waiting Command
   *      0            New OTA Firmware
   *      1            CBKE Update Request
   *      4            Stay Awake Request HAN
   *      5            Stay Awake Request WAN
   *      6-8          Push Historical Metering Data Attribute Set
   *      9-11         Push Historical Prepayment Data Attribute Set
   *      12           Push All Static Data - Basic Cluster
   *      13           Push All Static Data - Metering Cluster
   *      14           Push All Static Data - Prepayment Cluster
   *      15           NetworkKeyActive
   *      21           Tunnel Message Pending
   *      22           GetSnapshot
   *      23           GetSampledData
   */

  if (i == GPF_INVALID_LOG_INDEX || notificationScheme != 0x02
      || (notificationFlagAttributeId != ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID
          && notificationFlagAttributeId != ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID)) {
    return FALSE;
  }

  // Since this request could result in many commands being sent back to the
  // sleepy device we schedule the work for the catchup event handler which can
  // deal with spacing out the commands.
  structuredData[i].remoteEndpoint = cmd->apsFrame->sourceEndpoint;
  structuredData[i].remoteNodeId = cmd->source;
  structuredData[i].functionalNotificationFlags |=
      (notificationFlagAttributeId == ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID) ? notificationFlagsN : 0;
  structuredData[i].notificationFlags4 |=
      (notificationFlagAttributeId == ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID) ? notificationFlagsN : 0;
  emberEventControlSetActive(emberAfPluginGasProxyFunctionCatchupEventControl);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

/** @brief Simple Metering Cluster Client Default Response
 *
 * This function is called when the client receives the default response from
 * the server.
 *
 * @param endpoint Destination endpoint  Ver.: always
 * @param commandId Command id  Ver.: always
 * @param status Status in default response  Ver.: always
 */
void emberAfSimpleMeteringClusterClientDefaultResponseCallback(int8u endpoint,
                                                               int8u commandId,
                                                               EmberAfStatus status)
{
  int8u i = findStructuredData(endpoint);

  if (commandId == ZCL_MIRROR_REMOVED_COMMAND_ID && status == EMBER_ZCL_STATUS_SUCCESS) {
    emberAfEndpointEnableDisable(endpoint, FALSE);
    emberAfPluginGasProxyFunctionPrintln("GPF: Disabling endpoint %u", endpoint);
    return;
  }

  if (i == GPF_INVALID_LOG_INDEX) {
    return;
  }

  if (commandId == ZCL_GET_SAMPLED_DATA_COMMAND_ID) {
    GpfSampleLog *sampleLog = &structuredData[i].dailyConsumptionLog;
    GpfSampleLog *otherSampleLog = &structuredData[i].profileDataLog;
    if (sampleLog->catchup && sampleLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
      stopSampleLogCatchup(endpoint, sampleLog, otherSampleLog);
    } else {
      sampleLog = &structuredData[i].profileDataLog;
      otherSampleLog = &structuredData[i].dailyConsumptionLog;
      if (sampleLog->catchup && sampleLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
        stopSampleLogCatchup(endpoint, sampleLog, otherSampleLog);
      }
    }
  } else if (commandId == ZCL_GET_SNAPSHOT_COMMAND_ID) {
    GpfSnapshotLog *snapshotLog = &structuredData[i].dailyReadLog;
    GpfSnapshotLog *otherSnapshotLog = &structuredData[i].billingDataLog.snapshot;
    if (snapshotLog->catchup && snapshotLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
      stopSnapshotLogCatchup(endpoint, snapshotLog, otherSnapshotLog);
    } else {
      snapshotLog = &structuredData[i].billingDataLog.snapshot;
      otherSnapshotLog = &structuredData[i].dailyReadLog;
      if (snapshotLog->catchup && snapshotLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
        stopSnapshotLogCatchup(endpoint, snapshotLog, otherSnapshotLog);
      }
    }
  }
}

/** @brief Prepayment Cluster Client Default Response
 *
 * This function is called when the client receives the default response from
 * the server.
 *
 * @param endpoint Destination endpoint  Ver.: always
 * @param commandId Command id  Ver.: always
 * @param status Status in default response  Ver.: always
 */
void emberAfPrepaymentClusterClientDefaultResponseCallback(int8u endpoint,
                                                           int8u commandId,
                                                           EmberAfStatus status)
{
  int8u i = findStructuredData(endpoint);

  if (i == GPF_INVALID_LOG_INDEX) {
    return;
  }

  if (commandId == ZCL_GET_PREPAY_SNAPSHOT_COMMAND_ID) {
    GpfPrepaySnapshotLog *prepaySnapshotLog = &structuredData[i].prepayDailyReadLog;
    GpfPrepaySnapshotLog *otherPrepaySnapshotLog = &structuredData[i].billingDataLog.prepaySnapshot;
    if (prepaySnapshotLog->catchup && prepaySnapshotLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
      stopPrepaySnapshotLogCatchup(endpoint, prepaySnapshotLog, otherPrepaySnapshotLog);
    } else {
      prepaySnapshotLog = &structuredData[i].billingDataLog.prepaySnapshot;
      otherPrepaySnapshotLog = &structuredData[i].prepayDailyReadLog;
      if (prepaySnapshotLog->catchup && prepaySnapshotLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
        stopPrepaySnapshotLogCatchup(endpoint, prepaySnapshotLog, otherPrepaySnapshotLog);
      }
    }
  } else if (commandId == ZCL_GET_TOP_UP_LOG_COMMAND_ID) {
    GpfTopUpLog *topUpLog = &structuredData[i].billingDataLog.topUp;
    if (topUpLog->catchup && topUpLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
      stopTopUpLogCatchup(endpoint, topUpLog);
    }
  } else if (commandId == ZCL_GET_DEBT_REPAYMENT_LOG_COMMAND_ID) {
    GpfDebtLog *debtLog = &structuredData[i].billingDataLog.debt;
    if (debtLog->catchup && debtLog->catchupSequenceNumber == emberAfCurrentCommand()->seqNum) {
      stopDebtLogCatchup(endpoint, debtLog);
    }
  }
}

/** @brief Simple Metering Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfSimpleMeteringClusterServerAttributeChangedCallback(int8u endpoint,
                                                                EmberAfAttributeId attributeId)
{
  int8u i = findStructuredData(endpoint);

  if (i == GPF_INVALID_LOG_INDEX
      || !structuredData[i].alternativeHistoricalConsumption.catchup) {
    return;
  }

  // When the node reboots we reset the Alternative Historical Consumption
  // Attribute Set and then set the EMBER_AF_METERING_FNF_PUSH_HISTORICAL_METERING_DATA_ATTRIBUTE_SET
  // flag within the functional notification flags attribute.  This will indicate
  // to the GSME that is should push the alternative historical cost consumption attributes.
  // We don't check that every attribute is sent but we do need to reset the
  // functional notification flags when the GSME pushes this data so we check
  // for one of the attributes and when we receive it then the flag is reset.
  if (attributeId == ZCL_PREVIOUS_DAY_ALTERNATIVE_CONSUMPTION_DELIVERED_ATTRIBUTE_ID) {
    structuredData[i].alternativeHistoricalConsumption.catchup = FALSE;
    clearNotificationFlag(endpoint,
                          ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                          EMBER_AF_METERING_FNF_PUSH_HISTORICAL_METERING_DATA_ATTRIBUTE_SET);
  }
}

/** @brief Prepayment Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfPrepaymentClusterServerAttributeChangedCallback(int8u endpoint,
                                                            EmberAfAttributeId attributeId)
{
  int8u i = findStructuredData(endpoint);

  if (i == GPF_INVALID_LOG_INDEX
    || !structuredData[i].historicalCostConsumption.catchup) {
    return;
  }

  // When the node reboots we reset the Historical Cost Consumption Information
  // Attribute Set and then set the EMBER_AF_METERING_FNF_PUSH_HISTORICAL_PREPAYMENT_DATA_ATTRIBUTE_SET
  // flag within the functional notification flags attribute.  This will indicate
  // to the GSME that is should push the historical cost consumption attributes.
  // We don't check that every attribute is sent but we do need to reset the
  // functional notification flags when the GSME pushes this data so we check
  // for one of the attributes and when we receive it then the flag is reset.
  if (attributeId == ZCL_PREVIOUS_DAY_COST_CONSUMPTION_DELIVERED_ATTRIBUTE_ID) {
    structuredData[i].historicalCostConsumption.catchup = FALSE;
    clearNotificationFlag(endpoint,
                          ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID,
                          EMBER_AF_METERING_FNF_PUSH_HISTORICAL_PREPAYMENT_DATA_ATTRIBUTE_SET);
  }
}
