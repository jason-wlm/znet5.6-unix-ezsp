// *******************************************************************
// * ami-simple-metering.c
// *
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

// this file contains all the common includes for clusters in the util
#include "../../include/af.h"
#include "../../util/common.h"
#include "simple-metering-server.h"
#include "simple-metering-test.h"

#define MAX_FAST_POLLING_PERIOD 15
#define PROVIDER_ID 0x44556677


// Bug: SE1P2-18
// Get Sampled Data Response does not send the maximum number of samples.
// If we want 5 samples(which we interpret as diff's between 2 consecutive sampling data points)
// we need 6 data points.
#define REAL_MAX_SAMPLES_PER_SESSION (EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLES_PER_SESSION + 1)


// Bug:SE1P2-19
int32u minIssuerEventId = 0x00000000;

typedef struct {
  int8u endpoint;
  int32u issuerEventId;
  int32u startTime;
  int16u sampleId;
  int8u  sampleType;
  int16u sampleRequestInterval;
  int16u maxNumberOfSamples;
  int8u  validSamples;
  int8u  samples[6][REAL_MAX_SAMPLES_PER_SESSION];
} EmberAfSimpleMeteringClusterSamplingData;

typedef struct SupplyEvent{
  int8u srcEndpoint;
  int8u destEndpoint;
  int16u nodeId;
  int32u providerId;
  int32u issuerEventId;
  int32u implementationDateTime;
  int8u proposedSupplyStatus;
} EmberAfSimpleMeteringClusterSupplyEvent;


static int32u fastPollEndTimeUtcTable[EMBER_AF_SIMPLE_METERING_CLUSTER_SERVER_ENDPOINT_COUNT];
static EmberAfSimpleMeteringClusterSamplingData samplingData[EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLING_SESSIONS];
static EmberAfSimpleMeteringClusterSupplyEvent changeSupply;

EmberEventControl emberAfPluginSimpleMeteringServerSamplingEventControl;
EmberEventControl emberAfPluginSimpleMeteringServerSupplyEventControl;

static int8u fastPolling = 0;

void emAfToggleFastPolling(int8u enableFastPolling)
{
  fastPolling = enableFastPolling;
}

static void fastPollEndTimeUtcTableInit(void)
{
  int8u i;
  for (i = 0; i < EMBER_AF_SIMPLE_METERING_CLUSTER_SERVER_ENDPOINT_COUNT; i++) {
    fastPollEndTimeUtcTable[i] = 0x00000000;
  }
}

static void samplingDataInit(void)
{
  int8u i;
  for(i = 0; i < EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLING_SESSIONS; i++) {
    samplingData[i].sampleId = 0xFFFF;
    samplingData[i].validSamples = 0x00;
  }
}

static int8u findSamplingSession(int16u sampleId)
{
  int8u i;
  for(i = 0; i < EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLING_SESSIONS; i++) {
    if(samplingData[i].sampleId == sampleId)
      return i;
  }
  return 0xFF;
}

static int8u findSamplingSessionByEventId(int32u issuerEventId)
{
  int8u i;
  for(i = 0; i < EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLING_SESSIONS; i++) {
    if(samplingData[i].issuerEventId == issuerEventId)
      return i;
  }
  return 0xFF;
}
void emberAfSimpleMeteringClusterServerInitCallback(int8u endpoint) 
{
  emAfTestMeterInit(endpoint);
  fastPollEndTimeUtcTableInit();
  samplingDataInit();
  emberAfScheduleServerTick(endpoint,
                            ZCL_SIMPLE_METERING_CLUSTER_ID,
                            MILLISECOND_TICKS_PER_SECOND);
}

void emberAfSimpleMeteringClusterServerTickCallback(int8u endpoint) 
{
  emAfTestMeterTick(endpoint); //run test module
  emberAfScheduleServerTick(endpoint,
                            ZCL_SIMPLE_METERING_CLUSTER_ID,
                            MILLISECOND_TICKS_PER_SECOND);
}

boolean emberAfSimpleMeteringClusterGetProfileCallback(int8u intervalChannel,
                                                       int32u endTime,
                                                       int8u numberOfPeriods)
{
  return emAfTestMeterGetProfiles(intervalChannel, endTime, numberOfPeriods);
}

