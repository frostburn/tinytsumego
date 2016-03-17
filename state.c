#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH (9)
#define HEIGHT (7)
#define STATE_SIZE (WIDTH * HEIGHT)
#define H_SHIFT (1ULL)
#define V_SHIFT (WIDTH)
#define D_SHIFT (WIDTH - 1ULL)
#define NORTH_WALL ((1ULL << WIDTH) - 1ULL)
#define WEST_WALL (0x40201008040201ULL)
#define WEST_BLOCK (0X3FDFEFF7FBFDFEFF)

#define H0 (NORTH_WALL)
#define H1 (H0 << V_SHIFT)
#define H2 (H1 << V_SHIFT)
#define H3 (H2 << V_SHIFT)
#define H4 (H3 << V_SHIFT)
#define H5 (H4 << V_SHIFT)
#define H6 (H5 << V_SHIFT)

#define V0 (WEST_WALL)
#define V1 (V0 << H_SHIFT)
#define V2 (V1 << H_SHIFT)
#define V3 (V2 << H_SHIFT)
#define V4 (V3 << H_SHIFT)
#define V5 (V4 << H_SHIFT)
#define V6 (V5 << H_SHIFT)
#define V7 (V6 << H_SHIFT)
#define V8 (V7 << H_SHIFT)

typedef unsigned long long int stones_t;

typedef struct state
{
    stones_t playing_area;
    stones_t player;
    stones_t opponent;
    stones_t ko;
    stones_t target;
    stones_t immortal;
    int passes;
    int ko_threats;
    int white_to_play;
} state;

typedef struct state_info
{
    int size;
    int width;
    int height;
    int num_moves;
    stones_t moves[STATE_SIZE + 1];
    int num_external;
    stones_t externals[STATE_SIZE];
    int external_sizes[STATE_SIZE];
    stones_t internal;
    int num_blocks;
    size_t exponents[STATE_SIZE];
    int shifts[STATE_SIZE];
    stones_t masks[STATE_SIZE];
    int color_symmetry;
    int mirror_v_symmetry;
    int mirror_h_symmetry;
    int mirror_d_symmetry;
    // No rotational or color + spatial symmetries implemented.
} state_info;

void print_stones(stones_t stones) {
    printf(" ");
    for (int i = 0; i < WIDTH; i++) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int i = 0; i < 64; i++) {
        if (i % V_SHIFT == 0) {
            printf("%d", i / V_SHIFT);
        }
        if ((1ULL << i) & stones) {
            printf(" @");
        }
        else {
            printf(" .");
        }
        if (i % V_SHIFT == V_SHIFT - 1){
            printf("\n");
        }
    }
    printf("\n");
}

void print_state(state *s) {
    stones_t black, white;
    if (s->white_to_play) {
        white = s->player;
        black = s->opponent;
    }
    else {
        black = s->player;
        white = s->opponent;
    }
    printf(" ");
    for (int i = 0; i < WIDTH; i++) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int i = 0; i < STATE_SIZE; i++) {
        if (i % V_SHIFT == 0) {
            printf("%d", i / V_SHIFT);
        }
        stones_t p = (1ULL << i);
        if (p & s->playing_area) {
            printf("\x1b[0;30;43m");  // Yellow BG
        }
        else {
            printf("\x1b[0m");
        }
        if (p & black) {
            printf("\x1b[30m");  // Black
            if (p & s->target) {
                printf(" b");
            }
            else if (p & s->immortal) {
                printf(" B");
            }
            else {
                printf(" @");
            }
        }
        else if (p & white) {
            printf("\x1b[37m");  // White
            if (p & s->target) {
                printf(" w");
            }
            else if (p & s->immortal) {
                printf(" W");
            }
            else {
                printf(" 0");
            }
        }
        else if (p & s->ko) {
            printf("\x1b[35m");
            printf(" *");
        }
        else if (p & s->playing_area) {
            printf("\x1b[35m");
            printf(" .");
        }
        else {
            printf("  ");
        }
        if (i % V_SHIFT == V_SHIFT - 1){
            printf("\x1b[0m\n");
        }
    }
    printf("passes = %d ko_threats = %d white_to_play = %d\n", s->passes, s->ko_threats, s->white_to_play);
}

void repr_state(state *s) {
    printf(
        "%llu %llu %llu %llu %llu %llu %d %d %d\n",
        s->playing_area,
        s->player,
        s->opponent,
        s->ko,
        s->target,
        s->immortal,
        s->passes,
        s->ko_threats,
        s->white_to_play
    );
}

void sscanf_state(const char *str, state *s) {
    sscanf(
        str,
        "%llu %llu %llu %llu %llu %llu %d %d %d",
        &(s->playing_area),
        &(s->player),
        &(s->opponent),
        &(s->ko),
        &(s->target),
        &(s->immortal),
        &(s->passes),
        &(s->ko_threats),
        &(s->white_to_play)
    );
}

int popcount(stones_t stones) {
    return __builtin_popcountll(stones);
}

