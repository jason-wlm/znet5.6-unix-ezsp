// *****************************************************************************
// * sleepy-message-queue-cli.c
// *
// * Routines for managing messages for sleepy end devices.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "sleepy-message-queue.h"


// Functions called in this .cli file that are defined elsewhere.
//extern EmberAfSleepyMessageId emberAfPluginSleepyMessageQueueStoreMessage( EmberAfSleepyMessage *pmsg, int32u timeoutSec );
//extern EmberAfSleepyMessageId emberAfPluginSleepyMessageQueueGetPendingMessageId( EmberEUI64 dstEui64 );
//extern boolean emberAfPluginSleepyMessageQueueGetPendingMessage( EmberAfSleepyMessageId sleepyMsgId, EmberAfSleepyMessage *pmsg );
//extern int8u emberAfPluginSleepyMessageQueueGetNumMessages( EmberEUI64 dstEui64 );

void emAfSleepyMessageQueueCliGetNextMessageEventTimeoutMs( void );
void emAfSleepyMessageQueueCliGetNumUnusedEntries( void );
void emAfSleepyMessageQueueCliStore( void );
void emAfSleepyMessageQueueCliGetPendingMsgId( void );
void emAfSleepyMessageQueueCliGetPendingMsg( void );
void emAfSleepyMessageQueueCliGetNumPendingMsg( void );
void emAfSleepyMessageQueueCliRemoveMsgId( void );
void emAfSleepyMessageQueueCliRemoveAllMsg( void );



void emAfSleepyMessageQueueCliGetNextMessageEventTimeoutMs(){
  int32u timeoutMs;
  int32u msTick;
  int8u msgId;

  msgId = (int8u)emberUnsignedCommandArgument( 0 );
  timeoutMs = emMessageMSecRemaining( msgId );
  msTick = halCommonGetInt32uMillisecondTick();
  //emberAfAppPrintln("==== REM TIME=%d msec, evtTmout=%d, msTick=%d", timeoutMs, msgTimeoutEvent.timeToExecute, msTick );
  emberAfAppPrintln("Remaining Time=%d msec, msTick=%d", timeoutMs, msTick );
}


// plugin sleepy-message-queue unusedCnt
void emAfSleepyMessageQueueCliGetNumUnusedEntries( void ){
  int8u cnt;
  cnt = emberAfPluginSleepyMessageQueueGetNumUnusedEntries();
  emberAfAppPrintln("Unused Message Queue Entries=%d", cnt );
}

// plugin sleepy-message-queue store <timeoutSec:4> <payload*:4> <payloadLength:2> <payloadId:2> <destEui64:8>
void emAfSleepyMessageQueueCliStore( void )
{
  //int8u *payload;
  //int16u length;
  //int16u payloadId;
  //EmberEUI64 dstEui64;
  EmberAfSleepyMessage msg;
  EmberAfSleepyMessageId msgId;
  
  int32u timeoutSec;
  
  timeoutSec = (int32u)emberUnsignedCommandArgument( 0 );
  msg.payload = (int8u *)emberUnsignedCommandArgument( 1 );
  msg.length = (int16u)emberUnsignedCommandArgument( 2 );
  msg.payloadId = (int16u)emberUnsignedCommandArgument( 3 );
  emberCopyStringArgument( 4, msg.dstEui64, EUI64_SIZE, FALSE );
  
  msgId = emberAfPluginSleepyMessageQueueStoreMessage( &msg, timeoutSec );
  if( msgId != EMBER_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID ){
    emberAfAppPrintln("Message Stored, msgID=%d, payloadID=%d", msgId, msg.payloadId );
  }
  else{
    emberAfAppPrintln("ERROR - Message not stored");
  }
}

// plugin sleepy-message-queue getPendingMsgId <destEui64:8>
void emAfSleepyMessageQueueCliGetPendingMsgId( void )
{
  EmberAfSleepyMessageId msgId;
  EmberEUI64 eui64;
  
  emberCopyStringArgument( 0, eui64, EUI64_SIZE, FALSE );
  msgId = emberAfPluginSleepyMessageQueueGetPendingMessageId( eui64 );
  emberAfAppPrintln("Message Pending ID=%d", msgId );
}

// plugin sleepy-message-queue getPendingMsg <msgId:1>
void emAfSleepyMessageQueueCliGetPendingMsg( void )
{
  EmberAfSleepyMessage   msg;
  EmberAfSleepyMessageId msgId;
  boolean found;
  
  msgId = (int8u)emberUnsignedCommandArgument( 0 );
  found = emberAfPluginSleepyMessageQueueGetPendingMessage( msgId, &msg );
  //emberAfAppPrintln("Pending Msg Found=%d, payload=%d, length=%d, ID=%d",
    //    found, (int32u)msg.payload, msg.length, msg.payloadId );
  emberAfAppPrintln("Pending Msg Found=%d", found);
}

// plugin sleepy-message-queue getNumMsg <destEui64:8>
void emAfSleepyMessageQueueCliGetNumPendingMsg( void )
{
  EmberEUI64 eui64;
  int8u cnt;
  
  emberCopyStringArgument( 0, eui64, EUI64_SIZE, FALSE );
  cnt = emberAfPluginSleepyMessageQueueGetNumMessages( eui64 );
  emberAfAppPrintln("Pending Msg Count=%d", cnt );
}

// plugin sleepy-message-queue remove <msgId:1>
void emAfSleepyMessageQueueCliRemoveMsgId( void ){
  EmberAfSleepyMessageId msgId;
  boolean status;

  msgId = (int8u)emberUnsignedCommandArgument( 0 );
  status = emberAfPluginSleepyMessageQueueRemoveMessage( msgId );
  if( status == TRUE ){
    emberAfAppPrintln("Removed Msg ID=%d", msgId );
  }
  else{
    emberAfAppPrintln("ERROR - Msg Remove Failed");
  }
}

// plugin sleepy-message-queue removeAll <destEui64:8>
void emAfSleepyMessageQueueCliRemoveAllMsg( void ){
  EmberEUI64 eui64;
  emberCopyStringArgument( 0, eui64, EUI64_SIZE, FALSE );
  emberAfPluginSleepyMessageQueueRemoveAllMessages( eui64 );
  emberAfAppPrintln( "Removed All Msgs from {%x %x %x %x %x %x %x %x}",
    eui64[0],eui64[1],eui64[2],eui64[3],eui64[4],eui64[5],eui64[6],eui64[7]);
}

void emAfSleepyMessageQueueCliGetCurrentInt32uMillisecondTick(){
  int32u tickMs;

  tickMs = halCommonGetInt32uMillisecondTick();
  emberAfAppPrintln("MS Tick=%d", tickMs );
}


