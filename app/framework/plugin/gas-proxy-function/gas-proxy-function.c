// *******************************************************************
// * gas-proxy-function.c
// *
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/gbz-message-controller/gbz-message-controller.h"
#include "app/framework/plugin/gas-proxy-function/gas-proxy-function.h"
#include "app/framework/plugin/price-server/price-server.h"
#include "app/framework/plugin/messaging-server/messaging-server.h"
#include "app/framework/plugin/calendar-common/calendar-common.h"
#include "app/framework/plugin/device-management-server/device-management-server.h"
#include "app/framework/plugin/meter-mirror/meter-mirror.h"
#include "gpf-structured-data.h"
#include "app/framework/plugin/events-server/events-server.h"

#define fieldLength(field) \
    (emberAfCurrentCommand()->bufLen - (field - emberAfCurrentCommand()->buffer));

// default configurations
#define DEFAULT_TABLE_SET_INDEX (0)
static boolean nonTomHandlingActive = FALSE;
static EmberAfGbzMessageParserState nonTomGbzRequestParser;
static EmberAfGbzMessageCreatorState nonTomGbzResponseCreator;
static int8u nonTomExpectedZclResps = 0;  // per command
static int8u nonTomResponsesCount = 0;    // per command
static int32u nonTomGbzStartTime = 0;

void hideEndpoint(int8u endpoint){
#ifdef EZSP_HOST
  EzspStatus status = ezspSetEndpointFlags(endpoint,
                                           EZSP_ENDPOINT_DISABLED);
  emberAfPluginGasProxyFunctionPrintln("GPF: hiding endpoint status: 0x%X", status);
#else
  // we need to standarize disabling/hiding/reset endpoints.
  emberAfPluginGasProxyFunctionPrintln("GPF: hiding endpoint not supported on Soc!");
#endif
}

void emberAfPluginGasProxyFunctionInitCallback(int8u endpoint) {
  emberAfPluginGasProxyFunctionInitStructuredData();
  // try to hide hidden endpoint from service discovery.
  hideEndpoint(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT);
}

/*  @brief  Log all GPF commands to GPF Event Log.
 *
 *  In GBCS v0.81, the GPF shall be capable of logging all Commands
 *  received and Outcomes in the GPF Event Log(4.6.3.8). If the message is
 *  too long, the data will be truncated.
 *  */
void emAfPluginGasProxyFunctionLogEvent(int8u * gbzCmd,
                                        int16u  gbzCmdLen,
                                        int8u * gbzResp,
                                        int16u  gbzRespLen, 
                                        int16u eventId,
                                        EmberAfGPFMessageType cmdType){
  EmberAfEvent event;
  int8u * logMsg = event.eventData;
  int8u logMsgLen = EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH + 1 ;
  char * tomHeader = "GPF TOM: ";
  int8u tomHeaderLen = 9;
  char * nonTomHeader = "GPF Non-TOM: ";
  int8u nonTomHeaderLen = 13;
  char * cmdHeader = "Cmd: ";
  int8u cmdHeaderLen = 5;
  char * respHeader = " Resp: ";
  int8u respHeaderLen = 7;

  logMsg[0] = 0x00;

  if (cmdType == EMBER_AF_GPF_MESSAGE_TYPE_TOM){
    emberAfAppendCharacters(logMsg,
                            EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH,
                            (int8u *)tomHeader,
                            tomHeaderLen);
  } else {
    emberAfAppendCharacters(logMsg,
                            EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH,
                            (int8u *)nonTomHeader,
                            nonTomHeaderLen);
  }

  if ((gbzCmd != NULL) && (gbzCmdLen > 0)){
    emberAfAppendCharacters(logMsg,
                            EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH,
                            (int8u *)cmdHeader,
                            cmdHeaderLen);
    emberAfAppendCharacters(logMsg,
                            EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH,
                            (int8u *)gbzCmd,
                            gbzCmdLen);
  }

  if ((gbzResp != NULL) && (gbzRespLen > 0)){
    emberAfAppendCharacters(logMsg,
                            EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH,
                            (int8u *)respHeader,
                            respHeaderLen);
    emberAfAppendCharacters(logMsg,
                            EMBER_AF_PLUGIN_EVENTS_SERVER_EVENT_DATA_LENGTH,
                            (int8u *)gbzResp,
                            gbzRespLen);
  }

  event.eventId = eventId;
  event.eventTime = emberAfGetCurrentTime();
  emberAfEventsServerAddEvent(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                              EMBER_ZCL_EVENT_LOG_ID_GENERAL_EVENT_LOG, 
                              &event);
}

static EmberAfGPFCommandStatus checkTomCommandSupport(EmberAfClusterCommand * cmd)
{
  if (cmd->direction == ZCL_DIRECTION_SERVER_TO_CLIENT){
    if (cmd->apsFrame->clusterId == ZCL_CALENDAR_CLUSTER_ID){
      if ((cmd->commandId == ZCL_PUBLISH_CALENDAR_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_DAY_PROFILE_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_WEEK_PROFILE_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_SEASONS_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_SPECIAL_DAYS_COMMAND_ID)){
        return EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED;
      }
    } else if (cmd->apsFrame->clusterId == ZCL_PRICE_CLUSTER_ID){
      if ((cmd->commandId == ZCL_PUBLISH_TARIFF_INFORMATION_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_BLOCK_THRESHOLDS_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_PRICE_MATRIX_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_CONVERSION_FACTOR_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_CALORIFIC_VALUE_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_BILLING_PERIOD_COMMAND_ID)
          || (cmd->commandId == ZCL_PUBLISH_BLOCK_PERIOD_COMMAND_ID)){
        return EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED;
      }
    } else if (cmd->apsFrame->clusterId == ZCL_MESSAGING_CLUSTER_ID){
      if (cmd->commandId == ZCL_DISPLAY_MESSAGE_COMMAND_ID){
        return EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED;
      }
    } else if (cmd->apsFrame->clusterId == ZCL_DEVICE_MANAGEMENT_CLUSTER_ID){
      if (cmd->commandId == ZCL_PUBLISH_CHANGE_OF_SUPPLIER_COMMAND_ID){
        return EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED;
      }
    }
  } else { // client to server
    if (cmd->apsFrame->clusterId == ZCL_EVENTS_CLUSTER_ID){
      if (cmd->commandId == ZCL_CLEAR_EVENT_LOG_REQUEST_COMMAND_ID){
        return EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED;
      }
    } else if (cmd->apsFrame->clusterId == ZCL_PREPAYMENT_CLUSTER_ID){
      if ((cmd->commandId == ZCL_EMERGENCY_CREDIT_SETUP_COMMAND_ID)
          || (cmd->commandId == ZCL_SET_OVERALL_DEBT_CAP_COMMAND_ID)
          || (cmd->commandId == ZCL_SET_LOW_CREDIT_WARNING_LEVEL_COMMAND_ID)
          || (cmd->commandId == ZCL_SET_MAXIMUM_CREDIT_LIMIT_COMMAND_ID)){
        return EMBER_AF_GPF_COMMAND_STATUS_IGNORED;
      }
    }
  }
 
  return EMBER_AF_GPF_COMMAND_STATUS_NOT_SUPPORTED;
}

