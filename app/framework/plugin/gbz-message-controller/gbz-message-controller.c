// *******************************************************************
// * gbz-message-controller.c
// *
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include <stdlib.h>     // malloc, free
#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/util/common.h"
#include "gbz-message-controller.h"
#include "app/framework/plugin/gas-proxy-function/gas-proxy-function.h"

// forward declarations
static int16u emAfPluginGbzMessageControllerAppendInNewMem(EmberAfGbzMessageCreatorState * state,
                                                           EmberAfGbzZclCommand * zclCmd,
                                                           EmberAfGbzMessageData * data);
static int16u emAfPluginGbzMessageControllerAppendInPlace(EmberAfGbzMessageCreatorState * state,
                                                          EmberAfGbzZclCommand * zclCmd,
                                                          EmberAfGbzMessageData * data);
static int16u emAfPluginGbzMessageControllerGetZclLength(EmberAfGbzZclCommand * cmd);
int16u emAfPluginGbzMessageControllerGetLength(EmberAfGbzZclCommand * cmd);
static int16u emAfPluginGbzMessageControllerGetResponsesLength(EmAfGbzUseCaseSpecificComponent * comp);
static int16u emAfPluginGbzMessageControllerAppendResponses(int8u * dst,
                                                            int16u maxLen,
                                                            EmAfGbzUseCaseSpecificComponent * responses);
static void emAfPluginGbzMessageControllerCreatorFreeResponses(EmberAfGbzMessageCreatorState * comp);
static void emAfPluginGbzMessageControllerFreeHeader(EmAfGbzPayloadHeader * header);

void emberAfPluginGbzMessageControllerNextCommand(EmberAfGbzMessageParserState * state,
                                                  EmberAfGbzZclCommand * gbzZclCommand) {
  if (state == NULL || gbzZclCommand == NULL){
    emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: invalid pointers");
    return;
  }
  
  MEMSET(gbzZclCommand, 0x00, sizeof(EmberAfGbzZclCommand));
  if (state->componentsParsed < state->componentsSize){
    int8u extendedHeaderControlField = 0;
    int16u gbzCmdLength = 0;
    boolean encryption = FALSE;

    // Most significant nibble:
    //  0x0 if ‘From Date Time’ not present
    //  Or
    //  0x1 if ‘From Date Time’ present

    //  Least significant nibble:
    //  0x0 (if not the last GBZ Use Case Specific Component in this Message)
    //  0x1 (if the last GBZ Use Case Specific Component in this Message)
    //  0x02(encrypted) (if not the last GBZ Use Case Specific Component in this Message)
    //  0x03(encrypted) (if the last GBZ Use Case Specific Component in this Message)
    
    extendedHeaderControlField = (int8u) state->command[state->parseIndex++];

    if (extendedHeaderControlField & GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTED_MSG_MASK){
      encryption = TRUE;
    } else {
      encryption = FALSE;
    }

    if (extendedHeaderControlField & GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_MASK){
      gbzZclCommand->hasFromDateTime = TRUE;
    } else {
      gbzZclCommand->hasFromDateTime = FALSE;
    }
    MEMCOPY(&gbzZclCommand->clusterId, &state->command[state->parseIndex], 2);
    state->parseIndex += 2;

    MEMCOPY(&gbzCmdLength, &state->command[state->parseIndex], 2);
    state->parseIndex += 2;

    if (gbzZclCommand->hasFromDateTime) {
      gbzZclCommand->fromDateTime = emberAfGetInt32u(state->command, state->parseIndex, state->length);
      state->parseIndex += 4;
    }

    gbzZclCommand->frameControl = (int8u) state->command[state->parseIndex++];
    gbzZclCommand->clusterSpecific = gbzZclCommand->frameControl & ZCL_CLUSTER_SPECIFIC_COMMAND;
    gbzZclCommand->mfgSpecific = gbzZclCommand->frameControl & ZCL_MANUFACTURER_SPECIFIC_MASK;
    gbzZclCommand->direction = (( gbzZclCommand->frameControl & ZCL_FRAME_CONTROL_DIRECTION_MASK)
                               ? ZCL_DIRECTION_SERVER_TO_CLIENT
                               : ZCL_DIRECTION_CLIENT_TO_SERVER);
    
    gbzZclCommand->transactionSequenceNumber = state->command[state->parseIndex++];
    gbzZclCommand->commandId = state->command[state->parseIndex++];

    if (encryption){
      EmberAfGbzMessageData data ;
      data.encryption = TRUE;

      data.encryptedDataLength = emberAfGetInt16u(state->command, state->parseIndex, state->length);
      data.encryptedData = &state->command[state->parseIndex + 2]; 

      emberAfPluginGbzMessageControllerDecryptDataCallback(&data);
      if (data.encryption){
        emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: Unable to decrypt ZCL payload: ");
        emberAfPluginGbzMessageControllerPrintBuffer(data.encryptedData,
                                                     data.encryptedDataLength,
                                                     TRUE);

        gbzZclCommand->payload = data.encryptedData;
        gbzZclCommand->payloadLength = data.encryptedDataLength;
      } else {
        // decryption successful
        MEMCOPY(&state->command[state->parseIndex + 2], data.plainData, data.plainDataLength);
        gbzZclCommand->payload = &state->command[state->parseIndex + 2];
        gbzZclCommand->payloadLength = data.plainDataLength;
      }
    } else {
      gbzZclCommand->payload = &state->command[state->parseIndex];
      gbzZclCommand->payloadLength = gbzCmdLength - GAS_PROXY_FUNCTION_GBZ_COMPONENT_ZCL_HEADER_LENGTH;
    }
    state->parseIndex += gbzCmdLength - 3;
  }
  state->componentsParsed += 1;
}

