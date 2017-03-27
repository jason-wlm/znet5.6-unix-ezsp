/*
 * simple-metering-server.h
 *
 *  Created on: Dec 11, 2014
 *      Author: romacdon
 */

#ifndef SIMPLE_METERING_SERVER_H_
#define SIMPLE_METERING_SERVER_H_

void emAfToggleFastPolling(int8u enableFastPolling);
void emberAfPluginSimpleMeteringClusterReadAttributesResponseCallback(EmberAfClusterId clusterId,
                                                                      int8u *buffer,
                                                                      int16u bufLen);
int16u emberAfPluginSimpleMeteringServerStartSampling(int16u requestedSampleId,
                                                      int32u issuerEventId,
                                                      int32u startSamplingTime,
                                                      int8u sampleType,
                                                      int16u sampleRequestInterval,
                                                      int16u maxNumberOfSamples,
                                                      int8u endpoint);

#endif /* SIMPLE_METERING_SERVER_H_ */