static EmberStatus emberAfPluginGasProxyFunctionReplayZclCommand(int8u srcEndpoint,
                                                                 int8u dstEndpoint,
                                                                 EmberAfGbzZclCommand * cmd)
{
  EmberNodeId dstId = emberGetNodeId();
  EmberStatus status;

  emberAfPluginGasProxyFunctionPrintln("GPF: Replaying following command from endpoint(%d) to (%d)", srcEndpoint, dstEndpoint);
  emberAfPluginGbzMessageControllerPrintCommandInfo(cmd);

  if (dstEndpoint == EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MIRROR_ENDPOINT
      && !emberAfIsDeviceEnabled(dstEndpoint)) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Cannot replay command. Mirror is not available.");
    emberAfFillExternalBuffer(((cmd->direction == ZCL_DIRECTION_SERVER_TO_CLIENT) ?
                               ZCL_FRAME_CONTROL_CLIENT_TO_SERVER : ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                              cmd->clusterId, \
                              ZCL_DEFAULT_RESPONSE_COMMAND_ID, \
                              "uu", \
                              cmd->commandId, \
                              EMBER_ZCL_STATUS_FAILURE);
    emberAfSetCommandEndpoints(srcEndpoint, srcEndpoint);
  } else {
    emberAfFillExternalBuffer(cmd->frameControl, \
                              cmd->clusterId, \
                              cmd->commandId, \
                              "");
    emberAfAppendToExternalBuffer(cmd->payload, cmd->payloadLength);
    emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  }

  appResponseData[1] = cmd->transactionSequenceNumber;

  // We assume that one response will be sent per command.  If more than one
  // response is to be received the expected response count will be updated.
  // Refer to emberAfPluginCalendarServerPublishInfoCallback.
  nonTomExpectedZclResps = 1;
  nonTomResponsesCount = 0;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstId);
  return status;
}

/* @brief check if a GBZ ZCL command matches the corresponding command
 * response
 *
 * return TRUE if pair matches. Otherwise, FALSE.
 * */
static boolean matchingZclCommands(EmberAfGbzZclCommand * cmd, EmberAfGbzZclCommand  * cmdResp) 
{
  boolean result = TRUE;

  // check command direction
  if ((cmd->frameControl & ZCL_FRAME_CONTROL_DIRECTION_MASK) == (cmdResp->frameControl & ZCL_FRAME_CONTROL_DIRECTION_MASK)){
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: No matching ZCL command direction (0x%X/0x%X) in ZCL cmd/cmd resp pair!",
        cmd->frameControl & ZCL_FRAME_CONTROL_DIRECTION_MASK,
        cmdResp->frameControl & ZCL_FRAME_CONTROL_DIRECTION_MASK);
    result = FALSE;
  }

  if (cmd->transactionSequenceNumber != cmdResp->transactionSequenceNumber) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: No matching ZCL tran seq number (0x%X/0x%X) in ZCL cmd/cmd resp pair!", cmd->transactionSequenceNumber, cmdResp->transactionSequenceNumber);
    result = FALSE;
  }

  if (cmd->clusterId != cmdResp->clusterId) {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: No matching ZCL cluster id (%2X/%2X) in ZCL cmd/cmd resp pair!", cmd->clusterId, cmdResp->clusterId);
    result = FALSE;
  }

  // the command id doesn't always matches in value (e.g. read attr(0x00) vs read attr resp (0x01)).
  // this is not being checked for now.
  /*if (cmd->commandId != cmdResp->commandId) {*/
    /*emberAfPluginGasProxyFunctionPrintln("GPF: ERR: No matching ZCL command id (%x/%x) in ZCL cmd/cmd resp pair!", cmd->commandId, cmdResp->commandId);*/
    /*result = FALSE;*/
  /*}*/

  return result;
}

/*
 * @brief Send a Tap Off Message (TOM) to the GPF 
 *
 * This function can be used to send a Tap Off Message (TOM) to the Gas 
 * Proxy Function (GPF) for processing. The message has been tapped off
 * and validated by the gas meter (GSME), so that now it can be applied to the GPF.
 *
 * @param gbzCommands  a pointer to the complete GBZ command data.
 * @param gbzCommandsLength the length of the GBZ command.
 * @param gbzCommandsResponse a pointer to all GBZ responses generated by the GSME.
 * @param gbzCommandsResponseLength the length of the GBZ responses.
 *
 * @return EMBER_SUCCESS if all messages have been successfully parsed, a non-zero
 *   status code on error.
 */
