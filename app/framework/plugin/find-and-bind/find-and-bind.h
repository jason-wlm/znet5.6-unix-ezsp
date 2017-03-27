//
// find-and-bind.h
//
// Author: Andrew Keesler
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#ifndef __FIND_AND_BIND_H__
#define __FIND_AND_BIND_H__

// -----------------------------------------------------------------------------
// Constants

#define EMBER_AF_FIND_AND_BIND_PLUGIN_NAME "Find and Bind"

// -----------------------------------------------------------------------------
// API

/** @brief Start target finding and binding operations
 *
 * A call to this function will commence the target finding and
 * binding operations. Specifically, the target will attempt to start
 * identifying on the endpoint that is passed as a parameter.
 *
 * @param endpoint The endpoint on which to begin target operations.
 *
 * @returns An ::EmberAfStatus value describing the success of the
 * commencement of the target operations.
 */
EmberAfStatus emberAfPluginFindAndBindTarget(int8u endpoint);

/** @brief Start initiator finding and binding operations.
 *
 * A call to this function will commence the initiator finding and
 * binding operations. Specifically, the initiator will attempt to start
 * searching for potential bindings that can be made with identifying
 * targets.
 *
 * @param endpoint The endpoint on which to begin initiator operations.
 *
 * @returns An ::EmberStatus value describing the success of the
 * commencement of the initiator operations.
 */
EmberStatus emberAfPluginFindAndBindInitiator(int8u endpoint);

#endif /* __FIND_AND_BIND_H__ */
