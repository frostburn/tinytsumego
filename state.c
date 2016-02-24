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

#define THREE_SLICE (0x7ULL)
#define FOUR_SLICE (0xFULL)
#define FIVE_SLICE (0x1FULL)

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
    int num_moves;
    stones_t moves[STATE_SIZE + 1];
    int num_external;
    stones_t externals[STATE_SIZE];
    int num_blocks;
    size_t exponents[STATE_SIZE];
    int shifts[STATE_SIZE];
    stones_t masks[STATE_SIZE];
    int symmetry; // 1 color, 2 mirror v/h, 3 mirror v/h/d  // TODO only v, only d, rotational?
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
            break;
        }
    }
}

stones_t flood(register stones_t source, register stones_t target) {
    source &= target;
    // Conditionals are costly.
    // if (!source){
    //     return source;
    // }
    register stones_t temp = WEST_BLOCK & target;
    source |= temp & ~(source + temp);
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
        (stones >> V_SHIFT)
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

int liberty_score(state *s) {
    stones_t player_controlled = s->player | liberties(s->player, s->playing_area & ~s->opponent);
    stones_t opponent_controlled = s->opponent | liberties(s->opponent, s->playing_area & ~s->player);
    return popcount(player_controlled) - popcount(opponent_controlled);
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
    stones_t kill = 0;
    stones_t empty = s->playing_area & ~s->player;
    stones_t chain = flood(north(move), s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }
    chain = flood(south(move), s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }
    chain = flood(west(move), s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }
    chain = flood(east(move), s->opponent);
    if (!liberties(chain, empty) && !(chain & s->immortal)) {
        kill |= chain;
        s->opponent ^= chain;
    }

    s->ko = 0;
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

// Is the state a legal position for atari go starting from an empty position.
int is_atari_legal(state *s) {
    stones_t p;
    stones_t chain;
    stones_t player = s->player;
    stones_t opponent = s->opponent;
    int player_popcount = popcount(player);
    int opponent_popcount = popcount(opponent);
    // Check for alternating play
    if (player_popcount > opponent_popcount) {
        return 0;
    }
    if (opponent_popcount > player_popcount + 1) {
        return 0;
    }
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j + 1 < HEIGHT; j += 2) {
            p = (1ULL | (1ULL << V_SHIFT)) << (i + j * V_SHIFT);
            chain = flood(p, player);
            player ^= chain;
            if (chain && !liberties(chain, s->playing_area & ~s->opponent)) {
                return 0;
            }
            chain = flood(p, opponent);
            opponent ^= chain;
            if (!chain) {
                continue;
            }
            stones_t libs = liberties(chain, s->playing_area & ~s->player);
            if (!(libs & (libs - 1))) {
                // Illegal or there is a stone that can be captured.
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
            if (chain && !liberties(chain, s->playing_area & ~s->opponent)) {
                return 0;
            }
            chain = flood(p, opponent);
            opponent ^= chain;
            if (!chain) {
                continue;
            }
            stones_t libs = liberties(chain, s->playing_area & ~s->player);
            if (!(libs & (libs - 1))) {
                // Illegal or there is a stone that can be captured.
                return 0;
            }
        }
    }
    return 1;
}

// Preleaves can be treated as leaves because the final move is obvious.
int is_atari_preleaf(state *s) {
    stones_t p;
    stones_t chain;
    stones_t opponent = s->opponent;
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j + 1 < HEIGHT; j += 2) {
            p = (1ULL | (1ULL << V_SHIFT)) << (i + j * V_SHIFT);
            chain = flood(p, opponent);
            if (!chain) {
                continue;
            }
            opponent ^= chain;
            stones_t libs = liberties(chain, s->playing_area & ~s->player);
            if (libs && !(libs & (libs - 1))) {
                // There is a stone that can be captured.
                return 1;
            }
        }
        if (!opponent) {
            return 0;
        }
    }
    if (HEIGHT % 2) {
        for (int i = 0; i < WIDTH ; i += 2) {
            p = 3ULL << (i + (HEIGHT - 1) * V_SHIFT);  // Assumes that end bits don't matter.
            chain = flood(p, opponent);
            if (!chain) {
                continue;
            }
            opponent ^= chain;
            stones_t libs = liberties(chain, s->playing_area & ~s->player);
            if (libs && !(libs & (libs - 1))) {
                // There is a stone that can be captured.
                return 1;
            }
        }
    }
    return 0;
}

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
        size_t e_size = popcount(external) + 1;
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
        size_t e_size = popcount(external) + 1;
        size_t num_filled = popcount(external & (s->player | s->opponent));
        assert(num_filled < e_size);
        key *= e_size;
        key += num_filled;
    }

    return key;
}

int from_atari_key(state *s, state_info *si, size_t key) {
    s->player = 0;
    s->opponent = 0;
    s->ko = 0;

    for (int i = 0; i < si->num_blocks; i++) {
        size_t e = si->exponents[i];
        int shift = si->shifts[i];
        s->player |= PLAYER_BLOCKS[key % e] << shift;
        s->opponent |= OPPONENT_BLOCKS[key % e] << shift;
        key /= e;
    }

    // for (int i = 1; i < si->num_moves; i++) {
    //     stones_t p = si->moves[i];
    //     if (key % 3 == 1) {
    //         s->player |= p;
    //     }
    //     else if (key % 3 == 2) {
    //         s->opponent |= p;
    //     }
    //     key /= 3;
    // }
    return is_atari_legal(s);
}

