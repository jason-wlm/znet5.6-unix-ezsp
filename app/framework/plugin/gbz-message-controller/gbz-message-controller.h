// *******************************************************************
// * gbz-message-controller.h
// *
// *
// * Copyright 2014 by Silicon Laboratories, Inc. All rights reserved.      *80*
// *******************************************************************

// debug prints
#define emberAfPluginGbzMessageControllerPrint(...)    emberAfAppPrint(__VA_ARGS__)
#define emberAfPluginGbzMessageControllerPrintln(...)  emberAfAppPrintln(__VA_ARGS__)
#define emberAfPluginGbzMessageControllerDebugExec(x)  emberAfAppDebugExec(x)
#define emberAfPluginGbzMessageControllerPrintBuffer(buffer, len, withSpace) emberAfAppPrintBuffer(buffer, len, withSpace)


// offsets
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_CONTROL_FIELD_OFFSET  (0)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_CLUSTER_ID_OFFSET     (2)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_GBZ_CMD_LENGTH_OFFSET (3)

#define GAS_PROXY_FUNCTION_GBZ_MESSAGE_COMMAND_HEADER_LENGTH              (3)
#define GAS_PROXY_FUNCTION_GBZ_MESSAGE_RESPONSE_HEADER_LENGTH             (3)
#define GAS_PROXY_FUNCTION_GBZ_MESSAGE_ALERT_HEADER_LENGTH                (9)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_FIELDS_LENGTH         (5)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_ZCL_HEADER_LENGTH                (3)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_LENGTH            (4)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_HEADER_FIELDS_LENGTH  (2)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_CIPHERED_INFO_LENGTH  (2)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_EXT_HEADER_FIELDS_LENGTH         (5)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_HEADER_FIELDS_LENGTH  (2)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_LENGTH            (4)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTION_CIPHERED_INFO_LENGTH  (2)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_LAST_MSG_MASK       (0x01)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_ENCRYPTED_MSG_MASK  (0x02)
#define GAS_PROXY_FUNCTION_GBZ_COMPONENT_FROM_DATE_TIME_MASK (0x10)

// Forward declaration
EmberAfStatus emberAfClusterSpecificCommandParse(EmberAfClusterCommand *cmd);

typedef enum {
  EMBER_AF_GBZ_NOT_LAST_UNENCRYPTED_MESSAGE = 0x00,
  EMBER_AF_GBZ_LAST_UNENCRYPTED_MESSAGE = 0x01,
  EMBER_AF_GBZ_NOT_LAST_ENCRYPTED_MESSAGE = 0x02,
  EMBER_AF_GBZ_LAST_ENCRYPTED_MESSAGE = 0x03,
} EmberAfGbzExtendedHeaderControlField;

typedef enum {
  EMBER_AF_GBZ_MESSAGE_COMMAND,
  EMBER_AF_GBZ_MESSAGE_RESPONSE,
  EMBER_AF_GBZ_MESSAGE_ALERT
} EmberAfGbzMessageType;

typedef struct {
  EmberAfClusterId  clusterId;
  int8u frameControl;
  int8u transactionSequenceNumber;
  int8u commandId;
  int8u * payload;
  int16u payloadLength;
  int8u direction;
  boolean clusterSpecific;
  boolean mfgSpecific;


  int32u fromDateTime;
  boolean hasFromDateTime; // Zigbee UTC Time.
  boolean encryption;
} EmberAfGbzZclCommand;

typedef struct {
  boolean freeRequired;
  int8u * command;
  EmberAfGbzMessageType type;
  int16u alertCode;
  int32u alertTimestamp;
  int16u profileId;
  int8u nextComponentZclSequenceNumber;
  int8u componentsSize;
  int8u componentsParsed;
  int16u parseIndex; // index to the next byte for parsing.
  int16u length;
  int16u messageCode; // "Message Code" for the corresponding Non TOM Command.
} EmberAfGbzMessageParserState;

typedef struct {
  int8u * payload;
  int8u payloadLength;
} EmAfGbzPayloadHeader;



/*
 * @brief a link list that keeps track of raw data that represents appended 
 *        GBZ Use Case Specific components.
 */
struct EmAfGbzUseCaseSpecificComponent{
  int8u * payload;
  int16u payloadLength;
  struct EmAfGbzUseCaseSpecificComponent * next;
};

typedef struct EmAfGbzUseCaseSpecificComponent EmAfGbzUseCaseSpecificComponent;

typedef struct {
  int8u * payload;
  int16u payloadLength;
  boolean freeRequired;
} EmberAfGbzMessageCreatorResult;

typedef struct {
  boolean allocateMemoryForResponses;

  // used when allocateMemoryForResponses is FALSE
  int8u * command;
  int16u commandIndex; // index to the next byte for appending.
  int16u commandLength;

  // used when allocateMemoryForResponses is TRUE
  EmAfGbzUseCaseSpecificComponent * responses;
  EmAfGbzUseCaseSpecificComponent * lastResponse;
  EmAfGbzPayloadHeader * header;

  // otherwise.
  int8u nextEncryptedComponentZclSequenceNumber;
  int8u nextComponentZclSequenceNumber;
  int8u nextAdditionalHeaderFrameCounter;
  int8u * componentsCount;
  int8u * lastExtHeaderControlField;
  int16u messageCode;
  
  // assembled result
  EmberAfGbzMessageCreatorResult result;

} EmberAfGbzMessageCreatorState;

/*
 * @brief Assemble appended ZCL responses into 1 big chunk of memory. 
 *
 */
EmberAfGbzMessageCreatorResult * emberAfPluginGbzMessageControllerCreatorAssemble(EmberAfGbzMessageCreatorState * state);

/*
 * @brief Grab the default response byte from the GBZ ZCL Command.
 *
 * @param ZCL status
 */
