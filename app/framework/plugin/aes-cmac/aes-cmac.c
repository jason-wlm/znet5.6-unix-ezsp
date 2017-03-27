// Copyright 2014 Silicon Laboratories, Inc.
//
// Implementation of the Cipher-based Message Authentication Code (CMAC)
// specified in the IETF memo "The AES-CMAC Algorithm".

#include PLATFORM_HEADER

#include "aes-cmac.h"

#if defined(EMBER_SCRIPTED_TEST)
#define HIDDEN
#else
#define HIDDEN static
#endif

// Exclusive-Or operation. For two equal length strings, x and y, x XOR y is
// their bit-wise exclusive-OR.
HIDDEN void xor128(const int8u *x, const int8u *y, int8u *out)
{
  int8u i;

  for (i=0; i<16; i++) {
    out[i] = x[i] ^ y[i];
  }
}

// Left-shift of the string x by 1 but. The most significant bit disappears, and
// a zero comes into the least significant bit.
HIDDEN void oneBitLeftShift(const int8u *x, int8u *out)
{
  int8s i;
  int8u overflow = 0x00;

  for (i=15; i>=0; i--) {
    out[i] = x[i] << 1;
    out[i] |= overflow;
    overflow = (x[i] & 0x80) ? 0x01: 0x00;
  }
}

// 10^i padded output of input x. Is the concatenation of x and a single '1'
// followed by the minimum number of '0's, so that the total length is equal
// to 128 bits.
HIDDEN void padding(const int8u *x, int8u length, int8u *out)
{
  int8u i;

  for(i=0; i<16; i++) {
    if (i<length) {
      out[i] = x[i];
    } else if (i == length) {
      out[i] = 0x80;
    } else {
      out[i] = 0x00;
    }
  }
}

extern void emGetKeyFromCore(int8u* key);
extern void emLoadKeyIntoCore(int8u* key);
extern void emStandAloneEncryptBlock(int8u *block);

HIDDEN void aesEncrypt(int8u *block, const int8u *key)
{
  int8u temp[16];

  ATOMIC(
         emGetKeyFromCore(temp);
         emLoadKeyIntoCore((int8u*)key);
         emStandAloneEncryptBlock(block);
         emLoadKeyIntoCore(temp);
       )
}

HIDDEN void generateSubKey(const int8u *key, int8u *outKey1, int8u *outKey2)
{
  int8u L[16];
  int8u constRb[16];

  initToConstRb(constRb);

  // Step 1
  initToConstZero(L);
  aesEncrypt(L, key);

  // Step 2
  oneBitLeftShift(L, outKey1); // // K1:= L << 1;

  if (MSB(L)) { // K1:= (L << 1) XOR const_Rb;
    xor128(outKey1, constRb, outKey1);
  }
  // Step 3
  oneBitLeftShift(outKey1, outKey2); // // K2 := K1 << 1;

  if (MSB(outKey1)) { // K2 := (K1 << 1) XOR const_Rb;
    xor128(outKey2, constRb, outKey2);
  }
}

void emberAfPluginAesMacAuthenticate(const int8u *key,
                                     const int8u *message,
                                     int8u messageLength,
                                     int8u *out)
{
  int8u key1[16];
  int8u key2[16];
  int8u lastBlock[16];
  int8u blockNum;
  boolean isLastBlockComplete;
  int8u i;

  // Step 1
  generateSubKey(key, key1, key2);

  // Step 2 (we perform ceil(x/y) by doing: (x + y - 1) / y).
  blockNum = (messageLength + 15) / 16;

  // Step 3
  if (blockNum == 0) {
    blockNum = 1;
    isLastBlockComplete = FALSE;
  } else {
    isLastBlockComplete = ((messageLength % 16) == 0);
  }

  // Step 4
  if (isLastBlockComplete) {
    xor128(message + (blockNum - 1)*16, key1, lastBlock);
  } else {
    padding(message + (blockNum - 1)*16, messageLength % 16, lastBlock);
    xor128(lastBlock, key2, lastBlock);
  }

  // Step 5
  initToConstZero(out);

  // Step 6
  for (i=0; i<blockNum - 1; i++) {
    xor128(out, message + i*16, out);
    aesEncrypt(out, key);
  }

  xor128(out, lastBlock, out);
  aesEncrypt(out, key);
}

