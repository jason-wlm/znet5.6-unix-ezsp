// *******************************************************************                                                                                        
// * device-management-client.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*                                                                              
// ******************************************************************* 

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/device-management-server/device-management-common.h"

static EmberAfDeviceManagementInfo info;
static EmberAfDeviceManagementPassword servicePassword;
static EmberAfDeviceManagementPassword consumerPassword;

static int8u numberOfAttributeSets = 9;
static EmberAfDeviceManagementAttributeTable attributeTable[9] = {
  {
    0x00,
    {
      {0x00,0x01}, //min inclusive, max not included
      {0x10,0x11},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    },
  },
  {
    0x01,
    {
      {0x00,0x0E},
      {0xB0,0xE8},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    },
  },
  {
    0x02,
    {
      {0x00,0x2A},
      {0x30,0x38},
      {0x50,0x52},
      {0x60,0x66},
      {0x70,0xA0},
      {0xB0,0xB9},
      {0xC0,0xFA},
    }
  },
  {
    0x03,
    {
      {0x00,0x01},
      {0xC0,0xC8},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    }
  },
  {
    0x04,
    {
      {0x00,0x06},
      {0x20,0x34},
      {0x41,0x44},
      {0xC0,0xF0},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    }
  },
  {
    0x05,
    {
      {0x00,0x03},
      {0xC0,0xD4},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    }
  },
  {
    0x06,
    {
      {0x00,0x05},
      {0x10,0x16},
      {0xC0,0xD8},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    }
  },
  {
    0x07,
    {
      {0x00,0x03},
      {0xC0,0xD5},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    }
  },
  {
    0x08,
    {
      {0x00,0x06},
      {0xC0,0xCF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
      {0xFF,0xFF},
    }
  }
};

static void emberAfDeviceManagementClusterPrintPendingStatus(EmberAfDeviceManagementChangePendingFlags mask)
{
  if (info.pendingUpdates & mask){
    emberAfDeviceManagementClusterPrintln("  Status: pending");
  } else {
    emberAfDeviceManagementClusterPrintln("  Status: not pending");
  }
}

static void sendDeviceManagementClusterReportWildCardAttribute(int8u attributeSet,
                                                               int8u endpoint)
{
  int8u attrSet,j,k;
  int8u eventConfiguration;
  EmberAfStatus status;
  boolean atLeastOneEvent = FALSE;
  int8u numberOfAttrSetsToReport = 1; // allow one iteratoion 
 
  if(attributeSet == 0xFF) {
    numberOfAttrSetsToReport = numberOfAttributeSets;
  }

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                             | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER),
                            ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                            ZCL_REPORT_EVENT_CONFIGURATION_COMMAND_ID,
                            "uu",
                            0, 
                            1);
// The maximum number of intervals possible in attributeRange.
  for(attrSet = 0; attrSet < numberOfAttrSetsToReport; attrSet++) {
    for(j = 0; j < 7; j++) {
      //min and max for each interval.
      int8u actualSet = (attributeSet == 0xFF) ? attrSet : attributeSet;
      for(k = attributeTable[actualSet].attributeRange[j].startIndex; k < attributeTable[actualSet].attributeRange[j].endIndex; k++) {
        int8u topByte = (attributeSet == 0xFF) ? attributeTable[attrSet].attributeSetId : attributeSet;
        int16u attributeId = ((topByte << 8) | k);
        if(emberAfLocateAttributeMetadata(endpoint,
                                          ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                          attributeId,
                                          CLUSTER_MASK_CLIENT,
                                          EMBER_AF_NULL_MANUFACTURER_CODE)) {
          status = emberAfReadClientAttribute(endpoint,
                                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                              attributeId,
                                              (int8u *)&eventConfiguration,
                                              sizeof(eventConfiguration));

          if(status == EMBER_SUCCESS) {
            //emberAfDeviceManagementClusterPrintln("Attributed Id 0x%2x",attributeId);
            atLeastOneEvent = TRUE;
            emberAfPutInt16uInResp(attributeId);
            emberAfPutInt8uInResp(eventConfiguration);
          } 
        }
      }
    }
  }

  if(atLeastOneEvent)
    emberAfSendResponse();
  else {
    status = EMBER_ZCL_STATUS_NOT_FOUND;
    emberAfDeviceManagementClusterPrintln("sending default response");
    emberAfSendImmediateDefaultResponse(status);
  }
}

static void writeDeviceManagementClusterWildCardAttribute(int8u attributeSet,
                                                          int8u endpoint,
                                                          int8u attributeConfiguration)
{
  int8u attrRange,attrId;
  int8u eventConfiguration;
  EmberAfStatus status;

  for(attrRange = 0; attrRange < 7; attrRange++) {
    for(attrId = attributeTable[attributeSet].attributeRange[attrRange].startIndex; attrId < attributeTable[attributeSet].attributeRange[attrRange].endIndex; attrId++) {
      int16u attributeId = ((attributeSet << 8) | attrId);
      if(emberAfLocateAttributeMetadata(endpoint,
                                        ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                        attributeId,
                                        CLUSTER_MASK_CLIENT,
                                        EMBER_AF_NULL_MANUFACTURER_CODE)) {
        status  = emberAfWriteAttribute(endpoint,
                                        ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                        attributeId,
                                        CLUSTER_MASK_CLIENT,
                                        (int8u*)&attributeConfiguration,
                                        ZCL_BITMAP8_ATTRIBUTE_TYPE);
      }
    }
  }
}

