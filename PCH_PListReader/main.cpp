//
//  main.cpp
//  PCH_PListReader
//
//  Created by Peter Huber on 2019-12-18.
//  Copyright © 2019 Peter Huber. All rights reserved.
//

#include <iostream>
#include <string>

#include "PCH_PList.hpp"

using namespace std;

int main(int argc, const char * argv[]) {
    
    // no error checking, just assume that a valid plist file has been passed, then pass control off to the reading routines
    
    int testInt = 0x1234;
    char testBuff[2];
    char *ptr = (char *)&testInt;
    testBuff[0] = *ptr;
    ptr++;
    testBuff[1] = *ptr;
    
    
    string filePath(argv[1]);
    
    PCH_PList test(filePath);
    
    return 0;
}
