#ifndef _PREPAYMENT_SNAPSHOT_STORAGE_H_
#define _PREPAYMENT_SNAPSHOT_STORAGE_H_


// 0 is technically valid, but we'll designate it to be our "invalid" value.
#define INVALID_SNAPSHOT_ID           0
#define INVALID_SNAPSHOT_SCHEDULE_ID  0


void emberAfPluginPrepaymentSnapshotStorageInitCallback( int8u endpoint );

int32u emberAfPluginPrepaymentSnapshotStorageTakeSnapshot( int8u endpoint, int32u snapshotCause );

int8u emberAfPluginPrepaymentServerGetSnapshotCallback( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint,
                                                        int32u startTime,
                                                        int32u endTime,
                                                        int8u  snapshotOffset,
                                                        int32u snapshotCause );


/** @brief Defined snapshot causes.
 */
enum{
  SNAPSHOT_CAUSE_GENERAL                        = ( ((int32u)1) << 0 ),
  SNAPSHOT_CAUSE_END_OF_BILLING_PERIOD          = ( ((int32u)1) << 1 ),
  SNAPSHOT_CAUSE_CHANGE_OF_TARIFF_INFO          = ( ((int32u)1) << 3 ),
  SNAPSHOT_CAUSE_CHANGE_OF_PRICE_MATRIX         = ( ((int32u)1) << 4 ),
  SNAPSHOT_CAUSE_MANUALLY_TRIGGERED_FROM_CLIENT = ( ((int32u)1) << 10 ),
  SNAPSHOT_CAUSE_CHANGE_OF_TENANCY              = ( ((int32u)1) << 12 ),
  SNAPSHOT_CAUSE_CHANGE_OF_SUPPLIER             = ( ((int32u)1) << 13 ),
  SNAPSHOT_CAUSE_CHANGE_OF_METER_MODE           = ( ((int32u)1) << 14 ),
  SNAPSHOT_CAUSE_TOP_UP_ADDITION                = ( ((int32u)1) << 18 ),
  SNAPSHOT_CAUSE_DEBT_OR_CREDIT_ADDITION        = ( ((int32u)1) << 19 ),
};
#define SNAPSHOT_CAUSE_ALL_SNAPSHOTS  0xFFFFFFFF



typedef struct{
  int32u snapshotId;
  int32u snapshotCauseBitmap;
  int32u snapshotTime;    // Time snapshot was taken.
  int8u  snapshotType;
//  EmberNodeId requestingId;

  // For now, only 1 snapshot type exists (Debt/Credit), so there is only 1 payload.
  int32s accumulatedDebt;
  int32u type1DebtRemaining;
  int32u type2DebtRemaining;
  int32u type3DebtRemaining;
  int32s emergencyCreditRemaining;
  int32s creditRemaining;

} EmberAfPrepaymentSnapshotPayload;


typedef struct{
  // Stores fields needed to schedule a new snapshot.
  int32u snapshotScheduleId;
  int32u snapshotStartTime;
  int32u snapshotCauseBitmap;
  EmberNodeId requestingId;
  int8u srcEndpoint;
  int8u dstEndpoint;
  int8u snapshotPayloadType;
} EmberAfPrepaymentSnapshotSchedulePayload;

EmberStatus emberAfPluginPrepaymentSnapshotStoragePublishSnapshot(EmberNodeId nodeId, 
                                                                  int8u srcEndpoint,
                                                                  int8u dstEndpoint,
                                                                  int32u snapshotTableIndex);


#endif  // #ifndef _PREPAYMENT_SNAPSHOT_STORAGE_H_