size_t bitscan(stones_t stones) {
    assert(stones);
    size_t index = 0;
    while (!((1ULL << index) & stones)) {
        index++;
    }
    return index;
}

size_t bitscan_left(stones_t stones) {
    assert(stones);
    size_t index = STATE_SIZE - 1;
    while (!((1ULL << index) & stones)) {
        index--;
    }
    return index;
}

stones_t rectangle(int width, int height) {
    stones_t r = 0;
    for (int i = 0; i < width; ++i)
    {
        for (int j = 0; j < height; ++j)
        {
            r |= 1ULL << (i * H_SHIFT + j * V_SHIFT);
        }
    }
    return r;
}

stones_t one(int x, int y) {
    return 1ULL << (x * H_SHIFT + y * V_SHIFT);
}

stones_t two(int x, int y) {
    return 3ULL << (x * H_SHIFT + y * V_SHIFT);
}

stones_t tvo(int x, int y) {
    return (1ULL | (1ULL << V_SHIFT)) << (x * H_SHIFT + y * V_SHIFT);
}

void dimensions(stones_t stones, int *width, int *height) {
    for (int i = WIDTH - 1; i >= 0; i--) {
        if (stones & (WEST_WALL << i)) {
            *width = i + 1;
            break;
        }
    }
    for (int i = HEIGHT - 1; i >= 0; i--) {
        if (stones & (NORTH_WALL << (i * V_SHIFT))) {
            *height = i + 1;
            return;
        }
    }
    assert(0);
}

stones_t flood(register stones_t source, register stones_t target) {
    source &= target;
    // Conditionals are costly.
    // if (!source){
    //     return source;
    // }
    // Must & source with WEST_BLOCK because otherwise this would overflow
    // 0 1 1  source
    // 0 0 0

    // 1 1 0  temp
    // 1 1 0
    // Disabled for now until further testing.
    // register stones_t temp = WEST_BLOCK & target;
    // source |= temp & ~((source & WEST_BLOCK) + temp);
    // The following might be worth it, but it is hard to say.
    // source |= (
    //     (source << V_SHIFT) |
    //     (source >> V_SHIFT)
    // ) & target;
    register stones_t temp;
    do {
        temp = source;
        source |= (
            ((source & WEST_BLOCK) << H_SHIFT) |
            ((source >> H_SHIFT) & WEST_BLOCK) |
            (source << V_SHIFT) |
            (source >> V_SHIFT)
        ) & target;
    } while (temp != source);
    return source;
}

stones_t north(stones_t stones) {
    return stones >> V_SHIFT;
}

stones_t south(stones_t stones) {
    return stones << V_SHIFT;
}

stones_t west(stones_t stones) {
    return (stones >> H_SHIFT) & WEST_BLOCK;
}

stones_t east(stones_t stones) {
    return (stones & WEST_BLOCK) << H_SHIFT;
}

stones_t cross(stones_t stones) {
    return (
        ((stones & WEST_BLOCK) << H_SHIFT) |
        ((stones >> H_SHIFT) & WEST_BLOCK) |
        (stones << V_SHIFT) |
        (stones >> V_SHIFT) |
        stones
    );
}

stones_t blob(stones_t stones) {
    stones |= ((stones & WEST_BLOCK) << H_SHIFT) | ((stones >> H_SHIFT) & WEST_BLOCK);
    return stones | (stones << V_SHIFT) | (stones >> V_SHIFT);
}

stones_t liberties(stones_t stones, stones_t empty) {
    return (
        ((stones & WEST_BLOCK) << H_SHIFT) |
        ((stones >> H_SHIFT) & WEST_BLOCK) |
        (stones << V_SHIFT) |
        (stones >> V_SHIFT)
    ) & ~stones & empty;
}

int target_dead(state *s) {
    return !!(s->target & ~(s->player | s->opponent));
}

int chinese_liberty_score(state *s) {
    stones_t player_controlled = s->player | liberties(s->player, s->playing_area & ~s->opponent);
    stones_t opponent_controlled = s->opponent | liberties(s->opponent, s->playing_area & ~s->player);
    return popcount(player_controlled) - popcount(opponent_controlled);
}

int japanese_liberty_score(state *s) {
    return popcount(liberties(s->player, s->playing_area & ~s->opponent)) - popcount(liberties(s->opponent, s->playing_area & ~s->player));
}

