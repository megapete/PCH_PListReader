//
//  PCH_NumericManipulations.c
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-20.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

#include "PCH_NumericManipulations.h"

#ifdef __APPLE__

int16_t PCH_SwapInt16HostToBig(int16_t x)
{
    return CFSwapInt16HostToBig(x);
}

int32_t PCH_SwapInt32HostToBig(int32_t x)
{
    return CFSwapInt32HostToBig(x);
}

int64_t PCH_SwapInt64HostToBig(int64_t x)
{
    return CFSwapInt64HostToBig(x);
}

int16_t PCH_SwapInt16BigToHost(int16_t x)
{
    return CFSwapInt16BigToHost(x);
}

int32_t PCH_SwapInt32BigToHost(int32_t x)
{
    return CFSwapInt32BigToHost(x);
}

int64_t PCH_SwapInt64BigToHost(int64_t x)
{
    return CFSwapInt64BigToHost(x);
}

PCH_FloatBigEndian PCH_SwapFloatHostToBig(float x)
{
    PCH_FloatBigEndian result;
    
    if (CFByteOrderGetCurrent() == CFByteOrderBigEndian)
    {
        memcpy(&result, &x, sizeof(result));
    }
    else
    {
        CFSwappedFloat32 tmp = CFConvertFloatHostToSwapped(x);
        memcpy(&result, &tmp, sizeof(result));
    }
    
    return result;
}

PCH_DoubleBigEndian PCH_SwapDoubleHostToBig(double x)
{
    PCH_DoubleBigEndian result;
    
    if (CFByteOrderGetCurrent() == CFByteOrderBigEndian)
    {
        memcpy(&result, &x, sizeof(result));
    }
    else
    {
        CFSwappedFloat64 tmp = CFConvertDoubleHostToSwapped(x);
        memcpy(&result, &tmp, sizeof(result));
    }
    
    return result;
}

float PCH_SwapFloatBigToHost(PCH_FloatBigEndian x)
{
    float result = FLT_MAX;
    
    if (CFByteOrderGetCurrent() == CFByteOrderBigEndian)
    {
        memcpy(&result, &x, sizeof(result));
    }
    else
    {
        CFSwappedFloat32 tmp;
        memcpy(&tmp, &x, sizeof(tmp));
        result = (float)CFConvertFloat32SwappedToHost(tmp);
    }
    
    return result;
}

double PCH_SwapDoubleBigToHost(PCH_DoubleBigEndian x)
{
    double result = DBL_MAX;
    
    if (CFByteOrderGetCurrent() == CFByteOrderBigEndian)
    {
        memcpy(&result, &x, sizeof(result));
    }
    else
    {
        CFSwappedFloat64 tmp;
        memcpy(&tmp, &x, sizeof(tmp));
        result = (float)CFConvertDoubleSwappedToHost(tmp);
    }
    
    return result;
}


#elif defined (_MSC_VER)

#endif
