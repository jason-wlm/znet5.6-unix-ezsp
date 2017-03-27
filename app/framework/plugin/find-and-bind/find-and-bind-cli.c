//
// find-and-bind-cli.c
//
// Andrew Keesler
//
// October 16, 2014
//
// CLI stuff for find-and-bind plugin.

#include "app/framework/include/af.h"

#include "find-and-bind.h"

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// -----------------------------------------------------------------------------
// CLI Command Definitions

void emberAfPluginFindAndBindCommand(void)
{
  boolean actionIsTarget = (emberStringCommandArgument(-1, NULL)[0] == 't');
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  EmberStatus status
    = (actionIsTarget
       ? emberAfPluginFindAndBindTarget(endpoint)
       : emberAfPluginFindAndBindInitiator(endpoint));
  
  emberAfCorePrintln("%p: %p: 0x%X. Endpoint: %d.",
                     EMBER_AF_FIND_AND_BIND_PLUGIN_NAME,
                     (actionIsTarget ? "Target" : "Initiator"),
                     status,
                     endpoint);
}


#endif /* 
          defined(EMBER_AF_GENERATE_CLI)
          || defined(EMBER_AF_API_COMMAND_INTERPRETER2)
       */
