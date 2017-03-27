/** @file ember-rf4ce-configuration.c
 * @brief User-configurable stack memory allocation.
 *
 *
 * \b Note: Application developers should \b not modify any portion
 * of this file. Doing so may lead to mysterious bugs. Allocations should be
 * adjusted only with macros in a custom CONFIGURATION_HEADER.
 *
 * <!--Copyright 2014 by Silicon Laboratories. All rights reserved.      *80*-->
 */
#include PLATFORM_HEADER
#include "stack/include/ember.h"
#include "stack/include/error.h"
#include "stack/include/ember-static-struct.h" // Required typedefs

#if defined (CORTEXM3)
  #include "hal/micro/micro.h"
#endif

// *****************************************
// Memory Allocations & declarations
// *****************************************

extern int8u emAvailableMemory[];
#ifdef XAP2B
  #define align(value) ((value) + ((value) & 1))
#else
  #define align(value) (value)
#endif

//------------------------------------------------------------------------------
// API Version

const int8u emApiVersion
  = (EMBER_API_MAJOR_VERSION << 4) + EMBER_API_MINOR_VERSION;

//------------------------------------------------------------------------------
// Packet Buffers

int8u emPacketBufferCount = EMBER_PACKET_BUFFER_COUNT;
int8u emPacketBufferFreeCount = EMBER_PACKET_BUFFER_COUNT;

// The actual memory for buffers.
int8u *emPacketBufferData = &emAvailableMemory[0];
#define END_emPacketBufferData          \
  (align(EMBER_PACKET_BUFFER_COUNT * 32))

int8u *emMessageBufferLengths = &emAvailableMemory[END_emPacketBufferData];
#define END_emMessageBufferLengths      \
  (END_emPacketBufferData + align(EMBER_PACKET_BUFFER_COUNT))

int8u *emMessageBufferReferenceCounts = &emAvailableMemory[END_emMessageBufferLengths];
#define END_emMessageBufferReferenceCounts      \
  (END_emMessageBufferLengths + align(EMBER_PACKET_BUFFER_COUNT))

int8u *emPacketBufferLinks = &emAvailableMemory[END_emMessageBufferReferenceCounts];
#define END_emPacketBufferLinks      \
  (END_emMessageBufferReferenceCounts + align(EMBER_PACKET_BUFFER_COUNT))

int8u *emPacketBufferQueueLinks = &emAvailableMemory[END_emPacketBufferLinks];
#define END_emPacketBufferQueueLinks      \
  (END_emPacketBufferLinks + align(EMBER_PACKET_BUFFER_COUNT))

//------------------------------------------------------------------------------
// NWK Retry Queue

EmRetryQueueEntry *emRetryQueue = (EmRetryQueueEntry *) &emAvailableMemory[END_emPacketBufferQueueLinks];
int8u emRetryQueueSize = EMBER_RETRY_QUEUE_SIZE;
#define END_emRetryQueue  \
 (END_emPacketBufferQueueLinks + align(EMBER_RETRY_QUEUE_SIZE * sizeof(EmRetryQueueEntry)))

//------------------------------------------------------------------------------
// RF4CE stack tables

EmberRf4cePairingTableEntry *emRf4cePairingTable = (EmberRf4cePairingTableEntry *) &emAvailableMemory[END_emRetryQueue];
int8u emRf4cePairingTableSize = EMBER_RF4CE_PAIRING_TABLE_SIZE;
#define END_emRf4cePairingTable         \
  (END_emRetryQueue + align(EMBER_RF4CE_PAIRING_TABLE_SIZE * sizeof(EmberRf4cePairingTableEntry)))

EmRf4ceOutgoingPacketInfoEntry *emRf4cePendingOutgoingPacketTable = (EmRf4ceOutgoingPacketInfoEntry *) &emAvailableMemory[END_emRf4cePairingTable];
int8u emRf4cePendingOutgoingPacketTableSize = EMBER_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE;
#define END_emRf4cePendingOutgoingPacketTable     \
  (END_emRf4cePairingTable + align(EMBER_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE * sizeof(EmRf4ceOutgoingPacketInfoEntry)))

//------------------------------------------------------------------------------
// Memory Allocation

#define END_stackMemory  END_emRf4cePendingOutgoingPacketTable

// On the XAP2B platform, emAvailableMemory is allocated automatically to fill
// the available space. On other platforms, we must allocate it here.
#if defined(XAP2B)
  extern int8u emAvailableMemoryTop[];
  const int16u emMinAvailableMemorySize = END_stackMemory;
#elif defined (CORTEXM3)
  VAR_AT_SEGMENT(int8u emAvailableMemory[END_stackMemory], __EMHEAP__);
#elif defined(EMBER_TEST)
  int8u emAvailableMemory[END_stackMemory];
  const int16u emAvailableMemorySize = END_stackMemory;
#else
  #error "Unknown platform."
#endif

// *****************************************
// Non-dynamically configurable structures
// *****************************************
PGM int8u emTaskCount = EMBER_TASK_COUNT;
EmberTaskControl emTasks[EMBER_TASK_COUNT];

// *****************************************
// Stack Generic Parameters
// *****************************************

PGM int8u emberStackProfileId[8] = { 0, };

int8u emDefaultStackProfile = EMBER_STACK_PROFILE;
int8u emDefaultSecurityLevel = EMBER_SECURITY_LEVEL;
int8u emSupportedNetworks = 1;
