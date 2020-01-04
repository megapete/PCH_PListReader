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

struct PCH_UnarchivedStruct
{
    // the name of the struct
    string name;
    
    // a vector of the structs from which this struct is derived
    vector<string> superClasses;
    
    // a struct to define each member
    struct memberDef
    {
        PCH_UnarchivedType type;
        string name;
        
        memberDef() {this->type = Undefined; name = "";}
    };
    
    // the vector of struct members
    vector<memberDef> members;
    
    PCH_UnarchivedStruct() {this->typeName = "Struct";}
    virtual ~PCH_UnarchivedStruct();
    
    string TypeName() {return this->typeName;}
    
protected:
    
    string typeName;
};

struct PCH_UnarchivedClass : PCH_UnarchivedStruct
{
    PCH_UnarchivedClass() {this->typeName = "Class";}
};

struct PCH_UnarchivedMember
{
    PCH_UnarchivedStruct *parentStruct;
    
    void *value;
    
};

class PCH_UnarchivedModel
{
    bool isValid;
    
    PCH_PList_Value *pchPlistRoot;
    
    PCH_UnarchivedModel(PCH_PList_Value *root);
};

#endif /* PCH_NSKeyedArchiver_Analyzer_hpp */
