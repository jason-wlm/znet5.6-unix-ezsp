// *****************************************************************************
// * meter-snapshot-client.c
// *
// * Code to handle meter snapshot client behavior.
// * 
// * Copyright 2013 by Silicon Labs. All rights reserved.                   *80*
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"

boolean emberAfSimpleMeteringClusterScheduleSnapshotResponseCallback(int32u issuerEventId,
                                                                     int8u* snapshotResponsePayload)
{
  // Forward the response payload to the backhaul for processing
  emberAfPluginMeterSnapshotClientScheduleSnapshotResponseCallback(issuerEventId,
                                                                   snapshotResponsePayload);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfSimpleMeteringClusterTakeSnapshotResponseCallback(int32u snapshotId,
                                                                 int8u snapshotConfirmation)
{
  // Forward the snapshot information to the backhaul for processing
  emberAfPluginMeterSnapshotClientTakeSnapshotResponseCallback(snapshotId,
                                                               snapshotConfirmation);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfSimpleMeteringClusterPublishSnapshotCallback(int32u snapshotId,
                                                            int32u snapshotTime,
                                                            int8u totalSnapshotsFound,
                                                            int8u commandIndex,
                                                            int8u totalCommands,
                                                            int32u snapshotCause,
                                                            int8u snapshotPayloadType,
                                                            int8u* snapshotPayload)
{
  // Forward the snapshot information to the backhaul for processing
  emberAfPluginMeterSnapshotClientPublishSnapshotCallback(snapshotId,
                                                          snapshotTime,
                                                          totalSnapshotsFound,
                                                          commandIndex,
                                                          totalCommands,
                                                          snapshotCause,
                                                          snapshotPayloadType,
                                                          snapshotPayload);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}
