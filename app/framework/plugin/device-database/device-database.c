// ****************************************************************************
// * device-database.c
// *
// * A list of all devices known in the network and their services.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                             *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/plugin/device-database/device-database.h"

//============================================================================
// Globals

static EmberAfDeviceInfo deviceDatabase[EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES];

#define INVALID_INDEX 0xFFFF

#define PLUGIN_NAME "Device-DB"

//============================================================================
// Forward Declarations

void emberAfPluginDeviceDatabaseDiscoveryCompleteCallback(const EmberAfDeviceInfo* device);

//============================================================================

void emberAfPluginDeviceDatabaseInitCallback(void)
{
  MEMSET(deviceDatabase, 0xFF, sizeof(EmberAfDeviceInfo) * EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES);
}

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseGetDeviceByIndex(int16u index)
{
  if (index > EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES) {
    return NULL;
  }

  if (emberAfMemoryByteCompare(deviceDatabase[index].eui64, EUI64_SIZE, 0xFF)) {
    return NULL;
  }

  return &(deviceDatabase[index]);
}

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseFindDeviceByStatus(EmberAfDeviceDiscoveryStatus status)
{
  int16u i;
  for (i = 0; i < EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES; i++) {
    if (!emberAfMemoryByteCompare(deviceDatabase[i].eui64, EUI64_SIZE, 0xFF)
        && (deviceDatabase[i].status & status)) {
      return &(deviceDatabase[i]);
    }
  }
  return NULL;
}

static EmberAfDeviceInfo* findDeviceByEui64(const EmberEUI64 eui64)
{
  int16u i;
  for (i = 0; i < EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES; i++) {
    if (0 == MEMCOMPARE(eui64, deviceDatabase[i].eui64, EUI64_SIZE)) {
      return &(deviceDatabase[i]);
    }
  }
  return NULL;
}

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseFindDeviceByEui64(EmberEUI64 eui64)
{
  return findDeviceByEui64(eui64);
}

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseAddDeviceWithAllInfo(const EmberAfDeviceInfo* newDevice)
{
  if (NULL != findDeviceByEui64(newDevice->eui64)) {
    emberAfCorePrint("Error: %p cannot add device that already exists: ", PLUGIN_NAME);
    emberAfPrintLittleEndianEui64(newDevice->eui64);
    emberAfCorePrintln("");    
    return NULL;
  }
  EmberEUI64 nullEui64;
  MEMSET(nullEui64, 0xFF, EUI64_SIZE);
  EmberAfDeviceInfo* device = findDeviceByEui64(nullEui64);  
  if (device != NULL) {
    MEMMOVE(device, newDevice, sizeof(EmberAfDeviceInfo));
    emberAfPluginDeviceDatabaseDiscoveryCompleteCallback(device);
  }
  return device;
}

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseAdd(EmberEUI64 eui64, int8u zigbeeCapabalities)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(eui64);
  if (device == NULL) {
    EmberEUI64 nullEui64;
    MEMSET(nullEui64, 0xFF, EUI64_SIZE);
    device = findDeviceByEui64(nullEui64);
    if (device != NULL) {
      MEMMOVE(device->eui64, eui64, EUI64_SIZE);
      device->status = EMBER_AF_DEVICE_DISCOVERY_STATUS_NEW;
      device->discoveryFailures = 0;
      device->capabilities = zigbeeCapabalities;
      device->endpointCount = 0;
    }
  }
  return device;
}

boolean emberAfPluginDeviceDatabaseEraseDevice(EmberEUI64 eui64)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(eui64);
  if (device != NULL) {
    MEMSET(device, 0xFF, sizeof(EmberAfDeviceInfo));
    return TRUE;
  }
  return FALSE;
}

boolean emberAfPluginDeviceDatabaseSetEndpoints(const EmberEUI64 eui64, 
                                                const int8u* endpointList,
                                                int8u endpointCount)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(eui64);
  if (device == NULL) {
    emberAfCorePrint("Error: %p cannot add endpoints.  No such device in database: ");
    emberAfPrintLittleEndianEui64(eui64);
    emberAfCorePrintln("");
    return FALSE;
  }

  // Clear all existing endpoints so there is no leftover clusters or endpoints.
  MEMSET(device->endpoints, 
         0xFF,
         sizeof(EmberAfEndpointInfoStruct) * EMBER_AF_MAX_ENDPOINTS_PER_DEVICE);

  device->endpointCount = (endpointCount < EMBER_AF_MAX_ENDPOINTS_PER_DEVICE
                           ? endpointCount
                           : EMBER_AF_MAX_ENDPOINTS_PER_DEVICE);

  int8u i;
  for (i = 0; i < device->endpointCount; i++) {
    device->endpoints[i].endpoint = *endpointList;
    device->endpoints[i].clusterCount = 0;
    endpointList++;
  }
  return TRUE;
}

static int8u findEndpoint(EmberAfDeviceInfo* device, int8u endpoint)
{
  int8u i;
  for (i = 0; i < EMBER_AF_MAX_ENDPOINTS_PER_DEVICE; i++) {
    if (endpoint == device->endpoints[i].endpoint) {
      return i;
    }
  }
  return 0xFF;
}

int8u emberAfPluginDeviceDatabaseGetDeviceEndpointFromIndex(const EmberEUI64 eui64, 
                                                            int8u index)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(eui64);
  if (device != NULL
      && index < EMBER_AF_MAX_ENDPOINTS_PER_DEVICE) {
    return device->endpoints[index].endpoint;
  }
  return 0xFF;
}

int8u emberAfPluginDeviceDatabaseGetIndexFromEndpoint(int8u endpoint,
                                                      const EmberEUI64 eui64)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(eui64);
  int8u index = (device != NULL 
                 ? findEndpoint(device, endpoint)
                 : 0xFF);
  return index;
}

