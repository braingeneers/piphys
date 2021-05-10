//
//  main.cpp
//  Libspi
//
//  Created by Jinghui Geng on 8/27/19.
//  Copyright © 2019 Jinghui Geng. All rights reserved.
//

#include <iostream>
#include "raspiboard.h"

using namespace std;

int main(int argc, const char * argv[]) {
    
    int commandLoop = 1;  // just to initiate the constructor
    
    cout <<"program start" << endl; 
    RaspiBoard theBoard(commandLoop);  

    return 0;
}