/*
 * @ Get the length of the overall GBZ message
 */
int16u emAfPluginGbzMessageControllerGetLength(EmberAfGbzZclCommand * cmd){
  int16u len = GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_FIELDS_LENGTH;

  // this isn't a valid configuration according to the GBCS spec.
  if (cmd->hasFromDateTime && cmd->encryption){
    return 0;
  }

  if (cmd->encryption){
    len += GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_HEADER_FIELDS_LENGTH;
  } else if (cmd->hasFromDateTime){
    len += GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_LENGTH;
  }

  len += GAS_PROXY_FUNCTION_GBZ_COMPONENT_ZCL_HEADER_LENGTH;

  if (cmd->encryption){
    len += GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_CIPHERED_INFO_LENGTH;
  }
  len += cmd->payloadLength;
  return len;
}

/*
 * @ Get the length of GBZ messages' command length
 */
int16u emAfPluginGbzMessageControllerGetCommandLength(EmberAfGbzZclCommand * cmd){
  int16u len = emAfPluginGbzMessageControllerGetLength(cmd);
  if (len != 0){
    return len - GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_FIELDS_LENGTH;
  } else {
    return len;
  }
}

void emberAfPluginGbzMessageControllerPrintCommandInfo(EmberAfGbzZclCommand * cmd){
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: cluster id: 0x%2x", cmd->clusterId);
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: command id: 0x%x", cmd->commandId);
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: frame control: 0x%x", cmd->frameControl);
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: direction: 0x%x", cmd->direction);
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: cluster specific: 0x%x", cmd->clusterSpecific);
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: mfg specific: 0x%x", cmd->mfgSpecific);
  emberAfPluginGbzMessageControllerPrintln("GBZ: ZCL command: trans. seq. number: 0x%x", cmd->transactionSequenceNumber);
  emberAfPluginGbzMessageControllerPrint("GBZ: ZCL command: payload: ");
  emberAfPluginGbzMessageControllerPrintBuffer(cmd->payload, cmd->payloadLength, TRUE);
  emberAfPluginGbzMessageControllerPrintln("");
}

boolean emberAfPluginGbzMessageControllerHasNextCommand(EmberAfGbzMessageParserState * state){
  if (state == NULL) { return FALSE; }
  return (state->componentsParsed < state->componentsSize);
}

int8u emberAfPluginGbzMessageControllerGetComponentSize(EmberAfGbzMessageParserState * state){
  if (state == NULL) { return 0; }
  return (state->componentsSize);
}

