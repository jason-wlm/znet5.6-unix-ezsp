
// *******************************************************************
// * comms-hub-tunnel-endpoints.c
// *
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/common.h"
#include "comms-hub-tunnel-endpoints.h"

#define MAX_NUMBER_DEVICES            EMBER_AF_PLUGIN_GBCS_DEVICE_LOG_DEVICE_LOG_SIZE
#define MAX_DEVICE_TUNNEL_ENDPOINTS   3

typedef struct{
  int16u nodeId;
  int8u endpoint;
} EmAfTunnelEndpointMap;

static EmAfTunnelEndpointMap endpointMap[ MAX_NUMBER_DEVICES ];

#define DEFAULT_ADDRESS  0xFFFE


void emberAfPluginTunnelingEndpointInit(void)
{
  int8u i;
  for( i=0; i<MAX_NUMBER_DEVICES; i++ ){
    endpointMap[i].nodeId = DEFAULT_ADDRESS;
    endpointMap[i].endpoint = INVALID_TUNNELING_ENDPOINT;
  }
}

// NOTE:  This function and file assume only 1 tunneling endpoint exists on each address.
// If devices will support more than 1 tunneling endpoint, an array of endpoints should
// be stored.
void emberAfPluginAddTunnelingEndpoint( int16u nodeId, int8u *endpointList, int8u numEndpoints ){
  int8u i;
  int8u firstUnused = MAX_NUMBER_DEVICES;

  if( numEndpoints > 0 ){
    for( i=0; i<MAX_NUMBER_DEVICES; i++ ){
      if( endpointMap[i].nodeId == nodeId ){
        return;
      }
      else if( (endpointMap[i].nodeId == DEFAULT_ADDRESS) && (firstUnused >= MAX_NUMBER_DEVICES) ){
        firstUnused = i;
      }
    }
    if( firstUnused < MAX_NUMBER_DEVICES ){
      i = firstUnused;
      endpointMap[i].nodeId = nodeId;
      endpointMap[i].endpoint = endpointList[0];
    }
  }
}

int8u emberAfPluginGetDeviceTunnelingEndpoint( int16u nodeId )
{
  int8u i;
  int8u endpoint = INVALID_TUNNELING_ENDPOINT;
  for( i=0; i<MAX_NUMBER_DEVICES; i++ ){
    if( endpointMap[i].nodeId == nodeId ){
      endpoint = endpointMap[i].endpoint;
      break;
    }
  }
  return endpoint;
}