boolean emberAfPluginDeviceDatabaseSetClustersForEndpoint(const EmberEUI64 eui64,
                                                          const EmberAfClusterList* clusterList)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(eui64);
  int8u index =  emberAfPluginDeviceDatabaseGetIndexFromEndpoint(clusterList->endpoint, eui64);
  if (index == 0xFF) {
    emberAfCorePrintln("Error: %p endpoint %d does not exist for device.", PLUGIN_NAME, clusterList->endpoint);
    return FALSE;
  }

  int8u doServer;
  device->endpoints[index].profileId = clusterList->profileId;
  device->endpoints[index].deviceId = clusterList->deviceId;
  device->endpoints[index].clusterCount = clusterList->inClusterCount + clusterList->outClusterCount;
  if (device->endpoints[index].clusterCount > EMBER_AF_MAX_CLUSTERS_PER_ENDPOINT) {
    emberAfCorePrintln("%p too many clusters (%d) for endpoint.  Limiting to %d",
                       PLUGIN_NAME,
                       device->endpoints[index].clusterCount,
                       EMBER_AF_MAX_CLUSTERS_PER_ENDPOINT);
    device->endpoints[index].clusterCount = EMBER_AF_MAX_CLUSTERS_PER_ENDPOINT;
  }
  int8u deviceClusterIndex = 0;
  for (doServer = 0; doServer < 2; doServer++) {
    int8u clusterPointerIndex;
    int8u count = (doServer ? clusterList->inClusterCount : clusterList->outClusterCount);
    const int16u* clusterPointer = (doServer ? clusterList->inClusterList : clusterList->outClusterList);

    for (clusterPointerIndex = 0;
         (clusterPointerIndex < count)
         && (deviceClusterIndex < device->endpoints[index].clusterCount);
         clusterPointerIndex++) {
      device->endpoints[index].clusters[deviceClusterIndex].clusterId = clusterPointer[clusterPointerIndex];
      device->endpoints[index].clusters[deviceClusterIndex].server = (doServer ? TRUE : FALSE);
      deviceClusterIndex++;
    }
  }
  return TRUE;  
}

boolean emberAfPluginDeviceDatabaseClearAllFailedDiscoveryStatus(int8u maxFailureCount)
{
  boolean atLeastOneCleared = FALSE;
  int16u i;
  for (i = 0; i < EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES; i++) {
    if (emberAfMemoryByteCompare(deviceDatabase[i].eui64, EUI64_SIZE, 0xFF)) {
      continue;
    }
    if (EMBER_AF_DEVICE_DISCOVERY_STATUS_FAILED == deviceDatabase[i].status
        && deviceDatabase[i].discoveryFailures < maxFailureCount) {
      deviceDatabase[i].status = EMBER_AF_DEVICE_DISCOVERY_STATUS_FIND_ENDPOINTS;
      atLeastOneCleared = TRUE;
    }
  }
  return atLeastOneCleared;
}

boolean emberAfPluginDeviceDatabaseSetStatus(const EmberEUI64 deviceEui64, EmberAfDeviceDiscoveryStatus newStatus)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(deviceEui64);
  if (device != NULL) {
    device->status = newStatus;
    if (device->status == EMBER_AF_DEVICE_DISCOVERY_STATUS_FAILED) {
      device->discoveryFailures++;
    } else if (device->status == EMBER_AF_DEVICE_DISCOVERY_STATUS_DONE) {
      emberAfPluginDeviceDatabaseDiscoveryCompleteCallback(device);
    }
    return TRUE;
  }
  return FALSE;
}

static void doesDeviceHaveCluster(const EmberAfDeviceInfo* device,
                                  EmberAfClusterId clusterToFind,
                                  boolean server,
                                  int8u* returnEndpoint)
{
  int8u i;
  *returnEndpoint = EMBER_AF_INVALID_ENDPOINT;
  for (i = 0; i < EMBER_AF_MAX_ENDPOINTS_PER_DEVICE; i++) {
    int8u j;
    for (j = 0; j < EMBER_AF_MAX_CLUSTERS_PER_ENDPOINT; j++) {
      if (device->endpoints[i].clusters[j].clusterId == clusterToFind
          && device->endpoints[i].clusters[j].server == server) {
        *returnEndpoint = device->endpoints[i].endpoint;
        return;
      }
    }
  }
}

EmberStatus emberAfPluginDeviceDatabaseDoesDeviceHaveCluster(EmberEUI64 deviceEui64,
                                                             EmberAfClusterId clusterToFind,
                                                             boolean server,
                                                             int8u* returnEndpoint)
{
  EmberAfDeviceInfo* device = findDeviceByEui64(deviceEui64);
  if (device == NULL) {
    return EMBER_INVALID_CALL;
  }
  doesDeviceHaveCluster(device, clusterToFind, server, returnEndpoint);
  return EMBER_SUCCESS;
}

void emberAfPluginDeviceDatabaseCreateNewSearch(EmberAfDeviceDatabaseIterator* iterator)
{
  iterator->deviceIndex = 0;
}

EmberStatus emberAfPluginDeviceDatabaseFindDeviceSupportingCluster(EmberAfDeviceDatabaseIterator* iterator,
                                                                   EmberAfClusterId clusterToFind,
                                                                   boolean server,
                                                                   int8u* returnEndpoint)
{
  if (iterator->deviceIndex >= EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES) {
    // This was the most appropriate error code I could come up with to say, "Search Complete".
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  doesDeviceHaveCluster(&(deviceDatabase[iterator->deviceIndex]), clusterToFind, server, returnEndpoint);
  iterator->deviceIndex++;
  return EMBER_SUCCESS;
}
