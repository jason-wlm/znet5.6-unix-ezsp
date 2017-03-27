// *******************************************************************
// * device-management-server.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/device-management-server/device-management-common.h"
#include "app/framework/plugin/device-management-server/device-management-server.h"

static EmberAfDeviceManagementInfo info;

void emberAfPluginDeviceManagementServerInitCallback(void){
  // invalidate passwords since 0 means forever valid
  info.servicePassword.durationInMinutes = 1;
  info.consumerPassword.durationInMinutes = 1;

  // since implementationDateTime of 0 is special. we'll be using 1 (12/31/1970)
  // to initialize as a non-valid pending time.
  info.cin.implementationDateTime = 1;
  info.pendingUpdates = 0;
}

boolean emberAfPluginDeviceManagementSetProviderId(int32u providerId)
{
  info.providerId = providerId;
  return TRUE;
}

boolean emberAfPluginDeviceManagementSetIssuerEventId(int32u issuerEventId)
{
  info.issuerEventId = issuerEventId;
  return TRUE;
}

boolean emberAfPluginDeviceManagementSetTariffType(EmberAfTariffType tariffType)
{
  info.tariffType = (EmberAfTariffType )tariffType;
  return TRUE;
}

boolean emberAfPluginDeviceManagementSetInfoGlobalData(int32u providerId,
                                                       int32u issuerEventId,
                                                       int8u tariffType)
{
  info.providerId = providerId;
  info.issuerEventId = issuerEventId;
  info.tariffType = (EmberAfTariffType ) tariffType;

  return TRUE;
}

boolean emberAfPluginDeviceManagementSetTenancy(EmberAfDeviceManagementTenancy *tenancy, 
                                                boolean validateOptionalFields)
{

  if (validateOptionalFields){
    if ((tenancy->providerId != info.providerId)
         || (tenancy->issuerEventId != info.issuerEventId)
         || (tenancy->tariffType != info.tariffType)) {
      return FALSE;
    }
  }

  if (tenancy == NULL) {
    MEMSET(&(info.tenancy), 0, sizeof(EmberAfDeviceManagementTenancy));
  } else {
    MEMMOVE(&(info.tenancy), tenancy, sizeof(EmberAfDeviceManagementTenancy));
  }

  return TRUE;
}

boolean emberAfPluginDeviceManagementGetTenancy(EmberAfDeviceManagementTenancy *tenancy)
{
  if (tenancy == NULL) {
    return FALSE;
  }

  MEMMOVE(tenancy, &(info.tenancy), sizeof(EmberAfDeviceManagementTenancy));

  return TRUE;
}