int make_move(state *s, stones_t move, int *num_kill) {
    stones_t old_player = s->player;
    if (!move) {
        if (s->ko){
            s->ko = 0;
        }
        else {
            s->passes += 1;
        }
        s->player = s->opponent;
        s->opponent = old_player;
        s->ko_threats = -s->ko_threats;
        s->white_to_play = !s->white_to_play;
        *num_kill = 0;
        // assert(is_legal(s));
        return 1;
    }
    stones_t old_opponent = s->opponent;
    stones_t old_ko = s->ko;
    int old_ko_threats = s->ko_threats;
    if (move & s->ko) {
        if (s->ko_threats <= 0) {
            return 0;
        }
        s->ko_threats--;
    }
    if (move & (s->player | s->opponent | ~s->playing_area)) {
        return 0;
    }
    s->player |= move;
    s->ko = 0;
    // Conditionals are expensive.
    // if (!liberties(move, s->opponent | s->player)) {
    //     *num_kill = 0;
    //     goto swap_players;
    // }
    stones_t kill = 0;
    stones_t empty = s->playing_area & ~s->player;
    stones_t chain = flood(move >> V_SHIFT, s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }
    chain = flood(move << V_SHIFT, s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }
    chain = flood((move >> H_SHIFT) & WEST_BLOCK, s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }
    chain = flood((move & WEST_BLOCK) << H_SHIFT, s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }

    *num_kill = popcount(kill);
    if (*num_kill == 1) {
        if (liberties(move, s->playing_area & ~s->opponent) == kill) {
            s->ko = kill;
        }
    }
    chain = flood(move, s->player);
    if (!liberties(chain, s->playing_area & ~s->opponent) && !(chain & s->immortal)) {
        s->player = old_player;
        s->opponent = old_opponent;
        s->ko = old_ko;
        s->ko_threats = old_ko_threats;
        return 0;
    }
    // swap_players:
    s->passes = 0;
    old_player = s->player;
    s->player = s->opponent;
    s->opponent = old_player;
    s->ko_threats = -s->ko_threats;
    s->white_to_play = !s->white_to_play;
    // assert(is_legal(s));
    return 1;
}

int is_legal(state *s) {
    stones_t p;
    stones_t chain;
    stones_t player = s->player;
    stones_t opponent = s->opponent;
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j + 1 < HEIGHT; j += 2) {
            p = (1ULL | (1ULL << V_SHIFT)) << (i + j * V_SHIFT);
            chain = flood(p, player);
            player ^= chain;
            if (chain && !liberties(chain, s->playing_area & ~s->opponent) && !(chain & s->immortal)) {
                return 0;
            }
            chain = flood(p, opponent);
            opponent ^= chain;
            if (chain && !liberties(chain, s->playing_area & ~s->player) && !(chain & s->immortal)) {
                return 0;
            }
        }
        if (!(player | opponent)) {
            return 1;
        }
    }
    if (HEIGHT % 2) {
        for (int i = 0; i < WIDTH ; i += 2) {
            p = 3ULL << (i + (HEIGHT - 1) * V_SHIFT);  // Assumes that end bits don't matter.
            chain = flood(p, player);
            player ^= chain;
            if (chain && !liberties(chain, s->playing_area & ~s->opponent) && !(chain & s->immortal)) {
                return 0;
            }
            chain = flood(p, opponent);
            opponent ^= chain;
            if (chain && !liberties(chain, s->playing_area & ~s->player) && !(chain & s->immortal)) {
                return 0;
            }
        }
    }
    // Checking for ko legality omitted.
    return 1;
}

/*
int is_legal(state *s) {
    stones_t p;
    stones_t chain;
    stones_t player = s->player;
    stones_t opponent = s->opponent;
    for (int i = 0; i < STATE_SIZE; i++) {
        p = 1ULL << i;
        chain = flood(p, player);
        player ^= chain;
        if (chain && !liberties(chain, s->playing_area & ~s->opponent) && !(chain & s->immortal)) {
            return 0;
        }
        chain = flood(p, opponent);
        opponent ^= chain;
        if (chain && !liberties(chain, s->playing_area & ~s->player) && !(chain & s->immortal)) {
            return 0;
        }
    }
    // Checking for ko legality omitted.
    return 1;
}
*/

