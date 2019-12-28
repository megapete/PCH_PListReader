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
    
    // no error checking, just assume that a valid plist file has been passed, then pass control off to the reading routines
    
    
    vector<double> ll;
    set<double> mm;
    map<string, double> kk;
    
    
    string filePath(argv[1]);
    
    PCH_PList test(filePath);
    
    test.TraversePlist();
    
    return 0;
}