EmberStatus emberAfPluginGasProxyFunctionTapOffMessageHandler(int8u * gbzCommands,
                                                              int16u  gbzCommandsLength,
                                                              int8u * gbzCommandsResponse,
                                                              int16u  gbzCommandsResponseLength)
{
  EmberAfGbzMessageParserState gbzCommandHandler;
  EmberAfGbzMessageParserState gbzCommandResponseHandler;
  EmberStatus status;

  emberAfPluginGbzMessageControllerParserInit(&gbzCommandHandler,
                                              EMBER_AF_GBZ_MESSAGE_COMMAND,
                                              gbzCommands,
                                              gbzCommandsLength,
                                              FALSE,
                                              0);
  emberAfPluginGbzMessageControllerParserInit(&gbzCommandResponseHandler,
                                              EMBER_AF_GBZ_MESSAGE_RESPONSE,
                                              gbzCommandsResponse,
                                              gbzCommandsResponseLength,
                                              FALSE,
                                              0);
  
  if (emberAfPluginGbzMessageControllerGetComponentSize(&gbzCommandHandler) != 
      emberAfPluginGbzMessageControllerGetComponentSize(&gbzCommandResponseHandler)){
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: GBZ command / command response does not have same number of components!");
    status = EMBER_BAD_ARGUMENT;
    goto kickout;
  }

  while(emberAfPluginGbzMessageControllerHasNextCommand(&gbzCommandHandler)){
    EmberAfGbzZclCommand cmd;
    EmberAfGbzZclCommand cmdResp;
    emberAfPluginGbzMessageControllerNextCommand(&gbzCommandHandler, &cmd);
    emberAfPluginGbzMessageControllerNextCommand(&gbzCommandResponseHandler, &cmdResp);
    
    if (matchingZclCommands(&cmd, &cmdResp)
        && emberAfPluginGbzMessageControllerGetZclDefaultResponse(&cmdResp) == EMBER_ZCL_STATUS_SUCCESS) {
      EmberApsFrame apsFrame;
      EmberAfClusterCommand clusterCmd;
      EmberAfGPFCommandStatus cmdStatus;
      MEMSET(&apsFrame, 0x00, sizeof(EmberApsFrame));
      MEMSET(&clusterCmd, 0x00, sizeof(EmberAfClusterCommand));
      emberAfPluginGasProxyFunctionPrintln("GPF: Updating ESI endpoint with following info: ");
      emberAfPluginGbzMessageControllerPrintCommandInfo(&cmd);

      apsFrame.profileId = gbzCommandHandler.profileId;
      apsFrame.clusterId = cmd.clusterId;
      clusterCmd.commandId = cmd.commandId;

      // we mangle the zcl dst ep to a proxy endpoint to intercept the server to client side
      // commands. however, certain commands (Events: ClearEventLogRequest)
      // doesn't need to be redirected to the proxy ep thanks to the correct
      // zcl direction.
      if ((cmd.clusterId == ZCL_EVENTS_CLUSTER_ID)
          && (cmd.commandId == ZCL_CLEAR_EVENT_LOG_REQUEST_COMMAND_ID)){
        // this TOM cmd is directed at the GSME 
        apsFrame.destinationEndpoint = (EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MIRROR_ENDPOINT);
      } else {
        apsFrame.destinationEndpoint = (EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT);
      }

      clusterCmd.apsFrame = &apsFrame;
      clusterCmd.seqNum = cmd.transactionSequenceNumber;
      clusterCmd.clusterSpecific = cmd.clusterSpecific;
      clusterCmd.mfgSpecific = cmd.mfgSpecific;
      clusterCmd.direction = cmd.direction;
      clusterCmd.buffer = cmd.payload;
      clusterCmd.bufLen = cmd.payloadLength;
      clusterCmd.payloadStartIndex = 0;
  
      status = checkTomCommandSupport(&clusterCmd);
      if (status == EMBER_AF_GPF_COMMAND_STATUS_SUPPORTED){
        EmberAfStatus parsingStatus;
        emberAfCurrentCommand() = &clusterCmd;
        emberAfPluginGasProxyFunctionPrintln("GPF: Passing cmd to cluster parser");
        parsingStatus = emberAfClusterSpecificCommandParse(&clusterCmd);
        if (parsingStatus != EMBER_ZCL_STATUS_SUCCESS){
          emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Unable to apply ZCL command (error: 0x%X)!", parsingStatus);
          status = EMBER_ERR_FATAL;
          goto kickout;
        }
      } else if (status == EMBER_AF_GPF_COMMAND_STATUS_IGNORED) {
        emberAfPluginGasProxyFunctionPrintln("GPF: Info: Command ignored: ZCL command(clus 0x%2X, cmd 0x%x) embedded within Tap Off Message.", clusterCmd.apsFrame->clusterId, clusterCmd.commandId);
      } else {
        // only EMBER_AF_GPF_COMMAND_STATUS_NOT_SUPPORTED is expected to be
        // here.
        emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Unsupported ZCL command(clus 0x%2X, cmd 0x%x) embedded within Tap Off Message!", clusterCmd.apsFrame->clusterId, clusterCmd.commandId);
        status = EMBER_ERR_FATAL;
        goto kickout;
      }
    } else {
        emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Unable to process ZCL cmd/resp pair with trans seq number (%d)",
                                             cmd.transactionSequenceNumber);
        status = EMBER_ERR_FATAL;
        goto kickout;
    }
  }

  status = EMBER_SUCCESS;

kickout:

  emberAfPluginGbzMessageControllerParserCleanup(&gbzCommandHandler);
  emberAfPluginGbzMessageControllerParserCleanup(&gbzCommandResponseHandler);

  if (status == EMBER_SUCCESS){
    emberAfPluginGasProxyFunctionPrintln("GPF: TOM message has been successfully processed and will be logged to Event cluster on ep(%d).",
                                        EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT);
  }

  // time to log these lovely TOM commands to the Event log.
  emAfPluginGasProxyFunctionLogEvent(gbzCommands,
                                     gbzCommandsLength,
                                     gbzCommandsResponse,
                                     gbzCommandsResponseLength, 
                                     (status == EMBER_SUCCESS)
                                     ?  GBCS_EVENT_ID_IMM_HAN_CMD_RXED_ACTED :
                                        GBCS_EVENT_ID_IMM_HAN_CMD_RXED_NOT_ACTED, 
                                     EMBER_AF_GPF_MESSAGE_TYPE_TOM);

  emberAfCurrentCommand() = NULL;
  return status;
}

static boolean sendNextNonTomZclCmd(void)
{
  EmberAfStatus status;
  EmberAfGbzZclCommand cmd;

  if (!emberAfPluginGbzMessageControllerHasNextCommand(&nonTomGbzRequestParser)) {
    return FALSE;
  }

  emberAfPluginGbzMessageControllerNextCommand(&nonTomGbzRequestParser, &cmd);
  nonTomGbzStartTime = (cmd.hasFromDateTime) ? cmd.fromDateTime : 0;
  if (cmd.clusterId == ZCL_PRICE_CLUSTER_ID
      || cmd.clusterId == ZCL_EVENTS_CLUSTER_ID
      || cmd.clusterId == ZCL_CALENDAR_CLUSTER_ID) {
    status = emberAfPluginGasProxyFunctionReplayZclCommand(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT,
                                                           EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                                           &cmd);
  } else if (cmd.clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID
             || cmd.clusterId == ZCL_PREPAYMENT_CLUSTER_ID) {
    status = emberAfPluginGasProxyFunctionReplayZclCommand(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT,
                                                           EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MIRROR_ENDPOINT,
                                                           &cmd);
  } else if (cmd.clusterId == ZCL_BASIC_CLUSTER_ID) {
    // EMAPPFWKV2-1308 - GCS21e (messageCode = 0x009E) wants us to read the
    // Basic cluster data from the mirror.
    status = emberAfPluginGasProxyFunctionReplayZclCommand(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT,
                                                           (nonTomGbzRequestParser.messageCode == 0x009E
                                                            ? EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MIRROR_ENDPOINT
                                                            : EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT),
                                                           &cmd);
  } else if (cmd.clusterId == ZCL_DEVICE_MANAGEMENT_CLUSTER_ID ) {
    if ((cmd.commandId == ZCL_PUBLISH_CHANGE_OF_TENANCY_COMMAND_ID)
        && (cmd.direction == ZCL_DIRECTION_SERVER_TO_CLIENT)) {
      status = emberAfPluginGasProxyFunctionReplayZclCommand(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT,
                                                             EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT,
                                                             &cmd);
    } else {
      status = emberAfPluginGasProxyFunctionReplayZclCommand(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT,
                                                             EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                                             &cmd);
    }
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Replaying following command");
    emberAfPluginGbzMessageControllerPrintCommandInfo(&cmd);
    status = EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND;
  }

  if (status != EMBER_ZCL_STATUS_SUCCESS){
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Unable to replay ZCL command (error: 0x%x)!", status);
    return FALSE;
  }

  return TRUE;
}