static stones_t PLAYER_BLOCKS[243] = {0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0, 8, 9, 8, 10, 11, 10, 8, 9, 8, 12, 13, 12, 14, 15, 14, 12, 13, 12, 8, 9, 8, 10, 11, 10, 8, 9, 8, 0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0, 16, 17, 16, 18, 19, 18, 16, 17, 16, 20, 21, 20, 22, 23, 22, 20, 21, 20, 16, 17, 16, 18, 19, 18, 16, 17, 16, 24, 25, 24, 26, 27, 26, 24, 25, 24, 28, 29, 28, 30, 31, 30, 28, 29, 28, 24, 25, 24, 26, 27, 26, 24, 25, 24, 16, 17, 16, 18, 19, 18, 16, 17, 16, 20, 21, 20, 22, 23, 22, 20, 21, 20, 16, 17, 16, 18, 19, 18, 16, 17, 16, 0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0, 8, 9, 8, 10, 11, 10, 8, 9, 8, 12, 13, 12, 14, 15, 14, 12, 13, 12, 8, 9, 8, 10, 11, 10, 8, 9, 8, 0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0};
static stones_t OPPONENT_BLOCKS[243] = {0, 0, 1, 0, 0, 1, 2, 2, 3, 0, 0, 1, 0, 0, 1, 2, 2, 3, 4, 4, 5, 4, 4, 5, 6, 6, 7, 0, 0, 1, 0, 0, 1, 2, 2, 3, 0, 0, 1, 0, 0, 1, 2, 2, 3, 4, 4, 5, 4, 4, 5, 6, 6, 7, 8, 8, 9, 8, 8, 9, 10, 10, 11, 8, 8, 9, 8, 8, 9, 10, 10, 11, 12, 12, 13, 12, 12, 13, 14, 14, 15, 0, 0, 1, 0, 0, 1, 2, 2, 3, 0, 0, 1, 0, 0, 1, 2, 2, 3, 4, 4, 5, 4, 4, 5, 6, 6, 7, 0, 0, 1, 0, 0, 1, 2, 2, 3, 0, 0, 1, 0, 0, 1, 2, 2, 3, 4, 4, 5, 4, 4, 5, 6, 6, 7, 8, 8, 9, 8, 8, 9, 10, 10, 11, 8, 8, 9, 8, 8, 9, 10, 10, 11, 12, 12, 13, 12, 12, 13, 14, 14, 15, 16, 16, 17, 16, 16, 17, 18, 18, 19, 16, 16, 17, 16, 16, 17, 18, 18, 19, 20, 20, 21, 20, 20, 21, 22, 22, 23, 16, 16, 17, 16, 16, 17, 18, 18, 19, 16, 16, 17, 16, 16, 17, 18, 18, 19, 20, 20, 21, 20, 20, 21, 22, 22, 23, 24, 24, 25, 24, 24, 25, 26, 26, 27, 24, 24, 25, 24, 24, 25, 26, 26, 27, 28, 28, 29, 28, 28, 29, 30, 30, 31};
static size_t BLOCK_KEYS[32] = {0, 1, 3, 4, 9, 10, 12, 13, 27, 28, 30, 31, 36, 37, 39, 40, 81, 82, 84, 85, 90, 91, 93, 94, 108, 109, 111, 112, 117, 118, 120, 121};

int from_key(state *s, state_info *si, size_t key) {
    stones_t fixed = s->target | s->immortal;
    s->player &= fixed;
    s->opponent &= fixed;
    s->ko = 0;

    for (int i = 0; i < si->num_external; i++) {
        stones_t external = si->externals[i];
        size_t e_size = si->external_sizes[i];
        size_t num_filled = key % e_size;
        key /= e_size;
        stones_t p = 1ULL;
        while (num_filled) {
            if (p & external) {
                s->player |= p;
                num_filled--;
            }
            p <<= 1;
        }
    }

    for (int i = 0; i < si->num_blocks; i++) {
        size_t e = si->exponents[i];
        int shift = si->shifts[i];
        s->player |= PLAYER_BLOCKS[key % e] << shift;
        s->opponent |= OPPONENT_BLOCKS[key % e] << shift;
        key /= e;
    }

    return is_legal(s);
}

size_t to_key(state *s, state_info *si) {
    size_t key = 0;
    for (int i = si->num_blocks - 1; i >= 0; i--) {
        size_t e = si->exponents[i];
        int shift = si->shifts[i];
        stones_t mask = si->masks[i];
        stones_t player = (s->player >> shift) & mask;
        stones_t opponent = (s->opponent >> shift) & mask;
        key *= e;
        key += BLOCK_KEYS[player] + 2 * BLOCK_KEYS[opponent];
    }

    for (int i = si->num_external - 1; i >= 0; i--) {
        stones_t external = si->externals[i];
        size_t e_size = si->external_sizes[i];
        size_t num_filled = popcount(external & (s->player | s->opponent));
        assert(num_filled < e_size);
        key *= e_size;
        key += num_filled;
    }

    return key;
}

size_t key_size(state_info *si) {
    size_t size = 1;
    for (int i = si->num_blocks - 1; i >= 0; i--) {
        size *= si->exponents[i];
    }

    for (int i = si->num_external - 1; i >= 0; i--) {
        stones_t external = si->externals[i];
        size *= popcount(external) + 1;
    }
    // TODO: Make sure that we're under 1 << 64.
    return size;
}

#define HB3 (H0 | H1 | H2)
#define HE7 (H0 | H4)
#define HC7 (H1 | H3 | H5)

stones_t s_mirror_v(stones_t stones) {
    assert(HEIGHT == 7);
    stones = ((stones >> (4 * V_SHIFT)) & HB3) | ((stones & HB3) << (4 * V_SHIFT)) | (stones & H3);
    return ((stones >> (2 * V_SHIFT)) & HE7) | ((stones & HE7) << (2 * V_SHIFT)) | (stones & HC7);
}

#define HE6 (H0 | H3)
#define HC6 (H1 | H4)

stones_t s_mirror_v6(stones_t stones) {
    stones = ((stones >> (3 * V_SHIFT)) & HB3) | ((stones & HB3) << (3 * V_SHIFT));
    return ((stones >> (2 * V_SHIFT)) & HE6) | ((stones & HE6) << (2 * V_SHIFT)) | (stones & HC6);
}