static void writeDeviceManagementClusterByLogTypeAttribute(int8u logType,
                                                           int8u endpoint,
                                                           int8u attributeConfiguration)
{
  int8u i,j,k;
  int8u eventConfiguration;
  EmberAfStatus status;
  boolean atLeastOneEvent = FALSE;

  for(i = 0; i < numberOfAttributeSets; i++) {
    for(j = 0; j < 7; j++) {
      for(k = attributeTable[i].attributeRange[j].startIndex; k < attributeTable[i].attributeRange[j].endIndex; k++) {
        int8u topByte = attributeTable[i].attributeSetId;
        int16u attributeId = ((topByte << 8) | k);
        if(emberAfLocateAttributeMetadata(endpoint,
                                          ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                          attributeId,
                                          CLUSTER_MASK_CLIENT,
                                          EMBER_AF_NULL_MANUFACTURER_CODE)) {
          status = emberAfReadClientAttribute(endpoint,
                                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                              attributeId,
                                              (int8u *)&eventConfiguration,
                                              sizeof(eventConfiguration));
          if(status == EMBER_SUCCESS) {
            if((eventConfiguration & 0x03) == logType){
              EmberAfStatus status  = emberAfWriteAttribute(endpoint,
                                                          ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                                          attributeId,
                                                          CLUSTER_MASK_CLIENT,
                                                          (int8u*)&attributeConfiguration,
                                                          ZCL_BITMAP8_ATTRIBUTE_TYPE);
              if(status == EMBER_ZCL_STATUS_SUCCESS) {
                emberAfDeviceManagementClusterPrintln("Writing %x to 0x%2x",attributeConfiguration,attributeId);
              }
            }
          }  
        }
      }
    }
  }
}

static void writeDeviceManagementClusterByMatchingAttribute(int8u currentConfiguration,
                                                            int8u endpoint,
                                                            int8u attributeConfiguration)
{
  int8u i,j,k;
  int8u eventConfiguration;
  EmberAfStatus status;
  boolean atLeastOneEvent = FALSE;

  for(i = 0; i < numberOfAttributeSets; i++) {
    for(j = 0; j < 7; j++) {
      for(k = attributeTable[i].attributeRange[j].startIndex; k < attributeTable[i].attributeRange[j].endIndex; k++) {
        int8u topByte = attributeTable[i].attributeSetId;
        int16u attributeId = ((topByte << 8) | k);
        if(emberAfLocateAttributeMetadata(endpoint,
                                          ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                          attributeId,
                                          CLUSTER_MASK_CLIENT,
                                          EMBER_AF_NULL_MANUFACTURER_CODE)) {
          status = emberAfReadClientAttribute(endpoint,
                                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                              attributeId,
                                              (int8u *)&eventConfiguration,
                                              sizeof(eventConfiguration));
          if(status == EMBER_SUCCESS) {
            if(eventConfiguration == currentConfiguration){
              EmberAfStatus status  = emberAfWriteAttribute(endpoint,
                                                          ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                                          attributeId,
                                                          CLUSTER_MASK_CLIENT,
                                                          (int8u*)&attributeConfiguration,
                                                          ZCL_BITMAP8_ATTRIBUTE_TYPE);
              if(status == EMBER_ZCL_STATUS_SUCCESS) {
                emberAfDeviceManagementClusterPrintln("Writing %x to 0x%2x",attributeConfiguration,attributeId);
              }
            }
          }  
        }
      }
    }
  }
}

static void writeDeviceManagementAttribute(int16u attributeId,
                                           int8u attributeConfiguration)
{
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfStatus status  = emberAfWriteAttribute(endpoint,
                                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                              attributeId,
                                              CLUSTER_MASK_CLIENT,
                                              (int8u*)&attributeConfiguration,
                                              ZCL_BITMAP8_ATTRIBUTE_TYPE);
  if(status == EMBER_ZCL_STATUS_SUCCESS) {
    emberAfDeviceManagementClusterPrintln("Wrote %u to %u",attributeConfiguration,attributeId);
  } else{
    emberAfDeviceManagementClusterPrintln("couldn't write %u to %u",attributeConfiguration,attributeId);    
  }
}

void emberAfDeviceManagementClusterClientInitCallback(int8u endpoint)
{
  info.providerId = 0;
  info.issuerEventId = 0;
  info.tariffType = (EmberAfTariffType) 0xFF;
  info.pendingUpdates = 0;
  attributeTable[0].attributeSetId = 0x01;
  // MEMSET(&servicePassword, 0x00, sizeof(servicePassword));
  // MEMSET(&consumerPassword, 0x00, sizeof(consumerPassword));
}

