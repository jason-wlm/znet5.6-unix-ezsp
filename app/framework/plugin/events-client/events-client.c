// *******************************************************************                                                                                        
// * events-client.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*                                                                              
// ******************************************************************* 

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"

#define LOG_PAYLOAD_CONTROL_MASK          0x0F
#define NUMBER_OF_EVENTS_MASK             0xF0
#define eventCrossesFrameBoundary(lpc)    ((lpc) & EventCrossesFrameBoundary)
#define numberOfEvents(lpc)               (((lpc) & NUMBER_OF_EVENTS_MASK) >> 4)

boolean emberAfEventsClusterPublishEventCallback(int8u logId,
                                                 int16u eventId,
                                                 int32u eventTime,
                                                 int8u eventControl,
                                                 int8u* eventData)
{
  emberAfEventsClusterPrint("RX: PublishEvent 0x%x, 0x%2x, 0x%4x, 0x%x, 0x%x",
                            logId,
                            eventId,
                            eventTime,
                            eventControl,
                            *eventData);

#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_EVENTS_CLUSTER)
  int8u eventDataLen = emberAfStringLength(eventData);
  if (eventDataLen > 0) {
    emberAfEventsClusterPrint(", ");
    emberAfEventsClusterPrintString(eventData);
  }
  emberAfEventsClusterPrintln("");
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_EVENTS_CLUSTER)

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfEventsClusterPublishEventLogCallback(int16u totalNumberOfEvents,
                                                    int8u commandIndex,
                                                    int8u totalCommands,
                                                    int8u logPayloadControl,
                                                    int8u* logPayload)
{
  emberAfEventsClusterPrint("RX: PublishEventLog 0x%2x, 0x%x, 0x%x, 0x%x",
                            totalNumberOfEvents,
                            commandIndex,
                            totalCommands,
                            logPayloadControl);

  #if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_EVENTS_CLUSTER)
  int16u logPayloadLen = (emberAfCurrentCommand()->bufLen
                          - (emberAfCurrentCommand()->payloadStartIndex
                             + sizeof(totalNumberOfEvents)
                             + sizeof(commandIndex)
                             + sizeof(totalCommands)
                             + sizeof(logPayloadControl)));
  int16u logPayloadIndex = 0;
  int8u i;
  if (NULL != logPayload) {
    for (i = 0; i < numberOfEvents(logPayloadControl); i++) {
      int8u logId;
      int16u eventId;
      int32u eventTime;
      int8u *eventData;
      int8u eventDataLen;
      logId = emberAfGetInt8u(logPayload, logPayloadIndex, logPayloadLen);
      logPayloadIndex++;
      eventId = emberAfGetInt16u(logPayload, logPayloadIndex, logPayloadLen);
      logPayloadIndex += 2;
      eventTime = emberAfGetInt32u(logPayload, logPayloadIndex, logPayloadLen);
      logPayloadIndex += 4;
      eventData = logPayload + logPayloadIndex;
      eventDataLen = emberAfGetInt8u(logPayload, logPayloadIndex, logPayloadLen);
      logPayloadIndex += (1 + eventDataLen);
      emberAfEventsClusterPrint(" [");
      emberAfEventsClusterPrint("0x%x, 0x%2x, 0x%4x, 0x%x", logId, eventId, eventTime, eventDataLen);
      if (eventDataLen > 0) {
        emberAfEventsClusterPrint(", ");
        emberAfEventsClusterPrintString(eventData);
      }
      emberAfEventsClusterPrint("]");
    }
  }
  emberAfEventsClusterPrintln("");
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_EVENTS_CLUSTER)

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}

boolean emberAfEventsClusterClearEventLogResponseCallback(int8u clearedEventsLogs)
{
  emberAfEventsClusterPrintln("RX: ClearEventLogResponse 0x%x",
                              clearedEventsLogs);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}