boolean emberAfSimpleMeteringClusterRequestFastPollModeCallback(int8u fastPollUpdatePeriod, 
                                                                int8u duration) 
{
  int8u endpoint;
  int8u ep;
  int8u appliedUpdateRate;
  EmberStatus status;
  int8u fastPollingUpdateAttribute;

  if(!fastPolling) {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND);
    return TRUE;
  }
  endpoint = emberAfCurrentEndpoint();
  ep = emberAfFindClusterServerEndpointIndex(endpoint,
                                                   ZCL_SIMPLE_METERING_CLUSTER_ID);
  
  appliedUpdateRate = fastPollUpdatePeriod;

  if (ep == 0xFF) {
    emberAfSimpleMeteringClusterPrintln("Invalid endpoint %x", emberAfCurrentEndpoint());
    return FALSE;
  }

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_SIMPLE_METERING_CLUSTER_ID,
                                      ZCL_FAST_POLL_UPDATE_PERIOD_ATTRIBUTE_ID,
                                      (int8u *)&fastPollingUpdateAttribute,
                                      sizeof(fastPollingUpdateAttribute));

  if(status == EMBER_SUCCESS) {
    if(fastPollUpdatePeriod < fastPollingUpdateAttribute) {
      appliedUpdateRate = fastPollingUpdateAttribute;
      emberAfSimpleMeteringClusterPrintln("Applying fast Poll rate %x ep %u",appliedUpdateRate,ep);
    }
  } else {
    emberAfSimpleMeteringClusterPrintln("Reading fast Poll Attribute failed. ep %u  status %x",ep,status);
    emberAfFillCommandSimpleMeteringClusterRequestFastPollModeResponse(0, 
                                                                       0);
    emberAfSendResponse();
    return TRUE;
  }

  if (emberAfGetCurrentTime() > fastPollEndTimeUtcTable[ep]) {
    duration = duration > MAX_FAST_POLLING_PERIOD ? MAX_FAST_POLLING_PERIOD : duration;
    fastPollEndTimeUtcTable[ep] = emberAfGetCurrentTime() + (duration * 60);
    emberAfSimpleMeteringClusterPrintln("Starting fast polling for %u minutes  End Time 0x%4x,current Time 0x%4x",duration,fastPollEndTimeUtcTable[ep],emberAfGetCurrentTime());
  } else {
    emberAfSimpleMeteringClusterPrintln("Fast polling mode currently active. ep %u fastPollEndTimeUtcTable[%u] 0x%4x current Time 0x%4x ",ep,ep,fastPollEndTimeUtcTable[ep],emberAfGetCurrentTime());
  }
  emberAfFillCommandSimpleMeteringClusterRequestFastPollModeResponse(appliedUpdateRate, 
                                                                     fastPollEndTimeUtcTable[ep]);
  emberAfSendResponse();
  return TRUE;
}

void emberAfSimpleMeteringClusterServerDefaultResponseCallback(int8u endpoint,
                                                               int8u commandId,
                                                               EmberAfStatus status)
{
  if(commandId == ZCL_REMOVE_MIRROR_COMMAND_ID &&
     status != EMBER_ZCL_STATUS_SUCCESS){
    emberAfSimpleMeteringClusterPrintln("Mirror remove FAILED status 0x%x",status);
  }
}