boolean emberAfPluginGbzMessageControllerParserInit(EmberAfGbzMessageParserState * state,
                                                    EmberAfGbzMessageType type,
                                                    int8u * gbzCommand, 
                                                    int16u gbzCommandLength,
                                                    boolean copyGbzCommand,
                                                    int16u messageCode)
{
  int16u profileId;
  int8u *gbzCommandCopy;

  if (state == NULL){ return FALSE; }

  MEMSET(state, 0x00, sizeof(EmberAfGbzMessageParserState));

  MEMCOPY(&profileId, gbzCommand, 2);

  if (profileId != SE_PROFILE_ID){
    emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: invalid profile id: 0x%2x ", profileId);
    return FALSE;
  }

  if (copyGbzCommand) {
    gbzCommandCopy = (int8u *) malloc(gbzCommandLength);
    if (gbzCommandCopy == NULL) {
      emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: cannot malloc %d bytes", gbzCommandLength);
      return FALSE;
    }
    MEMCOPY(gbzCommandCopy, gbzCommand, gbzCommandLength);
  } else {
    gbzCommandCopy = gbzCommand;
  }

  state->messageCode = messageCode;
  state->type = type;
  state->nextComponentZclSequenceNumber = 0;
  state->componentsParsed = 0;
  state->freeRequired = copyGbzCommand;
  state->command = gbzCommandCopy;
  state->parseIndex = 2; // skip profileId since we already extracted it
  state->length = gbzCommandLength;
  state->profileId = profileId;
  state->componentsSize = emberAfGetInt8u(state->command, state->parseIndex, state->length);
  state->parseIndex += 1;

  if (type == EMBER_AF_GBZ_MESSAGE_ALERT) {
    state->alertCode = emberAfGetInt16u(state->command, state->parseIndex, state->length);
    state->parseIndex += 2;
    state->alertTimestamp = emberAfGetInt32u(state->command, state->parseIndex, state->length);
    state->parseIndex += 4;
  }

  return TRUE;
}

void emberAfPluginGbzMessageControllerParserCleanup(EmberAfGbzMessageParserState * state)
{
  if (state->freeRequired) {
    free(state->command);
  }
  MEMSET(state, 0x00, sizeof(EmberAfGbzMessageParserState));
}