stones_t s_mirror_v5(stones_t stones) {
    return (
        ((stones & H0) << (4 * V_SHIFT)) |
        ((stones & H1) << (2 * V_SHIFT)) |
        (stones & H2) |
        ((stones & H3) >> (2 * V_SHIFT)) |
        ((stones & H4) >> (4 * V_SHIFT))
    );
}

stones_t s_mirror_v4(stones_t stones) {
    return (
        ((stones & H0) << (3 * V_SHIFT)) |
        ((stones & H1) << V_SHIFT) |
        ((stones & H2) >> V_SHIFT) |
        ((stones & H3) >> (3 * V_SHIFT))
    );
}

stones_t s_mirror_v3(stones_t stones) {
    return (
        ((stones & H0) << (2 * V_SHIFT)) |
        (stones & H1) |
        ((stones & H2) >> (2 * V_SHIFT))
    );
}

stones_t s_mirror_v2(stones_t stones) {
    return (
        ((stones & H0) << V_SHIFT) |
        ((stones & H1) >> V_SHIFT)
    );
}

#define VB3 (V0 | V1 | V2)
#define VBC3 (V3 | V4 | V5)
#define VE9 (V0 | V3 | V6)
#define VC9 (V1 | V4 | V7)

stones_t s_mirror_h(stones_t stones) {
    assert(WIDTH == 9);
    stones = ((stones >> 6) & VB3) | ((stones & VB3) << 6) | (stones & VBC3);
    return ((stones >> 2) & VE9) | ((stones & VE9) << 2) | (stones & VC9);
}

#define VB4 (VB3 | V3)
#define VBE2 (V0 | V1 | V4 | V5)
#define VBE1 (V0 | V2 | V4 | V6)

stones_t s_mirror_h8(stones_t stones) {
    stones = ((stones >> 4) & VB4) | ((stones & VB4) << 4);
    stones = ((stones >> 2) & VBE2) | ((stones & VBE2) << 2);
    return ((stones >> 1) & VBE1) | ((stones & VBE1) << 1);
}

#define VE7 (V0 | V4)
#define VC7 (V1 | V3 | V5)

stones_t s_mirror_h7(stones_t stones) {
    stones = ((stones >> 4) & VB3) | ((stones & VB3) << 4) | (stones & V3);
    return ((stones >> 2) & VE7) | ((stones & VE7) << 2) | (stones & VC7);
}

#define VE6 (V0 | V3)
#define VC6 (V1 | V4)

stones_t s_mirror_h6(stones_t stones) {
    stones = ((stones >> 3) & VB3) | ((stones & VB3) << 3);
    return ((stones >> 2) & VE6) | ((stones & VE6) << 2) | (stones & VC6);
}

stones_t s_mirror_h5(stones_t stones) {
    return (
        ((stones & V0) << 4) |
        ((stones & V1) << 2) |
        (stones & V2) |
        ((stones & V3) >> 2) |
        ((stones & V4) >> 4)
    );
}

stones_t s_mirror_h4(stones_t stones) {
    return (
        ((stones & V0) << 3) |
        ((stones & V1) << 1) |
        ((stones & V2) >> 1) |
        ((stones & V3) >> 3)
    );
}

stones_t s_mirror_h3(stones_t stones) {
    return (
        ((stones & V0) << 2) |
        (stones & V1) |
        ((stones & V2) >> 2)
    );
}

stones_t s_mirror_h2(stones_t stones) {
    return (
        ((stones & V0) << 1) |
        ((stones & V1) >> 1)
    );
}

stones_t s_mirror_d(stones_t stones) {
    assert(HEIGHT == 7 && HEIGHT < WIDTH);
    return (
        (stones & 0x1004010040100401ULL) |
        ((stones & 0x8020080200802ULL) << D_SHIFT) |
        ((stones >> D_SHIFT) & 0x8020080200802ULL) |
        ((stones & 0x40100401004ULL) << (2 * D_SHIFT)) |
        ((stones >> (2 * D_SHIFT)) & 0x40100401004ULL) |
        ((stones & 0x200802008ULL) << (3 * D_SHIFT)) |
        ((stones >> (3 * D_SHIFT)) & 0x200802008ULL) |
        ((stones & 0x1004010ULL) << (4 * D_SHIFT)) |
        ((stones >> (4 * D_SHIFT)) & 0x1004010ULL) |
        ((stones & 0x8020ULL) << (5 * D_SHIFT)) |
        ((stones >> (5 * D_SHIFT)) & 0x8020ULL) |
        ((stones & 0x40ULL) << (6 * D_SHIFT)) |
        ((stones >> (6 * D_SHIFT)) & 0x40ULL)
    );
}

void snap(state *s) {
    for (int i = 0; i < WIDTH; i++) {
        if (s->playing_area & (WEST_WALL << i)){
            s->playing_area >>= i;
            s->player >>= i;
            s->opponent >>= i;
            s->ko >>= i;
            s->target >>= i;
            s->immortal >>= i;
            break;
        }
    }
    for (int i = 0; i < HEIGHT * V_SHIFT; i += V_SHIFT) {
        if (s->playing_area & (NORTH_WALL << i)){
            s->playing_area >>= i;
            s->player >>= i;
            s->opponent >>= i;
            s->ko >>= i;
            s->target >>= i;
            s->immortal >>= i;
            return;
        }
    }
}

