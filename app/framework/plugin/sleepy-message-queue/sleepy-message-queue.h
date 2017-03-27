// *******************************************************************
// * sleepy-message-queue.h
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*
// *******************************************************************

#ifndef _SLEEPY_MESSAGE_QUEUE_H_
#define _SLEEPY_MESSAGE_QUEUE_H_

typedef int8u EmberAfSleepyMessageId;

typedef struct
{
  int8u *payload;
  int16u length;
  int16u payloadId;
  EmberEUI64 dstEui64;
} EmberAfSleepyMessage;

#define EMBER_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID 0xFF

/**
 * @brief Initialize the sleepy message queue.
 *
 **/
void  emberAfPluginSleepyMessageQueueInitCallback( void );

/**
 * @brief Returns the number of unused entries in the sleepy message queue.
 *
 **/
int8u emberAfPluginSleepyMessageQueueGetNumUnusedEntries( void );

/**
 * @brief Stores an EmberAfSleepyMessage to the sleepy message queue if an entry is available.
 * @param pmsg Pointer to an EmberAfSleepyMessage structure containing information about the message that should be stored.
 * @param timeoutSec The time in seconds that the message should be stored in the sleepy message queue.
 * @return The EmberAfSleepyMessageId assigned to the message if stored, or EMBER_AF_PLUGING_SLEEPY_MESSAGE_INVALID_ID if the message could not be stored to the queue.  The message may not be stored if the queue is full, or the time duration exceeds the maximum duration.
 **/
EmberAfSleepyMessageId emberAfPluginSleepyMessageQueueStoreMessage( EmberAfSleepyMessage *pmsg, int32u timeoutSec );

/**
 * @brief Returns the number of milliseconds remaining until the sleepy message expires.
 * @param sleepyMsgId The EmberAfSleepyMessageId of the message whose timeout should be found.
 * @return The number of milliseconds until the specified message expires, or 0xFFFFFFFF if a matching active message cannot be found.
 **/
int32u emMessageMSecRemaining( EmberAfSleepyMessageId sleepyMsgId );

/**
 * @brief Returns the next EmberAfSleepyMessageId value (that will expire next) for a given EmberEUI64.
 * @param dstEui64 The EmberEUI64 value of a device whose EmberAfSleepyMessageId is being queried.
 * @return The EmberAfSleepyMessageId value of the next-expiring message for the specified EmberEUI64 if a match was found, or EMBER_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID if a matching entry was not found.
 **/
EmberAfSleepyMessageId emberAfPluginSleepyMessageQueueGetPendingMessageId( EmberEUI64 dstEui64 );

/**
 * @brief Searches the sleepy message queue for an entry with the specified EmberAfSleepyMessageId.  
 *
 * If a match was found, this copies the message into the EmberAfSleepyMessage structure pointer.
 *
 * @param sleepyMsgId The EmberAfSleepyMessageId of the EmberAfSleepyMessage structure that should be looked up in the sleepy message queue.
 * @param pmsg A pointer to an EmberAfSleepyMessage structure.  If a message is found in the sleepy message queue with a matching EmberAfSleepyMessageId, it will be copied to this structure.
 * @return TRUE if a matching message was found or FALSE if a match was not found.
 **/
boolean emberAfPluginSleepyMessageQueueGetPendingMessage( EmberAfSleepyMessageId sleepyMsgId, EmberAfSleepyMessage *pmsg );

/**
 * @brief Returns the time in milliseconds until the next message in the sleepy message queue will timeout.
 * @return The remaining time in milliseconds until the next message will timeout.
 **/
int32u emberAfPluginSleepyMessageQueueGetNextMessageEventTimeoutMs( void );

/**
 * @brief Returns the number of messages in the sleepy message queue that are buffered for a given EmberEUI64.
 * @param dstEui64 The destination EUI64 that should be used to count matching messages in the sleepy message queue.
 * @return The number of messages in the sleepy message queue that are being sent to the specified EmberEUI64.
 **/
int8u emberAfPluginSleepyMessageQueueGetNumMessages( EmberEUI64 dstEui64 );

/**
 * @brief Removes the message from the sleepy message queue with the specified EmberAfSleepyMessageId.
 * @param sleepyMsgId The EmberAfSleepyMessageId that should be removed from the sleepy message queue.
 * @return TRUE if a matching EmberAfSleepyMessageId was found and removed from the sleepy message queue, or FALSE if not.
 **/
boolean emberAfPluginSleepyMessageQueueRemoveMessage(  EmberAfSleepyMessageId  sleepyMsgId  );

/**
 * @brief Removes all messages from the sleepy message queue whose destination address matches the specified EmberEUI64.
 * @param dstEui64 The EmberEUI64 to search for in the sleepy message queue.  All entries with a matching destination EUI64 should be removed.
 **/
void emberAfPluginSleepyMessageQueueRemoveAllMessages(  EmberEUI64 dstEui64  );



#endif  // #ifndef _SLEEPY_MESSAGE_QUEUE_H_

