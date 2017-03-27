// *******************************************************************
// * gas-proxy-function.h
// *
// *
// * Copyright 2014 by Silicon Laboratories, Inc. All rights reserved.      *80*
// *******************************************************************

#define emberAfPluginGasProxyFunctionPrint(...)                          emberAfAppPrint(__VA_ARGS__)
#define emberAfPluginGasProxyFunctionPrintln(...)                        emberAfAppPrintln(__VA_ARGS__)
#define emberAfPluginGasProxyFunctionDebugExec(x)                        emberAfAppDebugExec(x)
#define emberAfPluginGasProxyFunctionPrintBuffer(buffer, len, withSpace) emberAfAppPrintBuffer(buffer, len, withSpace)
#define emberAfPluginGasProxyFunctionPrintString(buffer)                 emberAfPrintString(EMBER_AF_PRINT_APP, (buffer))

typedef enum {
  CS11_MESSAGE_CODE             = 0x0015,
  GCS01a_MESSAGE_CODE           = 0x006B,
  GCS01b_MESSAGE_CODE           = 0x00A3,
  GCS05_MESSAGE_CODE            = 0x006F,
  GCS07_MESSAGE_CODE            = 0x0071,
  GCS09_MESSAGE_CODE            = 0x0072,
  GCS13a_MESSAGE_CODE           = 0x0074,
  GCS13b_MESSAGE_CODE           = 0x00B8,
  GCS13c_MESSAGE_CODE           = 0x00B6,
  GCS14_MESSAGE_CODE            = 0x0075,
  GCS17_MESSAGE_CODE            = 0x0078,
  GCS21d_MESSAGE_CODE           = 0x009D,
  GCS21e_MESSAGE_CODE           = 0x009E,
  GCS21j_MESSAGE_CODE           = 0x00BF,
  GCS23_MESSAGE_CODE            = 0x007C,
  GCS25_MESSAGE_CODE            = 0x007E,
  GCS33_MESSAGE_CODE            = 0x0082,
  GCS38_MESSAGE_CODE            = 0x0084,
  GCS44_MESSAGE_CODE            = 0x0088,
  GCS46_MESSAGE_CODE            = 0x0089,
  GCS60_MESSAGE_CODE            = 0x008D,
  CS10a_MESSAGE_CODE            = 0x0014,
  CS10b_MESSAGE_CODE            = 0x00A1,
  GCS61_MESSAGE_CODE            = 0x00A0,
  GCS16a_MESSAGE_CODE           = 0x0077,
  GCS16b_MESSAGE_CODE           = 0x0096,
  GCS15b_MESSAGE_CODE           = 0x00C3,
  GCS15c_MESSAGE_CODE           = 0x0076,
  GCS15d_MESSAGE_CODE           = 0x00C4,
  GCS15e_MESSAGE_CODE           = 0x00C5,
  GCS21f_MESSAGE_CODE           = 0x009F,
  GCS21b_MESSAGE_CODE           = 0x00B5,
  GCS53_MESSAGE_CODE            = 0x008B,
  TEST_ENCRYPTED_MESSAGE_CODE   = 0xFFFE,
  TEST_MESSAGE_CODE             = 0xFFFF,
} GBCSUseCaseMessageCode;

EmberStatus emberAfPluginGasProxyFunctionNonTapOffMessageHandler(int8u * gbzCommands,
                                                                 int16u  gbzCommandsLength,
                                                                 int16u  messageCode);

EmberStatus emberAfPluginGasProxyFunctionTapOffMessageHandler(int8u * gbzCommand, 
                                                              int16u  gbzCommandLength,
                                                              int8u * gbzCommandResponse,
                                                              int16u  gbzCommandResponseLength);
typedef enum {
  EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED,
  EMBER_AF_GPF_COMMAND_STATUS_NOT_SUPPORTED,
  EMBER_AF_GPF_COMMAND_STATUS_IGNORED,
} EmberAfGPFCommandStatus;

typedef enum {
  EMBER_AF_GPF_MESSAGE_TYPE_TOM,
  EMBER_AF_GPF_MESSAGE_TYPE_NON_TOM,
} EmberAfGPFMessageType;

int32u emAfGasProxyFunctionGetGbzStartTime(void);
void emAfGasProxyFunctionAlert(int16u alertCode,
                               EmberAfClusterCommand *cmd,
                               int16u messageCode);
