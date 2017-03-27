// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-zrc20-ha-server.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE ZRC2.0 HA Server plugin is not compatible with the legacy CLI.
#endif


void emberAfPluginRf4ceZrc20HaServerClearAllHaAttributesCommand(void)
{
    emberAfPluginRf4ceZrc20HaServerClearAllHaAttributes();

    emberAfAppPrintln("clear all HA attributes 0x00");
}

void emberAfPluginRf4ceZrc20HaServerGetHaAttributeCommand(void)
{
    int8u i;
    int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
    int8u haInstanceId = (int8u)emberUnsignedCommandArgument(1);
    int8u haAttributeId = (int8u)emberUnsignedCommandArgument(2);
    int8u contents[5];
    EmberAfRf4ceZrcHomeAutomationAttribute haAttribute =
    {
        .contents = contents
    };
    EmberAfRf4ceGdpAttributeStatus status;

    status = emberAfPluginRf4ceZrc20HaServerGetHaAttribute(pairingIndex,
                                                           haInstanceId,
                                                           haAttributeId,
                                                           &haAttribute);

    emberAfAppPrint("attribute: 0x%x, 0x%x, 0x%x; value: ",
                    pairingIndex,
                    haInstanceId,
                    haAttributeId);
    for (i=0; i<haAttribute.contentsLength; i++)
    {
        emberAfAppPrint("0x%x ", haAttribute.contents[i]);
    }
    emberAfAppPrintln("");
    emberAfAppPrintln("%p 0x%x", "get HA attribute", status);
}

void emberAfPluginRf4ceZrc20HaServerSetHaAttributeCommand(void)
{
    int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
    int8u haInstanceId = (int8u)emberUnsignedCommandArgument(1);
    int8u haAttributeId = (int8u)emberUnsignedCommandArgument(2);
    int8u contentsLength;
    int8u contents[5];
    EmberAfRf4ceZrcHomeAutomationAttribute haAttribute =
    {
            .contents = contents
    };
    EmberAfRf4ceGdpAttributeStatus status;

    haAttribute.contents = emberStringCommandArgument(3, &contentsLength);

    status = emberAfPluginRf4ceZrc20HaServerSetHaAttribute(pairingIndex,
                                                           haInstanceId,
                                                           haAttributeId,
                                                           &haAttribute);

    emberAfAppPrintln("%p 0x%x", "set HA attribute", status);
}

void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingClearCommand(void)
{
    emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingClear();

    emberAfAppPrintln("clear logical devices and mappings 0x00");
}

void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingAddCommand(void)
{
    int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
    int8u haInstanceId = (int8u)emberUnsignedCommandArgument(1);
    DestStruct dest = {
        .type = (int8u)emberUnsignedCommandArgument(2),
        .indexOrDestination = (int16u)emberUnsignedCommandArgument(3),
        .sourceEndpoint = (int8u)emberUnsignedCommandArgument(4),
        .destinationEndpoint = (int8u)emberUnsignedCommandArgument(5)
    };
    EmberStatus status;

    status = emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingAdd(pairingIndex,
                                                                                        haInstanceId,
                                                                                        &dest);

    emberAfAppPrintln("%p 0x%x", "add logical device and mapping", status);
}

void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingRemoveCommand(void)
{
    DestStruct dest = {
        .type = (int8u)emberUnsignedCommandArgument(0),
        .indexOrDestination = (int16u)emberUnsignedCommandArgument(1),
        .sourceEndpoint = (int8u)emberUnsignedCommandArgument(2),
        .destinationEndpoint = (int8u)emberUnsignedCommandArgument(3)
    };
    EmberStatus status;

    status = emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingRemove(&dest);

    emberAfAppPrintln("%p 0x%x", "remove logical device and mapping", status);
}

void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingGetCommand(void)
{
    int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
    int8u haInstanceId = (int8u)emberUnsignedCommandArgument(1);
    DestStruct dest;
    EmberStatus status;

    if (EMBER_SUCCESS ==
            (status = emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationGet(pairingIndex,
                                                                           haInstanceId,
                                                                           &dest)))
    {
        emberAfAppPrint("logicalDevice: ");
        emberAfAppPrint("0x%x ", dest.type);
        emberAfAppPrint("0x%2x ", dest.indexOrDestination);
        emberAfAppPrint("0x%x ", dest.sourceEndpoint);
        emberAfAppPrintln("0x%x", dest.destinationEndpoint);
    }

    emberAfAppPrintln("%p 0x%x", "get logical device", status);
}

void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingPrintCommand(void)
{
    int8u i = 0, j = 0, destIndex, tmpDestIndex;
    DestStruct dest = {
        .type = (int8u)emberUnsignedCommandArgument(0),
        .indexOrDestination = (int16u)emberUnsignedCommandArgument(1),
        .sourceEndpoint = (int8u)emberUnsignedCommandArgument(2),
        .destinationEndpoint = (int8u)emberUnsignedCommandArgument(3)
    };
    EmberStatus status;

    if (EMBER_SUCCESS ==
            (status = emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(&dest,
                                                                        &destIndex)))
    {
        emberAfAppPrint("logicalDevice: ");
        emberAfAppPrint("0x%x ", dest.type);
        emberAfAppPrint("0x%2x ", dest.indexOrDestination);
        emberAfAppPrint("0x%x ", dest.sourceEndpoint);
        emberAfAppPrintln("0x%x", dest.destinationEndpoint);
        emberAfAppPrint("instances mapped to it: ");
        for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
        {
            for (j=0; j<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; j++)
            {
                emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(i,
                                                                   j,
                                                                   &tmpDestIndex);
                if (tmpDestIndex == destIndex)
                {
                    emberAfAppPrint("%d-%d, ", i, j);
                }
            }
        }
    }

    emberAfAppPrintln("\n%p 0x%x", "print logical device and mapping", status);
}

void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingPrintAllCommand(void)
{
    int8u i, j, status = 0;
    DestStruct dest;
    int8u destIndex;

    emberAfAppPrintln("logicalDevices table:");
    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        if (0xFF == GetLogicalDeviceDestination(i, &dest))
        {
            status = 0xFF;
            break;
        }
        emberAfAppPrint("[%d] ", i);
        emberAfAppPrint("0x%x ", dest.type);
        emberAfAppPrint("0x%2x ", dest.indexOrDestination);
        emberAfAppPrint("0x%x ", dest.sourceEndpoint);
        emberAfAppPrintln("0x%x", dest.destinationEndpoint);
    }

    emberAfAppPrintln("instanceToLogicalDevice table:");
    for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
    {
        emberAfAppPrint("[%d] ", i);
        for (j=0; j<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; j++)
        {
            if (EMBER_SUCCESS != emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(i,
                                                                                    j,
                                                                                    &destIndex))
            {
                status = 0xFF;
                break;
            }
            if (j%16 == 0 && j != 0)
            {
                emberAfAppPrintln("");
                emberAfAppPrint("    ");
            }
            emberAfAppPrint("0x%x ", destIndex);
        }
        emberAfAppPrintln("");
    }

    emberAfAppPrintln("%p 0x%x", "print all logical devices and mappings", status);
}