int32u emAfGasProxyFunctionGetGbzStartTime(void)
{
  return (!nonTomHandlingActive) ? 0 : nonTomGbzStartTime;
}

void emAfGasProxyFunctionAlert(int16u alertCode,
                               EmberAfClusterCommand *cmd, 
                               int16u messageCode)
{
  EmberAfGbzZclCommand gbzZclCmd = {0};
  EmberAfGbzMessageCreatorState creator;
  EmberAfGbzMessageCreatorResult *gbzResponse;

  gbzZclCmd.clusterId = cmd->apsFrame->clusterId;
  gbzZclCmd.frameControl = cmd->buffer[0]; // first byte of the aps payload is the frame control
  gbzZclCmd.transactionSequenceNumber = 0;
  gbzZclCmd.commandId = cmd->commandId;
  gbzZclCmd.payload = &cmd->buffer[cmd->payloadStartIndex];
  gbzZclCmd.payloadLength = cmd->bufLen - cmd->payloadStartIndex;
  gbzZclCmd.direction = cmd->direction;
  gbzZclCmd.clusterSpecific = cmd->clusterSpecific;
  gbzZclCmd.mfgSpecific = cmd->mfgSpecific;
  gbzZclCmd.hasFromDateTime = FALSE;
  gbzZclCmd.encryption = FALSE;

  emberAfPluginGbzMessageControllerCreatorInit(&creator,
                                               EMBER_AF_GBZ_MESSAGE_ALERT,
                                               alertCode,
                                               emberAfGetCurrentTime(),
                                               messageCode,
                                               NULL,
                                               0);
  emberAfPluginGbzMessageControllerAppendCommand(&creator,
                                                 &gbzZclCmd);
  gbzResponse = emberAfPluginGbzMessageControllerCreatorAssemble(&creator);

  emberAfPluginGasProxyFunctionPrintln("GPF: Calling Alert WAN callback with the following ZCL command");
  emberAfPluginGbzMessageControllerPrintCommandInfo(&gbzZclCmd);

  emberAfPluginGasProxyFunctionAlertWANCallback(alertCode, gbzResponse->payload, gbzResponse->payloadLength);
  gbzResponse->freeRequired = FALSE;

  emberAfPluginGbzMessageControllerCreatorCleanup(&creator);
}

/**
 * @brief Send a Non Tap Off Message (Non-TOM) to the GPF
 *
 * This function can be used to send a Non Tap Off Message (Non-TOM) to the Gas
 * Proxy Function (GPF) for processing. Each embedded ZCL command, within the
 * GBZ message, will be sent to the local mirror and the corresponding responses
 * will be written out to the provided response buffer. Once all responses have been collected,
 * emberAfPluginGasProxyFunctionNonTapOffMessageHandlerCompletedCallback() will be invoked.
 *
 * @param gbzCommands       a pointer to the complete GBZ command data.
 * @param gbzCommandsLength the length of the GBZ command.
 * @param messageCode       "Message Code" for the corresponding Non TOM command.
 *
 * @return EMBER_SUCCESS if all messages have been successfully parsed, a non-zero
 *                       status code on error.
 *
 */
EmberStatus emberAfPluginGasProxyFunctionNonTapOffMessageHandler(int8u * gbzCommands,
                                                                 int16u  gbzCommandsLength, 
                                                                 int16u  messageCode)
{
  EmberStatus status = EMBER_SUCCESS;

  if (nonTomHandlingActive) {
    emberAfPluginGasProxyFunctionPrintln("GPF: cannot process two non tap off messages at the same time");
    // we are returning directly so no internal bookkeeping state is changed.
    return EMBER_INVALID_CALL;
  }

  if (!emberAfPluginGbzMessageControllerParserInit(&nonTomGbzRequestParser,
                                                   EMBER_AF_GBZ_MESSAGE_COMMAND,
                                                   gbzCommands,
                                                   gbzCommandsLength,
                                                   TRUE,
                                                   messageCode)) {
    status = EMBER_ERR_FATAL;
    goto kickout;
  }

  if (!emberAfPluginGbzMessageControllerCreatorInit(&nonTomGbzResponseCreator,
                                                    EMBER_AF_GBZ_MESSAGE_RESPONSE,
                                                    0,
                                                    0,
                                                    messageCode,
                                                    NULL,
                                                    0)) {
    status = EMBER_ERR_FATAL;
    goto kickout;
  }

  nonTomHandlingActive = TRUE;
  status = sendNextNonTomZclCmd() ? EMBER_SUCCESS : EMBER_ERR_FATAL;

kickout:
  if (status != EMBER_SUCCESS) {
    emberAfPluginGbzMessageControllerParserCleanup(&nonTomGbzRequestParser);
    emberAfPluginGbzMessageControllerCreatorCleanup(&nonTomGbzResponseCreator);
    nonTomExpectedZclResps = 0;
    nonTomResponsesCount = 0;
    nonTomHandlingActive = FALSE;

    // Log Non-Actioned Non-TOM
    emAfPluginGasProxyFunctionLogEvent(gbzCommands,
                                       gbzCommandsLength,
                                       NULL,
                                       0, 
                                       GBCS_EVENT_ID_IMM_HAN_CMD_RXED_NOT_ACTED, 
                                       EMBER_AF_GPF_MESSAGE_TYPE_NON_TOM);
  }
  return status;
}

/*
 * @ brief Stores captured Zcl Command Responses internally. 
 *   
 *   If the response payloads contain user sensitive material,
 *   the payloads are encrypted before being stored.
 *
 */