int16u emberAfPluginGbzMessageControllerCreatorInit(EmberAfGbzMessageCreatorState * state,
                                                     EmberAfGbzMessageType type,
                                                     int16u alertCode,
                                                     int32u timestamp,
                                                     int16u messageCode,
                                                     int8u * gbzCommand, 
                                                     int16u gbzCommandLength)
{
  int8u totalByteWritten = 0;
  if (state == NULL){ return 0; }

  // init
  MEMSET(state, 0x00, sizeof(EmberAfGbzMessageCreatorState));

  if (gbzCommand == NULL){ //inits for appends that requires mem allocation
    // inits
    int8u headerSize = 0;
    int8u * headerPayload = NULL;
    int8u headerWriteIndex = 0;

    state->allocateMemoryForResponses = TRUE;
    state->responses = NULL;

    if (type == EMBER_AF_GBZ_MESSAGE_ALERT){
      headerSize = GAS_PROXY_FUNCTION_GBZ_MESSAGE_ALERT_HEADER_LENGTH;
    } else {
      headerSize = GAS_PROXY_FUNCTION_GBZ_MESSAGE_COMMAND_HEADER_LENGTH;
    }

    // allocate mem for the header struct / header payload
    state->header = (EmAfGbzPayloadHeader *)malloc(sizeof(EmAfGbzPayloadHeader));
    if (state->header == NULL){
      emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: GBZ creator unable to allocate memory!");
      return 0;
    }

    state->header->payload = (int8u *)malloc(sizeof(int8u)*headerSize);
    if (state->header == NULL){
      emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: GBZ creator unable to allocate memory!");
      return 0;
    }
    state->header->payloadLength = headerSize;
    headerPayload = state->header->payload;

    emberAfCopyInt16u(headerPayload, headerWriteIndex, SE_PROFILE_ID);
    headerWriteIndex += 2;

    emberAfCopyInt8u(headerPayload, headerWriteIndex, 0);
    state->componentsCount = &headerPayload[headerWriteIndex];
    headerWriteIndex += 1;
    if (type == EMBER_AF_GBZ_MESSAGE_ALERT) {
      emberAfCopyInt16u(headerPayload, headerWriteIndex, alertCode);
      headerWriteIndex += 2;
      emberAfCopyInt32u(headerPayload, headerWriteIndex, timestamp);
      headerWriteIndex += 4;
    }

    totalByteWritten = headerSize;
  } else {
    // bookkeeping inits
    state->allocateMemoryForResponses = FALSE;
    state->responses = NULL;

    MEMSET(gbzCommand, 0, gbzCommandLength);
    state->command = gbzCommand;
    state->commandLength = gbzCommandLength;
    state->commandIndex = 0;
    state->nextEncryptedComponentZclSequenceNumber = 0;

    emberAfCopyInt16u(state->command, state->commandIndex, SE_PROFILE_ID);
    state->commandIndex += 2;

    emberAfCopyInt8u(state->command, state->commandIndex, 0);
    state->componentsCount = &state->command[state->commandIndex];
    state->commandIndex += 1;
    if (type == EMBER_AF_GBZ_MESSAGE_ALERT) {
      emberAfCopyInt16u(state->command, state->commandIndex, alertCode);
      state->commandIndex += 2;
      emberAfCopyInt32u(state->command, state->commandIndex, timestamp);
      state->commandIndex += 4;
    }

    totalByteWritten = state->commandIndex;
  }

  state->nextComponentZclSequenceNumber = 0;
  state->nextAdditionalHeaderFrameCounter = 0;
  state->lastExtHeaderControlField = NULL;
  state->messageCode = messageCode;
  return totalByteWritten;
}

int16u emberAfPluginGbzMessageControllerAppendCommand(EmberAfGbzMessageCreatorState * creator,
                                                      EmberAfGbzZclCommand * zclCmd)
{
  EmberAfGbzMessageData data = {0};
  int16u result;
  
  if (creator == NULL || zclCmd == NULL){
    emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: Unable to append due to NULL pointer!");
    return 0;
  }

  // test encryption mechanism as sanity check before modifying any
  // internal bookkeeping attributes.
  data.encryption = FALSE;
  data.plainData = zclCmd->payload;
  data.plainDataLength = zclCmd->payloadLength;
  
  if (emberAfPluginGbzMessageControllerGetEncryptPayloadFlag(creator, zclCmd)){
    emberAfPluginGbzMessageControllerEncryptDataCallback(&data);
    if (!data.encryption){
      emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: Failed to encrypt following ZCL payload: ");
      emberAfPluginGbzMessageControllerPrintBuffer(zclCmd->payload, zclCmd->payloadLength, TRUE);
      emberAfPluginGbzMessageControllerPrintln("");
      return 0;
    }
    zclCmd->encryption = TRUE;
  } else {
    zclCmd->encryption = FALSE;
  }

  if (creator->allocateMemoryForResponses){
    result = emAfPluginGbzMessageControllerAppendInNewMem(creator, 
                                                          zclCmd,
                                                          &data);
  } else {
    result = emAfPluginGbzMessageControllerAppendInPlace(creator, 
                                                         zclCmd,
                                                         &data);
  }
  return result;
}

