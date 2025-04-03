#include <linux/slab.h>

#include "game.h"


const line_t lines[4] = {
    {1, 0, 0, 0, BOARD_SIZE - GOAL + 1, BOARD_SIZE},             // ROW
    {0, 1, 0, 0, BOARD_SIZE, BOARD_SIZE - GOAL + 1},             // COL
    {1, 1, 0, 0, BOARD_SIZE - GOAL + 1, BOARD_SIZE - GOAL + 1},  // PRIMARY
    {1, -1, 0, GOAL - 1, BOARD_SIZE - GOAL + 1, BOARD_SIZE},     // SECONDARY
};

static char check_line_segment_win(const uint32_t t, int i, int j, line_t line)
{
    char last = GET_VAL(t, i, j);
    if (last == BLANK)
        return 0;
    for (int k = 1; k < GOAL; k++) {
        if (last != GET_VAL(t, i + k * line.i_shift, j + k * line.j_shift)) {
            return 0;
        }
    }

    return last;
}

char check_win(const uint32_t t)
{
    for (int i_line = 0; i_line < 4; ++i_line) {
        line_t line = lines[i_line];
        for (int i = line.i_lower_bound; i < line.i_upper_bound; ++i) {
            for (int j = line.j_lower_bound; j < line.j_upper_bound; ++j) {
                char win = check_line_segment_win(t, i, j, line);
                if (win != LOSE)
                    return win;
            }
        }
    }
    // Handle the case of draw
    // If all grids are filled and no one wins, return 'D'
    // If there is an empty grid, return 0
    for (int i = 0; i < N_GRIDS; i++)
        if (GET_VAL(t, i) == BLANK)
            return 0;
    return 'D';
}

fixed_point_t calculate_win_value(char win, char player)
{
    if (win == player)
        return 1U << FIXED_SCALE_BITS;
    if (win == (player ^ PLAYER1 ^ PLAYER2))
        return 0U;
    return 1U << (FIXED_SCALE_BITS - 1);
}

int *available_moves(const uint32_t table)
{
    int *moves = kzalloc(N_GRIDS * sizeof(int), GFP_KERNEL);
    int m = 0;
    for (int i = 0; i < N_GRIDS; i++)
        if (GET_VAL(table, i) == BLANK)
            moves[m++] = i;
    if (m < N_GRIDS)
        moves[m] = -1;
    return moves;
}
