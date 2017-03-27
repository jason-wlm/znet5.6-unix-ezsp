// *******************************************************************
// * device-management-server.h
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*
// *******************************************************************

#include "app/framework/plugin/device-management-server/device-management-common.h"

boolean emberAfPluginDeviceManagementSetTenancy(EmberAfDeviceManagementTenancy *tenancy, 
                                                boolean validateOptionalFields);
boolean emberAfPluginDeviceManagementGetTenancy(EmberAfDeviceManagementTenancy *tenancy);

boolean emberAfPluginDeviceManagementSetSupplier(int8u endpoint, EmberAfDeviceManagementSupplier *supplier);
boolean emberAfPluginDeviceManagementGetSupplier(EmberAfDeviceManagementSupplier *supplier);

boolean emberAfPluginDeviceManagementSetInfoGlobalData(int32u providerId,
                                                       int32u issuerEventId,
                                                       int8u tariffType);

boolean emberAfPluginDeviceManagementSetSiteId(EmberAfDeviceManagementSiteId *siteId);
boolean emberAfPluginDeviceManagementGetSiteId(EmberAfDeviceManagementSiteId *siteId);

boolean emberAfPluginDeviceManagementSetCIN(EmberAfDeviceManagementCIN *cin);
boolean emberAfPluginDeviceManagementGetCIN(EmberAfDeviceManagementCIN *cin);

boolean emberAfPluginDeviceManagementSetPassword(EmberAfDeviceManagementPassword *password);
boolean emberAfPluginDeviceManagementGetPassword(EmberAfDeviceManagementPassword *password,
                                                 int8u passwordType);

void emberAfDeviceManagementServerPrint(void);

boolean emberAfDeviceManagementClusterUpdateSiteId(EmberNodeId dstAddr,
                                                   int8u srcEndpoint,
                                                   int8u dstEndpoint);

boolean emberAfPluginDeviceManagementSetProviderId(int32u providerId);
boolean emberAfPluginDeviceManagementSetIssuerEventId(int32u issuerEventId);
boolean emberAfPluginDeviceManagementSetTariffType(EmberAfTariffType tariffType);

boolean emberAfDeviceManagementClusterPublishChangeOfTenancy(EmberNodeId dstAddr,
                                                             int8u srcEndpoint,
                                                             int8u dstEndpoint);
boolean emberAfDeviceManagementClusterPublishChangeOfSupplier(EmberNodeId dstAddr,
                                                              int8u srcEndpoint,
                                                              int8u dstEndpoint);

void emberAfDeviceManagementClusterSetPendingUpdates(EmberAfDeviceManagementChangePendingFlags pendingUpdatesMask);
void emberAfDeviceManagementClusterGetPendingUpdates(EmberAfDeviceManagementChangePendingFlags *pendingUpdatesMask);
boolean emberAfDeviceManagementClusterUpdateCIN(EmberNodeId dstAddr,
                                                int8u srcEndpoint,
                                                int8u dstEndpoint);

boolean emberAfDeviceManagementClusterSendRequestNewPasswordResponse(int8u passwordType,
                                                                     EmberNodeId dstAddr,
                                                                     int8u srcEndpoint,
                                                                     int8u dstEndpoint);