boolean emberAfSimpleMeteringClusterGetSampledDataCallback(int16u sampleId,
                                                           int32u earliestSampleTime,
                                                           int8u sampleType,
                                                           int16u numberOfSamples)
{
  int8u i;
  emberAfSimpleMeteringClusterPrintln("sampleId %u earliestSampleTime %u sampleType %u numberOfSamples %u",sampleId,earliestSampleTime,sampleType,numberOfSamples);
  for(i = 0; i < EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLING_SESSIONS; i++) {
    int8u j;
    int32u diff;
    if(samplingData[i].sampleId == sampleId &&
      samplingData[i].sampleId < 0xFFFF &&
       samplingData[i].startTime <= earliestSampleTime &&
       samplingData[i].sampleType == sampleType) {

      //If we have only one sample data point or lesser, we don't have enough information
      //for even one sample.
      if(samplingData[i].validSamples <= 1)
        goto kickout;

      //Bug SE1P2-19: Use the max field from the client.

      if(numberOfSamples > samplingData[i].validSamples - 1){
        numberOfSamples = samplingData[i].validSamples - 1;
      }

      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND \
                           | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT), \
                          ZCL_SIMPLE_METERING_CLUSTER_ID, \
                          ZCL_GET_SAMPLED_DATA_RESPONSE_COMMAND_ID, \
                          "vwuvv", \
                          samplingData[i].sampleId, \
                          samplingData[i].startTime, \
                          samplingData[i].sampleType, \
                          samplingData[i].sampleRequestInterval, \
                          numberOfSamples);


      emberAfSimpleMeteringClusterPrintln("numberOfSamples 0x%2x", numberOfSamples);



      
      for(j = 0; j < numberOfSamples; j++) {
        int32u b = (samplingData[i].samples[j+1][3] << 24) | (samplingData[i].samples[j+1][2] << 16) | (samplingData[i].samples[j+1][1] << 8) | (samplingData[i].samples[j+1][0] << 0);
        int32u a = (samplingData[i].samples[j][3] << 24) | (samplingData[i].samples[j][2] << 16) | (samplingData[i].samples[j][1] << 8) | (samplingData[i].samples[j][0] << 0);

        diff = b - a;
        emberAfPutInt24uInResp(diff);
        emberAfSimpleMeteringClusterPrintln("index %u numberOfSamples %u diff %u",j,numberOfSamples,diff);
      }
      emberAfSendResponse();
      return TRUE;
    }    
  }

  kickout:
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  return TRUE;
}

int16u emberAfPluginSimpleMeteringServerStartSampling(int16u requestedSampleId,
                                                      int32u issuerEventId,
                                                      int32u startSamplingTime,
                                                      int8u sampleType,
                                                      int16u sampleRequestInterval,
                                                      int16u maxNumberOfSamples,
                                                      int8u endpoint)
{
  int32u delay;
  int32u currentTime;
  static int16u nextSampleId = 0x0001;
  int16u sampleId;
  int8u index;


  emberAfSimpleMeteringClusterPrintln("StartSampling: requestedSampleId %u issuerEventId %u startSamplingTime %u sampleType %u sampleRequestInterval %u maxNumberOfSamples %u endpoint %u",requestedSampleId,issuerEventId,startSamplingTime,sampleType,sampleRequestInterval,maxNumberOfSamples,endpoint);



  // Find an unused sampling session table entry
  index = findSamplingSession(0xFFFF);
  if (index == 0xFF) {
    emberAfSimpleMeteringClusterPrintln("ERR: No available entries in sampling session table");
    return 0xFFFF;
  }

  // if the caller requested a specific sampleId then check to make sure it's
  // not already in use.
  if (requestedSampleId != 0xFFFF) {
    if (findSamplingSession(requestedSampleId) != 0xFF) {
      emberAfSimpleMeteringClusterPrintln("ERR: requested sampleId already in use");
      return 0xFFFF;
    } else {
      sampleId = requestedSampleId;
    }
  } else {
    sampleId = nextSampleId;
    while (findSamplingSession(sampleId) != 0xFF) {
      sampleId = (sampleId == 0xFFFE) ? 1 : (sampleId + 1);
    }
    nextSampleId = (sampleId == 0xFFFE) ? 1 : (sampleId + 1);
  }

  samplingData[index].sampleId = sampleId;
  samplingData[index].issuerEventId = issuerEventId;
  samplingData[index].startTime = startSamplingTime;
  samplingData[index].sampleType = sampleType;
  samplingData[index].sampleRequestInterval = sampleRequestInterval;

  //This is a hardcoded limit in EmberAfSimpleMeteringClusterSamplingData.
  if(maxNumberOfSamples > REAL_MAX_SAMPLES_PER_SESSION) {
    maxNumberOfSamples = REAL_MAX_SAMPLES_PER_SESSION;
  }
  samplingData[index].maxNumberOfSamples = maxNumberOfSamples;
  samplingData[index].endpoint = endpoint;

  currentTime = emberAfGetCurrentTime();
  if(startSamplingTime < currentTime) startSamplingTime = currentTime;
  delay = startSamplingTime - currentTime;
  emberEventControlSetDelayMS(emberAfPluginSimpleMeteringServerSamplingEventControl,
                              (delay*1000));
  return sampleId;
}