static void captureNonTomZclCmdResp(EmberAfClusterCommand * cmd)
{
  EmberAfGbzZclCommand gbzZclCmd = {0};
  EmberAfGbzMessageCreatorResult *gbzResponse;
  emberAfPluginGasProxyFunctionPrintln("GPF: Intercepting ZCL cmd(0x%X) on endpoint: %d", cmd->commandId, emberAfCurrentEndpoint());

  if (!nonTomHandlingActive){
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Not expecting a Non-TOM response.");
    return;
  }

  gbzZclCmd.clusterId = cmd->apsFrame->clusterId;
  gbzZclCmd.frameControl = cmd->buffer[0]; // first byte of the aps payload is the frame control
  gbzZclCmd.transactionSequenceNumber = cmd->seqNum;
  gbzZclCmd.commandId = cmd->commandId;
  gbzZclCmd.payload = &cmd->buffer[cmd->payloadStartIndex];
  gbzZclCmd.payloadLength = cmd->bufLen - cmd->payloadStartIndex;
  gbzZclCmd.direction = cmd->direction;
  gbzZclCmd.clusterSpecific = cmd->clusterSpecific;
  gbzZclCmd.mfgSpecific = cmd->mfgSpecific;
  gbzZclCmd.hasFromDateTime = FALSE;
  gbzZclCmd.encryption = FALSE;

  emberAfPluginGasProxyFunctionPrintln("GPF: Saving following ZCL response for Non-TOM message response.");
  emberAfPluginGbzMessageControllerPrintCommandInfo(&gbzZclCmd);

  emberAfPluginGbzMessageControllerAppendCommand(&nonTomGbzResponseCreator,
                                                 &gbzZclCmd);

  // allocate final GBZ Non-TOM response buffer
  if (nonTomExpectedZclResps == ++nonTomResponsesCount
      && !sendNextNonTomZclCmd()) {
    gbzResponse = emberAfPluginGbzMessageControllerCreatorAssemble(&nonTomGbzResponseCreator);

    emberAfPluginGasProxyFunctionPrintln("GPF: Total length for GBZ response: %d", gbzResponse->payloadLength);
    emberAfPluginGasProxyFunctionPrint("GPF: GBZ response payload: ");
    emberAfPluginGasProxyFunctionPrintBuffer(gbzResponse->payload, gbzResponse->payloadLength, TRUE);
    emberAfPluginGasProxyFunctionPrintln("");
    emberAfPluginGasProxyFunctionNonTapOffMessageHandlerCompletedCallback(gbzResponse->payload, gbzResponse->payloadLength);
    gbzResponse->freeRequired = FALSE;

    // Log Actioned Non TOM
    emAfPluginGasProxyFunctionLogEvent(nonTomGbzRequestParser.command,
                                       nonTomGbzRequestParser.length,
                                       gbzResponse->payload,
                                       gbzResponse->payloadLength,
                                       GBCS_EVENT_ID_IMM_HAN_CMD_RXED_ACTED,
                                       EMBER_AF_GPF_MESSAGE_TYPE_NON_TOM);

    emberAfPluginGbzMessageControllerParserCleanup(&nonTomGbzRequestParser);
    emberAfPluginGbzMessageControllerCreatorCleanup(&nonTomGbzResponseCreator);
    nonTomExpectedZclResps = 0;
    nonTomResponsesCount = 0;
    nonTomHandlingActive = FALSE;
  }
}

/* @brief Capture ZCL response from replayed Non-TOM messages. The captured responses
 * is packed up and delivered to the application via a GBZ message buffer
 */
boolean emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd){
  if (!nonTomHandlingActive
      || (cmd->source != emberGetNodeId())){
    return FALSE;
  }
  
  if (cmd->apsFrame->destinationEndpoint == EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT){
    if ((cmd->apsFrame->sourceEndpoint == EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT)
        ||(cmd->apsFrame->sourceEndpoint == EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_MIRROR_ENDPOINT)){ 
      captureNonTomZclCmdResp(cmd);
      return TRUE;
    } else if ((cmd->apsFrame->sourceEndpoint == EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_HIDDEN_CLIENT_SIDE_ENDPOINT)
               && (cmd->commandId == ZCL_DEFAULT_RESPONSE_COMMAND_ID)) {
      captureNonTomZclCmdResp(cmd);
      return TRUE;
    }
  }
  return FALSE;
}

boolean emberAfPriceClusterPublishCalorificValueCallback(int32u issuerEventId,
                                                         int32u startTime,
                                                         int32u calorificValue,
                                                         int8u calorificValueUnit,
                                                         int8u calorificValueTrailingDigit)
{
  EmberAfStatus status;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishCalorificValue 0x%4X, 0x%4X, 0x%4X, 0x%X, 0x%X",
                             issuerEventId,
                             startTime,
                             calorificValue,
                             calorificValueUnit,
                             calorificValueTrailingDigit);

  status = emberAfPluginPriceServerCalorificValueAdd(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                                     issuerEventId,
                                                     startTime,
                                                     calorificValue, 
                                                     calorificValueUnit, 
                                                     calorificValueTrailingDigit);
  if (status != EMBER_ZCL_STATUS_SUCCESS){
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Unable to update calorific value (status:0x%X).", status);
  }
  return TRUE;
}
 
boolean emberAfPriceClusterPublishConversionFactorCallback(int32u issuerEventId,
                                                           int32u startTime,
                                                           int32u conversionFactor,
                                                           int8u conversionFactorTrailingDigit)
{
  EmberStatus status;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishConversionFactor 0x%4X, 0x%4X, 0x%4X, 0x%X",
                             issuerEventId,
                             startTime,
                             conversionFactor,
                             conversionFactorTrailingDigit);

  status = emberAfPluginPriceServerConversionFactorAdd(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                                       issuerEventId, 
                                                       startTime, 
                                                       conversionFactor, 
                                                       conversionFactorTrailingDigit);
  if (status != EMBER_ZCL_STATUS_SUCCESS){
    emberAfPluginGasProxyFunctionPrintln("GPF: ERR: Unable to update conversion factor (status:0x%X).", status);
  }
  return TRUE;
}

boolean emberAfCalendarClusterPublishCalendarCallback(int32u providerId,
                                                      int32u issuerEventId,
                                                      int32u issuerCalendarId,
                                                      int32u startTime,
                                                      int8u  calendarType,
                                                      int8u  calendarTimeReference,
                                                      int8u  *calendarName,
                                                      int8u  numberOfSeasons,
                                                      int8u  numberOfWeekProfiles,
                                                      int8u  numberOfDayProfiles)
{
  boolean status;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishCalendar 0x%4x, 0x%4x, 0x%4x, 0x%4x, 0x%x, \"",
                                       providerId,
                                       issuerEventId,
                                       issuerCalendarId,
                                       startTime,
                                       calendarType);
  emberAfPluginGasProxyFunctionPrintString(calendarName);
  emberAfPluginGasProxyFunctionPrintln("\", %d, %d, %d",
                                       numberOfSeasons,
                                       numberOfWeekProfiles,
                                       numberOfDayProfiles);
  status = emberAfCalendarCommonAddCalInfo(providerId,
                                           issuerEventId,
                                           issuerCalendarId,
                                           startTime,
                                           calendarType,
                                           calendarName,
                                           numberOfSeasons,
                                           numberOfWeekProfiles,
                                           numberOfDayProfiles);

  if (status){
    emberAfPluginGasProxyFunctionPrintln("GPF: Updated: Calendar");
    emberAfPluginGasProxyFunctionPrintln("GPF:          providerId: 0x%4X", providerId);
    emberAfPluginGasProxyFunctionPrintln("GPF:          issuerEventId: 0x%4X", issuerEventId);
    emberAfPluginGasProxyFunctionPrintln("GPF:          issuerCalendarId: 0x%4X", issuerCalendarId);
    emberAfPluginGasProxyFunctionPrintln("GPF:          startTimeUtc: 0x%4X", startTime);
    emberAfPluginGasProxyFunctionPrintln("GPF:          calendarType: 0x%X", calendarType);
    emberAfPluginGasProxyFunctionPrint("GPF:          calendarName: ");
    emberAfPluginGasProxyFunctionPrintString(calendarName);
    emberAfPluginGasProxyFunctionPrintln("");
    emberAfPluginGasProxyFunctionPrintln("GPF:          numberOfSeasons: 0x%X", numberOfSeasons);
    emberAfPluginGasProxyFunctionPrintln("GPF:          numberOfWeekProfiles: 0x%X", numberOfWeekProfiles);
    emberAfPluginGasProxyFunctionPrintln("GPF:          numberOfDayProfiles: 0x%X", numberOfDayProfiles);
  }

  return TRUE;
}

