// *****************************************************************************
// * sleepy-message-queue.c
// *
// * Routines for managing messages for sleepy end devices.
// *
// * Copyright 2014 by Silicon Laboratories, Inc.
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "sleepy-message-queue.h"
#include "stack/include/event.h"

//------------------------------------------------------------------------------
// Forward Declaration

EmberEventControl emberAfPluginSleepyMessageQueueTimeoutEventControl;

// Use a shorter name to make the code more readable
#define msgTimeoutEvent emberAfPluginSleepyMessageQueueTimeoutEventControl


//------------------------------------------------------------------------------
// Internal Prototypes & Structure Definitions

// Internal supporting functions
static void  emSleepyMessageQueueInitEntry( int8u index );
static void  emRestartMessageTimer( void );
static int8u emGetFirstUnusedQueueIndex( void );

//static void emRefreshMessageTimeouts( void );
static int32u emGetElapsedMillisecondsSinceLastEvent( void );


typedef struct
{
  EmberAfSleepyMessage sleepyMsg;
  int8u  status;
  int32u timeoutMSec;
} emSleepyMessage;

#define SLEEPY_MSG_QUEUE_NUM_ENTRIES  EMBER_AF_PLUGIN_SLEEPY_MESSAGE_QUEUE_SLEEPY_QUEUE_SIZE
#define MAX_MESSAGE_TIMEOUT_SEC  (0x7fffffff >> 10)


emSleepyMessage SleepyMessageQueue[ SLEEPY_MSG_QUEUE_NUM_ENTRIES ];

enum
{
  SLEEPY_MSG_QUEUE_STATUS_UNUSED = 0x00,
  SLEEPY_MSG_QUEUE_STATUS_USED   = 0x01,
};

//------------------------------------------------------------------------------

void emberAfPluginSleepyMessageQueueInitCallback()
{
  // Initialize sleepy buffer plugin.
  int8u x;
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ )
  {
    emSleepyMessageQueueInitEntry( x );
  }
  emberAfAppPrintln( "Initialized Sleepy Message Queue" );
}

static void emSleepyMessageQueueInitEntry( int8u x )
{
  if( x < SLEEPY_MSG_QUEUE_NUM_ENTRIES )
  {
    SleepyMessageQueue[x].sleepyMsg.payload = NULL;
    SleepyMessageQueue[x].sleepyMsg.length = 0;
    SleepyMessageQueue[x].sleepyMsg.payloadId = 0;
    MEMSET( SleepyMessageQueue[x].sleepyMsg.dstEui64, 0, EUI64_SIZE );
    SleepyMessageQueue[x].status = SLEEPY_MSG_QUEUE_STATUS_UNUSED;
    SleepyMessageQueue[x].timeoutMSec = 0;
  }
}

int8u emberAfPluginSleepyMessageQueueGetNumUnusedEntries(){
  int8u x;
  int8u cnt = 0;
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ){
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_UNUSED ){
      cnt++;
    }
  }
  return cnt;
}


static int8u emGetFirstUnusedQueueIndex()
{
  int8u x;
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ )
  {
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_UNUSED )
    {
      break;
    }
  }
  if( x >= SLEEPY_MSG_QUEUE_NUM_ENTRIES ){
    x = EMBER_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID;
  }
  return x;
}

