// *******************************************************************
// * tunnel-manager.h
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*
// *******************************************************************

#ifndef TUNNEL_MANAGER_H_
#define TUNNEL_MANAGER_H_

#define EMBER_AF_PLUGIN_COMMS_HUB_FUNCTION_TUNNEL_LIMIT \
  (EMBER_AF_PLUGIN_TUNNELING_CLIENT_TUNNEL_LIMIT + EMBER_AF_PLUGIN_TUNNELING_SERVER_TUNNEL_LIMIT)
#define EM_AF_PLUGIN_COMMS_HUB_FUNCTION_NULL_TUNNEL_INDEX 0xFF

/**
 * @brief Initialize internal data structures.
 *
 * This function should be called from the plugin init callback.
 *
 * @param localEndpoint The local endpoint from which all tunnels will be
 * created.  It is also the endpint to which the GSME will request a tunnel.
 */
void emAfPluginCommsHubFunctionTunnelInit(int8u localEndpoint);

/**
 * @brief Create a tunnel to a device of type ESME, HCALCS or PPMID.
 *
 * As defined in section 10.2.2.1 of the GBCS version 0.8 and copied below the
 * CHF will call this function to request a tunnel be created with the specified
 * destination.
 *
 * "When a Communications Hub has successfully established a shared secret key
 * using CBKE with a Device of type ESME, HCALCS or PPMID, the CHF shall send a
 * RequestTunnel command to the Device to request a tunnel association with the
 * Device.
 *
 * Where an ESME, a HCALCS or a PPMID remains in the CHF Device Log, the CHF
 * shall send a RequestTunnel command to the Device whenever:
 *  -  0xFFFF seconds have elapsed since receipt of the most recent
 *     RequestTunnelResponse command from that Device; or
 *  -  the CHF receives a Remote Party Message addressed to the Device but does
 *     not have a functioning tunnel association with the Device; or
 *  -  the CHF powers on.
 *
 *  Where the CHF receives a RequestTunnelResponse command from a Device with a
 *  TunnelStatus of 0x01 (Busy), the CHF shall send another RequestTunnel
 *  command three minutes later.
 *
 *  Where the CHF receives a RequestTunnelResponse command from a Device with a
 *  TunnelStatus of 0x02 (No More Tunnel IDs), the CHF shall send a CloseTunnel
 *  command for any TunnelID that may relate to an active tunnel association
 *  with that Device and, after receiving responses to all such commands, send
 *  another RequestTunnel command."
 *
 * @param remoteDeviceId The EUI64 of the device to which a tunnel is to be created
 * @param remoteEndpoint The remote endpoint to which the tunnel is to be created
 * @return TRUE if successful or FALSE if an error occurred
 */
boolean emAfPluginCommsHubFunctionTunnelCreate(EmberEUI64 remoteDeviceId,
                                               int8u remoteEndpoint);

/**
 * @brief Transfer data to a server through a tunnel.
 *
 * This function can be used to transfer data to a server through a tunnel.
 *
 * @param remoteDeviceId The EUI64 of the device to which data is to be sent
 * @param headerLen The length of any header that may prefix the data
 * @param header Buffer containing the raw octets of the header.
 * @param dataLen The length in octets of the data.
 * @param data Buffer containing the raw octets of the data.
 * @return TRUE if successful or FALSE if an error occurred
 */
boolean emAfPluginCommsHubFunctionTunnelSendData(EmberEUI64 remoteDeviceId,
                                                 int16u headerLen,
                                                 int8u *header,
                                                 int16u dataLen,
                                                 int8u *data);

/**
 * @brief Close a tunnel.
 *
 * This function can be used to close a tunnel.
 *
 * @param remoteDeviceId The EUI64 of the device to which the tunnel will be closed
 * @return TRUE if successful or FALSE if an error occurred
 */
boolean emAfPluginCommsHubFunctionTunnelDestroy(EmberEUI64 remoteDeviceId);

/**
 * @brief Cleanup a tunnel.
 *
 * This function can be used to cleanup all state associated with a tunnel.
 *
 * @param remoteDeviceId The EUI64 of the device to which the tunnel state
 * is to be cleaned up.
 */
void emAfPluginCommsHubFunctionTunnelCleanup(EmberEUI64 remoteDeviceId);

boolean emAfPluginCommsHubFunctionTunnelExists(EmberEUI64 deviceEui64);


#endif /* TUNNEL_MANAGER_H_ */