boolean emberAfCalendarClusterPublishDayProfileCallback(int32u providerId,
                                                        int32u issuerEventId,
                                                        int32u issuerCalendarId,
                                                        int8u dayId,
                                                        int8u totalNumberOfScheduleEntries,
                                                        int8u commandIndex,
                                                        int8u totalNumberOfCommands,
                                                        int8u calendarType,
                                                        int8u *dayScheduleEntries)
{





  boolean status;
  int16u startTime;
  int8u data;
  int16u dayScheduleIndex = 0;
  int16u dayScheduleEntriesLength = fieldLength(dayScheduleEntries);

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishDayProfile 0x%4x, 0x%4x, 0x%4x, %d, %d, %d, %d, 0x%x",
                                       providerId,
                                       issuerEventId,
                                       issuerCalendarId,
                                       dayId,
                                       totalNumberOfScheduleEntries,
                                       commandIndex,
                                       totalNumberOfCommands,
                                       calendarType);

  status = emberAfCalendarCommonAddDayProfInfo(issuerCalendarId,
                                               dayId,
                                               dayScheduleEntries, 
                                               dayScheduleEntriesLength);
  return TRUE;
}

/** @brief Publish Info
 *
 * This function is called by the calendar-server plugin after receiving any of
 * the following commands and just before it starts publishing the response:
 * GetCalendar, GetDayProfiles, GetSeasons, GetSpecialDays, and GetWeekProfiles.
 *
 * @param publishCommandId ZCL command to be published  Ver.: always
 * @param clientNodeId Destination nodeId  Ver.: always
 * @param clientEndpoint Destination endpoint  Ver.: always
 * @param totalCommands Total number of publish commands to be sent  Ver.:
 * always
 */
void emberAfPluginCalendarServerPublishInfoCallback(int8u publishCommandId,
                                                    EmberNodeId clientNodeId,
                                                    int8u clientEndpoint,
                                                    int8u totalCommands)
{
  // If the publish is a result of a non-TOM use case then the clientNodeId
  // will be our nodeId and if so we need to update the number of expected
  // response to include all of the publish commands to be sent. We subtract
  // one from the total commands as we always expect at least one response
  // when we initialize the expected responses count.
  if (nonTomHandlingActive && (clientNodeId == emberGetNodeId())) {
    int8u expectedZclResps = nonTomExpectedZclResps + (totalCommands - 1);
    emberAfPluginGasProxyFunctionPrintln("GPF: Updating number of Non-TOM expected responses from %d to %d",
                                         nonTomExpectedZclResps,
                                         expectedZclResps);
    nonTomExpectedZclResps = expectedZclResps;
  }
}

boolean emberAfPriceClusterPublishBlockThresholdsCallback(int32u providerId,
                                                          int32u issuerEventId,
                                                          int32u startTime,
                                                          int32u issuerTariffId,
                                                          int8u commandIndex,
                                                          int8u numberOfCommands,
                                                          int8u subpayloadControl,
                                                          int8u* payload){
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishBlockThresholds 0x%4X, 0x%4X, 0x%4X, 0x%4X, 0x%X, 0x%X, 0x%X",
                                       providerId,
                                       issuerEventId,
                                       startTime,
                                       issuerTariffId,
                                       commandIndex,
                                       numberOfCommands,
                                       subpayloadControl);
  emberAfPriceAddBlockThresholdsTableEntry(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                           providerId,
                                           issuerEventId,
                                           startTime,
                                           issuerTariffId,
                                           commandIndex,
                                           numberOfCommands,
                                           subpayloadControl,
                                           payload);

  return TRUE;
} 

boolean emberAfPriceClusterPublishTariffInformationCallback(int32u providerId,
                                                            int32u issuerEventId,
                                                            int32u issuerTariffId,
                                                            int32u startTime,
                                                            int8u tariffTypeChargingScheme,
                                                            int8u *tariffLabel,
                                                            int8u numberOfPriceTiersInUse,
                                                            int8u numberOfBlockThresholdsInUse,
                                                            int8u unitOfMeasure,
                                                            int16u currency,
                                                            int8u priceTrailingDigit,
                                                            int32u standingCharge,
                                                            int8u tierBlockMode,
                                                            int32u blockThresholdMultiplier,
                                                            int32u blockThresholdDivisor) {
  EmberAfPriceCommonInfo info;
  EmberAfScheduledTariff tariff;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishTariffInformationReceived");
  emberAfPriceClusterPrint("RX: PublishTariffInformation 0x%4x, 0x%4x, 0x%4x, 0x%4x, 0x%x, \"",
                           providerId,
                           issuerEventId,
                           issuerTariffId,
                           startTime,
                           tariffTypeChargingScheme);

  emberAfPriceClusterPrintString(tariffLabel);
  emberAfPriceClusterPrint("\"");
  emberAfPriceClusterPrint(", 0x%x, 0x%x, 0x%x, 0x%2x, 0x%x",
                           numberOfPriceTiersInUse,
                           numberOfBlockThresholdsInUse,
                           unitOfMeasure,
                           currency,
                           priceTrailingDigit);
  emberAfPriceClusterPrintln(", 0x%4x, 0x%x, 0x%4x, 0x%4x",
                             standingCharge,
                             tierBlockMode,
                             blockThresholdMultiplier,
                             blockThresholdDivisor);
  emberAfPriceClusterFlush();
  info.startTime = startTime;
  info.issuerEventId = issuerEventId;
  tariff.providerId = providerId;
  tariff.issuerTariffId = issuerTariffId;
  tariff.tariffTypeChargingScheme = tariffTypeChargingScheme;
  MEMCOPY(tariff.tariffLabel, tariffLabel, emberAfStringLength(tariffLabel) + 1);
  tariff.numberOfPriceTiersInUse = numberOfPriceTiersInUse;
  tariff.numberOfBlockThresholdsInUse = numberOfBlockThresholdsInUse;
  tariff.unitOfMeasure = unitOfMeasure;
  tariff.currency = currency;
  tariff.priceTrailingDigit = priceTrailingDigit;
  tariff.standingCharge = standingCharge;
  tariff.tierBlockMode = tierBlockMode;
  tariff.blockThresholdMultiplier = blockThresholdMultiplier;
  tariff.blockThresholdDivisor = blockThresholdDivisor;
  tariff.status |= FUTURE;


  emberAfPriceAddTariffTableEntry(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                  &info,
                                  &tariff);
  return TRUE;
}