void emberAfDeviceManagementClusterClientTickCallback(int8u endpoint)
{
  int32u currentTime = emberAfGetCurrentTime();
  int32u nextTick = currentTime;

  // Action pending changes
  if (info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK) {
    if (currentTime >= info.tenancy.implementationDateTime) {
      emberAfDeviceManagementClusterPrintln("DEVICE MANAGEMENT CLIENT: Enacting change of tenancy at time 0x%4x\n",
                                            currentTime);
      emberAfPluginDeviceManagementClientEnactChangeOfTenancyCallback(endpoint,
                                                                      &(info.tenancy));
      info.pendingUpdates &= ~(EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK);
    } else if (info.tenancy.implementationDateTime - currentTime < nextTick) {
      nextTick = info.tenancy.implementationDateTime - currentTime;
    }
  }

  if (info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_SUPPLIER_PENDING_MASK) {
    if (currentTime >= info.supplier.implementationDateTime) {
      emberAfDeviceManagementClusterPrintln("DEVICE MANAGEMENT CLIENT: Enacting change of supplier at time 0x%4x\n",
                                            currentTime);
      emberAfPluginDeviceManagementClientEnactChangeOfSupplierCallback(endpoint,
                                                                       &(info.supplier));
      info.pendingUpdates &= ~(EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_SUPPLIER_PENDING_MASK);
    } else if (info.supplier.implementationDateTime - currentTime < nextTick) {
      nextTick = info.supplier.implementationDateTime - currentTime;
    }
  }

  if (info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK) {
    if (currentTime >= info.siteId.implementationDateTime) {
      EmberAfStatus status = emberAfWriteAttribute(endpoint,
                                                   ZCL_SIMPLE_METERING_CLUSTER_ID,
                                                   ZCL_SITE_ID_ATTRIBUTE_ID,
                                                   CLUSTER_MASK_SERVER,
                                                   (int8u*)&(info.siteId.siteId),
                                                   ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
      if (status == EMBER_ZCL_STATUS_SUCCESS){
        info.pendingUpdates &= ~(EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK);
      }
      emberAfDeviceManagementClusterPrintln("DEVICE MANAGEMENT CLIENT: Enacting site id update at time 0x%4x: %d\n",
                                            currentTime,
                                            status);
    } else if (info.siteId.implementationDateTime - currentTime < nextTick) {
      nextTick = info.siteId.implementationDateTime - currentTime;
    }
  }

  if (info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK) {
    if (currentTime >= info.cin.implementationDateTime) {
      EmberAfStatus status = emberAfWriteAttribute(endpoint,
                                                   ZCL_SIMPLE_METERING_CLUSTER_ID,
                                                   ZCL_CUSTOMER_ID_NUMBER_ATTRIBUTE_ID,
                                                   CLUSTER_MASK_SERVER,
                                                   (int8u*)&(info.cin.cin),
                                                   ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
      if (status == EMBER_ZCL_STATUS_SUCCESS){
        info.pendingUpdates &= ~(EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK);
      }
      emberAfDeviceManagementClusterPrintln("DEVICE MANAGEMENT CLIENT: Enacting customer id number update at time 0x%4x: %d\n",
                                            currentTime,
                                            status);
    } else if (info.cin.implementationDateTime - currentTime < nextTick) {
      nextTick = info.cin.implementationDateTime - currentTime;
    }
  }

  if(info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SERVICE_PASSWORD_PENDING_MASK) {
     emberAfDeviceManagementClusterPrintln("DEVICE MANAGEMENT CLIENT: Service password current time 0x%4x: implementationTime 0x%4x\n",
                                            currentTime,
                                            servicePassword.implementationDateTime);
    if(currentTime >= servicePassword.implementationDateTime) {
      info.servicePassword.implementationDateTime = servicePassword.implementationDateTime;
      info.servicePassword.durationInMinutes = servicePassword.durationInMinutes;
      info.servicePassword.passwordType = servicePassword.passwordType;
      MEMCOPY(&info.servicePassword.password,&servicePassword.password,emberAfStringLength(servicePassword.password) + 1);
      info.pendingUpdates &= (~EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SERVICE_PASSWORD_PENDING_MASK);
    } else if (servicePassword.implementationDateTime - currentTime < nextTick) {
      nextTick = servicePassword.implementationDateTime - currentTime;
    }
  }


  if(info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CONSUMER_PASSWORD_PENDING_MASK) {
    if(currentTime >= consumerPassword.implementationDateTime) {
      info.consumerPassword.implementationDateTime = consumerPassword.implementationDateTime;
      info.consumerPassword.durationInMinutes = consumerPassword.durationInMinutes;
      info.consumerPassword.passwordType = consumerPassword.passwordType;
      MEMCOPY(&info.consumerPassword.password,&consumerPassword.password,emberAfStringLength(consumerPassword.password) + 1);
      info.pendingUpdates &= (~EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CONSUMER_PASSWORD_PENDING_MASK);
    } else if (consumerPassword.implementationDateTime - currentTime < nextTick) {
      nextTick = consumerPassword.implementationDateTime - currentTime;
    }
  }

  

  // Reschedule the next tick, if necessary
  if (info.pendingUpdates != 0) {
    emberAfScheduleClientTick(endpoint,
                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                              nextTick * MILLISECOND_TICKS_PER_SECOND);
  }
}

boolean emberAfDeviceManagementClusterPublishChangeOfTenancyCallback(int32u supplierId,
                                                                     int32u issuerEventId,
                                                                     int8u tariffType,
                                                                     int32u implementationDateTime,
                                                                     int32u proposedTenancyChangeControl)
{
  int32u currentTime = emberAfGetCurrentTime();
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfStatus status;

  emberAfDeviceManagementClusterPrintln("RX: PublishChangeOfTenancy: 0x%4X, 0x%4X, 0x%X, 0x%4X, 0x%4X",
                                        supplierId,
                                        issuerEventId,
                                        tariffType,
                                        implementationDateTime,
                                        proposedTenancyChangeControl);

  if (info.providerId == 0) {
    info.providerId = supplierId;
  }

  if (issuerEventId > info.issuerEventId) {
    info.issuerEventId = issuerEventId;
  }

  info.tariffType = (EmberAfTariffType)tariffType;

  info.tenancy.implementationDateTime = implementationDateTime;
  info.tenancy.tenancy = proposedTenancyChangeControl;

  // Even if we aren't to immediately action the change of tenancy, 
  // we still set these attributes accordingly
  status = emberAfWriteAttribute(endpoint,
                                 ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                 ZCL_CHANGE_OF_TENANCY_UPDATE_DATE_TIME_ATTRIBUTE_ID,
                                 CLUSTER_MASK_CLIENT,
                                 (int8u*)&implementationDateTime,
                                 ZCL_UTC_TIME_ATTRIBUTE_TYPE);

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto kickout;
  }

  status = emberAfWriteAttribute(endpoint,
                                 ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                 ZCL_PROPOSED_TENANCY_CHANGE_CONTROL_ATTRIBUTE_ID,
                                 CLUSTER_MASK_CLIENT,
                                 (int8u*)&proposedTenancyChangeControl,
                                 ZCL_INT32U_ATTRIBUTE_TYPE);

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto kickout;
  }

  // If the time has passed since the change of tenancy was to be implemented, take action
  if (currentTime >= implementationDateTime) {
    emberAfPluginDeviceManagementClientEnactChangeOfTenancyCallback(endpoint,
                                                                    &(info.tenancy));
    info.pendingUpdates &= ~EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK;
  } else {
    // Otherwise, wait until the time of implementation
    emberAfScheduleClientTick(endpoint,
                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                              (implementationDateTime - currentTime) 
                              * MILLISECOND_TICKS_PER_SECOND);
    info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK;
  }

kickout:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}

