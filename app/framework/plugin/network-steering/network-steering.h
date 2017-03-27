

extern PGM int8u emAfNetworkSteeringPluginName[];

extern PGM_P emAfPluginNetworkSteeringStateNames[];
extern int8u emAfPluginNetworkSteeringPanIdIndex;
extern int8u emAfPluginNetworkSteeringCurrentChannel;
extern int8u emAfPluginNetworkSteeringTotalBeacons;
extern int8u emAfPluginNetworkSteeringJoinAttempts;

int8u emAfPluginNetworkSteeringGetMaxPossiblePanIds(void);
void emAfPluginNetworkSteeringClearStoredPanIds(void);
int16u* emAfPluginNetworkSteeringGetStoredPanIdPointer(int8u index);

extern int32u emAfPluginNetworkSteeringPrimaryChannelMask;
extern int32u emAfPluginNetworkSteeringSecondaryChannelMask;

EmberStatus emberAfPluginNetworkSteeringStart(void);

void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status, 
                                                  int8u totalBeacons,
                                                  int8u joinAttempts,
                                                  boolean defaultKeyUsed);

typedef int8u EmberAfPluginNetworkSteeringJoiningState;
enum {
  EMBER_AF_NETWORK_STEERING_STATE_NONE                             = 0,
  EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_INSTALL_CODE    = 1,
  EMBER_AF_NETWORK_STEERING_STATE_SCAN_SECONDARY_FOR_INSTALL_CODE  = 2,
  EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_DEFAULT_KEY     = 3,
  EMBER_AF_NETWORK_STEERING_STATE_SCAN_SECONDARY_FOR_DEFAULT_KEY   = 4,
};

extern EmberAfPluginNetworkSteeringJoiningState emAfPluginNetworkSteeringState;

EmberStatus emberAfPluginNetworkSteeringStop(void);
