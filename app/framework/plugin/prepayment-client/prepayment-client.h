#ifndef _PREPAYMENT_CLIENT_H_
#define _PREPAYMENT_CLIENT_H_


void emberAfPluginPrepaymentClientChangePaymentMode( EmberNodeId nodeId, int8u srcEndpoint, int8u dstEndpoint, int32u providerId, int32u issuerEventId, int32u implementationDateTime, int16u proposedPaymentControlConfiguration, int32u cutOffValue );
  

boolean emberAfPluginPrepaymentClusterChangePaymentModeResponseCallback( int8u friendlyCredit, int32u friendlyCreditCalendarId, int32u emergencyCreditLimit, int32u emergencyCreditThreshold );

  
#endif  // #ifndef _PREPAYMENT_CLIENT_H_