boolean emberAfSimpleMeteringClusterStartSamplingCallback(int32u issuerEventId,
                                                          int32u startSamplingTime,
                                                          int8u sampleType,
                                                          int16u sampleRequestInterval,
                                                          int16u maxNumberOfSamples)
{

  static int8u firstIssuerId = 0;

  // Bug: SE1P2-19
  // Issuer Eventid should not be ignored. We accept *anything* for the first time.
  // After that, we only accept values greater than the max id received so far(which
  // should be the last valid issuerId received).
  if(firstIssuerId == 0) {
    minIssuerEventId = issuerEventId;
    firstIssuerId = 1;
  } else if(issuerEventId <= minIssuerEventId && startSamplingTime != 0xFFFFFFFF) {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE);
    emberAfSimpleMeteringClusterPrintln("Rejecting StartSamplingCallback issuerEventId %u minIssuerEventId %u",issuerEventId,minIssuerEventId);
    return TRUE;
  } else {
    minIssuerEventId = issuerEventId;
  }

  // Bug: SE1P2-17
  // StartSampling Event ID field is ignored by the metering server
  //Special time reserved to cancel a startSampling Request.
  if(startSamplingTime == 0xFFFFFFFF) {
    int8u eventIndex = findSamplingSessionByEventId(issuerEventId);
    //Event not found.
    if(eventIndex == 0xFF) {
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
      return TRUE;
    } else {
      samplingData[eventIndex].sampleId = 0xFFFF;
      samplingData[eventIndex].validSamples = 0x00;
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
      return TRUE;
    }
  }

  int16u sampleId = emberAfPluginSimpleMeteringServerStartSampling(0xFFFF,
                                                                   issuerEventId,
                                                                   startSamplingTime,
                                                                   sampleType,
                                                                   sampleRequestInterval,
                                                                   maxNumberOfSamples,
                                                                   emberAfCurrentEndpoint());
  emberAfFillCommandSimpleMeteringClusterStartSamplingResponse(sampleId);
  emberAfSendResponse();
  return TRUE;
}

void emberAfPluginSimpleMeteringServerSamplingEventHandler(void)
{
  int8u i;
  EmberAfAttributeType dataType;
  EmberStatus status;
  emberEventControlSetInactive(emberAfPluginSimpleMeteringServerSamplingEventControl);

  // Now let's adjust the summation
  for(i = 0; i < EMBER_AF_PLUGIN_SIMPLE_METERING_SERVER_MAX_SAMPLING_SESSIONS; i++) {
    if(samplingData[i].startTime <= emberAfGetCurrentTime() &&
       samplingData[i].validSamples <= samplingData[i].maxNumberOfSamples &&
       samplingData[i].sampleId < 0xFFFF) {
      status = emberAfReadAttribute(samplingData[i].endpoint,
                                    ZCL_SIMPLE_METERING_CLUSTER_ID,
                                    ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                                    CLUSTER_MASK_SERVER,
                                    samplingData[i].samples[samplingData[i].validSamples],
                                    6,
                                    &dataType);
      if(status == EMBER_SUCCESS) {
        emberAfSimpleMeteringClusterPrintln("Sample %u: 0x%x%x%x%x%x%x",
                                            samplingData[i].validSamples,
                                            samplingData[i].samples[samplingData[i].validSamples][0],
                                            samplingData[i].samples[samplingData[i].validSamples][1],
                                            samplingData[i].samples[samplingData[i].validSamples][2],
                                            samplingData[i].samples[samplingData[i].validSamples][3],
                                            samplingData[i].samples[samplingData[i].validSamples][4],
                                            samplingData[i].samples[samplingData[i].validSamples][5]);
        samplingData[i].validSamples++;
      }
      emberAfSimpleMeteringClusterPrintln("Interval %u",samplingData[i].sampleRequestInterval);
      emberEventControlSetDelayMS(emberAfPluginSimpleMeteringServerSamplingEventControl,
                                  samplingData[i].sampleRequestInterval * 1000);
    }
  }
}




