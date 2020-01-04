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
    
    int nextIndexToOpen = (int)topDict.val->value.uidValue;
    
    
    
    // if we get to the end, we have successfully decoded the file
    this->isValid = true;
    
    // save the original plist pointer so we can re-accessit if needed
    this->pchPlistRoot = root;
}