EmberAfGbzMessageCreatorResult * emberAfPluginGbzMessageControllerCreatorAssemble(EmberAfGbzMessageCreatorState * state){
  int16u len = 0;

  EmberAfGbzMessageCreatorResult  * result = &state->result;
  
  result->freeRequired = FALSE;
  result->payloadLength = 0;
  result->payload = NULL;

  if (state->allocateMemoryForResponses){
    // get total length from all responses
    // allocate memory
    // stuff each response
    // into the main payload
    // free the responses
    int8u * responsePayload;
    if (state->header == NULL){
      emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: Header not intialized! Unable to combine responses.");
      result->payloadLength = 0;
      return NULL;
    } 

    // count overall length
    len = state->header->payloadLength;
    len += emAfPluginGbzMessageControllerGetResponsesLength(state->responses);
  
    responsePayload = (int8u *)malloc(sizeof(int8u) * len);
    if (responsePayload == NULL){
      emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: Unable to allocate memory!");
      result->payloadLength= 0;
      return NULL;
    } else {
      result->freeRequired = TRUE;
      result->payload = responsePayload;
      result->payloadLength = len;
    }

    // copy over payload
    MEMCOPY(responsePayload, state->header->payload, state->header->payloadLength);
    emAfPluginGbzMessageControllerAppendResponses(&responsePayload[state->header->payloadLength],
                                                  len - state->header->payloadLength, 
                                                  state->responses);

    // clean up
    emAfPluginGbzMessageControllerCreatorFreeResponses(state);
    state->lastResponse = NULL;
    emAfPluginGbzMessageControllerFreeHeader(state->header);
  } else {
    result->payload = state->command;
    result->payloadLength = state->commandIndex;
    result->freeRequired = FALSE;
  }

  return &state->result;
}

void emberAfPluginGbzMessageControllerCreatorCleanup(EmberAfGbzMessageCreatorState * state){
  // Try to free up respones as well in case user never called Assemble().
  if (state->allocateMemoryForResponses){
    emAfPluginGbzMessageControllerCreatorFreeResponses(state);
  }

  if (state->result.freeRequired){
    free(state->result.payload);
  }
  MEMSET(state, 0x00, sizeof(EmberAfGbzMessageCreatorState));
}

static int16u emAfPluginGbzMessageControllerAppendInNewMem(EmberAfGbzMessageCreatorState * state,
                                                           EmberAfGbzZclCommand * zclCmd,
                                                           EmberAfGbzMessageData * data)
{
  int16u len = emAfPluginGbzMessageControllerGetLength(zclCmd);
  int16u index = 0;

  if (len > 0){
    EmAfGbzUseCaseSpecificComponent * comp;
    int8u * payload;
    comp = (EmAfGbzUseCaseSpecificComponent*) malloc(sizeof(EmAfGbzUseCaseSpecificComponent));
    comp->payload = payload = (int8u *) malloc(sizeof(int8u)*len);
    comp->payloadLength = len;
    comp->next = NULL;

    // mark old "last component" as no longer the last component.
    if (state->lastExtHeaderControlField != NULL) {
      *(state->lastExtHeaderControlField) &= ~GAS_PROXY_FUNCTION_GBZ_COMPONENT_LAST_MSG_MASK;
    }

    *state->componentsCount += 1;

    //Extended Header Control Field
    state->lastExtHeaderControlField = payload + index++;

    if (zclCmd->encryption){
      *(state->lastExtHeaderControlField) = EMBER_AF_GBZ_LAST_ENCRYPTED_MESSAGE;
    } else {
      *(state->lastExtHeaderControlField) = EMBER_AF_GBZ_LAST_UNENCRYPTED_MESSAGE;
    }

    //Extended Header Cluster ID
    emberAfCopyInt16u(payload, index, zclCmd->clusterId);
    index += 2;

    //Extended Header GBZ Command Length
    emberAfCopyInt16u(payload,
                      index,
                      emAfPluginGbzMessageControllerGetZclLength(zclCmd));
    index += 2;

    if (data->encryption){
      // Additional Header Control
      payload[index++] = 0x00; // reserved value

      // Additional Header Frame Counter
      payload[index++] = state->nextAdditionalHeaderFrameCounter++;
    }

    // ZCL header
    payload[index++] = zclCmd->frameControl;
    payload[index++] = state->nextComponentZclSequenceNumber++;
    payload[index++] = zclCmd->commandId;

    // ZCL payload
    if (data->encryption){ 
      // data should be encrypted payloads by now.
      emberAfCopyInt16u(payload,
                        index,
                        data->encryptedDataLength);
      index += 2;
      MEMCOPY(&payload[index],
              data->encryptedData,
              data->encryptedDataLength);
      index += data->encryptedDataLength;

      free(data->encryptedData);
      data->encryptedData = NULL;
    } else {
      MEMCOPY(&payload[index], zclCmd->payload, zclCmd->payloadLength);
      index += zclCmd->payloadLength;
    }
    
    // append linklist node to end 
    if (state->responses == NULL){
      state->responses = comp;
      state->lastResponse = comp;
    } else {
      state->lastResponse->next = comp;
      state->lastResponse = comp;
    }
  }
  
  return len;
}


