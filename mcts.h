#pragma once

#include "xoroshiro.h"

#define ITERATIONS 100000

struct mcts_info {
    struct state_array xoro_obj;
    int nr_active_nodes;
};

int mcts(const uint32_t table, char player);
void mcts_init(void);
