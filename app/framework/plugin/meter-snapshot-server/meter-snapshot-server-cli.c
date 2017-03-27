// *******************************************************************                                                                                        
// * events-server-cli.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*                                                                              
// ******************************************************************* 

#include "app/framework/include/af.h"

#ifndef EMBER_AF_GENERATE_CLI
  #error The Meter Snapshot Server plugin is not compatible with the legacy CLI.
#endif

// plugin meter-snapshot-server take
void emAfMeterSnapshotServerCliTake(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int32u cause = (int32u)emberUnsignedCommandArgument(1);
  int8u snapshotConfirmation;

  // Attempt to take the snapshot
  emberAfPluginMeterSnapshotServerTakeSnapshotCallback(endpoint,
                                                       cause,
                                                       &snapshotConfirmation);
}

// plugin meter-snapshot-server publish
void emAfMeterSnapshotServerCliPublish(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int32u startTime = (int32u)emberUnsignedCommandArgument(3);
  int32u endTime = (int32u)emberUnsignedCommandArgument(4);
  int32u offset = (int8u)emberUnsignedCommandArgument(5);
  int32u cause = (int32u)emberUnsignedCommandArgument(6);
  int8u snapshotCriteria[13];

  // Package the snapshot criteria for our callback to process
  emberAfCopyInt32u((int8u *)snapshotCriteria, 0, startTime);
  emberAfCopyInt32u((int8u *)snapshotCriteria, 4, endTime);
  snapshotCriteria[8] = offset;
  emberAfCopyInt32u((int8u *)snapshotCriteria, 9, cause);

  emberAfPluginMeterSnapshotServerGetSnapshotCallback(srcEndpoint,
                                                      dstEndpoint,
                                                      nodeId,
                                                      snapshotCriteria);
}