boolean emberAfPriceClusterPublishPriceMatrixCallback(int32u providerId,
                                                      int32u issuerEventId,
                                                      int32u startTime,
                                                      int32u issuerTariffId,
                                                      int8u commandIndex,
                                                      int8u numberOfCommands,
                                                      int8u subPayloadControl,
                                                      int8u* payload)
{
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishPriceMatrix 0x%4X, 0x%4X, 0x%4X, 0x%4X, 0x%X, 0x%X, 0x%X",
                                       providerId,
                                       issuerEventId,
                                       startTime,
                                       issuerTariffId,
                                       commandIndex,
                                       numberOfCommands,
                                       subPayloadControl);

  emberAfPriceAddPriceMatrixRaw(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                providerId,
                                issuerEventId,
                                startTime,
                                issuerTariffId,
                                commandIndex,
                                numberOfCommands,
                                subPayloadControl,
                                payload);

  return TRUE;
}

boolean emberAfCalendarClusterPublishSeasonsCallback(int32u providerId,
                                                     int32u issuerEventId,
                                                     int32u issuerCalendarId,
                                                     int8u commandIndex,
                                                     int8u totalNumberOfCommands,
                                                     int8u *seasonEntries)
{





  boolean status;
  EmberAfDate sessionStartDate;
  int8u weekIdRef;
  int16u seasonEntriesIndex = 0;
  int16u seasonEntriesLength = fieldLength(seasonEntries);

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishSeasons 0x%4x, 0x%4x, 0x%4x, %d, %d",
                                        providerId,
                                        issuerEventId,
                                        issuerCalendarId,
                                        commandIndex,
                                        totalNumberOfCommands);

  status = emberAfCalendarServerAddSeasonsInfo(issuerCalendarId,
                                               seasonEntries,
                                               seasonEntriesLength);
  return TRUE;
}

boolean emberAfCalendarClusterPublishSpecialDaysCallback(int32u providerId,
                                                         int32u issuerEventId,
                                                         int32u issuerCalendarId,
                                                         int32u startTime,
                                                         int8u calendarType,
                                                         int8u totalNumberOfSpecialDays,
                                                         int8u commandIndex,
                                                         int8u totalNumberOfCommands,
                                                         int8u *specialDayEntries)
{
  int16u speicalDaysEntriesLength = fieldLength(specialDayEntries);
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishSpecialDays 0x%4x, 0x%4x, 0x%4x, 0x%4x, 0x%x, %d, %d, %d, [",
                                       providerId,
                                       issuerEventId,
                                       issuerCalendarId,
                                       startTime,
                                       calendarType,
                                       totalNumberOfSpecialDays,
                                       commandIndex,
                                       totalNumberOfCommands);
  // TODO: print specialDayEntries
  emberAfPluginGasProxyFunctionPrintln("]");

  emberAfCalendarCommonAddSpecialDaysInfo(issuerCalendarId,
                                          totalNumberOfSpecialDays,
                                          specialDayEntries,
                                          speicalDaysEntriesLength);
  return TRUE; 
}

boolean emberAfCalendarClusterPublishWeekProfileCallback(int32u providerId,
                                                         int32u issuerEventId,
                                                         int32u issuerCalendarId,
                                                         int8u weekId,
                                                         int8u dayIdRefMonday,
                                                         int8u dayIdRefTuesday,
                                                         int8u dayIdRefWednesday,
                                                         int8u dayIdRefThursday,
                                                         int8u dayIdRefFriday,
                                                         int8u dayIdRefSaturday,
                                                         int8u dayIdRefSunday)
{
  boolean status;
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishWeekProfile 0x%4x, 0x%4x, 0x%4x, %d, %d, %d, %d, %d, %d, %d, %d",
                                providerId,
                                issuerEventId,
                                issuerCalendarId,
                                weekId,
                                dayIdRefMonday,
                                dayIdRefTuesday,
                                dayIdRefWednesday,
                                dayIdRefThursday,
                                dayIdRefFriday,
                                dayIdRefSaturday,
                                dayIdRefSunday);
  
  status = emberAfCalendarServerAddWeekProfInfo(issuerCalendarId, 
                                                weekId,
                                                dayIdRefMonday,
                                                dayIdRefTuesday,
                                                dayIdRefWednesday,
                                                dayIdRefThursday,
                                                dayIdRefFriday,
                                                dayIdRefSaturday,
                                                dayIdRefSunday);
  return TRUE;
}

boolean emberAfMessagingClusterDisplayMessageCallback(int32u messageId,
                                                      int8u messageControl,
                                                      int32u startTime,
                                                      int16u durationInMinutes,
                                                      int8u* msg, 
                                                      int8u optionalExtendedMessageControl)
{
  EmberAfPluginMessagingServerMessage message;
  int8u msgLength = emberAfStringLength(msg) + 1;

  if (msgLength > EMBER_AF_PLUGIN_MESSAGING_SERVER_MESSAGE_SIZE) {
    emberAfPluginGasProxyFunctionPrint("GPF: ERR: Message too long for messaging server message buffer.");
    return TRUE;
  }

  message.messageId = messageId;
  message.messageControl = messageControl;
  message.startTime = startTime;
  message.durationInMinutes = durationInMinutes;
  MEMCOPY(message.message, msg, msgLength);
  message.extendedMessageControl = optionalExtendedMessageControl;

  emberAfPluginGasProxyFunctionPrint("GPF: RX: DisplayMessage"
                                     " 0x%4x, 0x%x, 0x%4x, 0x%2x, \"",
                                     messageId,
                                     messageControl,
                                     startTime,
                                     durationInMinutes);
  emberAfPluginGasProxyFunctionPrintString(msg);
  emberAfPluginGasProxyFunctionPrint(", 0x%X", optionalExtendedMessageControl);
  emberAfPluginGasProxyFunctionPrintln("\"");

  emberAfPluginMessagingServerSetMessage(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                         &message);
  return TRUE;
}

