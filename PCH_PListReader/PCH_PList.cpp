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

#include <fstream>
#include <cassert>

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
    
    // The calculation of filelength comes from https://stackoverflow.com/questions/22984956/tellg-function-give-wrong-size-of-file/22986486#22986486
    pFile.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize fileLength = pFile.gcount();
    pFile.clear(); //  Since ignore will have set eof, we clear it
    
    streampos trailerStart = fileLength - PCH_PLIST_TRAILER_LENGTH;
    pFile.seekg(trailerStart + (streampos)6);
    char tByteBuff;
    pFile.read(&tByteBuff, 1);
    int offset_table_offset_size = (int)tByteBuff;
    pFile.read(&tByteBuff, 1);
    int object_ref_size = (int)tByteBuff;
    assert(object_ref_size <= 8);
    
    char eBuff[8];
    pFile.read(eBuff, 8);
    uint64_t numObjects;
    memcpy(&numObjects, eBuff, 8);
    numObjects = PCH_SwapInt64BigToHost(numObjects);
    
    pFile.read(eBuff, 8);
    uint64_t top_object_offset;
    memcpy(&top_object_offset, eBuff, 8);
    top_object_offset = PCH_SwapInt64BigToHost(top_object_offset);
    
    pFile.read(eBuff, 8);
    int64_t offset_table_start;
    memcpy(&offset_table_start, eBuff, 8);
    offset_table_start = PCH_SwapInt64BigToHost(offset_table_start);
    
    pFile.clear(); // The last read will have set eof, we clear it
    pFile.seekg(0, std::ios_base::beg);
    
    // read the header and store it (no, I don't know why, but it seems like a good idea)
    pFile.read(this->headerBuffer, PCH_PLIST_HEADER_LENGTH);
    
    while (pFile.tellg() < offset_table_start)
    {
        char mBuff;
        pFile.read(&mBuff, 1);
        
        uint8_t markerByte = mBuff;
        uint8_t upperNibble = markerByte & 0xF0;
        upperNibble >>= 4;
        uint8_t lowerNibble = markerByte & 0x0F;
        
        switch (upperNibble) {
            
            // null, bool, and fill types
            case 0x0:
            {
                if (lowerNibble == 0x0)
                {
                    this->topLevel.push_back(PCH_PList_Entry(nullType, 0, NULL));
                }
                else if (lowerNibble == 0x08)
                {
                    this->topLevel.push_back(PCH_PList_Entry(boolFalseType, 0, NULL));
                }
                else if (lowerNibble == 0x09)
                {
                    this->topLevel.push_back(PCH_PList_Entry(boolTrueType, 0, NULL));
                }
                else if (lowerNibble == 0x0F)
                {
                    this->topLevel.push_back(PCH_PList_Entry(fillType, 0, NULL));
                }
                else RETURN_FALSE_BAD_NIBBLE
                break;
            }
                
            // integer types
            case 0x01:
            {
                // bitwise shifting to calculate power of 2
                int numberOfBytesToRead = 1 << (int)lowerNibble;
                
                if (numberOfBytesToRead > 8)
                {
                    // 128-bit integers are a bit of a mess. I'll develop this only if really I need it.
                    cerr << "Reading of 128-bit integers has not been implemented yet.";
                    assert(0);
                    break;
                }
                else
                {
                    char buffer[8] = {0};
                    char *buffPtr = buffer;
                    buffPtr += (8 - numberOfBytesToRead);
                    pFile.read(buffPtr, numberOfBytesToRead);
                    int64_t data;
                    memcpy(&data, buffer, 8);
                    data = PCH_SwapInt64BigToHost(data);
                    int64_t *dataPtr = new int64_t(data);
                    this->topLevel.push_back(PCH_PList_Entry(int64Type, sizeof(int64_t), dataPtr));
                }
                
                break;
            }
            
            // real types
            case 0x02:
            {
                // bitwise shifting to calculate power of 2
                int numberOfBytesToRead = 1 << (int)lowerNibble;
                
                cerr << pFile.tellg() << endl;
                
                if (numberOfBytesToRead < sizeof(float))
                {
                    assert(0);
                }
                if (numberOfBytesToRead == sizeof(float))
                {
                    char buffer[numberOfBytesToRead];
                    pFile.read(buffer, numberOfBytesToRead);
                    float data;
                    PCH_FloatBigEndian bigData;
                    memcpy(&bigData, buffer, numberOfBytesToRead);
                    data = PCH_SwapFloatBigToHost(bigData);
                    double *dataPtr = new double(data);
                    this->topLevel.push_back(PCH_PList_Entry(doubleType, sizeof(double), dataPtr));
                }
                else // must be double
                {
                    char buffer[numberOfBytesToRead];
                    pFile.read(buffer, numberOfBytesToRead);
                    double data;
                    PCH_DoubleBigEndian bigData;
                    memcpy(&bigData, buffer, numberOfBytesToRead);
                    data = PCH_SwapDoubleBigToHost(bigData);
                    double *dataPtr = new double(data);
                    this->topLevel.push_back(PCH_PList_Entry(doubleType, sizeof(double), dataPtr));
                }
                
                break;
            }
                
            // date
            case 0x03:
            {
                // dates are 64-bit floats, ie: doubles
                int numberOfBytesToRead = 8;
                char buffer[numberOfBytesToRead];
                pFile.read(buffer, numberOfBytesToRead);
                double data;
                PCH_DoubleBigEndian bigData;
                memcpy(&bigData, buffer, numberOfBytesToRead);
                data = PCH_SwapDoubleBigToHost(bigData);
                double *dataPtr = new double(data);
                this->topLevel.push_back(PCH_PList_Entry(dateType, sizeof(double), dataPtr));
                
                break;
            }
                
            // data
            case 0x04:
            {
                // data is represented just like strings
                
                int64_t count = (int64_t)lowerNibble;
                
                if (count == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&count, countBuff, 8);
                    count = PCH_SwapInt64BigToHost(count);
                }
                
                char *cResult = new char[count];
                pFile.read(cResult, count);
                
                this->topLevel.push_back(PCH_PList_Entry(dataType, count, cResult));
                
                break;
            }
                
            // ASCII string
            case 0x05:
            {
                cerr << pFile.tellg() << endl;
                
                int64_t count = (int64_t)lowerNibble;
                
                if (count == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&count, countBuff, 8);
                    count = PCH_SwapInt64BigToHost(count);
                }
                
                char cResult[count];
                pFile.read(cResult, count);

                auto result = new string(cResult, count);
                cerr << result->c_str() << endl;
                
                this->topLevel.push_back(PCH_PList_Entry(asciiStringType, count, result));
                
                break;
            }
                
            // Unicode string
            case 0x06:
            {
                int64_t count = (int64_t)lowerNibble;
                
                if (count == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&count, countBuff, 8);
                    count = PCH_SwapInt64BigToHost(count);
                }
                
                uint16_t wcharBuff;
                wstring resultString(L"");
                for (int i=0; i<count; i++)
                {
                    pFile.read((char *)&wcharBuff, 2);
                    
                    wcharBuff = PCH_SwapInt16BigToHost(wcharBuff);
                    
                    wchar_t nextChar = wcharBuff;
                    
                    resultString += nextChar;
                }
                
                auto result = new wstring(resultString, count);
                
                this->topLevel.push_back(PCH_PList_Entry(unicodeStringType, count, result));
                
                break;
            }
                
            // UID
            case 0x08:
            {
                int64_t count = (int64_t)lowerNibble + 1;
                
                char *cResult = new char[count];
                pFile.read(cResult, count);
                
                this->topLevel.push_back(PCH_PList_Entry(uidType, count, cResult));
                
                break;
            }
                
            // array or set
            case 0x0A:
            case 0x0C:
            {
                ObjectType obType = (upperNibble == 0x0A ? arrayType : setType);
                
                int64_t count = (int64_t)lowerNibble;
                
                if (count == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&count, countBuff, 8);
                    count = PCH_SwapInt64BigToHost(count);
                }
                
                vector<int64_t> values;
                for (int64_t i=0; i<count; i++)
                {
                    char valBuff[sizeof(int64_t)] = {0};
                    char *valBuffPtr = valBuff + (sizeof(int64_t) - object_ref_size);
                    pFile.read(valBuffPtr, object_ref_size);
                    int64_t value;
                    memcpy(&value, valBuff, sizeof(int64_t));
                    value = PCH_SwapInt64BigToHost(value);
                    values.push_back(value);
                }
                
                auto result = new vector<int64_t>(values);
                
                this->topLevel.push_back(PCH_PList_Entry(obType, count * sizeof(int64_t), result));
                
                break;
            }
            
            // dictionary
            case 0x0D:
            {
                int64_t count = (int64_t)lowerNibble;
                
                if (count == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&count, countBuff, 8);
                    count = PCH_SwapInt64BigToHost(count);
                }
                
                vector<int64_t> keys;
                vector<int64_t> values;
                
                for (int64_t i=0; i<count; i++)
                {
                    char keyBuff[sizeof(int64_t)] = {0};
                    char *keyBuffPtr = keyBuff + (sizeof(int64_t) - object_ref_size);
                    pFile.read(keyBuffPtr, object_ref_size);
                    int64_t key;
                    memcpy(&key, keyBuff, sizeof(int64_t));
                    key = PCH_SwapInt64BigToHost(key);
                    keys.push_back(key);
                }
                
                for (int64_t i=0; i<count; i++)
                {
                    char valBuff[sizeof(int64_t)] = {0};
                    char *valBuffPtr = valBuff + (sizeof(int64_t) - object_ref_size);
                    pFile.read(valBuffPtr, object_ref_size);
                    int64_t value;
                    memcpy(&value, valBuff, sizeof(int64_t));
                    value = PCH_SwapInt64BigToHost(value);
                    values.push_back(value);
                }
                
                auto result = new vector<PCH_PList_Dict>();
                
                for (int64_t i=0; i<count; i++)
                {
                    PCH_PList_Dict nextEntry(keys[i], values[i]);
                    result->push_back(nextEntry);
                }
                
                this->topLevel.push_back(PCH_PList_Entry(dictType, count * sizeof(PCH_PList_Dict), result));
                
                break;
            }
                
            default:
            {
                cerr << "An unimplemented marker type was encountered.";
                assert(0);
                break;
            }
        }
    }
    
    cerr << "Done reading objects\n";
    
    return true;
}


