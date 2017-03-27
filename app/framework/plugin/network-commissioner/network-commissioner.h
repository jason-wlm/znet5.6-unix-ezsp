//
// network-commissioner.h
//
// Andrew Keesler
//
// January 2, 2015
//
// Interface for driving the network commissioning process as specified by
// the Base Device specification (doc 13-0402).
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#ifndef __NETWORK_COMMISSIONER_H__
#define __NETWORK_COMMISSIONER_H__

#include "af.h"

#define EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_PLUGIN_NAME "Network Commissioner"

// -----------------------------------------------------------------------------
// Public API

/** @brief Start the network commissioner process.
 *
 * @param endpoint The endpoint on which to perform the commissioning.
 *
 * A call to this function will kick off a network commissioning process.
 */
EmberStatus emberAfPluginNetworkCommissionerStart(int8u endpoint);

#endif /* __NETWORK_COMMISSIONER_H__ */