void mirror_v_full(state *s) {
    s->playing_area = s_mirror_v(s->playing_area);
    s->player = s_mirror_v(s->player);
    s->opponent = s_mirror_v(s->opponent);
    s->ko = s_mirror_v(s->ko);
    s->target = s_mirror_v(s->target);
    s->immortal = s_mirror_v(s->immortal);
    snap(s);
}

void mirror_h_full(state *s) {
    s->playing_area = s_mirror_h(s->playing_area);
    s->player = s_mirror_h(s->player);
    s->opponent = s_mirror_h(s->opponent);
    s->ko = s_mirror_h(s->ko);
    s->target = s_mirror_h(s->target);
    s->immortal = s_mirror_h(s->immortal);
    snap(s);
}

void mirror_d_full(state *s) {
    s->playing_area = s_mirror_d(s->playing_area);
    s->player = s_mirror_d(s->player);
    s->opponent = s_mirror_d(s->opponent);
    s->ko = s_mirror_d(s->ko);
    s->target = s_mirror_d(s->target);
    s->immortal = s_mirror_d(s->immortal);
    snap(s);
}

void mirror_v(state *s, state_info *si) {
    if (si->height == 7) {
        s->player = s_mirror_v(s->player);
        s->opponent = s_mirror_v(s->opponent);
        s->ko = s_mirror_v(s->ko);
    }
    else if (si->height == 6) {
        s->player = s_mirror_v6(s->player);
        s->opponent = s_mirror_v6(s->opponent);
        s->ko = s_mirror_v6(s->ko);
    }
    else if (si->height == 5) {
        s->player = s_mirror_v5(s->player);
        s->opponent = s_mirror_v5(s->opponent);
        s->ko = s_mirror_v5(s->ko);
    }
    else if (si->height == 4) {
        s->player = s_mirror_v4(s->player);
        s->opponent = s_mirror_v4(s->opponent);
        s->ko = s_mirror_v4(s->ko);
    }
    else if (si->height == 3) {
        s->player = s_mirror_v3(s->player);
        s->opponent = s_mirror_v3(s->opponent);
        s->ko = s_mirror_v3(s->ko);
    }
    else if (si->height == 2) {
        s->player = s_mirror_v2(s->player);
        s->opponent = s_mirror_v2(s->opponent);
        s->ko = s_mirror_v2(s->ko);
    }
}

void mirror_h(state *s, state_info *si) {
    if (si->width == 9) {
        s->player = s_mirror_h(s->player);
        s->opponent = s_mirror_h(s->opponent);
        s->ko = s_mirror_h(s->ko);
    }
    else if (si->width == 8) {
        s->player = s_mirror_h8(s->player);
        s->opponent = s_mirror_h8(s->opponent);
        s->ko = s_mirror_h8(s->ko);
    }
    else if (si->width == 7) {
        s->player = s_mirror_h7(s->player);
        s->opponent = s_mirror_h7(s->opponent);
        s->ko = s_mirror_h7(s->ko);
    }
    else if (si->width == 6) {
        s->player = s_mirror_h6(s->player);
        s->opponent = s_mirror_h6(s->opponent);
        s->ko = s_mirror_h6(s->ko);
    }
    else if (si->width == 5) {
        s->player = s_mirror_h5(s->player);
        s->opponent = s_mirror_h5(s->opponent);
        s->ko = s_mirror_h5(s->ko);
    }
    else if (si->width == 4) {
        s->player = s_mirror_h4(s->player);
        s->opponent = s_mirror_h4(s->opponent);
        s->ko = s_mirror_h4(s->ko);
    }
    else if (si->width == 3) {
        s->player = s_mirror_h3(s->player);
        s->opponent = s_mirror_h3(s->opponent);
        s->ko = s_mirror_h3(s->ko);
    }
    else if (si->width == 2) {
        s->player = s_mirror_h2(s->player);
        s->opponent = s_mirror_h2(s->opponent);
        s->ko = s_mirror_h2(s->ko);
    }
}

void mirror_d(state *s) {
    s->player = s_mirror_d(s->player);
    s->opponent = s_mirror_d(s->opponent);
    s->ko = s_mirror_d(s->ko);
}

int less_than(state *a, state *b, state_info *si) {
    stones_t a_player = a->player & si->internal;
    stones_t b_player = b->player & si->internal;
    if (a_player < b_player) {
        return 1;
    }
    else if (a_player == b_player){
        stones_t a_opponent = a->opponent & si->internal;
        stones_t b_opponent = b->opponent & si->internal;
        if (a_opponent < b_opponent) {
            return 1;
        }
        else if (a_opponent == b_opponent) {
            if (a->ko < b->ko) {
                return 1;
            }
            else if (a->ko == b->ko) {
                size_t key_a = 0;
                size_t key_b = 0;
                stones_t a_filling = a->player | a->opponent;
                stones_t b_filling = b->player | b->opponent;
                for (int i = si->num_external - 1; i >= 0; i--) {
                    stones_t external = si->externals[i];
                    size_t e_size = si->external_sizes[i];
                    key_a *= e_size;
                    key_b *= e_size;
                    key_a += popcount(external & a_filling);
                    key_b += popcount(external & b_filling);
                }
                return key_a < key_b;
            }
        }
    }
    return 0;
}

