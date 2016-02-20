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

    int num_moves;
    stones_t moves[STATE_SIZE + 1];
} state;


size_t bitscan_left(stones_t stone);

int is_legal(state *s);

void init_state(state *s) {
    assert(!(~s->playing_area & (s->player | s->opponent | s->ko | s->target | s->immortal)));
    assert(!(s->player & s->opponent));
    assert(!((s->player | s->opponent) & s->ko));

    stones_t open = s->playing_area & ~(s->target | s->immortal);
    s->num_moves = 1;
    s->moves[0] = 0;
    for (int i = 0;i < STATE_SIZE; i++) {
        stones_t p = 1ULL << i;
        if (p & open) {
            s->moves[s->num_moves++] = p;
        }
    }
}

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
        if (p & s->player) {
            printf("\x1b[30m");  // Black
            if (p & s->target) {
                printf(" p");
            }
            else if (p & s->immortal) {
                printf(" P");
            }
            else {
                printf(" @");
            }
        }
        else if (p & s->opponent) {
            printf("\x1b[37m");  // White
            if (p & s->target) {
                printf(" o");
            }
            else if (p & s->immortal) {
                printf(" O");
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
    printf("passes = %d ko_threats = %d\n", s->passes, s->ko_threats);
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
    if (!source){
        return source;
    }
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

int make_move(state *s, stones_t move) {
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
    if (popcount(kill) == 1) {
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

int from_key(state *s, size_t key) {
    stones_t fixed = s->target | s->immortal;
    s->player &= fixed;
    s->opponent &= fixed;
    s->ko = 0;

    for (int i = 1; i < s->num_moves; i++) {
        stones_t p = s->moves[i];
        if (key % 3 == 1) {
            s->player |= p;
        }
        else if (key % 3 == 2) {
            s->opponent |= p;
        }
        key /= 3;
    }
    return is_legal(s);
}

size_t to_key(state *s) {
    size_t key = 0;
    for (int i = s->num_moves - 1; i > 0; i--) {
        stones_t p = s->moves[i];
        key *= 3;
        if (p & s->player) {
            key += 1;
        }
        else if (p & s->opponent) {
            key += 2;
        }
    }
    return key;
}

size_t max_key(state *s) {
    size_t exponent = 0;
    size_t key = 0;
    stones_t p = 1ULL << (STATE_SIZE - 1);
    stones_t open = s->playing_area & ~(s->target | s->immortal);
    while (p) {
        if (p & open){
            key *= 3;
            key += 2;
            exponent++;
        }
        p >>= 1;
    }
    assert(exponent <= 40);
    return key;
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
    make_move(s, 1ULL << (2 + 3 * V_SHIFT));
    make_move(s, 1ULL << (4 + 1 * V_SHIFT));
    print_state(s);

    size_t key = to_key(s);
    printf("%zu\n", key);
    printf("%d\n", from_key(s, key));
    print_state(s);

    print_stones(NORTH_WALL);
    print_stones(WEST_WALL);
    print_stones(WEST_BLOCK);

    int width;
    int height;

    dimensions(rectangle(4, 3), &width, &height);
    printf("%d, %d\n", width, height);

    *s = (state) {rectangle(6, 8), WEST_WALL, 2ULL, 0, 2ULL, WEST_WALL, 0};
    max_key(s);
    state z_;
    state *z = &z_;
    *z = *s;
    assert(from_key(z, to_key(s)));
    assert(z->player == s->player && z->opponent == s->opponent);
    time_t t = time(0);
    srand(t);
    for (int i = 0; i < 3000; i++) {
       if (make_move(s, 1ULL << (rand() % WIDTH + (rand() % HEIGHT) * V_SHIFT))) {
            print_state(s);
            from_key(s, to_key(s));
            if (target_dead(s)) {
                break;
            }
        }
    }
    printf("%ld\n", t);

    return 0;
}
#endif
