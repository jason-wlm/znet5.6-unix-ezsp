
extern int8u emAfRouteErrorCount;
extern int8u emAfDeliveryFailureCount;

extern EmberEventControl emberAfPluginConcentratorUpdateEventControl;

#define LOW_RAM_CONCENTRATOR  EMBER_LOW_RAM_CONCENTRATOR
#define HIGH_RAM_CONCENTRATOR EMBER_HIGH_RAM_CONCENTRATOR

#define emAfConcentratorStartDiscovery emberAfPluginConcentratorQueueDiscovery
void emAfConcentratorStopDiscovery(void);


int32u emberAfPluginConcentratorQueueDiscovery(void);
void emberAfPluginConcentratorStopDiscovery(void);

/** @brief Starts periodic many-to-one route discovery.
 * Periodic discovery is started by default on bootup,
 * but this function may be used if discovery has been
 * stopped by a call to ::emberConcentratorStopDiscovery().
 */
void emberConcentratorStartDiscovery(void);

/** @brief Stops periodic many-to-one route discovery. */
void emberConcentratorStopDiscovery(void);