void normalize_external(state *s, state_info *si) {
    for (int i = 0; i < si->num_external; i++) {
        stones_t external = si->externals[i];
        int num_filled = popcount(external & (s->player | s->opponent));
        s->player &= ~external;
        s->opponent &= ~external;
        stones_t p = 1ULL;
        while (num_filled) {
            if (p & external) {
                s->player |= p;
                num_filled--;
            }
            p <<= 1;
        }
    }
}

void canonize(state *s, state_info *si) {
    if (si->color_symmetry) {
        s->white_to_play = 0;
    }

    state temp_ = *s;
    state *temp = &temp_;

    if (si->mirror_v_symmetry) {
        mirror_v(temp, si);
        if (less_than(temp, s, si)) {
            *s = *temp;
        }
    }
    if (si->mirror_h_symmetry) {
        mirror_h(temp, si);
        if (less_than(temp, s, si)) {
            *s = *temp;
        }
        if (si->mirror_v_symmetry) {
            mirror_v(temp, si);
            if (less_than(temp, s, si)) {
                *s = *temp;
            }
        }
    }

    if (si->mirror_d_symmetry) {
        mirror_d(temp);
        if (less_than(temp, s, si)) {
            *s = *temp;
        }
        if (si->mirror_v_symmetry) {
            mirror_v(temp, si);
            if (less_than(temp, s, si)) {
                *s = *temp;
            }
        }
        if (si->mirror_h_symmetry) {
            mirror_h(temp, si);
            if (less_than(temp, s, si)) {
                *s = *temp;
            }
            if (si->mirror_v_symmetry) {
                mirror_v(temp, si);
                if (less_than(temp, s, si)) {
                    *s = *temp;
                }
            }
        }
    }
}

int is_canonical(state *s, state_info *si) {
    if (si->color_symmetry) {
        if (s->white_to_play) {
            return 0;
        }
    }
    state temp_ = *s;
    state *temp = &temp_;

    if (si->mirror_v_symmetry) {
        mirror_v(temp, si);
        if (less_than(temp, s, si)) {
            return 0;
        }
    }
    if (si->mirror_h_symmetry) {
        mirror_h(temp, si);
        if (less_than(temp, s, si)) {
            return 0;
        }
        if (si->mirror_v_symmetry) {
            mirror_v(temp, si);
            if (less_than(temp, s, si)) {
                return 0;
            }
        }
    }

    if (si->mirror_d_symmetry) {
        mirror_d(temp);
        if (less_than(temp, s, si)) {
            return 0;;
        }
        if (si->mirror_v_symmetry) {
            mirror_v(temp, si);
            if (less_than(temp, s, si)) {
                return 0;;
            }
        }
        if (si->mirror_h_symmetry) {
            mirror_h(temp, si);
            if (less_than(temp, s, si)) {
                return 0;;
            }
            if (si->mirror_v_symmetry) {
                mirror_v(temp, si);
                if (less_than(temp, s, si)) {
                    return 0;;
                }
            }
        }
    }
    return 1;
}

// Externals are groups of intersections where it doesn't matter
// which player has filled them or in which order.
// Player target liberties shared by opponent immortal liberties can only be filled once.
// The ones that don't matter are those that can never take a liberty of any other chain.
void process_external(state_info *si, stones_t player_target, stones_t opponent_immortal, stones_t playing_area) {
    for (int i = 0;i < STATE_SIZE; i++) {
        stones_t p = 1ULL << i;
        stones_t chain = flood(p, player_target);
        if (chain) {
            stones_t external = liberties(chain, playing_area) & liberties(opponent_immortal, playing_area);
            stones_t everything_else = playing_area & ~(chain | opponent_immortal | external);
            external &= ~cross(everything_else);
            if (external) {
                si->externals[si->num_external++] = external;
            }
            player_target ^= chain;
        }
    }
}

