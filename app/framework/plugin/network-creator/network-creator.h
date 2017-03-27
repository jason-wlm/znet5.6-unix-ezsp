//
// network-creator.h
//
// Author: Andrew Keesler
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*
//

#ifndef __NETWORK_CREATOR_H__
#define __NETWORK_CREATOR_H__

// -----------------------------------------------------------------------------
// Constants

#define EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME "NWK Creator"

// -----------------------------------------------------------------------------
// Globals

// TODO: get rid of these externs when the time is nigh.
extern int32u emAfPluginNetworkCreatorPrimaryChannelMask;
extern int32u emAfPluginNetworkCreatorSecondaryChannelMask;

// -----------------------------------------------------------------------------
// API

/** @brief Commands the network creator to form a network
 *  with the following qualities.
 *
 *  @param centralizedNetwork Whether or not to form a network
 *  using centralized security. If this argument is false, then
 *  a network with distributed security will be formed.
 *
 *  @return Status of the commencement of the network creator
 *  process.
 */
EmberStatus emberAfPluginNetworkCreatorForm(boolean centralizedNetwork);

/** @brief Stops the network creator form process.
 */
void emberAfPluginNetworkCreatorStop(void);

#endif /* __NETWORK_CREATOR_H__ */
