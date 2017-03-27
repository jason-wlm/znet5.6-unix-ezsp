/*
 * File: byte-utilities.c
 * Description: Data store and fetch routines.
 *
 * Copyright 2014 Silicon Laboratories, Inc.                                *80*
 */

#include PLATFORM_HEADER

// Copy bytes from source to destination, reversing the order.
void emberReverseMemCopy(int8u* dest, const int8u* src, int8u length)
{
  int8u i;
  int8u j=(length-1);

  for( i = 0; i < length; i++) {
    dest[i] = src[j];
    j--;
  }
}

int16u emberFetchLowHighInt16u(const int8u *contents)
{
  return HIGH_LOW_TO_INT(contents[1], contents[0]);
}

int16u emberFetchHighLowInt16u(const int8u *contents)
{
  return HIGH_LOW_TO_INT(contents[0], contents[1]);
}

void emberStoreLowHighInt16u(int8u *contents, int16u value)
{
  contents[0] = LOW_BYTE(value);
  contents[1] = HIGH_BYTE(value);
}

void emberStoreHighLowInt16u(int8u *contents, int16u value)
{
  contents[0] = HIGH_BYTE(value);
  contents[1] = LOW_BYTE(value);
}

void emStoreInt32u(boolean lowHigh, int8u* contents, int32u value)
{
  int8u ii;
  for (ii = 0; ii < 4 ; ii++) {
    int8u index = ( lowHigh ? ii : 3 - ii );
    contents[index] = (int8u)(value & 0xFF);
    value = (value >> 8);
  }
}

int32u emFetchInt32u(boolean lowHigh, int8u* contents)
{
  int8u b0;
  int8u b1;
  int8u b2;
  int8u b3;

  if (lowHigh) {
    b0 = contents[3];
    b1 = contents[2];
    b2 = contents[1];
    b3 = contents[0];
  } else {
    b0 = contents[0];
    b1 = contents[1];
    b2 = contents[2];
    b3 = contents[3];
  }
  return ((((int32u) ((int16u) (((int16u) b0) << 8) | ((int16u) b1)))
           << 16)
          | (int32u) ((int16u) (((int16u) b2) << 8) | ((int16u) b3)));
}
