// *******************************************************************
// * price-client-cli.c
// *
// *
// * Copyright 2012 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/util/serial/command-interpreter2.h"
#include "app/framework/plugin/price-client/price-client.h"

void emAfPriceClientCliPrint(void);
void emAfPriceClientCliTableClear(void);
void emAfPriceClientCliConversionFactorPrintByEventId(void);
void emAfPriceClientCliCalorificValuePrintByEventId(void);
void emAfPriceClientCliSetCppEventAuth(void);
void emAfPriceClientCliCo2ValueTablePrintCurrent(void);
void emAfPriceClientCliBillingPeriodPrintCurrent(void);
void emAfPriceClientCliTierLabelTablePrintTariffId(void);
void emAfPriceClientCliConsolidatedBillTablePrint(void);
void emAfPriceClientCliCppEventPrint(void);
void emAfPriceClientCliCreditPaymentTablePrint(void);
void emAfPriceClientCliCreditPaymentPrintEntryByEventId(void);
void emAfPriceClientClieCurrencyConversionPrintCurrentCurrency(void);


#if !defined(EMBER_AF_GENERATE_CLI)
EmberCommandEntry emberAfPluginPriceClientCommands[] = {
  emberCommandEntryAction("print",  emAfPriceClientCliPrint, "u", "Print the price info"),
  emberCommandEntryTerminator(),
};
#endif // EMBER_AF_GENERATE_CLI

// plugin price-client init <endpoint:1>
void emAfPriceClientCliInit(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emberAfPriceClusterClientInitCallback( endpoint );
}

// plugin price-client print <endpoint:1>
void emAfPriceClientCliPrint(void) 
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emAfPluginPriceClientPrintInfo(endpoint);
}

// plugin price-client printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliPrintEvent(void) 
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int32u issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  emAfPluginPriceClientPrintByEventId( endpoint, issuerEventId );
}

// plugin price-client table-clear <endpoint:1>
void emAfPriceClientCliTableClear(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emAfPriceClearPriceTable(endpoint);
}

// plugin price-client conv-factor printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliConversionFactorPrintByEventId(void){
  int8u  i;
  int32u issuerEventId;
  int8u  endpoint;
  endpoint = (int8u)emberUnsignedCommandArgument(0);
  issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  i = emAfPriceGetConversionFactorIndexByEventId( endpoint, issuerEventId );
  emAfPricePrintConversionFactorEntryIndex( endpoint, i );
}

// plugin price-client calf-value printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliCalorificValuePrintByEventId(void){
  int8u  i;
  int32u issuerEventId;
  int8u  endpoint;
  endpoint = (int8u)emberUnsignedCommandArgument(0);
  issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  i = emAfPriceGetCalorificValueIndexByEventId( endpoint, issuerEventId );
  emAfPricePrintCalorificValueEntryIndex( endpoint, i );
}

// plugin price-client co2-value print <endpoint:1>
void emAfPriceClientCliCo2ValueTablePrintCurrent(void){
  int8u i;
  int8u  endpoint;
  endpoint = (int8u)emberUnsignedCommandArgument(0);
  i = emberAfPriceClusterGetActiveCo2ValueIndex( endpoint );
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CO2_TABLE_SIZE ){
    emAfPricePrintCo2ValueTablePrintIndex( endpoint, i );
  }
  else{
    emberAfPriceClusterPrintln("NO CURRENT CO2 VALUE");
  }
}

// plugin price-client bill-period printCurrent <endpoint:1>
void emAfPriceClientCliBillingPeriodPrintCurrent(void){
  int8u  i;
  int8u  endpoint;
  endpoint = (int8u)emberUnsignedCommandArgument(0);
  i = emAfPriceGetActiveBillingPeriodIndex( endpoint );
  emAfPricePrintBillingPeriodTableEntryIndex( endpoint, i );
}

// plugin price-client block-period printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliBlockPeriodPrintEntryByEventId(void){
  int32u issuerEventId;
  int8u  endpoint;
  int8u  i;
 
  endpoint = (int8u)emberUnsignedCommandArgument(0);
  issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  i = emAfPriceGetBlockPeriodTableIndexByEventId( endpoint, issuerEventId );
  emAfPricePrintBlockPeriodTableIndex( endpoint, i );
}