boolean emberAfDeviceManagementClusterPublishChangeOfSupplierCallback(int32u currentProviderId,
                                                                      int32u issuerEventId,
                                                                      int8u  tariffType,
                                                                      int32u proposedProviderId,
                                                                      int32u providerChangeImplementationTime,
                                                                      int32u providerChangeControl,
                                                                      int8u* proposedProviderName,
                                                                      int8u* proposedProviderContactDetails)
{
  EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
  int32u currentTime = emberAfGetCurrentTime();
  int8u endpoint = emberAfCurrentEndpoint();

  emberAfDeviceManagementClusterPrintln("RX: PublishChangeOfSupplier: 0x%4X, 0x%4X, 0x%X, 0x%4X, 0x%4X, 0x%4X, ",
                                        currentProviderId,
                                        issuerEventId,
                                        tariffType,
                                        proposedProviderId,
                                        providerChangeImplementationTime,
                                        providerChangeControl);
  emberAfDeviceManagementClusterPrintString(proposedProviderName);
  emberAfDeviceManagementClusterPrintln(", ");
  emberAfDeviceManagementClusterPrintString(proposedProviderContactDetails);
  emberAfDeviceManagementClusterPrintln("");

  if (proposedProviderName == NULL) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto kickout;
  }

  if (info.providerId == 0) {
    info.providerId = currentProviderId;
  }

  if (info.issuerEventId == 0) {
    info.issuerEventId = issuerEventId;
  }

  info.tariffType = (EmberAfTariffType) tariffType;

  info.supplier.proposedProviderId = proposedProviderId;
  info.supplier.implementationDateTime = providerChangeImplementationTime; 
  info.supplier.providerChangeControl = providerChangeControl;
  emberAfCopyString(info.supplier.proposedProviderName,
                    proposedProviderName,
                    EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_PROPOSED_PROVIDER_NAME_LENGTH);
  emberAfCopyString(info.supplier.proposedProviderContactDetails,
                    proposedProviderContactDetails,
                    EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_PROPOSED_PROVIDER_CONTACT_DETAILS_LENGTH);
   
  // If the time has passed since the change of supplier was to be implemented, take action
  if (currentTime >= providerChangeImplementationTime) {
    emberAfPluginDeviceManagementClientEnactChangeOfSupplierCallback(endpoint,
                                                                     &(info.supplier));
    info.pendingUpdates &= ~EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_SUPPLIER_PENDING_MASK;
  } else {
    // Otherwise, wait until the time of implementation
    emberAfScheduleClientTick(endpoint,
                              ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                              (providerChangeImplementationTime - currentTime) 
                              * MILLISECOND_TICKS_PER_SECOND);
    info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_SUPPLIER_PENDING_MASK;
  }