void emberAfPluginSimpleMeteringServerSupplyEventHandler(void)
{
  int32u changeTime;
  int32u currentTime;
  emberEventControlSetInactive(emberAfPluginSimpleMeteringServerSupplyEventControl);

  emberAfReadServerAttribute(changeSupply.srcEndpoint,
                             ZCL_SIMPLE_METERING_CLUSTER_ID,
                             ZCL_PROPOSED_CHANGE_SUPPLY_IMPLEMENTATION_TIME_ATTRIBUTE_ID,
                             (int8u *)&changeTime,
                             sizeof(changeTime));
  currentTime = emberAfGetCurrentTime();
  if(changeTime > currentTime) {
    int32u delay = changeTime - currentTime;
    emberEventControlSetDelayMS(emberAfPluginSimpleMeteringServerSupplyEventControl,
                                delay*1000 );    
  } else {

    emberAfFillCommandSimpleMeteringClusterSupplyStatusResponse(changeSupply.providerId,
                                                                changeSupply.issuerEventId,
                                                                changeSupply.implementationDateTime,
                                                                changeSupply.proposedSupplyStatus);
    emberAfSetCommandEndpoints(changeSupply.srcEndpoint, changeSupply.destEndpoint);
    emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, changeSupply.nodeId);
  }
}

boolean emberAfSimpleMeteringClusterResetLoadLimitCounterCallback(int32u providerId,
                                                                  int32u issuerEventId)
{
  int8u counter = 0;
  emberAfSimpleMeteringClusterPrintln("Reset Load Counter providerId %u issuerEventId %u",providerId,issuerEventId);
  emberAfWriteAttribute(emberAfCurrentEndpoint(),
                        ZCL_SIMPLE_METERING_CLUSTER_ID,
                        ZCL_LOAD_LIMIT_COUNTER_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        &counter,
                        ZCL_INT8U_ATTRIBUTE_TYPE);  
  return TRUE;
}

boolean emberAfSimpleMeteringClusterChangeSupplyCallback(int32u providerId,
                                                         int32u issuerEventId,
                                                         int32u requestDateTime,
                                                         int32u implementationDateTime,
                                                         int8u proposedSupplyStatus,
                                                         int8u supplyControlBits)
{
  int32u delay;
  EmberAfClusterCommand *cmd;

  emberAfSimpleMeteringClusterPrintln("Change Supply Callback providerId %u issuerEventId %u implementationDateTime %u supplyStatus %u",providerId,issuerEventId,implementationDateTime,supplyControlBits);
  // TODO: fix this hard-coded check
  if(providerId != PROVIDER_ID){
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_AUTHORIZED);
    return TRUE;
  }
  if(implementationDateTime < emberAfGetCurrentTime() && implementationDateTime != 0){
    emberAfSimpleMeteringClusterPrintln("implementationDateTime %u",implementationDateTime);
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_VALUE);
    return TRUE;
  }
  if(implementationDateTime == 0xFFFFFFFF) {
    emberAfSimpleMeteringClusterPrintln("Canceling change supply");
    emberEventControlSetInactive(emberAfPluginSimpleMeteringServerSupplyEventControl);
  }
  if(implementationDateTime == 0x00000000 ) {
    emberAfWriteAttribute(emberAfCurrentEndpoint(),
                          ZCL_SIMPLE_METERING_CLUSTER_ID,
                          ZCL_PROPOSED_CHANGE_SUPPLY_IMPLEMENTATION_TIME_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER,
                          (int8u *)&implementationDateTime,
                          ZCL_UTC_TIME_ATTRIBUTE_TYPE);

    emberAfWriteAttribute(emberAfCurrentEndpoint(),
                          ZCL_SIMPLE_METERING_CLUSTER_ID,
                          ZCL_PROPOSED_CHANGE_SUPPLY_STATUS_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER,
                          &proposedSupplyStatus,
                          ZCL_ENUM8_ATTRIBUTE_TYPE); 

    if((supplyControlBits & 0x01) == 0x01 || proposedSupplyStatus == 2) {
      emberAfFillCommandSimpleMeteringClusterSupplyStatusResponse(providerId,
                                                                  issuerEventId,
                                                                  implementationDateTime,
                                                                  proposedSupplyStatus);
      emberAfSendResponse();
    } else {
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    }
  } else {
    emberAfWriteAttribute(emberAfCurrentEndpoint(),
                          ZCL_SIMPLE_METERING_CLUSTER_ID,
                          ZCL_PROPOSED_CHANGE_SUPPLY_IMPLEMENTATION_TIME_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER,
                          (int8u *)&implementationDateTime,
                          ZCL_UTC_TIME_ATTRIBUTE_TYPE);

    emberAfWriteAttribute(emberAfCurrentEndpoint(),
                          ZCL_SIMPLE_METERING_CLUSTER_ID,
                          ZCL_PROPOSED_CHANGE_SUPPLY_STATUS_ATTRIBUTE_ID,
                          CLUSTER_MASK_SERVER,
                          &proposedSupplyStatus,
                          ZCL_ENUM8_ATTRIBUTE_TYPE); 
    emberAfFillCommandSimpleMeteringClusterSupplyStatusResponse(providerId,
                                                                issuerEventId,
                                                                implementationDateTime,
                                                                proposedSupplyStatus);
    delay = implementationDateTime - emberAfGetCurrentTime();
    cmd = emberAfCurrentCommand();
    changeSupply.srcEndpoint = cmd->apsFrame->destinationEndpoint;
    changeSupply.destEndpoint = cmd->apsFrame->sourceEndpoint;
    changeSupply.nodeId = cmd->source; 
    changeSupply.providerId = providerId;
    changeSupply.issuerEventId = issuerEventId;
    changeSupply.implementationDateTime = implementationDateTime;
    changeSupply.proposedSupplyStatus = proposedSupplyStatus;
    if((supplyControlBits & 0x01) == 0x01) {
      emberEventControlSetDelayMS(emberAfPluginSimpleMeteringServerSupplyEventControl,
                                  delay*1000 );      
    } else {
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    }
  }

  return TRUE;
}

