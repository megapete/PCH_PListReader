//
//  PCH_PList.hpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-18.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

#ifndef PCH_PList_hpp
#define PCH_PList_hpp

#include <stdio.h>

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "PCH_NumericManipulations.h"

using namespace std;

#define PCH_PLIST_HEADER_LENGTH     8   // bytes
#define PCH_PLIST_TRAILER_LENGTH    32  // bytes

// forward declarations
struct PCH_PList_Entry;
struct PCH_PList_Dict;

// our class
class PCH_PList
{
    
public:
    
    enum ObjectType
    {
        nullType,
        boolFalseType,
        boolTrueType,
        fillType,
        int64Type,
        int128Type,
        doubleType,
        dateType,
        dataType,
        asciiStringType,
        unicodeStringType,
        uidType,
        arrayType,
        setType,
        dictType
    };
    
    // instance variables
    
    // indicator for whether the instance has been initialized
    bool isValid;
    
    // buffer to hold the 8-byte header
    char headerBuffer[PCH_PLIST_HEADER_LENGTH];
    
    // constructors & destructor
    PCH_PList();
    PCH_PList(string pathName);
    virtual ~PCH_PList();
    
    bool InitializeWithFile(string filePath);
    
private:
    
    // ivars
    vector<PCH_PList_Entry> topLevel;
    
    // methods
    // bool DecodeMarker(ifstream& file, vector<PCH_PList_Entry> parent);
};

// 
struct PCH_PList_Entry
{
    PCH_PList::ObjectType entryType;
    size_t dataSize;
    void *data;
    
    // constructor
    PCH_PList_Entry(PCH_PList::ObjectType entryType, size_t dataSize, void *data) : entryType(entryType), dataSize(dataSize), data(data) {}
    
};

struct PCH_PList_Dict
{
    uint64_t keyOffset;
    uint64_t valueOffset;
    
    // constructor
    PCH_PList_Dict(uint64_t keyOffset, uint64_t valueOffset) : keyOffset(keyOffset), valueOffset(valueOffset) {}
};

#endif /* PCH_PList_hpp */
