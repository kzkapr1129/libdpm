//
//  dmp.cc
//  DPMatching
//
//  Created by 中山一輝 on 2018/10/20.
//  Copyright © 2018年 中山一輝. All rights reserved.
//

#include "dpm.h"
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <cstdio>

namespace dpm {
    static const float WEIGHT_UP = 1.5f;
    static const float WEIGHT_LEFT = 1.5f;
    
    enum Route {
        FROM_UNKNOWN = 0,
        FROM_LEFT,
        FROM_UP,
        FROM_LEFT_UP,
    };
    
    template<typename TYPE>
    static void disp_map(const TYPE* map, int h, int w, const char* tagname, ...);
    static int createCostRouteMap(const uint8_t* score_map, int h, int w,
            int startX, uint8_t* cost_map, Route* route_map);
    static int getLowestCostPair(const uint8_t* cost_map, const Route* route_map,
            int h, int w, int startX, Pair* start, Pair* end, uint8_t* cost);
}

#ifdef DEBUG
template<typename TYPE>
static void dpm::disp_map(const TYPE* map, int h, int w, const char* tagname, ...) {
    va_list ap;
    va_start(ap, tagname);
    char* tag = NULL;
    vasprintf(&tag, tagname, ap);
    va_end(ap);
    
    if (tag) {
        printf("========== %s\n", tag);
        free(tag);

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                printf("%03d ", map[i * w + j]);
            }
            printf("\n");
        }
        printf("\n");
    }
}
#endif

int dpm::createCostRouteMap(const uint8_t* score_map, int height, int width,
                       int startX, uint8_t* cost_map, Route* route_map) {
    
    // 開始位置のindex
    const int start_index = /*h * width + */startX;
    
    // 各マップの開始位置を初期化
    cost_map[start_index]  = 0;
    route_map[start_index] = FROM_UNKNOWN;
    
    // コスト・ルートマップの作成
    for (int h = 0; h < height; h++) {
        for (int w = startX; w < width; w++) {
            int64_t index = h * width + w;
            uint8_t min_cost = 0xFF;
            Route route = FROM_UNKNOWN;
            
            // 上方向からindex番地に来ることができるか
            if (index - width >= 0) {
                int32_t total_cost = cost_map[index - width] + score_map[index] * WEIGHT_UP;
                uint8_t clamp_cost = std::min((int32_t)0xFF, total_cost);
                if (clamp_cost < min_cost) {
                    min_cost = clamp_cost;
                    route = FROM_UP;
                }
            }
            
            // 左上方向からindex番地に来ることができるか
            if (index - width >= 0 && (w-1) >= startX) {
                int32_t total_cost = cost_map[index - width - 1] + score_map[index];
                uint8_t clamp_cost = std::min((int32_t)0xFF, total_cost);
                if (clamp_cost < min_cost) {
                    min_cost = clamp_cost;
                    route = FROM_LEFT_UP;
                }
            }
            
            // 左方向からindex番地に来ることができるか
            if ((w-1) >= startX) {
                int32_t total_cost = cost_map[index - 1] + score_map[index] * WEIGHT_LEFT;
                uint8_t clamp_cost = std::min((int32_t)0xFF, total_cost);
                if (clamp_cost < min_cost) {
                    min_cost = clamp_cost;
                    route = FROM_LEFT;
                }
            }
            
            // index番地が開始位置の場合は更新しない
            if (index != start_index) {
                cost_map[index]  = min_cost;
                route_map[index] = route;
            }
        }
    }

#ifdef DEBUG
    disp_map<uint8_t>(cost_map, height, width, "COST_MAP %d", startX);
    disp_map<Route>(route_map, height, width, "ROUTE_MAP");
#endif

    return 0;
}

int dpm::getLowestCostPair(const uint8_t* cost_map, const Route* route_map,
        int height, int width, int startX, Pair* start, Pair* end, uint8_t* cost) {
    
    // マッチ開始・最終点の初期化
    start->values1_i = 0;
    start->values2_i = startX;
    end->values1_i = height - 1;
    end->values2_i = width - 1;
    
    // 終了点からルートを辿る
    int w = width - 1;
    int h = height - 1;
    while (route_map[h * width + w] != FROM_UNKNOWN) {
        switch (route_map[h * width + w]) {
            case FROM_LEFT:
                if (h == (height - 1) && (w-1) >= 0 && (w-1) < width) {
                    end->values2_i = w-1;
                }
                w--;
                break;
            case FROM_UP:
                if ((h-1) == 0 && (w-1) >= 0 && (w-1) < width) {
                    start->values2_i = w-1;
                }
                h--;
                break;
            case FROM_LEFT_UP:
                if ((h-1) == 0 && (w-1) >= 0 && (w-1) < width) {
                    start->values2_i = w-1;
                }
                w--;
                h--;
                break;
            case FROM_UNKNOWN:
                // fall through
            default:
                break;
        }
    }
    
    *cost = cost_map[end->values1_i * width + end->values2_i];
    
    return 0;
}

int dpm::match(const uint8_t* values1, int len1,
        const uint8_t* values2, int len2, ScoreFunc scoreFunc,
        Result* result) {
    
    // スコアッマップ
    uint8_t* score_map = new uint8_t[len1 * len2];
    // コストマップ
    uint8_t* cost_map  = new uint8_t[len1 * len2];
    // ルートマップ
    Route*   route_map = new Route[len1 * len2];
    
    // メモリ確保の失敗確認
    if (score_map == NULL || cost_map == NULL || route_map == NULL) {
        delete [] score_map;
        delete [] cost_map;
        delete [] route_map;
        return ENOMEM;
    }
    
    // スコアマップの作成
    for (int i = 0; i < len1; i++) {
        for (int j = 0; j < len2; j++) {
            uint8_t c1 = values1[i];
            uint8_t c2 = values2[j];
            score_map[i * len2 + j] = scoreFunc(c1, c2);
        }
    }
    
#ifdef DEBUG
    disp_map<uint8_t>(score_map, len1, len2, "SCORE_MAP");
#endif
    
    // コストが最小になるルートを探す
    uint8_t  minCost = 0xFF;
    for (int x = 0; x < len2 - len1; x++) {
        // 開始点をX軸方向にずらしながらコストマップ、ルートマップを作成する
        createCostRouteMap(score_map, len1, len2, x, cost_map, route_map);
        
        Pair start, end;
        uint8_t cost;
        
        // マッチしたデータ列の開始点・終了点とそれのコストを返却する
        getLowestCostPair(cost_map, route_map, len1, len2, x, &start, &end, &cost);
        
        // コストが最小になる開始点・終了点を保存する
        if (cost < minCost) {
            // 発見したデータ列長が実際の長さより短い場合は無視する
            if (end.values2_i - start.values2_i >= len1) {
                // 発見したデータ列を保存する
                minCost = cost;
                result->start = start;
                result->end = end;
                result->score = cost;
            }
        }
    }

    // メモリの解放
    delete [] score_map;
    delete [] cost_map;
    delete [] route_map;
    
    return 0;
}
