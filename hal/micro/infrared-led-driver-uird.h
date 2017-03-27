// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __INFRARED_LED_DRIVER_UIRD_H__
#define __INFRARED_LED_DRIVER_UIRD_H__

#define HAL_INFRARED_LED_UIRD_DB_STRUCT_SIZE             ( 80)
#define HAL_INFRARED_LED_UIRD_ENCR_DB_STRUCT_SIZE        ( 52)

#define HAL_INFRARED_LED_UIRD_DB_ENTRIES                 ( 3)
#define HAL_INFRARED_LED_UIRD_ENCR_DB_ENTRIES            ( 3)


extern const int8u halInfraredLedUirdDatabase[ HAL_INFRARED_LED_UIRD_DB_ENTRIES][ HAL_INFRARED_LED_UIRD_DB_STRUCT_SIZE];
extern const int8u halInfraredLedUirdEncryptDatabase[ HAL_INFRARED_LED_UIRD_ENCR_DB_ENTRIES][ HAL_INFRARED_LED_UIRD_ENCR_DB_STRUCT_SIZE];


int  halInfraredLedUirdDecode(const int8u *irdPtr, int8u irdLen);

void halInfraredLedUirdSetDecryptKey( int32u decryptKey);
int  halInfraredLedUirdDecrypt(int8u dstPtr[], const int8u srcPtr[], int srcLen);


#endif // __INFRARED_LED_DRIVER_UIRD_H__
