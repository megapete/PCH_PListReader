//
//  PCH_NSKeyedArchiver_Analyzer.cpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-31.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

#include "PCH_NSKeyedArchiver_Analyzer.hpp"

string PCH_UnarchivedBase::TypeName()
{
    switch (this->type) {
        
        case Bool:
        {
            return string("Bool");
            break;
        }
            
        case Int:
        {
            return string("Int");
            break;
        }
            
        case Double:
        {
            return string("Int");
            break;
        }
            
        case Date:
        {
            return string("Date");
            break;
        }
            
        case Data:
        {
            return string("Data");
            break;
        }
            
        case String:
        {
            return string("String");
            break;
        }
            
        case Array:
        {
            return string("Array");
            break;
        }
            
        case Set:
        {
            return string("Set");
            break;
        }
            
        case Dict:
        {
            return string("Dict");
            break;
        }
            
        case Enum:
        {
            return string("Enum");
            break;
        }
            
        case Class:
        {
            return string("Class");
            break;
        }
            
        case Struct:
        {
            return string("Struct");
            break;
        }
            
        default:
            return string("Undefined");
            break;
    }
}

PCH_UnarchivedModel::PCH_UnarchivedModel(PCH_PList_Value *root)
{
    // assume that this is not a valid list
    this->isValid = false;
    
    // if the root is not a dictionary, this can't be an archive, so just return
    if (root->valueType != PCH_PList_Value::Dict)
    {
        cerr << "Root is not a dictionary!" << endl;;
        return;
    }
    
    // The next block of code uses the logic from https://digitalinvestigation.wordpress.com/2012/04/04/geek-post-nskeyedarchiver-files-what-are-they-and-how-can-i-use-them/ as to the actualk structure of an NSKeyedArchiver file. We do two things in the loop: assure ourselves that this is indeed a valid NSKeyedArchiver-created file and save the most important data (the $object array and the $top dictionary) into local vars.
    
    bool foundArchiver = false;
    int archiverVersion = 0;
    // vector<PCH_PList_Value *> objects;
    PCH_PList_Value::dictStruct topDict;
    bool topIsValid = false;
    for (int i=0; i<root->value.dictValue->size(); i++)
    {
        PCH_PList_Value::dictStruct nextEntry = root->value.dictValue->at(i);
        
        if (nextEntry.key->valueType == PCH_PList_Value::AsciiString)
        {
            if (nextEntry.key->value.asciiStringValue->compare("$archiver") == 0)
            {
                if (nextEntry.val->valueType == PCH_PList_Value::AsciiString)
                {
                    if (nextEntry.val->value.asciiStringValue->compare("NSKeyedArchhiver"))
                    {
                        foundArchiver = true;
                    }
                }
            }
            else if (nextEntry.key->value.asciiStringValue->compare("$objects") == 0)
            {
                if (nextEntry.val->valueType == PCH_PList_Value::Array)
                {
                    this->objects = *nextEntry.val->value.arrayValue;
                }
            }
            else if (nextEntry.key->value.asciiStringValue->compare("$version") == 0)
            {
                if (nextEntry.val->valueType == PCH_PList_Value::Int)
                {
                    archiverVersion = (int)nextEntry.val->value.intValue;
                }
            }
            else if (nextEntry.key->value.asciiStringValue->compare("$top") == 0)
            {
                if (nextEntry.val->valueType == PCH_PList_Value::Dict)
                {
                    topDict = nextEntry.val->value.dictValue->at(0);
                    
                    if (topDict.key->valueType == PCH_PList_Value::AsciiString)
                    {
                        if (topDict.key->value.asciiStringValue->compare("root") == 0)
                        {
                            topIsValid = true;
                        }
                    }
                }
            }
        }
    }
    
    if ((!foundArchiver) || (this->objects.size() == 0) || (archiverVersion != PCH_NSKEYEDARCHIVER_VERSION) || (!topIsValid))
    {
        cerr << "This is not an archive!" << endl;
        return;
    }
    
    int topIndex = (int)topDict.val->value.uidValue;
    
    this->rootItem = this->ExpandObjectAtIndex(topIndex);
    
    // save the original plist pointer so we can re-access it if needed
    this->pchPlistRoot = root;
}


PCH_UnarchivedBase *PCH_UnarchivedModel::ExpandObjectAtIndex(const int index)
{
    PCH_PList_Value *item = this->objects.at(index);
    
    return this->ExpandValue(item);
}


PCH_UnarchivedBase *PCH_UnarchivedModel::ExpandValue(PCH_PList_Value *plistValue)
{
    PCH_PList_Value::pch_value_type vType = plistValue->valueType;
    
    if (vType == PCH_PList_Value::Dict)
    {
        vector<PCH_PList_Value::dictStruct> theDict = *plistValue->value.dictValue;
        
        PCH_PList_Value *classPtr = PCH_PList_Value::ValueForStringKey(theDict, "$class");
        if (classPtr != nullptr)
        {
            // this is a class definition, so hand off
            return this->ExpandClassDefinitionWith(theDict);
        }
    }
    else if (vType == PCH_PList_Value::Int)
    {
        PCH_UnarchivedMember newMember;
    }
    
    return nullptr;
}


PCH_UnarchivedClass *PCH_UnarchivedModel::ExpandClassDefinitionWith(const vector<PCH_PList_Value::dictStruct> dict)
{
    PCH_UnarchivedClass *result = new PCH_UnarchivedClass();
    
    // start out by creating the basic definition of the class
    int classUID = (int)PCH_PList_Value::ValueForStringKey(dict, "$class")->value.uidValue;
    
    vector<PCH_PList_Value::dictStruct> defDict = *this->objects.at(classUID)->value.dictValue;
    
    result->name = *PCH_PList_Value::ValueForStringKey(defDict, "classname")->value.asciiStringValue;
    
    vector<PCH_PList_Value *> superArray = *PCH_PList_Value::ValueForStringKey(defDict, "classname")->value.arrayValue;
    
    for (int i=0; i<superArray.size(); i++)
    {
        result->supers.push_back(*superArray.at(i)->value.asciiStringValue);
    }
    
    // Now go through the members (if any). Essentially, any entry that doesn't have the key '$class' is a member of the class
    for (int i=0; i<dict.size(); i++)
    {
        string nextKey = *dict.at(i).key->value.asciiStringValue;
        
        if (nextKey.compare("$class") != 0)
        {
            PCH_UnarchivedClass::memberDef nextMember;
            nextMember.name = nextKey;
            
            nextMember.baseType = *this->ExpandValue(PCH_PList_Value::ValueForStringKey(dict, nextKey));
            
            result->members.push_back(nextMember);
        }
    }
    
    return result;
}
