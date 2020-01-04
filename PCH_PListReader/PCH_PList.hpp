//
//  PCH_PList.hpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-18.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

// A C++ class to encapsulate a binary ".plist" file. While a plist file is often represented as a text file in XML format, this class is NOT designed to work with text-based plist (XML) files. The layout of a binary plist file is defined below (from https://opensource.apple.com/source/CF/CF-550/CFBinaryPList.c ). Note that all numerical references contained in the file are in big-endian form, which requires a conversion to small-endian for most modern computer systems (basically, all PCs and all Intel-based Macs). A lot of the other info used here comes from https://medium.com/@karaiskc/understanding-apples-binary-property-list-format-281e6da00dbd

/* BINARY PLIST FILE FORMAT
 
HEADER (8 bytes)
    magic number ("bplist")
    file format version

OBJECT TABLE
    variable-sized objects

    Object Formats (marker byte followed by additional info in some cases)
    null    0000 0000
    bool    0000 1000                       // false
    bool    0000 1001                       // true
    fill    0000 1111                       // fill byte
    int     0001 nnnn    ...                // # of bytes is 2^nnnn, big-endian bytes
    real    0010 nnnn    ...                // # of bytes is 2^nnnn, big-endian bytes
    date    0011 0011    ...                // 8 byte float follows, big-endian bytes
    data    0100 nnnn    [int]    ...       // nnnn is number of bytes unless 1111 then int count follows, followed by bytes
    string  0101 nnnn    [int]    ...       // ASCII string, nnnn is # of chars, else 1111 then int count, then bytes
    string  0110 nnnn    [int]    ...       // Unicode string, nnnn is # of chars, else 1111 then int count, then big-endian 2-byte uint16_t
            0111 xxxx                       // unused
    uid     1000 nnnn    ...                // nnnn+1 is # of bytes
            1001 xxxx                       // unused
    array   1010 nnnn    [int]    objref*   // nnnn is count, unless '1111', then int count follows
            1011 xxxx                       // unused
    set     1100 nnnn    [int]    objref*   // nnnn is count, unless '1111', then int count follows
    dict    1101 nnnn    [int]    keyref*   objref*    // nnnn is count, unless '1111', then int count follows
            1110 xxxx                       // unused
            1111 xxxx                       // unused

OFFSET TABLE
    list of ints, byte size of which is given in trailer
    -- these are the byte offsets into the file
    -- number of these is in the trailer

TRAILER (32 bytes)
    byte size of offset ints in offset table
    byte size of object refs in arrays and dicts
    number of offsets in offset table (also is number of objects)
    element # in offset table which is top level object
    offset table offset

*/

#ifndef PCH_PList_hpp
#define PCH_PList_hpp

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#include "PCH_NumericManipulations.h"

using namespace std;

// The header and trailer lengths were found at https://medium.com/@karaiskc/understanding-apples-binary-property-list-format-281e6da00dbd
#define PCH_PLIST_HEADER_LENGTH     8   // bytes
#define PCH_PLIST_TRAILER_LENGTH    32  // bytes

// forward declarations for structs used in the PCH_PList class
struct PCH_PList_Entry;
struct PCH_PList_Dict;
struct PCH_PList_Value;



// The PCH_PList class, which is the C++ encapsulation of a binary plist file. The usual way to use the class is by using the constructor that takes a file path as an argument, after which the class will be populated (assuming that the file is a valid binary plist file). The other way is to create an instance using the default constructor (the one without arguments), then  call InitializeWithFile() before using the instance.

class PCH_PList
{
    
public:
    
    // the different object formats as listed in the file format
    enum ObjectType
    {
        nullType,
        boolFalseType,
        boolTrueType,
        fillType,
        int64Type,
        // int128Type,
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
    
    enum ErrorType
    {
        noError,
        errorCouldNotOpenFile,
        errorNotValidPlistFile,
        errorUnknownObjectType,
        errorIllegalRealLength
    };
    
    // Instance variables
    
    // Indicator for whether the instance has been initialized. Calling programs should test this flag before calling any of the routines to ensure that the instance has been initialized correctly.
    bool isValid;
    
    // buffer to hold the 8-byte header
    char headerBuffer[PCH_PLIST_HEADER_LENGTH];
    
    // the root of the plist (usually a dictionary)
    PCH_PList_Value *plistRoot;
    
    // The number of spaces per "indent" (used by the TraversePlist() call)
    int numSpacesPerTab = 4;
    
    // constructors & destructor
    PCH_PList();
    PCH_PList(string pathName);
    virtual ~PCH_PList();
    
    // Function to initialize the class using the file at 'filepath'. The function returns an PCH_PList::ErrorType, which gives a bit of information as to why the function failed (if the call is successful, it returns PCH_PList::ErrorType::noError).
    ErrorType InitializeWithFile(string filePath);
    
    // Function to traverse the PCH_PList. This function can be used to view a textual representation of the plist file in a "pseudo-XML" style.
    void TraversePlist(ostream& outStream = cout);
    
private:
    
    // ivars
    // the basic object array for the objects represented in the file
    vector<PCH_PList_Entry *> objectArray;
    
    // methods
    PCH_PList_Value *GetValue(PCH_PList_Entry *entry);
    
    void TraverseNode(ostream& outStream, PCH_PList_Value *node, int numTabs);
};

// The plist file is converted into a list of actual objects, each of which is saved as the following structure. Using this method (a type specifier and a union of possible types, only one of which will actually be used by the object) lets us create concrete-named objects instead of using void pointers and a bunch of ugly casting.
struct PCH_PList_Value
{
    enum pch_value_type {Null, Bool, Int, Double, Date, Data, AsciiString, UnicodeString, Uid, Array, Set, Dict} valueType;
    
    // dictionaries are saved as vectors of distStructs (defined here)
    struct dictStruct
    {
        PCH_PList_Value *key;
        PCH_PList_Value *val;
    };
    
    union pch_value
    {
        bool boolValue;
        int64_t intValue;
        double doubleValue;
        double dateValue;
        vector<char> *dataValue;
        string *asciiStringValue;
        wstring *uniStringValue;
        int64_t uidValue;
        vector<PCH_PList_Value *> *arrayValue;
        vector<PCH_PList_Value *> *setValue;
        vector<dictStruct> *dictValue;
        
    } value;
    
    // constructor
    PCH_PList_Value() {this->valueType = Null;}
    
    // destructor
    ~PCH_PList_Value();
};

// Each object in the file is stored into a PCH_PList_Entry for subsequent processing
struct PCH_PList_Entry
{
    PCH_PList::ObjectType entryType; // all object types have this ivar set
    size_t dataSize; // only those entries whose size is non-fixed need to have this ivar set
    void *data; // a pointer to the actual data for the type (this is set using the 'new' operator so that it can be deleted in the destructor)
    
    // constructor
    PCH_PList_Entry(PCH_PList::ObjectType entryType, size_t dataSize, void *data) : entryType(entryType), dataSize(dataSize), data(data) {}
    
    // destructor
    virtual ~PCH_PList_Entry();
};

// For dictionaries, we need to store both a key and a value offset, so create a struct for it
struct PCH_PList_Dict
{
    uint64_t keyOffset;
    uint64_t valueOffset;
    
    // constructor
    PCH_PList_Dict(uint64_t keyOffset, uint64_t valueOffset) : keyOffset(keyOffset), valueOffset(valueOffset) {}
};

#endif /* PCH_PList_hpp */