boolean emberAfSimpleMeteringClusterLocalChangeSupplyCallback(int8u proposedSupplyStatus)
{
  if(proposedSupplyStatus < 1 || proposedSupplyStatus > 2)
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_AUTHORIZED);
  else{
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    emberAfSimpleMeteringClusterPrintln("Setting localSupply Status %u",proposedSupplyStatus);
  }
  return TRUE;
}                                                                 

boolean emberAfSimpleMeteringClusterSetSupplyStatusCallback(int32u issuerEventId,
                                                            int8u supplyTamperState,
                                                            int8u supplyDepletionState,
                                                            int8u supplyUncontrolledFlowState,
                                                            int8u loadLimitSupplyState)
{
  emberAfWriteAttribute(emberAfCurrentEndpoint(),
                        ZCL_SIMPLE_METERING_CLUSTER_ID,
                        ZCL_SUPPLY_TAMPER_STATE_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        &supplyTamperState,
                        ZCL_ENUM8_ATTRIBUTE_TYPE);

  emberAfWriteAttribute(emberAfCurrentEndpoint(),
                        ZCL_SIMPLE_METERING_CLUSTER_ID,
                        ZCL_SUPPLY_DEPLETION_STATE_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        &supplyDepletionState,
                        ZCL_ENUM8_ATTRIBUTE_TYPE);

  emberAfWriteAttribute(emberAfCurrentEndpoint(),
                        ZCL_SIMPLE_METERING_CLUSTER_ID,
                        ZCL_SUPPLY_UNCONTROLLED_FLOW_STATE_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        &supplyUncontrolledFlowState,
                        ZCL_ENUM8_ATTRIBUTE_TYPE);

  emberAfWriteAttribute(emberAfCurrentEndpoint(),
                        ZCL_SIMPLE_METERING_CLUSTER_ID,
                        ZCL_LOAD_LIMIT_SUPPLY_STATE_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        &loadLimitSupplyState,
                        ZCL_ENUM8_ATTRIBUTE_TYPE);
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfSimpleMeteringClusterSetUncontrolledFlowThresholdCallback(int32u providerId,
                                                                         int32u issuerEventId,
                                                                         int16u uncontrolledFlowThreshold,
                                                                         int8u unitOfMeasure,
                                                                         int16u multiplier,
                                                                         int16u divisor,
                                                                         int8u stabilisationPeriod,
                                                                         int16u measurementPeriod)
{
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}


boolean emberAfSimpleMeteringClusterMirrorReportAttributeResponseCallback(int8u notificationScheme,
                                                                          int8u* notificationFlags)
{
  if (notificationScheme == 0x01) {
    int32u functionalFlags = emberAfGetInt32u(notificationFlags,0,4);
    emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID, functionalFlags);
  } else if (notificationScheme == 0x02) {
    int32u functionalFlags = emberAfGetInt32u(notificationFlags,0,20);
    int32u notificationFlags2 = emberAfGetInt32u(notificationFlags,4,20);
    int32u notificationFlags3 = emberAfGetInt32u(notificationFlags,8,20);
    int32u notificationFlags4 = emberAfGetInt32u(notificationFlags,12,20);
    int32u notificationFlags5 = emberAfGetInt32u(notificationFlags,16,20);
    emberAfSimpleMeteringClusterPrintln("functionalFlags 0x%4x notificationFlags2-5 0x%4x 0x%4x 0x%4x 0x%4x",
                                        functionalFlags, notificationFlags2, notificationFlags3,
                                        notificationFlags4, notificationFlags5);
    emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(ZCL_FUNCTIONAL_NOTIFICATION_FLAGS_ATTRIBUTE_ID, functionalFlags);
    emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(ZCL_NOTIFICATION_FLAGS_2_ATTRIBUTE_ID, notificationFlags2);
    emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(ZCL_NOTIFICATION_FLAGS_3_ATTRIBUTE_ID, notificationFlags3);
    emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(ZCL_NOTIFICATION_FLAGS_4_ATTRIBUTE_ID, notificationFlags4);
    emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(ZCL_NOTIFICATION_FLAGS_5_ATTRIBUTE_ID, notificationFlags5);
  } else {
    return FALSE;
  }
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

void emberAfPluginSimpleMeteringClusterReadAttributesResponseCallback(EmberAfClusterId clusterId,
                                                                      int8u *buffer,
                                                                      int16u bufLen)
{
  int16u bufIndex = 0;

  if (clusterId != ZCL_SIMPLE_METERING_CLUSTER_ID
      || emberAfCurrentCommand()->direction != ZCL_FRAME_CONTROL_CLIENT_TO_SERVER) {
    return;
  }

  // Each record in the response has a two-byte attribute id and a one-byte
  // status.  If the status is SUCCESS, there will also be a one-byte type and
  // variable-length data.
  while (bufIndex + 3 <= bufLen) {
    EmberAfStatus status;
    EmberAfAttributeId attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer,
                                                                          bufIndex,
                                                                          bufLen);
    bufIndex += 2;
    status = (EmberAfStatus)emberAfGetInt8u(buffer, bufIndex, bufLen);
    bufIndex++;
    if (status == EMBER_ZCL_STATUS_SUCCESS) {
      int8u dataSize;
      int8u dataType = emberAfGetInt8u(buffer, bufIndex, bufLen);
      bufIndex++;

      // For strings, the data size is the length of the string (specified by
      // the first byte of data) plus one for the length byte itself.  For
      // everything else, the size is just the size of the data type.
      dataSize = (emberAfIsThisDataTypeAStringType(dataType)
                  ? emberAfStringLength(buffer + bufIndex) + 1
                  : emberAfGetDataSize(dataType));

      if (bufIndex + dataSize <= bufLen) {
        // The Notification Attribute Set is in the range of 0x0000 - 0x00FF
        // and are all 32 bit BitMap types. Each application may decide to
        // handle the notification flags differently so we'll callback to the
        // application for each notification flags attribute.
        if (attributeId < 0x0100) {
          int32u bitMap = emberAfGetInt32u(buffer, bufIndex, bufLen);
          emberAfSimpleMeteringClusterPrintln("Attribute value 0x%4x", bitMap);
          emberAfPluginSimpleMeteringServerProcessNotificationFlagsCallback(attributeId, bitMap);
        }
      }
      bufIndex += dataSize;
    }
  }
}