#define MAX_DELAY_QS  0x7FFF
#define MAX_DELAY_MS  0x7FFF
static void emRestartMessageTimer()
{
  int32u smallestTimeoutIndex = SLEEPY_MSG_QUEUE_NUM_ENTRIES;
  int32u smallestTimeoutMSec = 0xFFFFFFFF;
  int32u timeNowMs;
  int32u remainingMs;
  int32u delayQs;
  int8u x;

  timeNowMs = halCommonGetInt32uMillisecondTick();
   
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ )
  {
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED )
    {
      if( timeGTorEqualInt32u( timeNowMs, SleepyMessageQueue[x].timeoutMSec ) ){
        // Timeout already expired - break out of loop - process immediately.
        smallestTimeoutIndex = x;
        smallestTimeoutMSec = 0;
        break;
      }
      else{
        remainingMs = elapsedTimeInt32u( timeNowMs, SleepyMessageQueue[x].timeoutMSec );
        if( remainingMs < smallestTimeoutMSec ){
          smallestTimeoutMSec = remainingMs;
          smallestTimeoutIndex = x;
        }
      }
    }
  }
  // Now know the smallest timeout index, and the smallest timeout value.
  if( smallestTimeoutIndex < SLEEPY_MSG_QUEUE_NUM_ENTRIES )
  {
    // Run the actual timer as a QS timer since that allows us to delay longer
    // than a u16 MS timer would.
    delayQs = smallestTimeoutMSec >> 8;
    delayQs++;    // Round up to the next quarter second tick.
    if( delayQs > MAX_DELAY_QS ){
      delayQs = MAX_DELAY_QS;
    }

    emberEventControlSetDelayQS( msgTimeoutEvent, delayQs );
    emberAfAppPrintln("Restarting sleepy message timer for %d Qsec.", delayQs );
  }
}
  
EmberAfSleepyMessageId emberAfPluginSleepyMessageQueueStoreMessage( EmberAfSleepyMessage *pmsg, int32u timeoutSec )
{
  int8u x;
 
  if( timeoutSec <= MAX_MESSAGE_TIMEOUT_SEC ){
    x = emGetFirstUnusedQueueIndex();
    if( x < SLEEPY_MSG_QUEUE_NUM_ENTRIES )
    {
      MEMCOPY( (int8u *)&SleepyMessageQueue[x].sleepyMsg, (int8u *)pmsg, sizeof(EmberAfSleepyMessage) );
      SleepyMessageQueue[x].status = SLEEPY_MSG_QUEUE_STATUS_USED;
      SleepyMessageQueue[x].timeoutMSec = (timeoutSec << 10) + halCommonGetInt32uMillisecondTick();
      
      // Reschedule timer after adding the new message since new message could have the shortest timeout.
      emRestartMessageTimer();
    }
    return x;
  }
  return EMBER_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID;
}

EmberAfSleepyMessageId emberAfPluginSleepyMessageQueueGetPendingMessageId( EmberEUI64 dstEui64 )
{
  int8u  smallestTimeoutIndex = SLEEPY_MSG_QUEUE_NUM_ENTRIES;
  int32u smallestTimeoutMSec = 0xFFFFFFFF;
  int32u timeNowMs;
  int32u remainingMs;
  int8u  x;
  int8u  stat;
  
  timeNowMs = halCommonGetInt32uMillisecondTick();
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ )
  {
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED )
    {
      stat = MEMCOMPARE( SleepyMessageQueue[x].sleepyMsg.dstEui64, dstEui64, EUI64_SIZE );
      if( !stat )
      {
        // Matching entry found - look for match with smallest timeout.
        if( timeGTorEqualInt32u( timeNowMs, SleepyMessageQueue[x].timeoutMSec ) ){
          // Timeout already expired - break out of loop - process immediately.
          smallestTimeoutIndex = x;
          smallestTimeoutMSec = 0;
          break;
        }
        else{
          remainingMs = elapsedTimeInt32u( timeNowMs, SleepyMessageQueue[x].timeoutMSec );
          if( remainingMs < smallestTimeoutMSec ){
            smallestTimeoutMSec = remainingMs;
            smallestTimeoutIndex = x;
          }
        }
      }
    }
  }
  if( smallestTimeoutIndex >= SLEEPY_MSG_QUEUE_NUM_ENTRIES )
  {
    smallestTimeoutIndex = EMBER_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID;
  }
  return smallestTimeoutIndex;
}

