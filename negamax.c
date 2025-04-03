#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/string.h>

#include "game.h"
#include "negamax.h"
#include "util.h"
#include "zobrist.h"

#define MAX_SEARCH_DEPTH 6

static int history_score_sum[N_GRIDS];
static int history_count[N_GRIDS];

static u64 hash_value;

static int cmp_moves(const void *a, const void *b)
{
    const int *_a = (int *) a, *_b = (int *) b;
    int score_a = 0, score_b = 0;

    if (history_count[*_a])
        score_a = history_score_sum[*_a] / history_count[*_a];
    if (history_count[*_b])
        score_b = history_score_sum[*_b] / history_count[*_b];
    return score_b - score_a;
}

static move_t negamax(uint32_t table,
                      int depth,
                      char player,
                      int alpha,
                      int beta)
{
    if (check_win(table) != 0 || depth == 0) {
        move_t result = {get_score(table, player), -1};
        return result;
    }
    const zobrist_entry_t *entry = zobrist_get(hash_value);
    if (entry)
        return (move_t){.score = entry->score, .move = entry->move};

    int score;
    move_t best_move = {-10000, -1};
    int *moves = available_moves(table);
    int n_moves = 0;
    while (n_moves < N_GRIDS && moves[n_moves] != -1)
        ++n_moves;

    sort(moves, n_moves, sizeof(int), cmp_moves, NULL);

    for (int i = 0; i < n_moves; i++) {
        table = SET_VAL_INDEX(table, moves[i], player);
        hash_value ^= zobrist_table[moves[i]][player == PLAYER2];
        if (!i)
            score =
                -negamax(table, depth - 1,
                         player == PLAYER2 ? PLAYER1 : PLAYER2, -beta, -alpha)
                     .score;
        else {
            score = -negamax(table, depth - 1,
                             player == PLAYER2 ? PLAYER1 : PLAYER2, -alpha - 1,
                             -alpha)
                         .score;
            if (alpha < score && score < beta)
                score = -negamax(table, depth - 1,
                                 player == PLAYER2 ? PLAYER1 : PLAYER2, -beta,
                                 -score)
                             .score;
        }
        history_count[moves[i]]++;
        history_score_sum[moves[i]] += score;
        if (score > best_move.score) {
            best_move.score = score;
            best_move.move = moves[i];
        }
        table = SET_VAL_INDEX(table, moves[i], BLANK);
        hash_value ^= zobrist_table[moves[i]][player == PLAYER2];
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break;
    }

    kfree((char *) moves);
    zobrist_put(hash_value, best_move.score, best_move.move);
    return best_move;
}

void negamax_init(void)
{
    zobrist_init();
    hash_value = 0;
}

move_t negamax_predict(uint32_t table, char player)
{
    memset(history_score_sum, 0, sizeof(history_score_sum));
    memset(history_count, 0, sizeof(history_count));
    move_t result;
    for (int depth = 2; depth <= MAX_SEARCH_DEPTH; depth += 2) {
        result = negamax(table, depth, player, -100000, 100000);
        zobrist_clear();
    }
    return result;
}
