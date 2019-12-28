//
//  main.cpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-18.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <map>


#include "PCH_PList.hpp"

using namespace std;

int main(int argc, const char * argv[]) {
    
    // no error checking, just assume that a valid plist file has been passed as the first argument followed by an optional output file name
    
    string filePath(argv[1]);
    
    PCH_PList inplist(filePath);

    if (!inplist.isValid)
    {
        cerr << "Could not create PCH_Plist instance!!!";
        return 1;
    }
    
    if (argc > 2)
    {
        ofstream outFile;
        
        outFile.open (argv[2], std::ofstream::out | std::ofstream::app);
        
        inplist.TraversePlist(outFile);
        
    }
    else
    {
        inplist.TraversePlist();
    }
    
    return 0;
}