boolean emberAfDeviceManagementClusterPublishChangeOfSupplierCallback(int32u currentProviderId,
                                                                      int32u issuerEventId,
                                                                      int8u tariffType,
                                                                      int32u proposedProviderId,
                                                                      int32u providerChangeImplementationTime,
                                                                      int32u providerChangeControl,
                                                                      int8u *proposedProviderName,
                                                                      int8u *proposedProviderContactDetails)
{
  EmberAfDeviceManagementSupplier supplier;
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishChangeOfSupplier: 0x%4X, 0x%4X, 0x%X, 0x%4X, 0x%4X, 0x%4X, ",
                                        currentProviderId,
                                        issuerEventId,
                                        tariffType,
                                        proposedProviderId,
                                        providerChangeImplementationTime,
                                        providerChangeControl);
  emberAfPluginGasProxyFunctionPrintString(proposedProviderName);
  emberAfPluginGasProxyFunctionPrintln(", ");
  emberAfPluginGasProxyFunctionPrintString(proposedProviderContactDetails);
  emberAfPluginGasProxyFunctionPrintln("");
 
  supplier.proposedProviderId = proposedProviderId; 
  supplier.implementationDateTime = providerChangeImplementationTime;
  supplier.providerChangeControl = providerChangeControl;
  MEMCOPY(&supplier.proposedProviderName,
          proposedProviderName,
          emberAfStringLength(proposedProviderName) + 1);
  MEMCOPY(&supplier.proposedProviderContactDetails,
          proposedProviderContactDetails,
          emberAfStringLength(proposedProviderContactDetails) + 1);
  
  emberAfPluginDeviceManagementSetSupplier(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT, &supplier);
  return TRUE;
}

boolean emberAfDeviceManagementClusterPublishChangeOfTenancyCallback(int32u providerId,
                                                                     int32u issuerEventId,
                                                                     int8u tariffType,
                                                                     int32u implementationDateTime,
                                                                     int32u proposedTenancyChangeControl) {
  EmberAfDeviceManagementTenancy tenancy;
  boolean result;
  EmberAfStatus status;

  emberAfPluginGasProxyFunctionPrintln("RX: PublishChangeOfTenancy: 0x%4X, 0x%4X, 0x%X, 0x%4X, 0x%4X",
                                        providerId,
                                        issuerEventId,
                                        tariffType,
                                        implementationDateTime,
                                        proposedTenancyChangeControl);

  tenancy.implementationDateTime = implementationDateTime;
  tenancy.tenancy = proposedTenancyChangeControl;
  tenancy.providerId = providerId;
  tenancy.issuerEventId = issuerEventId;
  tenancy.tariffType = tariffType;


  result = emberAfPluginDeviceManagementSetTenancy(&tenancy, TRUE);

  if (result){
    emberAfPluginGasProxyFunctionPrintln("GPF: Updated: Tenancy");
    emberAfPluginGasProxyFunctionPrintln("              implementationTime: 0x%4X", tenancy.implementationDateTime);
    emberAfPluginGasProxyFunctionPrintln("              tenancy: 0x%4X", tenancy.tenancy);
    status = EMBER_ZCL_STATUS_SUCCESS;
  } else {
    emberAfPluginGasProxyFunctionPrintln("GPF: Unable to update tenancy due to mismatching information.");
    status = EMBER_ZCL_STATUS_FAILURE;
  }

  emberAfSendImmediateDefaultResponse(status);
  return TRUE;
}
/** @brief Simple Metering Cluster Request Mirror
 *
 */
boolean emberAfSimpleMeteringClusterRequestMirrorCallback(void)
{
  EmberEUI64 otaEui;
  int16u endpointId;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: RequestMirror");

  if (emberLookupEui64ByNodeId(emberAfResponseDestination, otaEui) == EMBER_SUCCESS) {
    endpointId = emberAfPluginMeterMirrorRequestMirror(otaEui);
    if (endpointId != 0xFFFF) {
      emberAfFillCommandSimpleMeteringClusterRequestMirrorResponse(endpointId);
      emberAfSendResponse();
    } else {
      emberAfPluginGasProxyFunctionPrintln("GPF: Invalid endpoint. Sending Default Response");
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_AUTHORIZED);
    }
  } else {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE);
  }
  return TRUE;
}

/** @brief Simple Metering Cluster Remove Mirror
 *
 */
boolean emberAfSimpleMeteringClusterRemoveMirrorCallback(void)
{
  EmberEUI64 otaEui;
  int16u endpointId;

  emberAfPluginGasProxyFunctionPrintln("GPF: RX: RemoveMirror");

  if (emberLookupEui64ByNodeId(emberAfResponseDestination, otaEui) == EMBER_SUCCESS) {
    endpointId = emberAfPluginMeterMirrorRemoveMirror(otaEui);
    emberAfFillCommandSimpleMeteringClusterMirrorRemoved(endpointId);
    emberAfSendResponse();
  } else {
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE);
  }
  return TRUE;
}

boolean emberAfPriceClusterPublishBlockPeriodCallback(int32u providerId,
                                                      int32u issuerEventId,
                                                      int32u blockPeriodStartTime,
                                                      int32u blockPeriodDuration,
                                                      int8u blockPeriodControl,
                                                      int8u blockPeriodDurationType,
                                                      int8u tariffType,
                                                      int8u tariffResolutionPeriod){
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishBlockPeriod 0x%4X, 0x%4X, 0x%4X, 0x%4X, 0x%X, 0x%X, 0x%X, 0x%X",
                                       providerId,             
                                       issuerEventId,          
                                       blockPeriodStartTime,   
                                       blockPeriodDuration,    
                                       blockPeriodControl,      
                                       blockPeriodDurationType, 
                                       tariffType,              
                                       tariffResolutionPeriod);
  emberAfPluginPriceServerBlockPeriodAdd(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                         providerId,
                                         issuerEventId,
                                         blockPeriodStartTime,
                                         blockPeriodDuration,
                                         blockPeriodControl,
                                         blockPeriodDurationType,
                                         0x01,
                                         0x01,
                                         tariffType,
                                         tariffResolutionPeriod);
  return TRUE;
}

boolean emberAfPriceClusterPublishBillingPeriodCallback(int32u providerId,
                                                        int32u issuerEventId,
                                                        int32u billingPeriodStartTime,
                                                        int32u billingPeriodDuration,
                                                        int8u billingPeriodDurationType,
                                                        int8u tariffType)
{
  emberAfPluginGasProxyFunctionPrintln("GPF: RX: PublishBillingPeriod 0x%4X, 0x%4X, 0x%4X, 0x%4X, 0x%X, 0x%X",
                                       providerId,
                                       issuerEventId,
                                       billingPeriodStartTime,
                                       billingPeriodDuration,
                                       billingPeriodDurationType,
                                       tariffType);

  emberAfPluginPriceServerBillingPeriodAdd(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION_ESI_ENDPOINT,
                                           billingPeriodStartTime,
                                           issuerEventId,
                                           providerId,
                                           billingPeriodDuration,
                                           billingPeriodDurationType,
                                           tariffType);
  return TRUE;
}