static int16u emAfPluginGbzMessageControllerAppendInPlace(EmberAfGbzMessageCreatorState * state,
                                                         EmberAfGbzZclCommand * zclCmd,
                                                         EmberAfGbzMessageData * data){
  // will new cmd fit into the current gbz message struct?
  int16u zclCommandLength;
  int16u numberOfAppendedBytes = state->commandIndex; // snapshot of state->commandIndex
  int16u * extHeaderGbzCmdLen = NULL;

  zclCommandLength = emAfPluginGbzMessageControllerGetLength(zclCmd);

  if (state->commandLength < state->commandIndex + zclCommandLength){
    emberAfPluginGbzMessageControllerPrintln("GBZ: ERR: Unable to append ZCL command to GBZ message: out of space (%d/%d).",
                                             state->commandIndex + zclCommandLength,
                                             state->commandLength);
    return 0;
  }

  // mark old "last component" as no longer the last component.
  if (state->lastExtHeaderControlField != NULL) {
    *(state->lastExtHeaderControlField) &= ~GAS_PROXY_FUNCTION_GBZ_COMPONENT_LAST_MSG_MASK;
  }

  // Component Size
  *state->componentsCount += 1;

  // point lastExtHeaderControlField to the new header control field.
  state->lastExtHeaderControlField = state->command + state->commandIndex++;

  // Extended Header Control Field
  if (zclCmd->encryption){
    *(state->lastExtHeaderControlField) = EMBER_AF_GBZ_LAST_ENCRYPTED_MESSAGE;
  } else {
    *(state->lastExtHeaderControlField) = EMBER_AF_GBZ_LAST_UNENCRYPTED_MESSAGE;
  }

  if (zclCmd->hasFromDateTime){
    *(state->lastExtHeaderControlField) |= GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_MASK;
  }

  // allocate extended header cluster id
  emberAfCopyInt16u(state->command, state->commandIndex, zclCmd->clusterId);
  state->commandIndex += 2;

  // allocate extended header command length
  emberAfCopyInt16u(state->command,
                    state->commandIndex,
                    emAfPluginGbzMessageControllerGetCommandLength(zclCmd));
  state->commandIndex += 2;

  if (zclCmd->encryption){
    // append additional header control
    state->command[state->commandIndex++] = 0x00;

    // append additional header frame counter
    state->command[state->commandIndex++] = state->nextEncryptedComponentZclSequenceNumber++;
  } else if (zclCmd->hasFromDateTime){
    // append "From Date Time" field
    emberAfCopyInt32u(state->command,
                      state->commandIndex,
                      zclCmd->fromDateTime);
    state->commandIndex += GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_LENGTH;
  }

  state->command[state->commandIndex++] = zclCmd->frameControl;
  state->command[state->commandIndex++] = state->nextComponentZclSequenceNumber++;
  state->command[state->commandIndex++] = zclCmd->commandId;

  if (zclCmd->encryption){ 
    // append "Length of Ciphered information"
    emberAfCopyInt16u(state->command,
                      state->commandIndex,
                      zclCmd->payloadLength);
    state->commandIndex += GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_CIPHERED_INFO_LENGTH;
  }

  // ZCl payload
  MEMCOPY(&state->command[state->commandIndex],
          zclCmd->payload,
          zclCmd->payloadLength);

    free(data->encryptedData);
    data->encryptedData = NULL;
  state->commandIndex += zclCmd->payloadLength;
  
  numberOfAppendedBytes = state->commandIndex - numberOfAppendedBytes;
  return numberOfAppendedBytes;
}

