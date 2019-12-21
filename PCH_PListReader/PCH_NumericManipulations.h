//
//  PCH_NumericManipulations.h
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-20.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

// This file will grow as I require these weirdo routines. The intention is to have a cross-platform method of calling the routines

#ifndef PCH_NumericManipulations_h
#define PCH_NumericManipulations_h

#include <stdio.h>
#include <string.h>
#include <float.h>

#ifdef __APPLE__

#include <CoreFoundation/CFByteOrder.h>

#elif defined (_MSC_VER)

#endif

typedef int32_t PCH_FloatBigEndian;
typedef int64_t PCH_DoubleBigEndian;

// Int swaps
int16_t PCH_SwapInt16HostToBig(int16_t x);
int32_t PCH_SwapInt32HostToBig(int32_t x);
int64_t PCH_SwapInt64HostToBig(int64_t x);

int16_t PCH_SwapInt16BigToHost(int16_t x);
int32_t PCH_SwapInt32BigToHost(int32_t x);
int64_t PCH_SwapInt64BigToHost(int64_t x);

// Float swaps
PCH_FloatBigEndian PCH_SwapFloatHostToBig(float x);
PCH_DoubleBigEndian PCH_SwapDoubleHostToBig(double x);

float PCH_SwapFloatBigToHost(PCH_FloatBigEndian x);
double PCH_SwapDoubleBigToHost(PCH_DoubleBigEndian x);

#endif /* PCH_NumericManipulations_h */
