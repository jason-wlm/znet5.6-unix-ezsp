// ****************************************************************************
// * update-tc-link-key.c
// *
// * This plugin will request a new link key upon joining a Zigbee network.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                             *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"

extern boolean emEnableR21StackBehavior;

#define SERVER_MASK_STACK_COMPLIANCE_REVISION_MASK 0xFE00
//Bits 9 to 15 are set to 21. Everything else is 0.
#define SERVER_MASK_STACK_COMPLIANCE_REVISION_21 0x2A00

void emberAfPluginUpdateTcLinkKeyInitCallback(void)
{
  #if !defined(EZSP_HOST)
  emEnableR21StackBehavior = TRUE;
  #endif
}

static int8u once = 0;
void emberAfPluginUpdateTcLinkKeyStackStatusCallback(EmberStatus status)
{

	if(status == EMBER_NETWORK_UP && once == 0) {
		once++;
		status = emberNodeDescriptorRequest(0x0000,
                                   			EMBER_AF_DEFAULT_APS_OPTIONS);
	}
}

boolean emberAfPreZDOMessageReceivedCallback(EmberNodeId sender,
                                             EmberApsFrame* apsFrame, 
                                             int8u* message,
                                             int16u length)
{
	if(apsFrame->clusterId == NODE_DESCRIPTOR_RESPONSE){
		int16u serverMask = message[12] | (message[13] << 8);
		if((serverMask & SERVER_MASK_STACK_COMPLIANCE_REVISION_MASK) == 
			SERVER_MASK_STACK_COMPLIANCE_REVISION_21) {
			emberAfCorePrintln("R21 device found. Requesting new link key.");			
  		#if !defined(EZSP_HOST)		
		EmberStatus status = emberUpdateLinkKey();
		#endif
		} 
	}
	return FALSE;
}

void emberZigbeeKeyEstablishmentHandler(EmberEUI64 partner, EmberKeyStatus status)
{
	emberAfCorePrintln("New Key Established partner %u status 0x%x",partner,status);
}