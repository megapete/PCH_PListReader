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

PCH_PList::PCH_PList()
{
    this->isValid = false;
    this->root = NULL;
}

PCH_PList::PCH_PList(string pathName)
{
    this->isValid = this->InitializeWithFile(pathName);
}

PCH_PList::~PCH_PList()
{
    
}

PCH_PList::ErrorType PCH_PList::InitializeWithFile(string filePath)
{
    ifstream pFile;
    
    pFile.open(filePath.c_str());
    
    if (!pFile.is_open())
    {
        return errorCouldNotOpenFile;
    }
    
    // This "safe" calculation of filelength comes from https://stackoverflow.com/questions/22984956/tellg-function-give-wrong-size-of-file/22986486#22986486
    pFile.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize fileLength = pFile.gcount();
    pFile.clear(); //  Since ignore will have set eof, we clear it
    
    // We start out by reading the data in the file's trailer
    streampos trailerStart = fileLength - PCH_PLIST_TRAILER_LENGTH;
    // The first 6 bytes of the trailer are unused by our class, so we skip past them
    pFile.seekg(trailerStart + (streampos)6);
    
    char tByteBuff;
    
    // get the offset_table_offset_size
    pFile.read(&tByteBuff, 1);
    int offset_table_offset_size = (int)tByteBuff;
    
    // get the object_ref_size
    pFile.read(&tByteBuff, 1);
    int object_ref_size = (int)tByteBuff;
    
    // get the number of objects in the file (note that as a number, this value is in Big-endian format, so we need to convert it to the host computer's method of numerical representation
    char eBuff[8];
    pFile.read(eBuff, 8);
    uint64_t numObjects;
    memcpy(&numObjects, eBuff, 8);
    numObjects = PCH_SwapInt64BigToHost(numObjects);
    
    // get the offset to the "top" object in the list of objects
    pFile.read(eBuff, 8);
    uint64_t top_object_offset;
    memcpy(&top_object_offset, eBuff, 8);
    top_object_offset = PCH_SwapInt64BigToHost(top_object_offset);
    
    // Get the location (in bytes from the beginning of the file) of the offset table
    pFile.read(eBuff, 8);
    int64_t offset_table_start;
    memcpy(&offset_table_start, eBuff, 8);
    offset_table_start = PCH_SwapInt64BigToHost(offset_table_start);
    
    pFile.clear(); // The last read will have set eof, we clear it and go back to the beginning of the file
    pFile.seekg(0, std::ios_base::beg);
    
    // read the header and store it
    pFile.read(this->headerBuffer, PCH_PLIST_HEADER_LENGTH);
    
    // Test the first 6 bytes of the header and make sure they are equal to the string "bplist", otherwise return false
    if (strncmp(this->headerBuffer, "bplist", 6) != 0)
    {
        cerr << "This is not a valid plist file";
        return errorNotValidPlistFile;
    }
    
    // Iterate through all the objects in the file. Basically, we read "objects" until we reach the beginning of the offset table (whose position we have already extracted from the file in the section above).
    while (pFile.tellg() < offset_table_start)
    {
        // read the next marker byte
        char mBuff;
        pFile.read(&mBuff, 1);
        uint8_t markerByte = mBuff;
        
        // each marker byte encodes two pieces of 4-bit information (we'll call them "highNibble" and "lowNibble").
        uint8_t highNibble = markerByte & 0xF0;
        // we want to shift the value of the high nibble down to its lower 4 bits, so we shift it to the right
        highNibble >>= 4;
        
        uint8_t lowNibble = markerByte & 0x0F;
        
        switch (highNibble) {
            
            // null, bool, and fill types
            case 0x0:
            {
                if (lowNibble == 0x0)
                {
                    this->objectArray.push_back(PCH_PList_Entry(nullType, 0, NULL));
                }
                else if (lowNibble == 0x08)
                {
                    this->objectArray.push_back(PCH_PList_Entry(boolFalseType, 0, NULL));
                }
                else if (lowNibble == 0x09)
                {
                    this->objectArray.push_back(PCH_PList_Entry(boolTrueType, 0, NULL));
                }
                else if (lowNibble == 0x0F)
                {
                    this->objectArray.push_back(PCH_PList_Entry(fillType, 0, NULL));
                }
                else
                {
                    cerr << "An unknown object type was encountered";
                    return errorUnknownObjectType;
                }
                break;
            }
                
            // integer types
            case 0x01:
            {
                // The number of bytes in the integer are encoded in the lowNibble, as 2^lowNibble.
                // We use bitwise shifting of the number 1 to calculate the power of 2
                int numberOfBytesToRead = 1 << (int)lowNibble;
                
                if (numberOfBytesToRead > 8)
                {
                    // 128-bit integers are a bit of a mess. I'll develop this if and only if really I need it.
                    cerr << "128-bit integers have not been implemented yet.";
                    return errorUnknownObjectType;
                    
                    break;
                }
                else
                {
                    // initialize an 8-byte buffer to all zeros
                    char buffer[8] = {0};
                    // get a pointer to the start of the buffer
                    char *buffPtr = buffer;
                    // we only want to copy as many bytes as specified by the lowNibble, so we advance the pointer to "pad" the number with zeroes
                    buffPtr += (8 - numberOfBytesToRead);
                    pFile.read(buffPtr, numberOfBytesToRead);
                    // copy the bytes we just read into an int64_t type
                    int64_t data;
                    memcpy(&data, buffer, 8);
                    // data is in Big-endian, so convert it as necessary
                    data = PCH_SwapInt64BigToHost(data);
                    // creata a new pointer with the converted data and add it to our object array
                    int64_t *dataPtr = new int64_t(data);
                    this->objectArray.push_back(PCH_PList_Entry(int64Type, sizeof(int64_t), dataPtr));
                }
                
                break;
            }
            
            // real (float and double) types
            case 0x02:
            {
                // The number of bytes in the number are encoded in the lowNibble, as 2^lowNibble. This value should be either 4 (float) or 8 (double)
                // We use bitwise shifting of the number 1 to calculate the power of 2
                int numberOfBytesToRead = 1 << (int)lowNibble;
                
                // make sure there are at least 4 bytes to read
                if (numberOfBytesToRead < sizeof(float))
                {
                    cerr << "Illegal number of bytes for real type";
                    return errorIllegalRealLength;
                }
                
                if (numberOfBytesToRead == sizeof(float))
                {
                    // initialize the buffer and read the data into it
                    char buffer[numberOfBytesToRead];
                    pFile.read(buffer, numberOfBytesToRead);
                    float data;
                    PCH_FloatBigEndian bigData;
                    // copy the data from the file into bigData and convert it from Big-endian
                    memcpy(&bigData, buffer, numberOfBytesToRead);
                    data = PCH_SwapFloatBigToHost(bigData);
                    // creata a new pointer with the converted data and add it to our object array
                    double *dataPtr = new double(data);
                    this->objectArray.push_back(PCH_PList_Entry(doubleType, sizeof(double), dataPtr));
                }
                else // must be double
                {
                    // see the comments above for reading a float
                    char buffer[numberOfBytesToRead];
                    pFile.read(buffer, numberOfBytesToRead);
                    double data;
                    PCH_DoubleBigEndian bigData;
                    memcpy(&bigData, buffer, numberOfBytesToRead);
                    data = PCH_SwapDoubleBigToHost(bigData);
                    double *dataPtr = new double(data);
                    this->objectArray.push_back(PCH_PList_Entry(doubleType, sizeof(double), dataPtr));
                }
                
                break;
            }
                
            // date
            case 0x03:
            {
                // dates are 64-bit (8-byte) real numbers, ie: doubles (see the comments for reading floats above for the procedure the code follows)
                int numberOfBytesToRead = 8;
                char buffer[numberOfBytesToRead];
                pFile.read(buffer, numberOfBytesToRead);
                double data;
                PCH_DoubleBigEndian bigData;
                memcpy(&bigData, buffer, numberOfBytesToRead);
                data = PCH_SwapDoubleBigToHost(bigData);
                double *dataPtr = new double(data);
                this->objectArray.push_back(PCH_PList_Entry(dateType, sizeof(double), dataPtr));
                
                break;
            }
                
            // data
            case 0x04:
            {
                // data is represented as a contiguous string of bytes
                
                // get the lowNibble value into a local variable. If this value is less than 0xF (decimal 15), it is the actual number of bytes of data that follow.
                int64_t count = (int64_t)lowNibble;
                
                // according to the format specification, for data objects, if the lowerNibble is equal to 1111 (hexadecinal 0xF), then the actual byte count is encoded differently
                if (count == 0xF)
                {
                    // The low nibble of the NEXT byte is used to calculate the number of bytes of data that are available for reading
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    // The number of bytes that hold the  is actually 2^countLen
                    countLen = 1 << countLen;
                    
                    // Initialize an 8-byte buffer to all zeroes
                    char countBuff[8] = {0};
                    // set a pointer to the beginning of the buffer
                    char *countBuffPtr = countBuff;
                    // we advance the pointer so we only read as many bytes as needed
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    // copy the result into count, but then we need to convert from Big-endian
                    memcpy(&count, countBuff, 8);
                    count = PCH_SwapInt64BigToHost(count);
                }
                
                // read and save the next count bytes
                char *cResult = new char[count];
                pFile.read(cResult, count);
                
                this->objectArray.push_back(PCH_PList_Entry(dataType, count, cResult));
                
                break;
            }
                
            // ASCII string
            case 0x05:
            {
                // the procedure to figure out how many bytes to read in is the same as for data objects, above
                int64_t charCount = (int64_t)lowNibble;
                
                if (charCount == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&charCount, countBuff, 8);
                    charCount = PCH_SwapInt64BigToHost(charCount);
                }
                
                char cResult[charCount];
                pFile.read(cResult, charCount);

                auto result = new string(cResult, charCount);
                
                this->objectArray.push_back(PCH_PList_Entry(asciiStringType, charCount, result));
                
                break;
            }
                
            // Unicode string
            case 0x06:
            {
                // Unicode strings are a pain because each character (wchar_t) is 16-bits (2-bytes) long. And those bytes are Big-endian. Sigh.
                // To start, we calculate start using the same method as for data objects, above.
                int64_t charCount = (int64_t)lowNibble;
                
                if (charCount == 0xF)
                {
                    char countLenBuff;
                    pFile.read(&countLenBuff, 1);
                    int64_t countLen = (int64_t)countLenBuff & 0x0F;
                    countLen = 1 << countLen;
                    
                    char countBuff[8] = {0};
                    char *countBuffPtr = countBuff;
                    countBuffPtr += (8 - countLen);
                    pFile.read(countBuffPtr, countLen);
                    memcpy(&charCount, countBuff, 8);
                    charCount = PCH_SwapInt64BigToHost(charCount);
                }
                
                // read in 2 bytes at a time, converting from Big-endian each time, and appeding the result to the resultString
                uint16_t wcharBuff;
                wstring resultString(L"");
                for (int i=0; i<charCount; i++)
                {
                    pFile.read((char *)&wcharBuff, 2);
                    
                    wcharBuff = PCH_SwapInt16BigToHost(wcharBuff);
                    
                    wchar_t nextChar = wcharBuff;
                    
                    resultString += nextChar;
                }
                
                auto result = new wstring(resultString, charCount);
                
                this->objectArray.push_back(PCH_PList_Entry(unicodeStringType, charCount, result));
                
                break;
            }
                
            // UID
            // The UID is the "User ID" on Mac OSX systems, but I don't understand why this would ever be useful information to save to a file. In any case, we take care of it.
            case 0x08:
            {
                // unlike just about every other type of object, the number of bytes to read the UID is (lowNibble + 1)
                int64_t count = (int64_t)lowNibble + 1;
                
                // The only documentation that I could find regarding the UID in a plist file is very unclear as to whether a) it's a number and b) if it is encoded as Big-endian. This code assumes a char array and ignores endian-ness.
                char *cResult = new char[count];
                pFile.read(cResult, count);
                
                this->objectArray.push_back(PCH_PList_Entry(uidType, count, cResult));
                
                break;
            }
                
            // array or set
            case 0x0A:
            case 0x0C:
            {
                // The code for extracting an array or a set is identical - the only time that the difference interests us is when we create the PCH_PList_Entry for the object
                ObjectType obType = (highNibble == 0x0A ? arrayType : setType);
                
                // extract the byte count using the same method as for data objects, above
                int64_t count = (int64_t)lowNibble;
                
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
                
                // The members of the array/set are actually indices into the object array itself. Naturally, the indices are in Big-endian format, which needs to be dealt with
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
                
                this->objectArray.push_back(PCH_PList_Entry(obType, count, result));
                
                break;
            }
            
            // dictionary
            case 0x0D:
            {
                // dictionairies are similar to arrays/sets, except that instead of a single index into the object array, there are two: one for the key and the other for the value. The methods used to extract these indices is the same as for arrays/sets.
                int64_t count = (int64_t)lowNibble;
                
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
                
                this->objectArray.push_back(PCH_PList_Entry(dictType, count, result));
                
                break;
            }
                
            default:
            {
                cerr << "An unknown object type was encountered";
                return errorUnknownObjectType;
                
                break;
            }
        }
    }
    
    cout << "Done reading objects\n";
    
    
    
    
    return noError;
}

