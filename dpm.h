//
//  dmp.h
//  DPMatching
//
//  Created by 中山一輝 on 2018/10/20.
//  Copyright © 2018年 中山一輝. All rights reserved.
//

#ifndef dmp_h
#define dmp_h

#include <stdint.h>
#include <vector>

namespace dpm {
    typedef uint8_t (*ScoreFunc) (uint8_t a, uint8_t b);
    
    struct Pair {
        int values1_i;
        int values2_i;
        
        Pair() : values1_i(0), values2_i(0) {}
    };

    struct Result {
        Pair start;
        Pair end;
        std::vector<Pair> route;
        uint8_t score;
    };

    int match(const uint8_t* values1, int len1,
            const uint8_t* values2, int len2,
            ScoreFunc score, Result* result);
}


#endif /* dmp_h */
