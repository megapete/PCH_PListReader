//
//  PCH_NumericManipulations.h
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-20.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

// These routines are used to convert the "endian-ness" of numerical representations. In particular, they convert between Big-endian representations and whatever representation the host computer uses.
// NOTE: This file will grow as I require these weirdo routines. For now, only Big-Endian routines are included (Little-endian routines will only be added if I need them for something). The intention is to have a cross-platform method of calling the routines.

#ifndef PCH_NumericManipulations_h
#define PCH_NumericManipulations_h

// This is required to let these C-language routines be called from C++
#ifdef __cplusplus
extern "C" {
#endif


// Standard includes
#include <stdio.h>
#include <string.h>
#include <float.h>


#ifdef __APPLE__
// include files required for compilation on Mac computers (assumes that either XCode has been installed, or at least Apple's Command Line Tools)

#include <CoreFoundation/CFByteOrder.h>

#elif defined (_MSC_VER)
// include files required for compilation on Windows Machines (ie: VS installed for C++ programming)

#endif


// Platform-independent way of representing floats (32 bit) and doubles (64 bit)
typedef uint32_t PCH_FloatBigEndian;
typedef uint64_t PCH_DoubleBigEndian;


// This is how Apple represents 128-bit integers, and if it's good enough for them, it's good enough for me
typedef struct
{
    int64_t high;
    uint64_t low;
    
} PCH_int128_t;

// Available routines

// Integer swaps from the host computer's representation to Big-endian
int16_t PCH_SwapInt16HostToBig(int16_t x);
int32_t PCH_SwapInt32HostToBig(int32_t x);
int64_t PCH_SwapInt64HostToBig(int64_t x);
PCH_int128_t PCH_SwapInt128HostToBig(PCH_int128_t x);

// Integer swaps from Big-endian to the host computer's representation
int16_t PCH_SwapInt16BigToHost(int16_t x);
int32_t PCH_SwapInt32BigToHost(int32_t x);
int64_t PCH_SwapInt64BigToHost(int64_t x);
PCH_int128_t PCH_SwapInt128HBigToHost(PCH_int128_t x);

// Float & double swaps from the host computer's representation to Big-endian
PCH_FloatBigEndian PCH_SwapFloatHostToBig(float x);
PCH_DoubleBigEndian PCH_SwapDoubleHostToBig(double x);

// Float & double swaps Big-endian to the host computer's representation
float PCH_SwapFloatBigToHost(PCH_FloatBigEndian x);
double PCH_SwapDoubleBigToHost(PCH_DoubleBigEndian x);

// close the 'extern "C" clause from above (for C++)
#ifdef __cplusplus
}
#endif

#endif /* PCH_NumericManipulations_h */
