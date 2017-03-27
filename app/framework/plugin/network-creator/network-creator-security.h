//
// network-creator-security.h
//
// Andrew Keesler
//
// September 30, 2014
//
// Compile-time security configuration for network-creator.
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#include "stack/include/ember-types.h"

#ifndef __NETWORK_CREATOR_SECURITY_H__
#define __NETWORK_CREATOR_SECURITY_H__

// -----------------------------------------------------------------------------
// Globals

extern boolean emAfPluginNetworkCreatorUseProfileInteropTestKey;

// -----------------------------------------------------------------------------
// Constants

// These are the global link keys that were used at the last profile interop.
// Defining them here for now so that they are easy to get to.
// TODO: remove these when we don't need them anymore.

#define PROFILE_INTEROP_CENTRALIZED_KEY           \
  {                                               \
    {0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,     \
     0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF}     \
  }

#define PROFILE_INTEROP_DISTRIBUTED_KEY           \
  {                                               \
    {0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,     \
     0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF}     \
  }

#endif /* __NETWORK_CREATOR_SECURITY_H__ */