PCH_PList_Value *PCH_PList::GetValue(PCH_PList_Entry *entry)
{
    auto entryType = entry->entryType;
    
    auto result = new PCH_PList_Value;
    
    switch (entryType) {
            
        case boolTrueType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Bool;
            result->value.boolValue = true;

            break;
        }
           
        case boolFalseType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Bool;
            result->value.boolValue = false;
            
            break;
        }
            
        case int64Type:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Int;
            result->value.intValue = *(int64_t*)(entry->data);
            
            break;
        }
            
        case dateType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Date;
            result->value.dateValue = *(double*)(entry->data);
            
            break;
        }
            
        case doubleType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Double;
            result->value.doubleValue = *(double*)(entry->data);
            
            break;
        }
            
        case dataType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Data;
            char *bytePtr = (char *)entry->data;
            // this method comes from the "Initializing from an array" part of https://www.geeksforgeeks.org/initialize-a-vector-in-cpp-different-ways/
            result->value.dataValue = new vector<char>(bytePtr, bytePtr+entry->dataSize);
            
            break;
        }
            
        case asciiStringType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::AsciiString;
            result->value.asciiStringValue = new string((char *)entry->data, entry->dataSize);
            
            break;
        }
        
        case unicodeStringType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::UnicodeString;
            result->value.uniStringValue = new wstring((wchar_t *)entry->data, entry->dataSize);
            
            break;
        }
            
        case uidType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Uid;
            char *bytePtr = (char *)entry->data;
            // this method comes from the "Initializing from an array" part of https://www.geeksforgeeks.org/initialize-a-vector-in-cpp-different-ways/
            result->value.uidValue = new vector<char>(bytePtr, bytePtr+entry->dataSize);
            
            break;
        }
            
        case arrayType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Array;
            
            result->value.arrayValue = new vector<PCH_PList_Value *>();
            
            vector<int64_t> indices = *(vector<int64_t> *)entry->data;
            
            for (int i=0; i<entry->dataSize; i++)
            {
                PCH_PList_Entry nextEntry = this->objectArray[indices[i]];
                
                result->value.arrayValue->push_back(GetValue(&nextEntry));
            }
            
            break;
        }
            
        case setType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Set;
            
            result->value.setValue = new vector<PCH_PList_Value *>();
            
            vector<int64_t> indices = *(vector<int64_t> *)entry->data;
            
            for (int i=0; i<entry->dataSize; i++)
            {
                PCH_PList_Entry nextEntry = this->objectArray[indices[i]];
                
                result->value.setValue->push_back(GetValue(&nextEntry));
            }
            
            break;
        }
            
        case dictType:
        {
            result->valueType = PCH_PList_Value::pch_value_type::Dict;
            
            result->value.dictValue = new vector<PCH_PList_Value::dictStruct>();
            
            // unlike the other collection types, the data field does not hold indices into the objectArray, but the key/value pairs (as PCH_PList_Dict's) - those pairs ARE indices into the object array
            vector<PCH_PList_Dict> dict = *(vector<PCH_PList_Dict> *)entry->data;
            
            for (int i=0; i<entry->dataSize; i++)
            {
                PCH_PList_Entry keyEntry = this->objectArray[dict[i].keyOffset];
                PCH_PList_Entry valEntry = this->objectArray[dict[i].valueOffset];
                 
                PCH_PList_Value::dictStruct tDict;
                tDict.key = GetValue(&keyEntry);
                tDict.val = GetValue(&valEntry);
                
                result->value.dictValue->push_back(tDict);
            }
            
            break;
        }
            
        default:
        {
            break;
        }
    }
    
    return result;
}