size_t max_key(state *s, state_info *si) {
    size_t key = 0;
    for (int i = si->num_blocks - 1; i >= 0; i--) {
        size_t e = si->exponents[i];
        key *= e;
        key += e - 1;
    }

    for (int i = si->num_external - 1; i >= 0; i--) {
        stones_t external = si->externals[i];
        size_t e_size = popcount(external) + 1;
        key *= e_size;
        key += e_size - 1;
    }
    // TODO: Make sure that we're under 1 << 64.
    return key;
}

stones_t s_mirror_v(stones_t stones) {
    assert(HEIGHT == 7);
    stones = ((stones >> (4 * V_SHIFT)) & 0x7FFFFFFULL) | ((stones & 0x7FFFFFFULL) << (4 * V_SHIFT)) | (stones & 0xFF8000000ULL);
    return ((stones >> (2 * V_SHIFT)) & 0x1FF0000001FFULL) | ((stones & 0x1FF0000001FFULL) << (2 * V_SHIFT)) | (stones & 0x3FE00FF803FE00ULL);
}

stones_t s_mirror_h(stones_t stones) {
    assert(WIDTH == 9);
    stones = ((stones >> 6) & 0x1C0E070381C0E07ULL) | ((stones & 0x1C0E070381C0E07ULL) << 6) | (stones & 0xE070381C0E07038ULL);
    return ((stones >> 2) & 0x1249249249249249ULL) | ((stones & 0x1249249249249249ULL) << 2) | (stones & 0x2492492492492492ULL);
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
            break;
        }
    }
    for (int i = 0; i < HEIGHT * V_SHIFT; i += V_SHIFT) {
        if (s->playing_area & (NORTH_WALL << i)){
            s->playing_area >>= i;
            s->player >>= i;
            s->opponent >>= i;
            s->ko >>= i;
            return;
        }
    }
}

void mirror_h(state *s) {
    s->playing_area = s_mirror_h(s->playing_area);
    s->player = s_mirror_h(s->player);
    s->opponent = s_mirror_h(s->opponent);
    s->ko = s_mirror_h(s->ko);
    snap(s);
}

void mirror_v(state *s) {
    s->playing_area = s_mirror_v(s->playing_area);
    s->player = s_mirror_v(s->player);
    s->opponent = s_mirror_v(s->opponent);
    s->ko = s_mirror_v(s->ko);
    snap(s);
}

void mirror_d(state *s) {
    s->player = s_mirror_d(s->player);
    s->opponent = s_mirror_d(s->opponent);
    s->ko = s_mirror_d(s->ko);
}

int less_than(state *a, state *b) {
    if (a->player < b->player) {
        return 1;
    }
    else if (a->player == b->player){
        if (a->opponent < b->opponent) {
            return 1;
        }
        else if (a->opponent == b->opponent) {
            return a->ko < b->ko;
        }
    }
    return 0;
}

void canonize(state *s, state_info *si) {
    if (!si->symmetry) {
        return;
    }
    s->white_to_play = 0;
    if (si->symmetry == 1) {
        return;
    }
    state temp_ = *s;
    state *temp = &temp_;

    mirror_v(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }
    mirror_h(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }
    mirror_v(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }

    if (si->symmetry == 2) {
        return;
    }

    mirror_d(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }
    mirror_v(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }
    mirror_h(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }
    mirror_v(temp);
    if (less_than(temp, s)) {
        *s = *temp;
    }
}

int is_canonical(state *s, state_info *si) {
    if (!si->symmetry) {
        return 1;
    }
    if (s->white_to_play) {
        return 0;
    }
    state temp_ = *s;
    state *temp = &temp_;

    mirror_v(temp);
    if (less_than(temp, s)) {
        return 0;
    }
    mirror_h(temp);
    if (less_than(temp, s)) {
        return 0;
    }
    mirror_v(temp);
    if (less_than(temp, s)) {
        return 0;
    }

    if (si->symmetry == 2) {
        return 1;
    }

    mirror_d(temp);
    if (less_than(temp, s)) {
        return 0;
    }
    mirror_v(temp);
    if (less_than(temp, s)) {
        return 0;
    }
    mirror_h(temp);
    if (less_than(temp, s)) {
        return 0;
    }
    mirror_v(temp);
    if (less_than(temp, s)) {
        return 0;
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
        open ^= si->externals[i];
    }

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
    if (s->target | s->immortal) {
        si->symmetry = 0;
    }
    else {
        si->symmetry = 1;
        state temp_ = *s;
        state *temp = &temp_;
        mirror_v(temp);
        if (s->playing_area == temp->playing_area) {
            mirror_h(temp);
            if (s->playing_area == temp->playing_area) {
                si->symmetry = 2;
                if (s->playing_area == s_mirror_d(s->playing_area)) {
                    si->symmetry = 3;
                }
            }
        }
    }
}

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
    max_key(s, si);
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
    assert(si->symmetry == 2);
    canonize(s, si);
    print_state(s);

    return 0;
}
#endif
