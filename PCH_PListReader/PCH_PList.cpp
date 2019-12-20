//
//  PCH_PList.cpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-18.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

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


