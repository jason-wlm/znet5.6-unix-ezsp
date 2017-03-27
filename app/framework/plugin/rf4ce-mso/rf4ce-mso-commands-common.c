// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

#ifdef EMBER_SCRIPTED_TEST
#include "rf4ce-mso-test.h"
#endif // EMBER_SCRIPTED_TEST

#if defined(EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR) && defined(EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT)
  #error The RF4CE MSO plugin operates as either an originator or a recipient
#endif

static PGM int8u minimumMessageLengths[] = {
  EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_RESPONSE, CHECK_VALIDATION_RESPONSE_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_RESPONSE,    SET_ATTRIBUTE_RESPONSE_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_RESPONSE,    GET_ATTRIBUTE_RESPONSE_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED,      USER_CONTROL_PRESSED_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED,     USER_CONTROL_REPEATED_1_0_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED,     USER_CONTROL_RELEASED_1_0_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_REQUEST,  CHECK_VALIDATION_REQUEST_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_REQUEST,     SET_ATTRIBUTE_REQUEST_LENGTH,
  EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_REQUEST,     GET_ATTRIBUTE_REQUEST_LENGTH,
};

int8u emAfRf4ceMsoBuffer[EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_PAYLOAD_LENGTH];
int8u emAfRf4ceMsoBufferLength;

EmberEventControl emberAfPluginRf4ceMsoUserControlEventControl;
EmberEventControl emberAfPluginRf4ceMsoCheckValidationEventControl;
EmberEventControl emberAfPluginRf4ceMsoSetGetAttributeEventControl;

static boolean checkCommand(int8u pairingIndex,
                            int16u vendorId,
                            const int8u *message,
                            int8u messageLength,
                            boolean rx,
                            EmberAfRf4ceMsoCommandCode *commandCode);
static boolean checkCommandCodeAndLength(EmberAfRf4ceMsoCommandCode commandCode,
                                         int8u messageLength,
                                         boolean rx);

EmberStatus emAfRf4ceMsoSend(int8u pairingIndex)
{
  if (emAfRf4ceMsoIsBlackedOut(pairingIndex)) {
    return EMBER_INVALID_CALL;
  }
  return emberAfRf4ceSendVendorSpecific(pairingIndex,
                                        EMBER_AF_RF4CE_PROFILE_MSO,
                                        emberAfRf4ceVendorId(),
                                        emAfRf4ceMsoBuffer,
                                        emAfRf4ceMsoBufferLength,
                                        0); // message tag - unused
}

EmberStatus emAfRf4ceMsoSendExtended(int8u pairingIndex,
                                     EmberRf4ceTxOption txOptions)
{
  if (emAfRf4ceMsoIsBlackedOut(pairingIndex)) {
    return EMBER_INVALID_CALL;
  }
  return emberAfRf4ceSendExtended(pairingIndex,
                                  EMBER_AF_RF4CE_PROFILE_MSO,
                                  emberAfRf4ceVendorId(),
                                  (txOptions
                                   | EMBER_RF4CE_TX_OPTIONS_VENDOR_SPECIFIC_BIT),
                                  emAfRf4ceMsoBuffer,
                                  emAfRf4ceMsoBufferLength,
                                  0); // message tag - unused
}

void emberAfPluginRf4ceProfileMsoMessageSentCallback(int8u pairingIndex,
                                                     int16u vendorId,
                                                     int8u messageTag,
                                                     const int8u *message,
                                                     int8u messageLength,
                                                     EmberStatus status)
{
  EmberAfRf4ceMsoCommandCode commandCode;
  if (checkCommand(pairingIndex,
                   vendorId,
                   message,
                   messageLength,
                   FALSE, // TX
                   &commandCode)) {
    emAfRf4ceMsoMessageSent(pairingIndex,
                            commandCode,
                            message,
                            messageLength,
                            status);
  }
}

void emberAfPluginRf4ceProfileMsoIncomingMessageCallback(int8u pairingIndex,
                                                         int16u vendorId,
                                                         EmberRf4ceTxOption txOptions,
                                                         const int8u *message,
                                                         int8u messageLength)
{
  EmberAfRf4ceMsoCommandCode commandCode;
  if (checkCommand(pairingIndex,
                   vendorId,
                   message,
                   messageLength,
                   TRUE, // RX
                   &commandCode)) {
    emAfRf4ceMsoIncomingMessage(pairingIndex,
                                commandCode,
                                message,
                                messageLength);
  }
}

static boolean checkCommand(int8u pairingIndex,
                            int16u vendorId,
                            const int8u *message,
                            int8u messageLength,
                            boolean rx,
                            EmberAfRf4ceMsoCommandCode *commandCode)
{
  // MSO is a vendor-specific profile so the vendor id must match.
  if (emberAfRf4ceVendorId() != vendorId) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "MSO",
                      (rx ? 'R' : 'T'),
                      "Invalid",
                      "vendor id");
    emberAfDebugPrintln(": 0x%2x", vendorId);
    return FALSE;
  }

  // Every MSO command has an MSO header.
  if (messageLength < MSO_HEADER_LENGTH) {
    emberAfDebugPrintln("%p: %cX: %p %p",
                        "MSO",
                        (rx ? 'R' : 'T'),
                        "Invalid",
                        "message");
    return FALSE;
  }

  if (emAfRf4ceMsoIsBlackedOut(pairingIndex)) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "MSO",
                      (rx ? 'R' : 'T'),
                      "Blacked-out",
                      "sender");
    emberAfDebugPrintln(": 0x%x", pairingIndex);
    return FALSE;
  }

  // The MSO header contains the frame control field, which contains the
  // command code.
  *commandCode = (message[MSO_HEADER_FRAME_CONTROL_OFFSET]
                  & MSO_HEADER_FRAME_CONTROL_COMMAND_CODE_MASK);
  return checkCommandCodeAndLength(*commandCode, messageLength, rx);
}

static boolean checkCommandCodeAndLength(EmberAfRf4ceMsoCommandCode commandCode,
                                         int8u messageLength,
                                         boolean rx)
{
  int8u i;
  for (i = 0; i < COUNTOF(minimumMessageLengths); i += 2) {
    if (commandCode == minimumMessageLengths[i]) {
      if (minimumMessageLengths[i + 1] <= messageLength) {
        return TRUE;
      } else {
        emberAfDebugPrint("%p: %cX: %p %p",
                          "MSO",
                          (rx ? 'R' : 'T'),
                          "Invalid",
                          "command length");
        emberAfDebugPrintln(": %d", messageLength);
        return FALSE;
      }
    }
  }
  emberAfDebugPrint("%p: %cX: %p %p",
                    "MSO",
                    (rx ? 'R' : 'T'),
                    "Invalid",
                    "command code");
  emberAfDebugPrintln(": 0x%x", commandCode);
  return FALSE;
}
