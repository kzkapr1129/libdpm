//
//  main.cpp
//  DPMatching
//
//  Created by 中山一輝 on 2018/10/18.
//  Copyright © 2018年 中山一輝. All rights reserved.
//

#include "dpm.h"
#include <math.h>
#include <iostream>
#include <vector>

uint8_t Score(uint8_t a, uint8_t b) {
    if (a < b) {
        return b - a;
    } else {
        return a - b;
    }
}

int main(int argc, const char * argv[]) {
    
    const unsigned char values1[] = {'2', '6', '4', '7'};
    const size_t len1 = sizeof(values1) / sizeof(unsigned char);
    const unsigned char values2[] = {'5', '6', '2', '6', '4', '7', '3'};
    const size_t len2 = sizeof(values2) / sizeof(unsigned char);
    
    dpm::Result result;
    if (dpm::match(values1, len1, values2, len2, Score, &result)) {
        fprintf(stderr, "Failed dpm::match\n");
    }
    
    printf("start (%d, %d)\n", result.start.values1_i, result.start.values2_i);
    printf("end   (%d, %d)\n", result.end.values1_i, result.end.values2_i);
    
    return 0;
}