kickout:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}

boolean emberAfDeviceManagementClusterRequestNewPasswordResponseCallback(int32u issuerEventId,
                                                                         int32u implementationDateTime,
                                                                         int16u durationInMinutes,
                                                                         int8u passwordType,
                                                                         int8u* password)
{
  int32u currentTime = emberAfGetCurrentTime();
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfDeviceManagementPassword *pass;
  EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

  if (password == NULL) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto kickout;
  }

  if (issuerEventId > info.issuerEventId){
    info.issuerEventId = issuerEventId;
  }

  if(implementationDateTime < currentTime && implementationDateTime != 0x00000000) {
    goto kickout;
  }

  if (passwordType & SERVICE_PASSWORD) {
    if(currentTime == implementationDateTime || implementationDateTime == 0x00000000) {
      pass = &(info.servicePassword);
    } else {
      pass = &(servicePassword);
      info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SERVICE_PASSWORD_PENDING_MASK;
      emberAfScheduleClientTick(endpoint,
                                ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                (implementationDateTime - currentTime) 
                                * MILLISECOND_TICKS_PER_SECOND);
    }
  } else if (passwordType & CONSUMER_PASSWORD) {
    if(currentTime == implementationDateTime || implementationDateTime == (0x00000000)) {
      pass = &(info.consumerPassword);
    } else {
      pass = &(consumerPassword);
      info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CONSUMER_PASSWORD_PENDING_MASK;
      emberAfScheduleClientTick(endpoint,
                                ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                (implementationDateTime - currentTime) 
                                * MILLISECOND_TICKS_PER_SECOND);
    }
  } else {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto kickout;
  }

  pass->implementationDateTime = implementationDateTime;
  pass->durationInMinutes = durationInMinutes;
  pass->passwordType = passwordType;
  emberAfCopyString(pass->password,
                    password,
                    EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_PASSWORD_LENGTH);

kickout:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}