EmberAfStatus emberAfPluginGbzMessageControllerGetZclDefaultResponse(EmberAfGbzZclCommand * cmd){
  if (cmd->clusterSpecific){
    return EMBER_ZCL_STATUS_SUCCESS;
  } else if ((!cmd->clusterSpecific) && cmd->payloadLength > 1) {
    return (EmberAfStatus) cmd->payload[1];
  } else {
    return EMBER_ZCL_STATUS_FAILURE;
  }
}

static void emAfPluginGbzMessageControllerCreatorFreeResponses(EmberAfGbzMessageCreatorState * state){
  EmAfGbzUseCaseSpecificComponent * next;
  EmAfGbzUseCaseSpecificComponent * cur = state->responses;

  while(cur != NULL){
    next = cur->next;
    free(cur->payload);
    cur->payloadLength = 0;
    cur->next = NULL;
    free(cur);
    cur = next;
  }
  
  state->responses = NULL;
}

/* 
 * @brief Cleanup dynamic memory used to store GBZ message header.
 * */
void emAfPluginGbzMessageControllerFreeHeader(EmAfGbzPayloadHeader * header){
  if (header == NULL){
    return;
  }

  free(header->payload);
  free(header);
}


/*  
 *  @brief Total storage needed to store all Use Case Specific Components.
 *
 *  @return number of bytes
 *
 */
int16u emAfPluginGbzMessageControllerGetResponsesLength(EmAfGbzUseCaseSpecificComponent * comp){
  int16u length = 0;
  EmAfGbzUseCaseSpecificComponent * next;
  EmAfGbzUseCaseSpecificComponent * cur = comp;
  while(cur != NULL){
    next = cur->next;
    length += cur->payloadLength;
    cur = next;
  }
  return length;
}

/*
 * @brief returns number of appended bytes
 */
static int16u emAfPluginGbzMessageControllerAppendResponses(int8u * dst,
                                                     int16u maxLen, 
                                                     EmAfGbzUseCaseSpecificComponent * responses ){
  int16u index = 0;
  EmAfGbzUseCaseSpecificComponent * next;
  EmAfGbzUseCaseSpecificComponent * cur = responses;
  while(cur != NULL){
    next = cur->next;
    
    if (index + cur->payloadLength <= maxLen){
      MEMCOPY(&dst[index], cur->payload, cur->payloadLength);
      index += cur->payloadLength;
    }

    cur = next;
  }
  return index;
}

/*
 * @ Get ZCL portions (header & payload) of the GBZ messages' command length
 */
static int16u emAfPluginGbzMessageControllerGetZclLength(EmberAfGbzZclCommand * cmd){
  int16u len = emAfPluginGbzMessageControllerGetLength(cmd);
  if (len != 0){
    return len - GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_FIELDS_LENGTH;
  } else {
    return len;
  }
}

