// ****************************************************************************
// * manufacturing-library-cli.c
// *
// * Commands for executing manufacturing related tests
// * 
// * Copyright 2014 by Silicon Labs. All rights reserved.                  *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "stack/include/mfglib.h"

// -----------------------------------------------------------------------------
// Globals

static int8u testPacket[] = { 
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
  0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
};

// The max packet size for 802.15.4 is 128, minus 1 byte for the length, and 2 bytes for the CRC.
#define MAX_BUFFER_SIZE 125

// the saved information for the first packet
static int8u savedPktLength = 0;
static int8s savedRssi = 0;
static int8u savedLinkQuality = 0;
static int8u savedPkt[MAX_BUFFER_SIZE];

static int16u mfgCurrentPacketCounter = 0;

static boolean inReceivedStream = FALSE;

static boolean mfgLibRunning = FALSE;
static boolean mfgToneTestRunning = FALSE;
static boolean mfgStreamTestRunning = FALSE;

static int16u  mfgTotalPacketCounter = 0;


// Add 1 for the length byte which is at the start of the buffer.
#pragma align sendBuff
static int8u   sendBuff[MAX_BUFFER_SIZE + 1];

#define PLUGIN_NAME "Mfglib"

#define MAX_CLI_MESSAGE_SIZE 16

EmberEventControl emberAfPluginManufacturingLibraryCheckSendCompleteEventControl;

static int16u savedPacketCount = 0;

#define CHECK_SEND_COMPLETE_DELAY_QS 2

// -----------------------------------------------------------------------------
// Forward Declarations


// -----------------------------------------------------------------------------

// This is unfortunate but there is no callback indicating when sending is complete
// for all packets.  So we must create a timer that checks whether the packet count
// has increased within the last second.
void emberAfPluginManufacturingLibraryCheckSendCompleteEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginManufacturingLibraryCheckSendCompleteEventControl);
  if (!inReceivedStream) {
    return;
  }

  if (savedPacketCount == mfgTotalPacketCounter) {
    inReceivedStream = FALSE;
    mfgCurrentPacketCounter = 0;
    emberAfCorePrintln("%p Send Complete %d packets",
                       PLUGIN_NAME,
                       mfgCurrentPacketCounter);
    emberAfCorePrintln("First packet: lqi %d, rssi %d, len %d",
                       savedLinkQuality,
                       savedRssi,
                       savedPktLength);
  } else {
    savedPacketCount = mfgTotalPacketCounter;
    emberEventControlSetDelayQS(emberAfPluginManufacturingLibraryCheckSendCompleteEventControl, 
                                CHECK_SEND_COMPLETE_DELAY_QS);
  }
}

static void fillBuffer(int8u* buff, int8u length, boolean random)
{
  int8u i;
  // length byte does not include itself. If the user asks for 10
  // bytes of packet this means 1 byte length, 7 bytes, and 2 bytes CRC
  // this example will have a length byte of 9, but 10 bytes will show
  // up on the receive side
  buff[0] = length;

  if (random) {
    for (i= 1; i < length; i+=2) {
      int16u randomNumber = halCommonGetRandom();
      buff[i] = (int8u)(randomNumber & 0xFF);
      buff[i+1] = (int8u)((randomNumber >> 8)) & 0xFF;
    }
  } else {
    MEMMOVE(buff + 1, testPacket, length);
  }
}

static void mfglibRxHandler(int8u *packet, 
                            int8u linkQuality, 
                            int8s rssi)
{
  // This increments the total packets for the whole mfglib session
  // this starts when mfglibStart is called and stops when mfglibEnd
  // is called.
  mfgTotalPacketCounter++;

  mfgCurrentPacketCounter++;

  // If this is the first packet of a transmit group then save the information
  // of the current packet. Don't do this for every packet, just the first one.
  if (!inReceivedStream) {
    inReceivedStream = TRUE;
    mfgCurrentPacketCounter = 1;
    savedRssi = rssi;
    savedLinkQuality = linkQuality;
    savedPktLength = *packet;
    MEMMOVE(savedPkt, (packet+1), savedPktLength); 
  }
}

void emAfMfglibStartCommand(void)
{
  boolean wantCallback = (boolean)emberUnsignedCommandArgument(0);
  EmberStatus status = mfglibStart(wantCallback ? mfglibRxHandler : NULL);
  emberAfCorePrintln("%p start, status 0x%X",
                     PLUGIN_NAME,
                     status);
  if (status == EMBER_SUCCESS) {
    mfgLibRunning = TRUE;
    mfgTotalPacketCounter = 0;
  }
}

void emAfMfglibStopCommand(void)
{
  EmberStatus status = mfglibEnd();
  emberAfCorePrintln("%p end, status 0x%X", 
                     PLUGIN_NAME,
                     status);
  emberAfCorePrintln("rx %d packets while in mfg mode", mfgTotalPacketCounter);
  if (status == EMBER_SUCCESS) {
    mfgLibRunning = FALSE;
  }
}