int32u emMessageMSecRemaining( EmberAfSleepyMessageId sleepyMsgId ){
  int32u remainingMs = 0xFFFFFFFF;
  int32u timeNowMs = halCommonGetInt32uMillisecondTick();

  if( (sleepyMsgId < SLEEPY_MSG_QUEUE_NUM_ENTRIES) && (SleepyMessageQueue[sleepyMsgId].status == SLEEPY_MSG_QUEUE_STATUS_USED) ){
    if( timeGTorEqualInt32u(timeNowMs, SleepyMessageQueue[sleepyMsgId].timeoutMSec) ){
      remainingMs = 0;
    }
    else{
      remainingMs = elapsedTimeInt32u( timeNowMs, SleepyMessageQueue[sleepyMsgId].timeoutMSec );
    }
  }
  return remainingMs;
}

#define PRINT_TIME(x)  PrintMessageTimeInfo(x)
void PrintMessageTimeInfo( int8u x ){
  if( (x < SLEEPY_MSG_QUEUE_NUM_ENTRIES) && (SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED) ){
    emberAfAppPrintln("==== USED MESSAGE, x=%d", x );
    emberAfAppPrintln("  currTime=%d, timeout=%d", halCommonGetInt32uMillisecondTick(), SleepyMessageQueue[x].timeoutMSec );
  }
}

boolean emberAfPluginSleepyMessageQueueGetPendingMessage( EmberAfSleepyMessageId sleepyMsgId, EmberAfSleepyMessage *pmsg )
{
  if( (sleepyMsgId < SLEEPY_MSG_QUEUE_NUM_ENTRIES) && (SleepyMessageQueue[sleepyMsgId].status == SLEEPY_MSG_QUEUE_STATUS_USED) )
  {
    MEMCOPY( (int8u *)pmsg, (int8u *)&SleepyMessageQueue[sleepyMsgId].sleepyMsg, sizeof(EmberAfSleepyMessage) );
    return TRUE;
  }
  return FALSE;
}

int8u emberAfPluginSleepyMessageQueueGetNumMessages( EmberEUI64 dstEui64 )
{
  int8u x;
  int8u stat;
  int8u matchCnt = 0;
  
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ )
  {
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED )
    {
      stat = MEMCOMPARE( SleepyMessageQueue[x].sleepyMsg.dstEui64, dstEui64, EUI64_SIZE );
      if( !stat )
      {
        matchCnt++;
      }
    }
  }
  return matchCnt;
}

boolean emberAfPluginSleepyMessageQueueRemoveMessage(  EmberAfSleepyMessageId  sleepyMsgId  )
{
  if( sleepyMsgId < SLEEPY_MSG_QUEUE_NUM_ENTRIES )
  {
    emSleepyMessageQueueInitEntry( sleepyMsgId );
    emRestartMessageTimer();    // Restart the message timer
    return TRUE;
  }
  return FALSE;
}

void emberAfPluginSleepyMessageQueueRemoveAllMessages(  EmberEUI64 dstEui64  )
{
  int8u x;
  int8u stat;
  int8u matchCnt = 0;
  
  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ )
  {
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED )
    {
      stat = MEMCOMPARE( SleepyMessageQueue[x].sleepyMsg.dstEui64, dstEui64, EUI64_SIZE );
      if( !stat )
      {
        emSleepyMessageQueueInitEntry( x );
        matchCnt++;
      }
    }
  }
  if( matchCnt ){
    // At least one entry was removed.  Restart the message timer.
    emRestartMessageTimer();
  }
}


void emberAfPluginSleepyMessageQueueTimeoutEventHandler(void)
{
  int32u timeNowMs;
  int8u x;
  
  emberEventControlSetInactive( msgTimeoutEvent );
  timeNowMs = halCommonGetInt32uMillisecondTick();

  for( x=0; x<SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ){
    if( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED ){
      if( timeGTorEqualInt32u( timeNowMs, SleepyMessageQueue[x].timeoutMSec ) ){
        // Found entry that expired.
        // Invoke callback to notify about message timeout.
        // Allow the callback the opportunity to access elements in this entry.
        emberAfAppPrintln("Expiring Message %d", x );
        emberAfPluginSleepyMessageQueueMessageTimedOutCallback( x );
        
        // After timeout callback completes, mark event as unused.
        emSleepyMessageQueueInitEntry( x );
      }
    }
  }
  
  // Restart timer if another message is buffered.
  emRestartMessageTimer();
}