// PCH_PList_Value may contain pointers in its value field, so delete them
PCH_PList_Value::~PCH_PList_Value()
{
    switch (this->valueType)
    {
        case Data:
        {
            delete this->value.dataValue;
            break;
        }
            
        case AsciiString:
        {
            delete this->value.asciiStringValue;
            break;
        }
            
        case UnicodeString:
        {
            delete this->value.uniStringValue;
            break;
        }
            
        case Uid:
        {
            delete this->value.uidValue;
            break;
        }
            
        case Array:
        {
            delete this->value.arrayValue;
            break;
        }
            
        case Set:
        {
            delete this->value.setValue;
            break;
        }
            
        case Dict:
        {
            delete this->value.dictValue;
            break;
        }
            
        default:
            break;
    }
}

// the PCH_PList_Entry holds a void*, so we need to delete it properly to avoid memory leaks
PCH_PList_Entry::~PCH_PList_Entry()
{
    switch (this->entryType)
    {
        case PCH_PList::ObjectType::int64Type:
        {
            delete (int64_t *)this->data;
            break;
        }
            
        case PCH_PList::ObjectType::doubleType:
        case PCH_PList::ObjectType::dateType:
        {
            delete (double *)this->data;
            break;
        }
            
        case PCH_PList::ObjectType::dataType:
        case PCH_PList::ObjectType::uidType:
        {
            delete (char *)this->data;
            break;
        }
            
        case PCH_PList::ObjectType::asciiStringType:
        {
            delete (string *)this->data;
            break;
        }
            
        case PCH_PList::ObjectType::unicodeStringType:
        {
            delete (wstring *)this->data;
            break;
        }
            
        case PCH_PList::ObjectType::arrayType:
        case PCH_PList::ObjectType::setType:
        {
            delete (vector<int64_t> *)this->data;
            break;
        }
            
        case PCH_PList::ObjectType::dictType:
        {
            delete (vector<PCH_PList_Dict> *)this->data;
            break;
        }
            
        default:
            break;
    }
}
