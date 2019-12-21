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



#include "PCH_PList.hpp"

using namespace std;

int main(int argc, const char * argv[]) {
    
    // no error checking, just assume that a valid plist file has been passed, then pass control off to the reading routines
    
    /*
    int8_t test8 = 3;
    int16_t test16 = 25;
    int32_t test32 = 257;
    int64_t test64 = 4567;
    
    ofstream file("test_file.bin", ios::binary);
    
    file.write(reinterpret_cast<const char *>(&test16), sizeof(test16));
    file.write(reinterpret_cast<const char *>(&test8), sizeof(test8));
    file.write(reinterpret_cast<const char *>(&test64), sizeof(test64));
    file.write(reinterpret_cast<const char *>(&test32), sizeof(test32));
    */
    
    int64_t testInt = 0;
    
    
    ifstream file("test_file.bin", ios::binary);
    
    file.read(reinterpret_cast<char *>(&testInt), 2);
    file.read(reinterpret_cast<char *>(&testInt), 1);
    file.read(reinterpret_cast<char *>(&testInt), 8);
    file.read(reinterpret_cast<char *>(&testInt), 4);
    
    int x = 1;

    char *y = (char*)&x;

    printf("%c\n",*y+48);
    
    
    
    // string filePath(argv[1]);
    
    // PCH_PList test(filePath);
    
    return 0;
}
