//
//  PCH_NSKeyedArchiver_Analyzer.cpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-31.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

#include "PCH_NSKeyedArchiver_Analyzer.hpp"

PCH_UnarchivedModel::PCH_UnarchivedModel(PCH_PList_Value *root)
{
    // assume that this is not a valid list
    this->isValid = false;
    
    // if the root is not a dictionary, this can't be an archive, so return
    if (root->valueType != PCH_PList_Value::Dict)
    {
        cerr << "Root is not a dictionary!" << endl;;
        return;
    }
    
    bool foundArchiver = false;
    vector<PCH_PList_Value *> *objects = nullptr;
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
                    objects = nextEntry.val->value.arrayValue;
                }
            }
        }
    }
    
    if ((!foundArchiver) || (objects == nullptr))
    {
        cerr << "This is not an archive!" << endl;
    }
    
    
    
    
    // if we get to the end, this is a valid NSKeyedArchiver file
    this->isValid = true;
}
