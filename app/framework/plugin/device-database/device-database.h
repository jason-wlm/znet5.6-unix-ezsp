

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseGetDeviceByIndex(int16u index);

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseFindDeviceByStatus(EmberAfDeviceDiscoveryStatus status);

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseFindDeviceByEui64(EmberEUI64 eui64);

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseAdd(EmberEUI64 eui64, int8u zigbeeCapabilities);

boolean emberAfPluginDeviceDatabaseEraseDevice(EmberEUI64 eui64);

boolean emberAfPluginDeviceDatabaseSetEndpoints(const EmberEUI64 eui64, 
                                                const int8u* endpointList,
                                                int8u endpointCount);

int8u emberAfPluginDeviceDatabaseGetDeviceEndpointFromIndex(const EmberEUI64 eui64, 
                                                            int8u index);

// Explicitly made the eui64 the second argument to prevent confusion between
// this function and the emberAfPluginDeviceDatabaseGetDeviceEndpointsFromIndex()
int8u emberAfPluginDeviceDatabaseGetIndexFromEndpoint(int8u endpoint,
                                                      const EmberEUI64 eui64);

boolean emberAfPluginDeviceDatabaseSetClustersForEndpoint(const EmberEUI64 eui64,
                                                          const EmberAfClusterList* clusterList);

boolean emberAfPluginDeviceDatabaseClearAllFailedDiscoveryStatus(int8u maxFailureCount);

const char* emberAfPluginDeviceDatabaseGetStatusString(EmberAfDeviceDiscoveryStatus status);

boolean emberAfPluginDeviceDatabaseSetStatus(const EmberEUI64 deviceEui64, EmberAfDeviceDiscoveryStatus newStatus);

const EmberAfDeviceInfo* emberAfPluginDeviceDatabaseAddDeviceWithAllInfo(const EmberAfDeviceInfo* newDevice);

EmberStatus emberAfPluginDeviceDatabaseDoesDeviceHaveCluster(EmberEUI64 deviceEui64,
                                                             EmberAfClusterId clusterToFind,
                                                             boolean server,
                                                             int8u* returnEndpoint);

void emberAfPluginDeviceDatabaseCreateNewSearch(EmberAfDeviceDatabaseIterator* iterator);

EmberStatus emberAfPluginDeviceDatabaseFindDeviceSupportingCluster(EmberAfDeviceDatabaseIterator* iterator,
                                                                   EmberAfClusterId clusterToFind,
                                                                   boolean server,
                                                                   int8u* returnEndpoint);

