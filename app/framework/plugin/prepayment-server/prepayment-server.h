#ifndef _PREPAYMENT_SERVER_H_
#define _PREPAYMENT_SERVER_H_


// Snapshot payload type, D7.2.4.2.2 - Publish Prepay Snapshot Command
enum{
  SNAPSHOT_PAYLOAD_TYPE_DEBT_OR_CREDIT_STATUS = 0x00,
};

#define SNAPSHOT_PAYLOAD_LEN 24 



/**
 * @brief Sends a Publish Prepay Snapshot command, sent in response to the Get Prepay Snapshot command..
 * @param nodeId The short address of the destination device.
 * @param srcEndpoint The source endpoint used in the ZigBee transmission.
 * @param dstEndpoint The destination endpoint used in the ZigBee transmission.
 * @param snapshotId A unique identifier allocated by the device that created the snapshot.
 * @param snapshotTime The UTC time when the snapshot was taken.
 * @param totalSnapshotsFound The number of snapshots that matched the criteria in the received Get Prepay Snapshot command.
 * @param commandIndex Indicates a fragment number if the entire payload won't fit into 1 message.
 * @param totalNumberOfCommands The total number of subcommands that will be sent.
 * @param snapshotCause A 32-bit bitmap that indicates the cause of the snapshot.
 * @param snapshotPayloadType An 8-bit enum that defines the format of the snapshot payload.
 * @param snapshotPayload Data that was created with the snapshot.
 *
 **/
void emberAfPluginPrepaymentServerPublishPrepaySnapshot( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint,
                                                         int32u snapshotId, int32u snapshotTime,
                                                         int8u totalSnapshotsFound, int8u commandIndex,
                                                         int8u totalNumberOfCommands,
                                                         int32u snapshotCause,
                                                         int8u  snapshotPayloadType,
                                                         int8u *snapshotPayload );


/**
 * @brief Sends a Publish Top Up Log command, sent when a top up is performed, or in response to a Get Top Up Log command.
 * @param nodeId  The short address of the destination device.
 * @param srcEndpoint The source endpoint used in the ZigBee transmission.
 * @param dstEndpoint The destination endpoint used in the ZigBee transmission.
 * @param commandIndex Indicates a fragment number if the entire payload won't fit into 1 message.
 * @param totalNumberOfCommands The total number of subcommands that will be sent.
 * @param topUpPayload Information that is sent from each top up log entry.
 *
 **/
void emberAfPluginPrepaymentServerPublishTopUpLog( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint,
                                                   int8u commandIndex, int8u totalNumberOfCommands,
                                                   TopUpPayload *topUpPayload  );

/**
 * @brief Sends a Publish Debt Log command.
 * @param nodeId The short address of the destination device.
 * @param srcEndpoint The source endpoint used in the ZigBee transmission.
 * @param dstEndpoint The destination endpoint used in the ZigBee transmission.
 * @param commandIndex Indicates a fragment number if the entire payload won't fit into 1 message.
 * @param totalNumberOfCommands The total number of subcommands that will be sent.
 * @param debtPayload Includes the contents of a debt record from the log.
 *
 **/
void emberAfPluginPrepaymentServerPublishDebtLog( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint,
                                                  int8u commandIndex, int8u totalNumberOfCommands,
                                                  DebtPayload *debtPayload );


#endif  // #ifndef _PREPAYMENT_SERVER_H_

