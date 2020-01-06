//
//  PCH_NSKeyedArchiver_Analyzer.hpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-31.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

// This class takes a PCH_PList instance where the original plist is an archive that was created with NSKeyedArchiver. It then interprets the PCH_PList in such a way that the PCH_PList can "easily" create C++ based classes and instances for those classes. This class should be used as a base class for more concrete classes that know the exact format of the original (Swift or ObjC) object model.

#ifndef PCH_NSKeyedArchiver_Analyzer_hpp
#define PCH_NSKeyedArchiver_Analyzer_hpp

#include <stdio.h>
#include "PCH_PList.hpp"

#include <string>
#include <vector>

using namespace std;

// This appears to be the only value allowed by Apple
#define PCH_NSKEYEDARCHIVER_VERSION  100000

enum PCH_UnarchivedType
{
    Undefined,
    Bool,
    Int,
    Double,
    Date,
    Data,
    String,
    Array,
    Set,
    Dict,
    Enum,
    Class,
    Struct
};

struct PCH_UnarchivedBase
{
    PCH_UnarchivedType type;
    
    PCH_UnarchivedBase() {this->type = Undefined;}
    PCH_UnarchivedBase(PCH_UnarchivedType wType) {this->type = wType;}
    
    string TypeName();
};

// I believe that all objects that can be serialized by NSKeyedArchive have to be _classes_, but I'm not 100% sure, so I'll allow the possibility for future expansion by allowing the definition of structs too.
struct PCH_UnarchivedStruct : PCH_UnarchivedBase
{
    // the name of the struct
    string name;
    
    // a vector of the structs from which this struct is derived
    vector<string> supers;
    
    // a struct to define each member
    struct memberDef
    {
        PCH_UnarchivedBase baseType;
        string name;
    };
    
    // the vector of struct members
    vector<memberDef> members;
    
    PCH_UnarchivedStruct() {this->type = Struct;}
    virtual ~PCH_UnarchivedStruct() {};
};

struct PCH_UnarchivedClass : PCH_UnarchivedStruct
{
    PCH_UnarchivedClass() {this->type = Class;}
    virtual ~PCH_UnarchivedClass() {};
};

struct PCH_UnarchivedMember:PCH_UnarchivedBase
{
    PCH_UnarchivedType type;
    
    union pch_memberValue
    {
        bool boolVal;
        int64_t intVal;
        double doubleVal;
        double dateVal;
        vector<char> *dataVal;
        string *stringVal;
        
    } value;
    
};

class PCH_UnarchivedModel
{
public:
    
    bool isValid;
    
    PCH_PList_Value *pchPlistRoot;
    
    PCH_UnarchivedBase *rootItem;
    
    PCH_UnarchivedModel(PCH_PList_Value *root);
    
private:
    
    int version; // always 100000
    
    vector<PCH_PList_Value *> objects;
    
    // members
    vector<PCH_UnarchivedMember> members;
    
    PCH_UnarchivedBase *ExpandValue(PCH_PList_Value *plistValue);
    
    PCH_UnarchivedBase *ExpandObjectAtIndex(const int index);
    
    PCH_UnarchivedClass *ExpandClassDefinitionWith(const vector<PCH_PList_Value::dictStruct> dict);
};

#endif /* PCH_NSKeyedArchiver_Analyzer_hpp */