boolean emberAfPluginGbzMessageControllerGetEncryptPayloadFlag(EmberAfGbzMessageCreatorState * creator, 
                                                               EmberAfGbzZclCommand * resp){
  int8u * commandId =  &(resp->commandId);
  int16u * clusterId = &(resp->clusterId);

  if (creator->messageCode == TEST_ENCRYPTED_MESSAGE_CODE){ return TRUE; }
  if (creator->messageCode == TEST_MESSAGE_CODE){ return FALSE; }


  if (!resp->clusterSpecific){
    // we do not encrypt default responses.
    if (*commandId == ZCL_DEFAULT_RESPONSE_COMMAND_ID){
      return FALSE;
    } else if (*commandId == ZCL_READ_ATTRIBUTES_RESPONSE_COMMAND_ID){
      if ((creator->messageCode == GCS13a_MESSAGE_CODE)
          || (creator->messageCode == GCS13b_MESSAGE_CODE)
          || (creator->messageCode == GCS13c_MESSAGE_CODE)){
        return TRUE;
      } else if (creator->messageCode == GCS14_MESSAGE_CODE){
        int16u bufIndex = 0;
        int16u bufLen = resp->payloadLength;
        int8u * buffer = resp->payload;

        if (resp->clusterId != ZCL_PREPAYMENT_CLUSTER_ID) { return FALSE; }

        // parse the response and act accordingly. 
        // we are doing the manual parsing here since a plugin specific 
        // callback has to parse the payload manually anyways.
        // Encrypt the payload if we see any of following attributes:
        //
        // Prepayment Info: Accumulated Debt
        // Prepayment Info: Emergency CreditRemaining
        // Prepayment Info: Credit Remaining
        // Payment-based Debt
        // Time-based Debt(1)
        // Time-based Debt(2)
        //
        // Each record in the response has a two-byte attribute id and a one-byte
        // status.  If the status is SUCCESS, there will also be a one-byte type and
        // variable-length data.

        while (bufIndex + 3 <= bufLen) {
          EmberAfStatus status;
          EmberAfAttributeId attributeId;
          attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer,
                                                             bufIndex,
                                                             bufLen);
          bufIndex += 2;
          status = (EmberAfStatus)emberAfGetInt8u(buffer, bufIndex, bufLen);
          bufIndex++;
          if (status == EMBER_ZCL_STATUS_SUCCESS) {
            int8u dataSize;
            int8u dataType = emberAfGetInt8u(buffer, bufIndex, bufLen);
            bufIndex++;

            // For strings, the data size is the length of the string (specified by
            // the first byte of data) plus one for the length byte itself.  For
            // everything else, the size is just the size of the data type.
            dataSize = (emberAfIsThisDataTypeAStringType(dataType)
                ? emberAfStringLength(buffer + bufIndex) + 1
                : emberAfGetDataSize(dataType));

            if (bufIndex + dataSize <= bufLen) {
              if (attributeId == ZCL_DEBT_AMOUNT_1_ATTRIBUTE_ID
                  || (attributeId == ZCL_DEBT_AMOUNT_2_ATTRIBUTE_ID)
                  || (attributeId == ZCL_DEBT_AMOUNT_3_ATTRIBUTE_ID)){
                  return TRUE;
                }
            }
            bufIndex += dataSize;
          }
        }        
      }
      return FALSE;
    }
  } else {
    switch(creator->messageCode) {
      case GCS15b_MESSAGE_CODE:
        if ((*clusterId == ZCL_PREPAYMENT_CLUSTER_ID) && 
            (*commandId == ZCL_PUBLISH_PREPAY_SNAPSHOT_COMMAND_ID)) {
          return TRUE;
        } else if ((*clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID) && 
            (*commandId == ZCL_PUBLISH_SNAPSHOT_COMMAND_ID)) {
          return TRUE;
        }
        break;
      case GCS15c_MESSAGE_CODE:
        if ((*clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID) && 
            (*commandId == ZCL_PUBLISH_SNAPSHOT_COMMAND_ID)) {
          return TRUE;
        }
        break;
      case GCS15d_MESSAGE_CODE:
        if ((*clusterId == ZCL_PREPAYMENT_CLUSTER_ID)
            && (*commandId == ZCL_PUBLISH_DEBT_LOG_COMMAND_ID)){
          return TRUE;
        }
        break;
      case GCS16a_MESSAGE_CODE:
        if ((*clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID)
            && (*commandId == ZCL_PUBLISH_SNAPSHOT_COMMAND_ID)){
          return TRUE;
        }
        break;
      case GCS16b_MESSAGE_CODE:
        if ((*clusterId == ZCL_PREPAYMENT_CLUSTER_ID)
            && (*commandId == ZCL_PUBLISH_PREPAY_SNAPSHOT_COMMAND_ID)){
          return TRUE;
        }
        break;
      case GCS17_MESSAGE_CODE:
        if ((*clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID)
            && (*commandId == ZCL_GET_SAMPLED_DATA_RESPONSE_COMMAND_ID)){
          return TRUE;
        }
        break;
      case GCS53_MESSAGE_CODE:
        if ((*clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID)
            && (*commandId == ZCL_PUBLISH_SNAPSHOT_COMMAND_ID)){
          return TRUE;
        }
        break;
      case GCS61_MESSAGE_CODE:
        if ((*clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID)
            && (*commandId == ZCL_GET_SAMPLED_DATA_RESPONSE_COMMAND_ID)){
          return TRUE;
        }
        break;
    }
  }

  return FALSE;
}