// plugin price-client tier-label printTariff <endpoint:1> <issuerTariffId:4>
void emAfPriceClientCliTierLabelTablePrintTariffId(void){
  int32u issuerTariffId;
  int8u  i;
  int8u  endpoint;
  endpoint = (int8u)emberUnsignedCommandArgument(0);
  issuerTariffId = (int32u)emberUnsignedCommandArgument(1);
  i = emAfPriceGetActiveTierLabelTableIndexByTariffId( endpoint, issuerTariffId );
  emAfPricePrintTierLabelTableEntryIndex( endpoint, i );
}


extern int8u emberAfPriceClusterDefaultCppEventAuthorization;

// plugin price-client cpp-event setAuth <cppEventAuth:1>
void emAfPriceClientCliSetCppEventAuth(void){
  emberAfPriceClusterDefaultCppEventAuthorization = (int8u)emberUnsignedCommandArgument(0);
}

// plugin price-client consol-bill print <endpoint:1> <index:1>
void emAfPriceClientCliConsolidatedBillTablePrint(void){
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u i = (int8u)emberUnsignedCommandArgument(1);
  emAfPricePrintConsolidatedBillTableIndex( endpoint, i );
}

// plugin price-client consol-bill printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliConsolidatedBillPrintEntryByEventId(void){
  int8u  i;
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int32u issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  i = emAfPriceConsolidatedBillTableGetIndexWithEventId( endpoint, issuerEventId );
  if( i >= EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE ){
    emberAfPriceClusterPrintln("NOT FOUND, Event ID=%d", issuerEventId );
  }
  else{
    emAfPricePrintConsolidatedBillTableIndex( endpoint, i );
  }
}

// plugin price-client consol-bill printCurrent <endpoint:1
void emAfPriceClientCliConsolidatedBillPrintCurrentEntry(void){
  int8u i;
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  i = emAfPriceConsolidatedBillTableGetCurrentIndex( endpoint );
  if( i >= EMBER_AF_PLUGIN_PRICE_CLIENT_CONSOLIDATED_BILL_TABLE_SIZE ){
    emberAfPriceClusterPrintln("NO CURRENT BILL" );
  }
  else{
    emAfPricePrintConsolidatedBillTableIndex( endpoint, i );
  }
}


// plugin price-client cpp-event print <endpoint:1>
void emAfPriceClientCliCppEventPrint(void){
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emberAfPricePrintCppEvent( endpoint );
}


// plugin price-client credit-pmt print <endpoint:1> <index:1>
void emAfPriceClientCliCreditPaymentTablePrint(void){
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  int8u i = (int8u)emberUnsignedCommandArgument(1);
  emAfPricePrintCreditPaymentTableIndex( endpoint, i );
}

// plugin price-client credit-pmt printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliCreditPaymentPrintEntryByEventId(void){
  int8u  i;
  int32u endpoint = (int32u)emberUnsignedCommandArgument(0);
  int32u issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  i = emAfPriceCreditPaymentTableGetIndexWithEventId( endpoint, issuerEventId );
  if( i >= EMBER_AF_PLUGIN_PRICE_CLIENT_CREDIT_PAYMENT_TABLE_SIZE ){
    emberAfPriceClusterPrintln("NOT FOUND, Event ID=%d", issuerEventId );
  }
  else{
    emAfPricePrintCreditPaymentTableIndex( endpoint, i );
  }
}

// plugin price-client currency-convers printEvent <endpoint:1> <issuerEventId:4>
void emAfPriceClientCliCurrencyConversionPrintByEventId(void){
  int8u i;
  int8u  endpoint = (int8u)emberUnsignedCommandArgument(0);
  int32u issuerEventId = (int32u)emberUnsignedCommandArgument(1);
  i = emberAfPriceClusterCurrencyConversionTableGetIndexByEventId( endpoint, issuerEventId );
  if( i >= EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE ){
    emberAfPriceClusterPrintln("NOT FOUND, Event ID=%d", issuerEventId );
  }
  else{
    emAfPricePrintCurrencyConversionTableIndex( endpoint, i );
  }
}

// plugin price-client currency-convers printCurrent <endpoint:1>
void emAfPriceClientClieCurrencyConversionPrintCurrentCurrency(void){
  int8u i;
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  i = emberAfPriceClusterGetActiveCurrencyIndex( endpoint );
  if( i < EMBER_AF_PLUGIN_PRICE_CLIENT_CURRENCY_CONVERSION_TABLE_SIZE ){
    emAfPricePrintCurrencyConversionTableIndex( endpoint, i );
  }
  else{
    emberAfPriceClusterPrintln("NO CURRENT CURRENCY");
  }
}