void emAfMfglibToneStartCommand(void)
{
  EmberStatus status = mfglibStartTone();
  emberAfCorePrintln("%p start tone 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgToneTestRunning = TRUE;
  }
}

void emAfMfglibToneStopCommand(void)
{
  EmberStatus status = mfglibStopTone();
  emberAfCorePrintln("%p stop tone 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgToneTestRunning = FALSE;
  }
}

void emAfMfglibStreamStartCommand(void)
{
  EmberStatus status = mfglibStartStream();
  emberAfCorePrintln("%p start stream 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgStreamTestRunning = TRUE;
  }
}

void emAfMfglibStreamStopCommand(void)
{
  EmberStatus status = mfglibStopStream();
  emberAfCorePrintln("%p stop stream 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgStreamTestRunning = FALSE;
  }
}

void emAfMfglibSendCommand(void)
{
  boolean random = (emberCommandName()[0] == 'r');
  int16u numPackets = (int16u)emberUnsignedCommandArgument(0);
  int8u length = (int16u)emberUnsignedCommandArgument(1);

  if (length > MAX_BUFFER_SIZE) {
    emberAfCorePrintln("Error: Length cannot be bigger than %d", MAX_BUFFER_SIZE);
    return;
  }

  if (numPackets == 0) {
    emberAfCorePrintln("Error: Number of packets cannot be 0.");
    return;
  }

  fillBuffer(sendBuff, length, random);

  // The second parameter to the mfglibSendPacket() is the 
  // number of "repeats", therefore we decrement numPackets by 1.
  numPackets--;
  EmberStatus status = mfglibSendPacket(sendBuff, numPackets);
  emberAfCorePrintln("%p send packet, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibSendMessageCommand(void)
{
  emberCopyStringArgument(0, sendBuff, MAX_CLI_MESSAGE_SIZE, FALSE);
  int16u numPackets = (int16u)emberUnsignedCommandArgument(0);

  if (numPackets == 0) {
    emberAfCorePrintln("Error: Number of packets cannot be 0.");
    return;
  }

  numPackets--;
  EmberStatus status = mfglibSendPacket(sendBuff, numPackets);
  emberAfCorePrintln("%p send message, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibStatusCommand(void)
{
  int8u channel = mfglibGetChannel();
  int8s power = mfglibGetPower();
  int16u powerMode = emberGetTxPowerMode();
  int8s synOffset = mfglibGetSynOffset();
  emberAfCorePrintln("Channel: %d", channel);
  emberAfCorePrintln("Power: %d", power);
  emberAfCorePrintln("Power Mode: 0x%2X", powerMode);
  emberAfCorePrintln("Syn Offset: %d", synOffset);
  emberAfCorePrintln("%p running: %p", PLUGIN_NAME, (mfgLibRunning ? "yes" : "no"));
  emberAfCorePrintln("%p tone test running: %p", PLUGIN_NAME, (mfgToneTestRunning ? "yes" : "no"));
  emberAfCorePrintln("%p stream test running: %p", PLUGIN_NAME, (mfgStreamTestRunning ? "yes": "no"));
  emberAfCorePrintln("Total %p packets received: %d", PLUGIN_NAME, mfgTotalPacketCounter);
}

void emAfMfglibSetChannelCommand(void)
{
  int8u channel = (int8u)emberUnsignedCommandArgument(0);
  EmberStatus status = mfglibSetChannel(channel);
  emberAfCorePrintln("%p set channel, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibSetPowerAndModeCommand(void)
{
  int8s power = (int8s)emberSignedCommandArgument(0);
  int16u mode = (int16u)emberUnsignedCommandArgument(1);
  EmberStatus status = mfglibSetPower(mode, power);
  emberAfCorePrintln("%p set power and mode, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibTestModCalCommand(void)
{
  int8u channel = (int8u)emberUnsignedCommandArgument(0);
  int32u durationMs = emberUnsignedCommandArgument(1);
  if (durationMs == 0) {
    emberAfCorePrintln("Performing continuous Mod DAC Calibation.  Reset part to stop.");
  } else {
    emberAfCorePrintln("Mod DAC Calibration running for %u ms", durationMs);
  }
  mfglibTestContModCal(channel, durationMs);
}

void emAfMfglibSynoffsetCommand(void)
{
  int8s synOffset = (int8s)emberSignedCommandArgument(0);
  boolean toneTestWasRunning = mfgToneTestRunning;
  if (toneTestWasRunning) {
    emAfMfglibToneStopCommand();
  }

  mfglibSetSynOffset(synOffset);

  if (toneTestWasRunning) {
    emAfMfglibToneStartCommand();
  }
}
