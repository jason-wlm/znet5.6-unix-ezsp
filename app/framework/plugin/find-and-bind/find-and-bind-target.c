//
// find-and-bind-target.c
//
// Author: Andrew Keesler
//
// Target functionality as described in the Base Device Behavior
// spec.
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#include "app/framework/include/af.h"

#include "find-and-bind.h"

// -----------------------------------------------------------------------------
// Public API

EmberAfStatus emberAfPluginFindAndBindTarget(int8u endpoint)
{
  // Write the identify time.
  int8u identifyTime = EMBER_AF_PLUGIN_FIND_AND_BIND_COMMISSIONING_TIME;
  EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

  if (emberAfContainsCluster(endpoint, ZCL_IDENTIFY_CLUSTER_ID))
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_IDENTIFY_CLUSTER_ID,
                                         ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                                         (int8u *)&identifyTime,
                                         ZCL_INT16U_ATTRIBUTE_TYPE);

  return status;
}
