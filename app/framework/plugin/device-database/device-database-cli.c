// ****************************************************************************
// * device-database.c
// *
// * A list of all devices known in the network and their services.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                             *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/plugin/device-database/device-database.h"

//============================================================================
// Globals

typedef struct {
  EmberAfDeviceDiscoveryStatus status;
  const char* string;
} DiscoveryStatusCodeToString;

static const DiscoveryStatusCodeToString statusToString[] = {
  { EMBER_AF_DEVICE_DISCOVERY_STATUS_NONE,           "None" },
  { EMBER_AF_DEVICE_DISCOVERY_STATUS_NEW,            "New"  },
  { EMBER_AF_DEVICE_DISCOVERY_STATUS_FIND_ENDPOINTS, "Find Endpoints" },
  { EMBER_AF_DEVICE_DISCOVERY_STATUS_FIND_CLUSTERS,  "Find Clusters"  },
  { EMBER_AF_DEVICE_DISCOVERY_STATUS_DONE,           "Done" },
  { EMBER_AF_DEVICE_DISCOVERY_STATUS_FAILED,         "Failed" },
};

const char unknownStatus[] = "???";

//============================================================================

const char* emberAfPluginDeviceDatabaseGetStatusString(EmberAfDeviceDiscoveryStatus status)
{
  int8u i;
  for (i = 0; i < sizeof(statusToString) / sizeof(DiscoveryStatusCodeToString); i++) {
    if (status == statusToString[i].status) {
      return statusToString[i].string;
    }
  }
  return unknownStatus;
}

void emAfPluginDeviceDatabasePrintAll(void)
{
  int8u i;
  int8u deviceCount = 0;
  for (i = 0; i < EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES; i++) {
    const EmberAfDeviceInfo* device = emberAfPluginDeviceDatabaseGetDeviceByIndex(i);
    if (device == NULL) {
      continue;
    }
    emberAfCorePrint("%d - ", i);
    deviceCount++;

    emberAfPrintLittleEndianEui64(device->eui64);
    emberAfCorePrintln(" - Capabilities: 0x%X - EPs: %d - Status: %p - Failures:%d",
                       device->capabilities,
                       device->endpointCount,
                       emberAfPluginDeviceDatabaseGetStatusString(device->status),
                       device->discoveryFailures);
  }
  emberAfCorePrintln("%d of %d devices in database", deviceCount, EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES);
}

void emAfPluginDeviceDatabasePrintDevice(void)
{
  EmberEUI64 eui64;
  emberCopyEui64Argument(0, eui64);
  const EmberAfDeviceInfo* device = emberAfPluginDeviceDatabaseFindDeviceByEui64(eui64);
  if (device == NULL) {
    emberAfCorePrintln("Error: Cannot find device in database.");
    return;
  }

  int8u i;
  for (i = 0; i < device->endpointCount; i++) {
    emberAfCorePrintln("EP: %d - Profile ID: 0x%2X - Device ID: 0x%2X - Clusters: %d",
                       device->endpoints[i].endpoint,
                       device->endpoints[i].profileId,
                       device->endpoints[i].deviceId,
                       device->endpoints[i].clusterCount);
    int8u j;
    for (j = 0; j < device->endpoints[i].clusterCount; j++) {
      emberAfCorePrintln("  Cluster: 0x%2X %p", 
                         device->endpoints[i].clusters[j].clusterId, 
                         (device->endpoints[i].clusters[j].server
                          ? "Server"
                          : "Client"));
    }
  }
  emberAfCorePrintln("%d total endpoints", device->endpointCount);
}

#define DUMMY_BASE_ENDPOINT    100
#define DUMMY_BASE_PROFILE_ID  0xA000
#define DUMMY_BASE_DEVICE_ID   0x0200
#define DUMMY_BASE_CLUSTER_ID  0x0000

void emAfPluginDeviceDatabaseAddDummyDevice(void)
{
  EmberAfDeviceInfo dummy;
  MEMSET(&dummy, 0, sizeof(EmberAfDeviceInfo));

  emberCopyEui64Argument(0, dummy.eui64);
  dummy.endpointCount = (int8u)emberUnsignedCommandArgument(1);
  int8u clusterCount = (int8u)emberUnsignedCommandArgument(2);

  if (dummy.endpointCount > EMBER_AF_MAX_ENDPOINTS_PER_DEVICE) {
    emberAfCorePrintln("Error: Cannot add more than %d endpoints", EMBER_AF_MAX_ENDPOINTS_PER_DEVICE);
    return;
  }

  if ((int16u)dummy.endpointCount + (int16u)DUMMY_BASE_ENDPOINT >= 255) {
    emberAfCorePrintln("Error:  Base endpoint of %d + number of endpoints %d would exceed 254", 
                        DUMMY_BASE_ENDPOINT,
                        dummy.endpointCount);
    return;
  }

  if (clusterCount > EMBER_AF_MAX_CLUSTERS_PER_ENDPOINT) {
    emberAfCorePrintln("Error: Cannot add more than %d clusters per endpoint.", EMBER_AF_MAX_CLUSTERS_PER_ENDPOINT);
    return;
  }
  if ((int32u)clusterCount + (int32u)DUMMY_BASE_CLUSTER_ID >= 65535) {
    emberAfCorePrintln("Error: Base cluster ID 0x%2X + number of clusters %d would exceed 65534",
                       DUMMY_BASE_CLUSTER_ID,
                       clusterCount);
    return;
  }

  dummy.status = EMBER_AF_DEVICE_DISCOVERY_STATUS_DONE;

  int8u i;
  for (i = 0; i < dummy.endpointCount; i++) {
    dummy.endpoints[i].endpoint = DUMMY_BASE_ENDPOINT + i;
    dummy.endpoints[i].deviceId = DUMMY_BASE_DEVICE_ID + i;
    dummy.endpoints[i].profileId = DUMMY_BASE_PROFILE_ID + i;
    dummy.endpoints[i].clusterCount = clusterCount;

    int8u j;
    for (j = 0; j < clusterCount; j++) {
      dummy.endpoints[i].clusters[j].clusterId = DUMMY_BASE_CLUSTER_ID + j;
      dummy.endpoints[i].clusters[j].server = TRUE;
    }
  }

  if (NULL == emberAfPluginDeviceDatabaseAddDeviceWithAllInfo(&dummy)) {
    emberAfCorePrintln("Error: Could not add device to database.");
  }
}

void emAfPluginDeviceDatabaseErase(void)
{
  EmberEUI64 eui64;
  emberCopyEui64Argument(0, eui64);
  if (!emberAfPluginDeviceDatabaseEraseDevice(eui64)) {
    emberAfCorePrintln("Error:  Could not delete device.");
  }
}

