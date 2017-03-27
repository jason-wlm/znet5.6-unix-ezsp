// *******************************************************************
// * device-maangement-server-cli.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "device-management-server.h"
#include "app/framework/plugin/device-management-server/device-management-common.h"
#ifndef EMBER_AF_GENERATE_CLI
  #error The Device Management Server plugin is not compatible with the legacy CLI.
#endif

void emAfDeviceManagementServerCliIssuerEventId(void);
void emAfDeviceManagementServerCliPassword(void);
void emAfDeviceManagementServerCliPrint(void);
void emAfDeviceManagementServerCliProviderId(void);
void emAfDeviceManagementServerCliSiteId(void);
void emAfDeviceManagementServerCliSupplier(void);
void emAfDeviceManagementServerCliTariffType(void);
void emAfDeviceManagementServerCliTenancy(void);

void emAfDeviceManagementServerCliPrint(void)
{
  emberAfDeviceManagementServerPrint();
}

void emAfDeviceManagementServerCliTenancy(void)
{
  EmberAfDeviceManagementTenancy tenancy;
  tenancy.implementationDateTime = (int32u) emberUnsignedCommandArgument(0);
  tenancy.tenancy = (int32u) emberUnsignedCommandArgument(1);

  emberAfPluginDeviceManagementSetTenancy(&tenancy, 
                                          FALSE);
}

void emAfDeviceManagementServerCliProviderId(void)
{
  int32u providerId = (int32u) emberUnsignedCommandArgument(0);
  emberAfPluginDeviceManagementSetProviderId(providerId);
}

void emAfDeviceManagementServerCliIssuerEventId(void)
{
  int32u issuerEventId = (int32u) emberUnsignedCommandArgument(0);
  emberAfPluginDeviceManagementSetIssuerEventId(issuerEventId);
}

void emAfDeviceManagementServerCliTariffType(void)
{
  EmberAfTariffType tariffType = (EmberAfTariffType) emberUnsignedCommandArgument(0);
  emberAfPluginDeviceManagementSetTariffType(tariffType);
}

void emAfDeviceManagementServerCliSupplier(void)
{
  int8u length;
  EmberAfDeviceManagementSupplier supplier;
  
  int8u endpoint = (int32u) emberUnsignedCommandArgument(0); 
  supplier.proposedProviderId = (int32u) emberUnsignedCommandArgument(1); 
  supplier.implementationDateTime = (int32u) emberUnsignedCommandArgument(2);
  supplier.providerChangeControl = (int32u) emberUnsignedCommandArgument(3); 
  length = emberCopyStringArgument(4,
                                   supplier.proposedProviderName + 1,
                                   EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_PROPOSED_PROVIDER_NAME_LENGTH,
                                   FALSE);
  supplier.proposedProviderName[0] = length;
  length = emberCopyStringArgument(5,
                                   supplier.proposedProviderContactDetails + 1,
                                   EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_PROPOSED_PROVIDER_CONTACT_DETAILS_LENGTH,
                                   FALSE);
  supplier.proposedProviderContactDetails[0] = length;
  emberAfPluginDeviceManagementSetSupplier(endpoint, &supplier);
}

void emAfDeviceManagementServerCliSiteId(void)
{
  int8u length;
  EmberAfDeviceManagementSiteId siteId;

  length = emberCopyStringArgument(0,
                                   siteId.siteId + 1,
                                   EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_SITE_ID_LENGTH,
                                   FALSE);
  siteId.siteId[0] = length;
  siteId.implementationDateTime = (int32u) emberUnsignedCommandArgument(1);

  emberAfPluginDeviceManagementSetSiteId(&siteId);
}

void emAfDeviceManagementServerCliCIN(void)
{
  int8u length;
  EmberAfDeviceManagementCIN cin;

  length = emberCopyStringArgument(0,
                                   cin.cin + 1,
                                   EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_CIN_LENGTH,
                                   FALSE);
  cin.cin[0] = length;
  cin.implementationDateTime = (int32u) emberUnsignedCommandArgument(1);

  emberAfPluginDeviceManagementSetCIN(&cin);
}

void emAfDeviceManagementServerCliPassword(void)
{
  int8u length;
  EmberAfDeviceManagementPassword password;

  length = emberCopyStringArgument(0,
                                   password.password + 1,
                                   EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_PASSWORD_LENGTH,
                                   FALSE);
  password.password[0] = length;
  password.implementationDateTime = (int32u) emberUnsignedCommandArgument(1);
  password.durationInMinutes = (int16u) emberUnsignedCommandArgument(2);
  password.passwordType = (int8u) emberUnsignedCommandArgument(3);

  emberAfPluginDeviceManagementSetPassword(&password);
}

// plugin device-management-server pub-chg-of-tenancy <dst:2> <src endpoint:1>  <dst endpoint:1>
void emAfDeviceManagementServerCliPublishChangeOfTenancy(void)
{
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  emberAfDeviceManagementClusterPublishChangeOfTenancy(dstAddr, srcEndpoint, dstEndpoint);
}

// plugin device-management-server pub-chg-of-tenancy <dst:2> <src endpoint:1>  <dst endpoint:1>
void emAfDeviceManagementServerCliPublishChangeOfSupplier(void)
{
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  emberAfDeviceManagementClusterPublishChangeOfSupplier(dstAddr, srcEndpoint, dstEndpoint);
}

// plugin device-management-server update-site-id <dst:2> <src endpoint:1>  <dst endpoint:1>
void emAfDeviceManagementServerCliUpdateSiteId(void)
{
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  emberAfDeviceManagementClusterUpdateSiteId(dstAddr, srcEndpoint, dstEndpoint);
}

// plugin device-management-server update-cin <dst:2> <src endpoint:1>  <dst endpoint:1>
void emAfDeviceManagementServerCliUpdateCIN(void)
{
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  emberAfDeviceManagementClusterUpdateCIN(dstAddr, srcEndpoint, dstEndpoint);
}

// plugin device-management-server pendingUpdatesMask <pendingUpdatesMask:1>
void emAfDeviceManagementServerCliPendingUpdates(void)
{
  EmberAfDeviceManagementChangePendingFlags pendingUpdatesMask = (int8u)emberUnsignedCommandArgument(0);
  emberAfDeviceManagementClusterSetPendingUpdates(pendingUpdatesMask);
}

void emAfDeviceManagementServerCliSendRequestNewPasswordResponse(void)
{
  int8u passwordType = (int8u)emberUnsignedCommandArgument(0);
  EmberNodeId dstAddr = (EmberNodeId)emberUnsignedCommandArgument(1);
  int8u srcEndpoint =  (int8u)emberUnsignedCommandArgument(2);
  int8u dstEndpoint =  (int8u)emberUnsignedCommandArgument(3);
  emberAfDeviceManagementClusterSendRequestNewPasswordResponse(passwordType,
                                                               dstAddr,
                                                               srcEndpoint,
                                                               dstEndpoint);
}