EmberAfStatus emberAfPluginGbzMessageControllerGetZclDefaultResponse(EmberAfGbzZclCommand * cmd);

/*
 * @brief Check the GBZ parser struct for any non-parsed commands.
 *
 * @param state struct containing the bookkeeping information of parsing GBZ messages
 */
boolean emberAfPluginGbzMessageControllerHasNextCommand(EmberAfGbzMessageParserState * state);

/*
 * @brief This function cleans any on resources allocated during the parsing of GBZ message.
 *
 * @param state a pre-allocated struct that will be updated to hold
 *   bookkeping information of parsing GBZ messages
 */
void emberAfPluginGbzMessageControllerParserCleanup(EmberAfGbzMessageParserState * state);

/*
 * @brief This function initializes proper parsing states for decoding of GBZ messages.
 *
 * The gbz message payload and payload length will be passed in as 
 * arguments. Iterator functions, emberAfPluginGbzMessageControllerHasNextCommand() and
 * emberAfPluginGbzMessageControllerNextCommand() will be used to iterate through
 * each of the embedded ZCL functions.
 *
 * @param state            a pre-allocated struct that will be updated to hold
 *                         bookkeeping information for parsing GBZ messages
 * @param gbzCommand       pointer to gbz messages
 * @param gbzCommandLength length of the gbz messages
 * @param copyGbzCommand   flag to indicate if parser should be storing the gbz
 *                         command locally for parsing.
 * @param messageCode      "Message Code" for the corresponding Non TOM command.
 *
 */
boolean emberAfPluginGbzMessageControllerParserInit(EmberAfGbzMessageParserState * state,
                                                    EmberAfGbzMessageType type,
                                                    int8u * gbzCommand, 
                                                    int16u gbzCommandLength,
                                                    boolean copyGbzCommand, 
                                                    int16u messageCode);

/*
 * @brief Append ZCL command to a given GBZ creator struct.
 * 
 * 
 * @param state a pre-allocated struct that will be updated to hold
 *   bookkeping information of creating GBZ messages
 * @param zclCmd struct containing information for new zcl cmd.
 *
 *  @return 0 - if the appending operation did not succeed
 *          else - number of appended bytes
 */ 
int16u emberAfPluginGbzMessageControllerAppendCommand(EmberAfGbzMessageCreatorState * state, EmberAfGbzZclCommand * zclCmd);

/* 
 * @brief This function initializes proper states for construction of GBZ messages.
 *
 * Depending on the value of the argument (gbzCommand), the creator will
 * behave differently. If a NULL value is passed, the creator assumes the
 * user wants the API to allocate memory to store the appended responses.
 * Otherwise, the creator will use the provided buffer as the destination for 
 * storing responses.
 *
 * Below is a general flow for the creation of ZCL messages into a GBZ
 * message. 
 *
 *  1. emberAfPluginGbzMessageControllerCreatorInit() - create
 *  2. emberAfPluginGbzMessageControllerAppendCommand() - append
 *  3. emberAfPluginGbzMessageControllerCreatorAssemble() - assemble result.
 *  4. emberAfPluginGbzMessageControllerCreatorCleanup() - memory clean up
 *
 * @param state      A pre-allocated struct that will be updated to hold
 *                   bookkeping information for creating GBZ messages
 * @param type       GBZ payload type: command, response, or alert
 * @param alertCode  When type is alert this field contains the alert code
 * @param timestamp  When type is alert this field contains the UTC when the alert occurred
 * @param gbzCommand NULL - if the user wants API to allocate memory to store
 *                          respones
 *                   Otherwsie - pointer to destination buffer for gbz messages
 * @param gbzCommandLength length of the gbz messages. this argument is
 *                         ignored if gbzCommand is NULL
 *
 */
int16u emberAfPluginGbzMessageControllerCreatorInit(EmberAfGbzMessageCreatorState * state,
                                                    EmberAfGbzMessageType type,   
                                                    int16u alertCode,
                                                    int32u timestamp,
                                                    int16u messageCode,
                                                    int8u * gbzCommand,
                                                    int16u gbzCommandLength); 

/*
 * @bref Return commands size of the given parser struct 
 *
 * @param state struct containing the bookkeeping information of parsing GBZ messages
 */

int8u emberAfPluginGbzMessageControllerGetComponentSize(EmberAfGbzMessageParserState * state);

/* 
 * @brief This function will cleanup/free all the allocated memory used to
 *        store the overall GBZ response
 */
void emberAfPluginGbzMessageControllerCreatorCleanup(EmberAfGbzMessageCreatorState * state);

/*  
 * @brief Get the next available ZCL command from the given GBZ parser struct.
 *
 * If any payload is encrypted, the decrypted data will overwrite the old
 * data.
 *
 * @param state a struct that rtains the bookkeeping info of parsing GBZ messages 
 * @param gbzZclCommand a pre-allocated buffer that will be modified with the
 *   next available ZCL command's information
 */
void emberAfPluginGbzMessageControllerNextCommand(EmberAfGbzMessageParserState * state, EmberAfGbzZclCommand * gbzZclCommand);

/*
 * @brief Print out all information retained in a EmberAfGbzZclCommand struct.
 */
void emberAfPluginGbzMessageControllerPrintCommandInfo(EmberAfGbzZclCommand  * gbzZclCommand);

/*
 * @ Get the length of the overall GBZ message
 */
int16u emAfPluginGbzMessageControllerGetLength(EmberAfGbzZclCommand * cmd);

/*
 * @ brief tells us whether we be encrypting the zcl payload or not
 */
boolean emberAfPluginGbzMessageControllerGetEncryptPayloadFlag(EmberAfGbzMessageCreatorState * state, 
                                                               EmberAfGbzZclCommand * resp);
