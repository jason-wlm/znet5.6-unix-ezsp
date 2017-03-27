/* Copyright 2014 Silicon Laboratories, Inc. */

#include "af.h"
#include "rf4ce-zrc20-ha-server.h"

static DestStruct logicalDevices[EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE];
static int8u instanceToLogicalDevice[EMBER_RF4CE_PAIRING_TABLE_SIZE][ZRC_HA_SERVER_NUM_OF_HA_INSTANCES];


EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(int8u pairingIndex,
                                                               int8u haInstanceId,
                                                               int8u destIndex)
{
    /* Pairing index out of range. */
    if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Entry index out of range. */
    if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Map logical device to instance. */
    instanceToLogicalDevice[pairingIndex][haInstanceId] = destIndex;

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceRemove(int8u destIndex)
{
    int8u i, j;
    int8u* tmpPoi;

    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
    {
        tmpPoi = &instanceToLogicalDevice[i][0];
        for (j=0; j<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; j++)
        {
            if (destIndex == tmpPoi[j])
            {
                tmpPoi[j] = 0xFF;
            }
        }
    }

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(int8u pairingIndex,
                                                               int8u haInstanceId,
                                                               int8u* destIndex)
{
    /* Pairing index out of range. */
    if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Entry index out of range. */
    if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    *destIndex = instanceToLogicalDevice[pairingIndex][haInstanceId];

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(DestStruct* dest,
                                                              int8u* index)
{
    int8u i;
    DestStruct tmpDest;

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        tmpDest = logicalDevices[i];
        if (tmpDest.type == dest->type
            && tmpDest.indexOrDestination == dest->indexOrDestination
            && tmpDest.sourceEndpoint == dest->sourceEndpoint
            && tmpDest.destinationEndpoint == dest->destinationEndpoint)
        {
            *index = i;
            return EMBER_SUCCESS;
        }
    }

    /* Did not find matching entry. */
    return EMBER_NOT_FOUND;
}


void emAfRf4ceZrc20ClearLogicalDevicesTable(void)
{
    MEMSET((int8u*)logicalDevices, 0xFF, sizeof(logicalDevices));
}

EmberStatus emAfRf4ceZrc20AddLogicalDeviceDestination(DestStruct* dest,
                                                      int8u* index)
{
    int8u i;

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        /* Empty entry. */
        if (logicalDevices[i].type == 0xFF)
        {
            MEMCOPY((int8u*)&logicalDevices[i], (int8u*)dest, sizeof(DestStruct));
            *index = i;
            return EMBER_SUCCESS;
        }
    }

    /* Table is full. */
    return EMBER_TABLE_FULL;
}

EmberStatus emAfRf4ceZrc20RemoveLogicalDeviceDestination(int8u destIndex)
{
    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    MEMSET((int8u*)&logicalDevices[destIndex], 0xFF, sizeof(DestStruct));
    return EMBER_SUCCESS;
}

int8u GetLogicalDeviceDestination(int8u i, DestStruct* dest)
{
    if (i >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return 0xFF;
    }

    MEMCOPY((int8u*)dest, (int8u*)&logicalDevices[i], sizeof(DestStruct));
    return i;
}

void emAfRf4ceZrc20ClearInstanceToLogicalDeviceTable(void)
{
    MEMSET(instanceToLogicalDevice, 0xFF, sizeof(instanceToLogicalDevice));
}

void DestLookup(int8u pairingIndex,
                int8u haInstanceId,
                DestStruct* dest)
{
    int8u destIndex = instanceToLogicalDevice[pairingIndex][haInstanceId];

    MEMCOPY((int8u*)dest,
            (int8u*)&logicalDevices[destIndex],
            sizeof(DestStruct));
}


