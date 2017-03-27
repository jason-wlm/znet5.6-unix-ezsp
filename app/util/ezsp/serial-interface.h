// File: serial-interface.h
// 
// Description: The interface down to the serial protocol being used to
// transport EZSP frames (either SPI or UART).
// 
// Copyright 2006-2010 by Ember Corporation. All rights reserved.           *80*

#ifndef __SERIAL_INTERFACE_H__
#define __SERIAL_INTERFACE_H__

// Macros for reading and writing frame bytes.
#define serialGetResponseByte(index)      (ezspFrameContents[(index)])
#define serialSetCommandByte(index, data) (ezspFrameContents[(index)] = (data))

// The length of the current EZSP frame.  The higher layer writes this when
// sending a command and reads it when processing a response.
extern int8u *ezspFrameLengthLocation;

// Macros for reading and writing the frame length.
#define serialSetCommandLength(length) (*ezspFrameLengthLocation = (length))
#define serialGetResponseLength()      (*ezspFrameLengthLocation)

// Returns the number of EZSP responses that have been received by the serial
// protocol and are ready to be collected by the EZSP layer via
// serialResponseReceived().
int8u serialPendingResponseCount(void);

// Checks whether a new EZSP response frame has been received. Returns
// EZSP_SUCCESS if a new response has been received. Returns
// EZSP_SPI_WAITING_FOR_RESPONSE or EZSP_ASH_NO_RX_DATA if no new response has
// been received. Any other return value means that an error has been detected
// by the serial protocol layer.
EzspStatus serialResponseReceived(void);

// Sends the current EZSP command frame. Returns EZSP_SUCCESS if the command was
// sent successfully. Any other return value means that an error has been
// detected by the serial protocol layer.
EzspStatus serialSendCommand(void);

// Set when the ncp has indicated it has a pending callback by seting the
// callback flag in the frame control byte or (uart version only) by sending
// an an ASH_WAKE byte between frames.
extern boolean ncpHasCallbacks;

// Tests that the host is able to properly hold off transmitting in response
// to the ncp's flow control request.
EzspStatus serialTestFlowControl(void);

//Returns the total Length of the incoming frame
int8u serialGetCommandLength(void);
#endif // __SERIAL_INTERFACE_H__