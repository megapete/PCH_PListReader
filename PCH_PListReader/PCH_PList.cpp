//
//  PCH_PList.cpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-18.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

/* PList Format
 
 HEADER
     magic number ("bplist")
     file format version

 OBJECT TABLE
     variable-sized objects

     Object Formats (marker byte followed by additional info in some cases)
     null       0000 0000
     bool       0000 1000                           // false
     bool       0000 1001                           // true
     fill       0000 1111                           // fill byte
     int        0001 nnnn    ...                    // # of bytes is 2^nnnn, big-endian bytes
     real       0010 nnnn    ...                    // # of bytes is 2^nnnn, big-endian bytes
     date       0011 0011    ...                    // 8 byte float follows, big-endian bytes
     data       0100 nnnn    [int]  ...             // nnnn is number of bytes unless 1111 then int count follows, followed by bytes
     string     0101 nnnn    [int]  ...             // ASCII string, nnnn is # of chars, else 1111 then int count, then bytes
     string     0110 nnnn    [int]  ...             // Unicode string, nnnn is # of chars, else 1111 then int count, then big-endian 2-byte uint16_t
                0111 xxxx                           // unused
     uid        1000 nnnn    ...                    // nnnn+1 is # of bytes
                1001 xxxx                           // unused
     array      1010 nnnn    [int]  objref*         // nnnn is count, unless '1111', then int count follows
                1011 xxxx                           // unused
     set        1100 nnnn    [int]  objref*         // nnnn is count, unless '1111', then int count follows
     dict       1101 nnnn    [int]  keyref* objref* // nnnn is count, unless '1111', then int count follows
                1110 xxxx                           // unused
                1111 xxxx                           // unused

 OFFSET TABLE
     list of ints, byte size of which is given in trailer
     -- these are the byte offsets into the file
     -- number of these is in the trailer

 TRAILER
     byte size of offset ints in offset table
     byte size of object refs in arrays and dicts
     number of offsets in offset table (also is number of objects)
     element # in offset
 
*/

#include "PCH_PList.hpp"

#include <iostream>
#include <fstream>

#define RETURN_FALSE_BAD_NIBBLE {pFile.close(); return false;}

PCH_PList::PCH_PList()
{
    this->isValid = false;
}

PCH_PList::PCH_PList(string pathName)
{
    this->isValid = this->InitializeWithFile(pathName);
}

PCH_PList::~PCH_PList()
{
    
}

bool PCH_PList::InitializeWithFile(string filePath)
{
    ifstream pFile;
    
    pFile.open(filePath.c_str());
    
    if (!pFile.is_open())
    {
        return false;
    }
    
    // read the header and store it (no, I don't know why, but it seems like a good idea)
    pFile.read(this->headerBuffer, PCH_PLIST_HEADER_LENGTH);
    
    bool doneTopLevel = false;
    while (!doneTopLevel)
    {
        char mBuff;
        pFile.read(&mBuff, 1);
        
        int8_t markerByte = mBuff;
        int8_t upperNibble = markerByte >> 4;
        int8_t lowerNibble = markerByte & 0x0F;
        
        switch (upperNibble) {
            
            case 0x0:
            {
                if (lowerNibble == 0x0)
                {
                    this->topLevel.push_back(PCH_PList_Entry(nullType, NULL, NULL));
                }
                else if (lowerNibble == 0x08)
                {
                    this->topLevel.push_back(PCH_PList_Entry(boolFalseType, NULL, NULL));
                }
                else if (lowerNibble == 0x09)
                {
                    this->topLevel.push_back(PCH_PList_Entry(boolTrueType, NULL, NULL));
                }
                else if (lowerNibble == 0x0F)
                {
                    this->topLevel.push_back(PCH_PList_Entry(fillType, NULL, NULL));
                }
                else RETURN_FALSE_BAD_NIBBLE
                break;
            }
                
            case 0x01:
            {
                // bitwise shifting to calculate power of 2
                int numberOfBytesToRead = 1 << lowerNibble;
                char buffer[numberOfBytesToRead];
                pFile.read(buffer, numberOfBytesToRead);
                
                
                break;
            }
            default:
            {
                break;
            }
        }
    }
    
    return true;
}