void init_state(state *s, state_info *si) {
    assert(!(~s->playing_area & (s->player | s->opponent | s->ko | s->target | s->immortal)));
    assert(!(s->player & s->opponent));
    assert(!((s->player | s->opponent) & s->ko));

    dimensions(s->playing_area, &(si->width), &(si->height));

    stones_t open = s->playing_area & ~(s->target | s->immortal);
    si->num_moves = 1;
    si->moves[0] = 0;
    si->size = 0;
    for (int i = 0;i < STATE_SIZE; i++) {
        stones_t p = 1ULL << i;
        if (p & open) {
            si->size++;
            si->moves[si->num_moves++] = p;
        }
    }

    si->num_external = 0;
    process_external(si, s->player & s->target, s->opponent & s->immortal, s->playing_area);
    process_external(si, s->opponent & s->target, s->player & s->immortal, s->playing_area);

    for (int i = 0;i < si->num_external; i++) {
        si->external_sizes[i] = popcount(si->externals[i]) + 1;
        open ^= si->externals[i];
    }
    si->internal = open;

    si->num_blocks = 0;
    for (int i = 0;i < STATE_SIZE; i++) {
        if (!((31ULL << i) & ~open)) {
            si->exponents[si->num_blocks] = 243;
            si->masks[si->num_blocks] = 31ULL;
            si->shifts[si->num_blocks++] = i;
            i += 4;
        }
        else if (!((15ULL << i) & ~open)) {
            si->exponents[si->num_blocks] = 81;
            si->masks[si->num_blocks] = 15ULL;
            si->shifts[si->num_blocks++] = i;
            i += 3;
        }
        else if (!((7ULL << i) & ~open)) {
            si->exponents[si->num_blocks] = 27;
            si->masks[si->num_blocks] = 7ULL;
            si->shifts[si->num_blocks++] = i;
            i += 2;
        }
        else if (!((3ULL << i) & ~open)) {
            si->exponents[si->num_blocks] = 9;
            si->masks[si->num_blocks] = 3ULL;
            si->shifts[si->num_blocks++] = i++;
        }
        else if ((1ULL << i) & open) {
            si->exponents[si->num_blocks] = 3;
            si->masks[si->num_blocks] = 1ULL;
            si->shifts[si->num_blocks++] = i;
        }
    }
    si->color_symmetry = !(s->target | s->immortal);
    state temp_;
    state *temp = &temp_;
    *temp = *s;
    mirror_v_full(temp);
    si->mirror_v_symmetry = (s->playing_area == temp->playing_area && s->target == temp->target && s->immortal == temp->immortal);
    *temp = *s;
    mirror_h_full(temp);
    si->mirror_h_symmetry = (s->playing_area == temp->playing_area && s->target == temp->target && s->immortal == temp->immortal);
    *temp = *s;
    mirror_d_full(temp);
    si->mirror_d_symmetry = (s->playing_area == temp->playing_area && s->target == temp->target && s->immortal == temp->immortal);
}

#include "atari_state.c"

#ifndef MAIN
int main() {
    stones_t a = 0x19bf3315;
    stones_t b = 1ULL;
    stones_t c = flood(b, a);
    print_stones(a);
    printf("%d\n", popcount(a));
    print_stones(b);
    printf("%d\n", popcount(b));
    print_stones(c);

    state s_ = (state) {rectangle(5, 4), 0, 0, 0, 0, 0, 0};
    state *s = &s_;
    state_info si_;
    state_info *si = &si_;
    init_state(s, si);
    int prisoners;
    make_move(s, 1ULL << (2 + 3 * V_SHIFT), &prisoners);
    make_move(s, 1ULL << (4 + 1 * V_SHIFT), &prisoners);
    print_state(s);

    size_t key = to_key(s, si);
    printf("%zu\n", key);
    printf("%d\n", from_key(s, si, key));
    print_state(s);

    print_stones(NORTH_WALL);
    print_stones(WEST_WALL);
    print_stones(WEST_BLOCK);

    int width;
    int height;

    dimensions(rectangle(4, 3), &width, &height);
    printf("%d, %d\n", width, height);

    *s = (state) {rectangle(6, 7), WEST_WALL, 2ULL, 0, 2ULL, WEST_WALL, 0};
    init_state(s, si);
    state z_;
    state *z = &z_;
    *z = *s;
    assert(from_key(z, si, to_key(s, si)));
    assert(z->player == s->player && z->opponent == s->opponent);
    time_t t = time(0);
    srand(t);
    for (int i = 0; i < 3000; i++) {
       if (make_move(s, one(rand() % WIDTH, rand() % HEIGHT), &prisoners)) {
            print_state(s);
            from_key(s, si, to_key(s, si));
            if (target_dead(s)) {
                break;
            }
        }
    }
    printf("%ld\n", t);

    print_stones(0x1C0E070381C0E07ULL);
    print_stones(0xE070381C0E07038ULL);
    print_stones(0x1249249249249249ULL);
    print_stones(0x2492492492492492ULL);

    print_stones(0x7FFFFFFULL);
    print_stones(0xFF8000000ULL);
    print_stones(0x1FF0000001FFULL);
    print_stones(0x3FE00FF803FE00ULL);

    print_stones(a);
    print_stones(s_mirror_h(a));
    print_stones(s_mirror_v(a));

    print_stones(0x1004010040100401ULL);
    print_stones(0x8020080200802ULL);

    print_stones(s_mirror_d(a));

    s->target = 0;
    s->immortal = 0;
    print_state(s);
    init_state(s, si);
    // assert(si->symmetry == 2);
    canonize(s, si);
    print_state(s);

    test_atari_state();

    return 0;
}
#endif