boolean emberAfDeviceManagementClusterUpdateSiteIdCallback(int32u issuerEventId,
                                                           int32u implementationDateTime,
                                                           int32u providerId,
                                                           int8u* siteId)
{
  int32u currentTime = emberAfGetCurrentTime();
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfStatus status = EMBER_ZCL_STATUS_FAILURE;

  if (info.providerId != providerId){
    status = EMBER_ZCL_STATUS_NOT_AUTHORIZED;
    goto kickout;
  }

  emberAfCopyString(info.siteId.siteId,
                    siteId,
                    EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_SITE_ID_LENGTH);

  // If implementationDateTime is 0xFFFFFFFF, cancel any pending change and replace it with
  // the incoming site ID
  // cancelling a scheduled update
  if (implementationDateTime == 0xFFFFFFFF) {
    if ((info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK)
        && (info.providerId == providerId)
        && (info.siteId.issuerEventId == issuerEventId)){
      info.pendingUpdates &= ~EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK;
      MEMSET(&(info.siteId), 0, sizeof(EmberAfDeviceManagementSiteId));
      status = EMBER_ZCL_STATUS_SUCCESS;
    } else {
      emberAfDeviceManagementClusterPrintln("Unable to cancel scheduled siteId update.");
      emberAfDeviceManagementClusterPrintln("Provider ID: 0x%4x", info.providerId);
      emberAfDeviceManagementClusterPrintln("Issuer Event ID: 0x%4x", info.issuerEventId);
      emberAfDeviceManagementClusterPrintPendingStatus(EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK);
      status = EMBER_ZCL_STATUS_FAILURE;
      goto kickout;
    }
  } else { // schedule siteid update or immediate update
    if (issuerEventId > info.issuerEventId){
      info.issuerEventId = issuerEventId;
    } else {
      status = EMBER_ZCL_STATUS_FAILURE;
      goto kickout;
    }

    // implementation time:
    //   0x00000000 - execute cmd immediately
    //   0xFFFFFFFF - cancel update (case covered by previous code blok)
    //   otherwise  - schedule for new pending action
    if (implementationDateTime == 0x00000000){
      
      // Are we meter or ihd? 
      // Meter - update attr and reply with error if not sucessful
      // Ihd - no update attr needed. reply success.
      status = emberAfWriteAttribute(endpoint,
                                     ZCL_SIMPLE_METERING_CLUSTER_ID,
                                     ZCL_SITE_ID_ATTRIBUTE_ID,
                                     CLUSTER_MASK_SERVER,
                                     (int8u*)siteId,
                                     ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
      if (emberAfContainsServer(emberAfCurrentEndpoint(), ZCL_SIMPLE_METERING_CLUSTER_ID)
          && (status != EMBER_ZCL_STATUS_SUCCESS)){
        emberAfDeviceManagementClusterPrintln("Unable to write siteId attr in Metering cluster: 0x%d", status);
        status = EMBER_ZCL_STATUS_FAILURE;
      } else {
        status = EMBER_ZCL_STATUS_SUCCESS;
      }
      info.pendingUpdates &= ~EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK;
      MEMSET(&(info.siteId), 0, sizeof(EmberAfDeviceManagementSiteId));
    } else {
      emberAfScheduleClientTick(endpoint,
                                ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                (implementationDateTime - currentTime)
                                * MILLISECOND_TICKS_PER_SECOND);
      info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK;
      emberAfCopyString(info.siteId.siteId,
                        siteId,
                        EMBER_AF_DEVICE_MANAGEMENT_MAXIMUM_SITE_ID_LENGTH);
      info.siteId.implementationDateTime = implementationDateTime;
      info.siteId.issuerEventId = issuerEventId;
      status = EMBER_ZCL_STATUS_SUCCESS;
    }
  }

kickout:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;

}

boolean emberAfDeviceManagementClusterSetSupplyStatusCallback(int32u issuerEventId,
                                                              int8u supplyTamperState,
                                                              int8u supplyDepletionState,
                                                              int8u supplyUncontrolledFlowState,
                                                              int8u loadLimitSupplyState)
{
  EmberAfDeviceManagementSupplyStatusFlags *flags = &(info.supplyStatusFlags);
  int8u endpoint = emberAfCurrentEndpoint();

  if (info.issuerEventId == 0) {
    info.issuerEventId = issuerEventId;
  }

  flags->supplyTamperState = supplyTamperState;
  flags->supplyDepletionState = supplyDepletionState;
  flags->supplyUncontrolledFlowState = supplyUncontrolledFlowState;
  flags->loadLimitSupplyState = loadLimitSupplyState;

  emberAfPluginDeviceManagementClientSetSupplyStatusCallback(endpoint,
                                                             flags);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfDeviceManagementClusterPublishUncontrolledFlowThresholdCallback(int32u providerId,
                                                                               int32u issuerEventId,
                                                                               int16u uncontrolledFlowThreshold,
                                                                               int8u unitOfMeasure,
                                                                               int16u multiplier,
                                                                               int16u divisor,
                                                                               int8u stabilisationPeriod,
                                                                               int16u measurementPeriod)
{
  EmberAfDeviceManagementUncontrolledFlowThreshold *threshold = &(info.threshold);
  int8u endpoint = emberAfCurrentEndpoint();

  if (info.providerId == 0) {
    info.providerId = providerId;
  }

  if (info.issuerEventId == 0) {
    info.issuerEventId = issuerEventId;
  }

  threshold->uncontrolledFlowThreshold = uncontrolledFlowThreshold;
  threshold->unitOfMeasure = unitOfMeasure;
  threshold->multiplier = multiplier;
  threshold->divisor = divisor;
  threshold->stabilisationPeriod = stabilisationPeriod;
  threshold->measurementPeriod = measurementPeriod;

  emberAfPluginDeviceManagementClientEnactUpdateUncontrolledFlowThresholdCallback(endpoint,
                                                                                  threshold);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfDeviceManagementClusterUpdateCINCallback(int32u issuerEventId,
                                                        int32u implementationDateTime,
                                                        int32u providerId,
                                                        int8u* customerIdNumber)
{
  int32u currentTime = emberAfGetCurrentTime();
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfStatus status = EMBER_ZCL_STATUS_FAILURE;

  if (info.providerId != providerId){
    status = EMBER_ZCL_STATUS_NOT_AUTHORIZED;
    goto kickout;
  }

  // cancelling a scheduled update
  if (implementationDateTime == 0xFFFFFFFF) {
    if ((info.pendingUpdates & EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK)
        && (info.providerId == providerId)
        && (info.cin.issuerEventId == issuerEventId)){
          info.pendingUpdates &= ~EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK;
          MEMSET(&(info.cin), 0, sizeof(EmberAfDeviceManagementCIN));
          status = EMBER_ZCL_STATUS_SUCCESS;
    } else {
      emberAfDeviceManagementClusterPrintln("Unable to cancel scheduled CIN update.");
      emberAfDeviceManagementClusterPrintln("Provider ID: 0x%4x", info.providerId);
      emberAfDeviceManagementClusterPrintln("Issuer Event ID: 0x%4x", info.issuerEventId);
      emberAfDeviceManagementClusterPrintPendingStatus(EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK);
      status = EMBER_ZCL_STATUS_FAILURE;
      goto kickout;
    }
  } else { // schedule CIN update or immediate update
    if (issuerEventId > info.issuerEventId){
      info.issuerEventId = issuerEventId;
    } else {
      status = EMBER_ZCL_STATUS_FAILURE;
      goto kickout;
    }

    // implementation time:
    //   0x00000000 - execute cmd immediately
    //   0xFFFFFFFF - cancel update (case covered by previous code blok)
    //   otherwise  - schedule for new pending action
    if (implementationDateTime == 0x00000000){
#ifdef EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER
      // only try to update attr if we actually implement cluster / ESI
      status = emberAfWriteAttribute(endpoint,
                                     ZCL_SIMPLE_METERING_CLUSTER_ID,
                                     ZCL_CUSTOMER_ID_NUMBER_ATTRIBUTE_ID,
                                     CLUSTER_MASK_SERVER,
                                     (int8u*)customerIdNumber,
                                     ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
      if (status != EMBER_ZCL_STATUS_SUCCESS){
        emberAfDeviceManagementClusterPrintln("Unable to write CIN attr in Metering cluster: 0x%d", status);
        status = EMBER_ZCL_STATUS_FAILURE;
      } else {
#endif
        status = EMBER_ZCL_STATUS_SUCCESS;
#ifdef EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER
      }
#endif
      info.pendingUpdates &= ~EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK;
    } else {
      emberAfScheduleClientTick(endpoint,
                                ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                (implementationDateTime - currentTime)
                                * MILLISECOND_TICKS_PER_SECOND);
      info.pendingUpdates |= EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK;
      status = EMBER_ZCL_STATUS_SUCCESS;
    }
    
    MEMCOPY(info.cin.cin, customerIdNumber, emberAfStringLength(customerIdNumber) + 1);
    info.cin.implementationDateTime = implementationDateTime;
    info.cin.issuerEventId = issuerEventId;
  }

kickout:
  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}


void emberAfDeviceManagementClientPrint(void)

{
  emberAfDeviceManagementClusterPrintln("== Device Management Information ==\n");

  emberAfDeviceManagementClusterPrintln("  == Tenancy ==");
  emberAfDeviceManagementClusterPrintln("  Implementation Date / Time: 0x%4x", info.tenancy.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("  Tenancy: 0x%4x", info.tenancy.tenancy);
  emberAfDeviceManagementClusterPrintPendingStatus(EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK);
  emberAfDeviceManagementClusterPrintln("");


  emberAfDeviceManagementClusterPrintln("  == Supplier ==");
  emberAfDeviceManagementClusterPrint("  Provider name: ");
  emberAfDeviceManagementClusterPrintString(info.supplier.proposedProviderName);
  emberAfDeviceManagementClusterPrintln("\n  Proposed Provider Id: 0x%4x", info.supplier.proposedProviderId);
  emberAfDeviceManagementClusterPrintln("  Implementation Date / Time: 0x%4x", info.supplier.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("  Provider Change Control: 0x%4x", info.supplier.providerChangeControl);
  emberAfDeviceManagementClusterPrintPendingStatus(EMBER_AF_DEVICE_MANAGEMENT_CHANGE_OF_TENANCY_PENDING_MASK);
  emberAfDeviceManagementClusterPrintln("");

  emberAfDeviceManagementClusterPrintln("  == Site ID ==");
  emberAfDeviceManagementClusterPrint("  Site ID: ");
  emberAfDeviceManagementClusterPrintString(info.siteId.siteId);
  emberAfDeviceManagementClusterPrintln("\n  Site Id Implementation Date / Time: 0x%4x", info.siteId.implementationDateTime);
  emberAfDeviceManagementClusterPrintPendingStatus(EMBER_AF_DEVICE_MANAGEMENT_UPDATE_SITE_ID_PENDING_MASK);
  emberAfDeviceManagementClusterPrintln("");

  emberAfDeviceManagementClusterPrintln("  == Customer ID Number ==");
  emberAfDeviceManagementClusterPrint("  Customer ID Number: ");
  emberAfDeviceManagementClusterPrintString(info.cin.cin);
  emberAfDeviceManagementClusterPrintln("\n  Customer ID Number Implementation Date / Time: 0x%4x\n", info.cin.implementationDateTime);
  emberAfDeviceManagementClusterPrintPendingStatus(EMBER_AF_DEVICE_MANAGEMENT_UPDATE_CIN_PENDING_MASK);

  emberAfDeviceManagementClusterPrintln("  == Passwords ==");
  emberAfDeviceManagementClusterPrintln("   = Service Password =");
  emberAfDeviceManagementClusterPrint("   Password: ");
  emberAfDeviceManagementClusterPrintString(info.servicePassword.password);
  emberAfDeviceManagementClusterPrintln("\n   Implementation Date / Time: 0x%4x", info.servicePassword.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("   Duration In Minutes: 0x%2x", info.servicePassword.durationInMinutes);
  emberAfDeviceManagementClusterPrintln("   Password Type: 0x%x\n", info.servicePassword.passwordType);

  emberAfDeviceManagementClusterPrintln("   = Consumer Password =");
  emberAfDeviceManagementClusterPrint("   Password: ");
  emberAfDeviceManagementClusterPrintString(info.consumerPassword.password);
  emberAfDeviceManagementClusterPrintln("\n   Implementation Date / Time: 0x%4x", info.consumerPassword.implementationDateTime);
  emberAfDeviceManagementClusterPrintln("   Duration In Minutes: 0x%2x", info.consumerPassword.durationInMinutes);
  emberAfDeviceManagementClusterPrintln("   Password Type: 0x%x\n", info.consumerPassword.passwordType);

  emberAfDeviceManagementClusterPrintln("  == Issuer Event ID ==");
  emberAfDeviceManagementClusterPrintln("  Issuer Event ID: 0x%4x\n", info.issuerEventId);

  emberAfDeviceManagementClusterPrintln("  == Provider ID ==");
  emberAfDeviceManagementClusterPrintln("  Provider Id: 0x%4x", info.providerId);

  emberAfDeviceManagementClusterPrintln("== End of Device Management Information ==");
}


boolean emberAfDeviceManagementClusterGetEventConfigurationCallback(int16u eventId)
{
  int8u endpoint = emberAfCurrentEndpoint();
  int8u eventConfiguration;
  EmberAfStatus status;
  if((eventId & 0xFF) == 0xFF) {
    //emberAfDeviceManagementClusterPrintln("Wildcard profile Id requested %u",eventId & 0xFF00);
    int8u attributeSet = (eventId & 0xFF00) >> 8;
    //emberAfDeviceManagementClusterPrintln("attribute set %u",attributeSet);
    if(attributeSet < 1 && attributeSet > 8 && attributeSet != 0xFF) {
      status = EMBER_ZCL_STATUS_NOT_FOUND;
      emberAfSendImmediateDefaultResponse(status);
      return TRUE;
    } else {
      sendDeviceManagementClusterReportWildCardAttribute(attributeSet,
                                                         endpoint);
      return TRUE;
    } 
  } else {
    emberAfDeviceManagementClusterPrintln("Get Event callback %u",eventId);
    status = emberAfReadClientAttribute(endpoint,
                                        ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                        eventId,
                                        (int8u *)&eventConfiguration,
                                        sizeof(eventConfiguration));
    emberAfDeviceManagementClusterPrintln("Get Event status %u eventConfiguration %u",status,eventConfiguration);
   
    if(status == EMBER_SUCCESS){ 
      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                                 | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER),
                                ZCL_DEVICE_MANAGEMENT_CLUSTER_ID,
                                ZCL_REPORT_EVENT_CONFIGURATION_COMMAND_ID,
                                "uu",
                                0, 
                                1);
      emberAfPutInt16uInResp(eventId);
      emberAfPutInt8uInResp(eventConfiguration);
      emberAfSendResponse();
      return TRUE;
    } else {
      status = EMBER_ZCL_STATUS_NOT_FOUND;
      emberAfSendImmediateDefaultResponse(status);
      return TRUE; 
    }
  }

}

boolean emberAfDeviceManagementClusterSetEventConfigurationCallback(int32u issuerEventId,
                                                                    int32u startDateTime,
                                                                    int8u eventConfiguration,
                                                                    int8u configurationControl,
                                                                    int8u* eventConfigurationPayload)
{
  int8u payloadIndex = 0;
  switch(configurationControl) {
    case EMBER_ZCL_EVENT_CONFIGURATION_CONTROL_APPLY_BY_LIST : {
      int8u numberOfEvents = eventConfigurationPayload[0];
      emberAfDeviceManagementClusterPrintln("Number of Events %u",numberOfEvents);
      payloadIndex = 1;
      while(payloadIndex < numberOfEvents * 2) {
        int16u attributeId = emberAfGetInt16u(eventConfigurationPayload, payloadIndex, numberOfEvents*2 + 1);
        payloadIndex += 2;
        emberAfDeviceManagementClusterPrintln("AttributeId 0x%2x",attributeId);
        writeDeviceManagementAttribute(attributeId, eventConfiguration);
      }
      break; 
    }
    case EMBER_ZCL_EVENT_CONFIGURATION_CONTROL_APPLY_BY_EVENT_GROUP: {
      int16u eventGroupId = emberAfGetInt16u(eventConfigurationPayload,0,2);
      int8u attributeSet = (eventGroupId & 0xFF00) >> 8;
      writeDeviceManagementClusterWildCardAttribute(attributeSet,
                                                    emberAfCurrentEndpoint(),
                                                    eventConfiguration);
      break;
    }
    case EMBER_ZCL_EVENT_CONFIGURATION_CONTROL_APPLY_BY_LOG_TYPE: {
      int8u logType = eventConfigurationPayload[0];
      writeDeviceManagementClusterByLogTypeAttribute(logType,
                                                     emberAfCurrentEndpoint(),
                                                     eventConfiguration);
      break;
    }
    case EMBER_ZCL_EVENT_CONFIGURATION_CONTROL_APPLY_BY_CONFIGURATION_MATCH:{
      int8u currentConfiguration = eventConfigurationPayload[0];
      writeDeviceManagementClusterByMatchingAttribute(currentConfiguration,
                                                     emberAfCurrentEndpoint(),
                                                     eventConfiguration);
      break;
    }
    default:
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_VALUE);
  }
  return TRUE;
}
