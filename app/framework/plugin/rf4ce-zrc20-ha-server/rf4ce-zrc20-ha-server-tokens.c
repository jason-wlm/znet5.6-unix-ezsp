/* Copyright 2014 Silicon Laboratories, Inc. */

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "rf4ce-zrc20-ha-server.h"


EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(int8u pairingIndex,
                                                               int8u haInstanceId,
                                                               int8u destIndex)
{
    InstStruct inst;

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

    /* Destination index out of range. */
    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Read out all instances per pairingIndex. */
    halCommonGetIndexedToken(&inst,
                             TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                             pairingIndex);

    /* Update mapping of the instance. */
    inst.instances[haInstanceId] = destIndex;

    /* Write all back. */
    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                             pairingIndex,
                             &inst);

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceRemove(int8u destIndex)
{
    int8u i, j;
    InstStruct inst;
    boolean needToWrite;

    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
    {
        /* Read out all instances per pairingIndex. */
        halCommonGetIndexedToken(&inst,
                                 TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                                 i);

        /* Look for matching destination index. */
        needToWrite = FALSE;
        for (j=0; j<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; j++)
        {
            /* If found, clear it and set write flag. */
            if (destIndex == inst.instances[j])
            {
                inst.instances[j] = 0xFF;
                needToWrite = TRUE;
            }
        }

        /* Update entry if changed. */
        if (needToWrite)
        {
            halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                                     i,
                                     &inst);
        }
    }

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(int8u pairingIndex,
                                                               int8u haInstanceId,
                                                               int8u* destIndex)
{
    InstStruct inst;

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

    /* Read out all instances per pairingIndex. */
    halCommonGetIndexedToken(&inst,
                             TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                             pairingIndex);

    /* Get mapping of the instance. */
    *destIndex = inst.instances[haInstanceId];

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(DestStruct* dest,
                                                              int8u* index)
{
    int8u i;
    DestStruct tmpDest;

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        halCommonGetIndexedToken(&tmpDest,
                                 TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                                 i);

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
    int8u i;
    DestStruct dest;

    MEMSET(&dest, 0xFF, sizeof(DestStruct));

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                                 i,
                                 &dest);
    }
}

void emAfRf4ceZrc20ClearInstanceToLogicalDeviceTable(void)
{
    int8u i;
    InstStruct inst;

    MEMSET(&inst, 0xFF, sizeof(InstStruct));

    for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
    {
        halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                                 i,
                                 &inst);
    }
}


EmberStatus emAfRf4ceZrc20AddLogicalDeviceDestination(DestStruct* dest,
                                        int8u* index)
{
    int8u i;
    DestStruct tmpDest;

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        halCommonGetIndexedToken(&tmpDest,
                                 TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                                 i);
        /* Empty entry. */
        if (tmpDest.type == 0xFF)
        {
            halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                                     i,
                                     dest);
            *index = i;
            return EMBER_SUCCESS;
        }
    }

    /* Table is full. */
    return EMBER_TABLE_FULL;
}

EmberStatus emAfRf4ceZrc20RemoveLogicalDeviceDestination(int8u destIndex)
{
    DestStruct dest;

    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    MEMSET(&dest, 0xFF, sizeof(DestStruct));

    halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                             destIndex,
                             &dest);
    return EMBER_SUCCESS;
}

int8u GetLogicalDeviceDestination(int8u i, DestStruct* dest)
{
    if (i >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return 0xFF;
    }

    halCommonGetIndexedToken(dest,
                             TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                             i);

    return i;
}


void DestLookup(int8u pairingIndex,
                int8u haInstanceId,
                DestStruct* dest)
{
    InstStruct inst;

    /* Read out all instances per pairingIndex. */
    halCommonGetIndexedToken(&inst,
                             TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                             pairingIndex);

    /* Get the destination mapped to the instance. */
    halCommonGetIndexedToken(dest,
                             TOKEN_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                             inst.instances[haInstanceId]);
}