boolean emberAfPluginDeviceManagementSetSupplier(int8u endpoint, EmberAfDeviceManagementSupplier *supplier)
{
  if (supplier == NULL) {
    MEMSET(&(info.supplier), 0, sizeof(EmberAfDeviceManagementSupplier));
  } else {
    MEMMOVE(&(info.supplier), supplier, sizeof(EmberAfDeviceManagementSupplier));
  }

  if (supplier->implementationDateTime <= emberAfGetCurrentTime()){
    EmberStatus status  = emberAfWriteAttribute(endpoint, 
                                                ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                                ZCL_PROVIDER_NAME_ATTRIBUTE_ID,
                                                CLUSTER_MASK_SERVER,
                                                (int8u *)&(supplier->proposedProviderName),
                                                ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
    if (status == EMBER_ZCL_STATUS_SUCCESS){
      status  = emberAfWriteAttribute(endpoint,
                                      ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                      ZCL_PROVIDER_CONTACT_DETAILS_ATTRIBUTE_ID,
                                      CLUSTER_MASK_SERVER,
                                      (int8u *)&(supplier->proposedProviderContactDetails),
                                      ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
    }
  } else {
    // TODO: need to setup timer to 'schedule' attribute update when implementation is reached.
  }

  return TRUE;
}

boolean emberAfPluginDeviceManagementGetSupplier(EmberAfDeviceManagementSupplier *supplier)
{
  if (supplier == NULL) {
    return FALSE;
  }

  MEMMOVE(supplier, &(info.supplier), sizeof(EmberAfDeviceManagementSupplier));

  return TRUE;
}

boolean emberAfPluginDeviceManagementSetSupply(EmberAfDeviceManagementSupply *supply)
{
  if (supply == NULL) {
    MEMSET(&(info.supply), 0, sizeof(EmberAfDeviceManagementSupply));
  } else {
    MEMMOVE(&(info.supply), supply, sizeof(EmberAfDeviceManagementSupply));
  }

  return TRUE;
}

boolean emberAfPluginDeviceManagementGetSupply(EmberAfDeviceManagementSupply *supply)
{
  if (supply == NULL) {
    return FALSE;
  }

  MEMMOVE(supply, &(info.supply), sizeof(EmberAfDeviceManagementSupply));

  return TRUE;
}

boolean emberAfPluginDeviceManagementSetSiteId(EmberAfDeviceManagementSiteId *siteId)
{
  if (siteId == NULL) {
    MEMSET(&(info.siteId), 0, sizeof(EmberAfDeviceManagementSiteId));
  } else {
    MEMMOVE(&(info.siteId), siteId, sizeof(EmberAfDeviceManagementSiteId));
  }

  return TRUE;
}

boolean emberAfPluginDeviceManagementGetSiteId(EmberAfDeviceManagementSiteId *siteId)
{
  if (siteId == NULL) {
    return FALSE;
  }

  MEMMOVE(siteId, &(info.siteId), sizeof(EmberAfDeviceManagementSiteId));

  return TRUE;
}

boolean emberAfPluginDeviceManagementSetCIN(EmberAfDeviceManagementCIN *cin)
{
  MEMCOPY(&(info.cin), cin, sizeof(EmberAfDeviceManagementCIN));
  return TRUE;
}

boolean emberAfPluginDeviceManagementGetCIN(EmberAfDeviceManagementCIN *cin)
{
  if (cin == NULL){
    return FALSE;
  }

  MEMCOPY(cin, &(info.cin), sizeof(EmberAfDeviceManagementCIN));
  return TRUE;
}

boolean emberAfPluginDeviceManagementSetPassword(EmberAfDeviceManagementPassword *password)
{
  if (password == NULL) {
    MEMSET(&(info.servicePassword), 0, sizeof(EmberAfDeviceManagementPassword));
    MEMSET(&(info.consumerPassword), 0, sizeof(EmberAfDeviceManagementPassword));
  } else {
    if (password->passwordType == SERVICE_PASSWORD) {
      MEMMOVE(&(info.servicePassword), password, sizeof(EmberAfDeviceManagementPassword));
    } else if (password->passwordType == CONSUMER_PASSWORD) {
      MEMMOVE(&(info.consumerPassword), password, sizeof(EmberAfDeviceManagementPassword));
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

boolean emberAfPluginDeviceManagementGetPassword(EmberAfDeviceManagementPassword *password,
                                                 int8u passwordType)
{
  if (password == NULL) {
    return FALSE;
  }

  switch (passwordType) {
    case SERVICE_PASSWORD:
      MEMMOVE(password, &(info.servicePassword), sizeof(EmberAfDeviceManagementPassword));
      break;
    case CONSUMER_PASSWORD:
      MEMMOVE(password, &(info.consumerPassword), sizeof(EmberAfDeviceManagementPassword));
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

void emberAfDeviceManagementServerPrint(void)
{
  emberAfDeviceManagementClusterPrintln("== Device Management Information ==\n");

  emberAfDeviceManagementClusterPrintln("  == Tenancy ==");
  emberAfDeviceManagementClusterPrintln("  Implementation Date / Time: %4x", info.tenancy.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("  Tenancy: %4x\n", info.tenancy.tenancy);

  emberAfDeviceManagementClusterPrintln("  == Supplier ==");
  emberAfDeviceManagementClusterPrint("  Provider name: ");
  emberAfDeviceManagementClusterPrintString(info.supplier.proposedProviderName);
  emberAfDeviceManagementClusterPrintln("\n  Proposed Provider Id: %4x", info.supplier.proposedProviderId);
  emberAfDeviceManagementClusterPrintln("  Implementation Date / Time: %4x", info.supplier.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("  Provider Change Control: %4x\n", info.supplier.providerChangeControl);

  emberAfDeviceManagementClusterPrintln("  == Supply ==");
  emberAfDeviceManagementClusterPrintln("  Request Date / Time: %4x", info.supply.requestDateTime);
  emberAfDeviceManagementClusterPrintln("  Implementation Date / Time: %4x", info.supply.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("  Supply Status: %x", info.supply.supplyStatus);
  emberAfDeviceManagementClusterPrintln("  Originator Id / Supply Control Bits: %x\n", info.supply.originatorIdSupplyControlBits);

  emberAfDeviceManagementClusterPrintln("  == Site ID ==");
  emberAfDeviceManagementClusterPrint("  Site ID: ");
  emberAfDeviceManagementClusterPrintString(info.siteId.siteId);
  emberAfDeviceManagementClusterPrintln("\n  Site Id Implementation Date / Time: %4x\n", info.siteId.implementationDateTime);

  emberAfDeviceManagementClusterPrintln("  == Customer ID Number ==");
  emberAfDeviceManagementClusterPrint("  Customer ID Number: ");
  emberAfDeviceManagementClusterPrintString(info.cin.cin);
  emberAfDeviceManagementClusterPrintln("\n  Customer ID Number Implementation Date / Time: %4x\n", info.cin.implementationDateTime);

  emberAfDeviceManagementClusterPrintln("  == Supply Status ==");
  emberAfDeviceManagementClusterPrintln("  Supply Status: %x\n", info.supplyStatus.supplyStatus);
  emberAfDeviceManagementClusterPrintln("  Implementation Date / Time: %4x", info.supplyStatus.implementationDateTime);

  emberAfDeviceManagementClusterPrintln("  == Passwords ==");

  emberAfDeviceManagementClusterPrintln("   = Service Password =");
  emberAfDeviceManagementClusterPrint("   Password: ");
  emberAfDeviceManagementClusterPrintString(info.servicePassword.password);
  emberAfDeviceManagementClusterPrintln("\n   Implementation Date / Time: %4x", info.servicePassword.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("   Duration In Minutes: %2x", info.servicePassword.durationInMinutes);
  emberAfDeviceManagementClusterPrintln("   Password Type: %x\n", info.servicePassword.passwordType);

  emberAfDeviceManagementClusterPrintln("   = Consumer Password =");
  emberAfDeviceManagementClusterPrint("   Password: ");
  emberAfDeviceManagementClusterPrintString(info.consumerPassword.password);
  emberAfDeviceManagementClusterPrintln("\n   Implementation Date / Time: %4x", info.consumerPassword.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("   Duration In Minutes: %2x", info.consumerPassword.durationInMinutes);
  emberAfDeviceManagementClusterPrintln("   Password Type: %x\n", info.consumerPassword.passwordType);

  emberAfDeviceManagementClusterPrintln("  == Uncontrolled Flow Threshold ==");
  emberAfDeviceManagementClusterPrintln("  Uncontrolled Flow Threshold: %2x", info.threshold.uncontrolledFlowThreshold);
  emberAfDeviceManagementClusterPrintln("  Multiplier: %2x", info.threshold.multiplier);
  emberAfDeviceManagementClusterPrintln("  Divisor: %2x", info.threshold.divisor);
  emberAfDeviceManagementClusterPrintln("  Measurement Period: %2x", info.threshold.measurementPeriod);
  emberAfDeviceManagementClusterPrintln("  Unit of Measure: %x", info.threshold.unitOfMeasure);
  emberAfDeviceManagementClusterPrintln("  Stabilisation Period: %x\n", info.threshold.stabilisationPeriod);

  emberAfDeviceManagementClusterPrintln("  == Issuer Event ID ==");
  emberAfDeviceManagementClusterPrintln("  Issuer Event Id: %4x", info.issuerEventId);

  emberAfDeviceManagementClusterPrintln("== End of Device Management Information ==");
}

boolean emberAfDeviceManagementClusterGetChangeOfTenancyCallback(void)
{
  EmberAfDeviceManagementTenancy *tenancy = &(info.tenancy);
  if ((info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK) == 0){
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  } else {
    emberAfFillCommandDeviceManagementClusterPublishChangeOfTenancy(info.providerId,
                                                                    info.issuerEventId,
                                                                    info.tariffType,
                                                                    tenancy->implementationDateTime,
                                                                    tenancy->tenancy);
    emberAfSendResponse();
  }

  return TRUE;
}

boolean emberAfDeviceManagementClusterPublishChangeOfTenancy(EmberNodeId dstAddr,
                                                             int8u srcEndpoint,
                                                             int8u dstEndpoint)
{
  EmberStatus status;
  EmberAfDeviceManagementTenancy *tenancy = &(info.tenancy);
  emberAfFillCommandDeviceManagementClusterPublishChangeOfTenancy(info.providerId,
                                                                  info.issuerEventId,
                                                                  info.tariffType,
                                                                  tenancy->implementationDateTime,
                                                                  tenancy->tenancy);
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);

  if (status != EMBER_SUCCESS){
    emberAfDeviceManagementClusterPrintln("Unable to unicast PublishChangeOfTenancy command: 0x%x", status);
  }
  return TRUE;
}

boolean emberAfDeviceManagementClusterGetChangeOfSupplierCallback(void)
{
  EmberAfDeviceManagementSupplier *supplier = &(info.supplier);

  if ((info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_SUPPLIER_PENDING_MASK) == 0 ) {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  } else {
    emberAfFillCommandDeviceManagementClusterPublishChangeOfSupplier(info.providerId,
                                                                    info.issuerEventId,
                                                                    info.tariffType,
                                                                    supplier->proposedProviderId,
                                                                    supplier->implementationDateTime,
                                                                    supplier->providerChangeControl,
                                                                    supplier->proposedProviderName,
                                                                    supplier->proposedProviderContactDetails);
    emberAfSendResponse();
  }

  return TRUE;
}

boolean emberAfDeviceManagementClusterPublishChangeOfSupplier(EmberNodeId dstAddr,
                                                              int8u srcEndpoint,
                                                              int8u dstEndpoint)
{
  EmberStatus status;
  EmberAfDeviceManagementSupplier *supplier = &(info.supplier);
  emberAfFillCommandDeviceManagementClusterPublishChangeOfSupplier(info.providerId,
                                                                  info.issuerEventId,
                                                                  info.tariffType,
                                                                  supplier->proposedProviderId,
                                                                  supplier->implementationDateTime,
                                                                  supplier->providerChangeControl,
                                                                  supplier->proposedProviderName,
                                                                  supplier->proposedProviderContactDetails);
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);

  if (status != EMBER_SUCCESS){
    emberAfDeviceManagementClusterPrintln("Unable to unicast PublishChangeOfSupplier command: 0x%x", status);
  }
  return TRUE;
}

boolean emberAfDeviceManagementClusterGetSiteIdCallback(void) 
{
  EmberAfDeviceManagementSiteId *siteId = &(info.siteId);

  if (info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK) {
    emberAfFillCommandDeviceManagementClusterUpdateSiteId(info.issuerEventId,
                                                          siteId->implementationDateTime,
                                                          info.providerId,
                                                          siteId->siteId);
    emberAfSendResponse();
  } else {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  }

  return TRUE;
}

boolean emberAfDeviceManagementClusterGetCINCallback(void) 
{
  EmberAfDeviceManagementCIN *cin = &(info.cin);

  if (info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK) {
    emberAfFillCommandDeviceManagementClusterUpdateCIN(info.issuerEventId,
                                                       cin->implementationDateTime,
                                                       info.providerId,
                                                       cin->cin);
    emberAfSendResponse();
  } else {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  }

  return TRUE;
}

boolean emberAfDeviceManagementClusterRequestNewPasswordCallback(int8u passwordType)
{
  EmberAfDeviceManagementPassword *password;
  switch (passwordType) {
    case SERVICE_PASSWORD:
      password = &(info.servicePassword);
      break;
    case CONSUMER_PASSWORD:
      password = &(info.consumerPassword);
      break;
    default:
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
      return TRUE;
  }
  
  // Is the password still valid?
  if ((password->durationInMinutes != 0)
      && (emberAfGetCurrentTime() > password->implementationDateTime + (password->durationInMinutes * 60))) {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  } else {
    emberAfFillCommandDeviceManagementClusterRequestNewPasswordResponse(info.issuerEventId,
                                                                        password->implementationDateTime,
                                                                        password->durationInMinutes,
                                                                        password->passwordType,
                                                                        password->password);

    emberAfSendResponse();
  }

  return TRUE;
}

boolean emberAfDeviceManagementClusterSendRequestNewPasswordResponse(int8u passwordType,
                                                                     EmberNodeId dstAddr,
                                                                     int8u srcEndpoint,
                                                                     int8u dstEndpoint)
{
  EmberStatus status;
  EmberAfDeviceManagementPassword *password;

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);

  switch (passwordType) {
    case SERVICE_PASSWORD:
      password = &(info.servicePassword);
      break;
    case CONSUMER_PASSWORD:
      password = &(info.consumerPassword);
      break;
    default:
      return FALSE;
  }

  // Is the password still valid?
  if ((password->durationInMinutes != 0)
      && (emberAfGetCurrentTime() > password->implementationDateTime + (password->durationInMinutes * 60))) {
    return FALSE;
  } else {
    emberAfFillCommandDeviceManagementClusterRequestNewPasswordResponse(info.issuerEventId,
                                                                        password->implementationDateTime,
                                                                        password->durationInMinutes,
                                                                        password->passwordType,
                                                                        password->password);

    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);
    if (status != EMBER_SUCCESS){
      return FALSE;
    } else {
      return TRUE;
    }
  }

}

boolean emberAfDeviceManagementClusterUpdateSiteId(EmberNodeId dstAddr,
                                                   int8u srcEndpoint,
                                                   int8u dstEndpoint)
{
  EmberStatus status;
  EmberAfDeviceManagementSiteId *siteId = &(info.siteId);
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);

  emberAfFillCommandDeviceManagementClusterUpdateSiteId(info.issuerEventId,
                                                        siteId->implementationDateTime,
                                                        info.providerId,
                                                        siteId->siteId);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);

  if (status != EMBER_SUCCESS){
    emberAfDeviceManagementClusterPrintln("Unable to unicast UpdateSiteId command: 0x%x", status);
  }
  return TRUE;
}

boolean emberAfDeviceManagementClusterUpdateCIN(EmberNodeId dstAddr,
                                                int8u srcEndpoint,
                                                int8u dstEndpoint)
{
  EmberStatus status;
  EmberAfDeviceManagementCIN *cin = &(info.cin);
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  
  info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK;

  emberAfFillCommandDeviceManagementClusterUpdateCIN(info.issuerEventId,
                                                     cin->implementationDateTime,
                                                     info.providerId,
                                                     cin->cin);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);

  if (status != EMBER_SUCCESS){
    emberAfDeviceManagementClusterPrintln("Unable to unicast UpdateCIN command: 0x%x", status);
  }
  return TRUE;
}

void emberAfDeviceManagementClusterSetPendingUpdates(EmberAfDeviceManagementChangePendingFlags pendingUpdatesMask)
{
  info.pendingUpdates = pendingUpdatesMask;
}

void emberAfDeviceManagementClusterGetPendingUpdates(EmberAfDeviceManagementChangePendingFlags *pendingUpdatesMask)
{
  *(pendingUpdatesMask) = info.pendingUpdates;
}


boolean emberAfDeviceManagementClusterReportEventConfigurationCallback(int8u commandIndex,
                                                                       int8u totalCommands,
                                                                       int8u* eventConfigurationPayload)
{
  return TRUE;
}

