// Beware, this version overflows easily.
size_t n_choose_k_alt(size_t n, size_t k) {
    size_t num = 1;
    size_t denom = 1;
    for (size_t i = 0; i < k; i++) {
        num *= n - i;
        denom *= k - i;
    }
    return num / denom;
}

size_t N_CHOOSE_K_TABLE[26 * 14] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 66, 78, 91, 105, 120, 136, 153, 171, 190, 210, 231, 253, 276, 300, 0, 0, 0, 1, 4, 10, 20, 35, 56, 84, 120, 165, 220, 286, 364, 455, 560, 680, 816, 969, 1140, 1330, 1540, 1771, 2024, 2300, 0, 0, 0, 0, 1, 5, 15, 35, 70, 126, 210, 330, 495, 715, 1001, 1365, 1820, 2380, 3060, 3876, 4845, 5985, 7315, 8855, 10626, 12650, 0, 0, 0, 0, 0, 1, 6, 21, 56, 126, 252, 462, 792, 1287, 2002, 3003, 4368, 6188, 8568, 11628, 15504, 20349, 26334, 33649, 42504, 53130, 0, 0, 0, 0, 0, 0, 1, 7, 28, 84, 210, 462, 924, 1716, 3003, 5005, 8008, 12376, 18564, 27132, 38760, 54264, 74613, 100947, 134596, 177100, 0, 0, 0, 0, 0, 0, 0, 1, 8, 36, 120, 330, 792, 1716, 3432, 6435, 11440, 19448, 31824, 50388, 77520, 116280, 170544, 245157, 346104, 480700, 0, 0, 0, 0, 0, 0, 0, 0, 1, 9, 45, 165, 495, 1287, 3003, 6435, 12870, 24310, 43758, 75582, 125970, 203490, 319770, 490314, 735471, 1081575, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 10, 55, 220, 715, 2002, 5005, 11440, 24310, 48620, 92378, 167960, 293930, 497420, 817190, 1307504, 2042975, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 11, 66, 286, 1001, 3003, 8008, 19448, 43758, 92378, 184756, 352716, 646646, 1144066, 1961256, 3268760, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 12, 78, 364, 1365, 4368, 12376, 31824, 75582, 167960, 352716, 705432, 1352078, 2496144, 4457400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 13, 91, 455, 1820, 6188, 18564, 50388, 125970, 293930, 646646, 1352078, 2704156, 5200300, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 14, 105, 560, 2380, 8568, 27132, 77520, 203490, 497420, 1144066, 2496144, 5200300};
size_t n_choose_k(size_t n, size_t k) {
    return N_CHOOSE_K_TABLE[n + 26 * k];
}

stones_t combination_from_key_alt(size_t key, size_t num_stones, size_t size, stones_t *moves) {
    if (!num_stones) {
        return 0;
    }
    for (size_t index = 0; index < size; index++) {
        size_t offset = n_choose_k(size - 1 - index, num_stones - 1);
        if (key < offset) {
            return moves[index] | combination_from_key_alt(key, num_stones - 1, size - 1 - index, moves + index + 1);
        }
        key -= offset;
    }
    assert(0);
}

stones_t combination_from_key(size_t key, size_t num_stones, size_t size, stones_t *moves) {
    if (!num_stones) {
        return 0;
    }
    num_stones--;
    for (size_t rindex = size - 1; rindex >= 0; rindex--) {
        size_t offset = N_CHOOSE_K_TABLE[rindex + 26 * num_stones];
        if (key < offset) {
            return moves[size - 1 - rindex] | combination_from_key(key, num_stones, rindex, moves + size - rindex);
        }
        key -= offset;
    }
    assert(0);
}

size_t combination_to_key(stones_t stones, size_t num_stones, size_t size, stones_t *moves) {
    if (!num_stones) {
        return 0;
    }
    size_t index;
    for (index = 0; index < size; index++) {
        if (moves[index] & stones) {
            break;
        }
    }
    size_t offset = 0;
    for (size_t i = 0; i < index; i++) {
        offset += n_choose_k(size - 1 - i, num_stones - 1);
    }
    return offset + combination_to_key(stones, num_stones - 1, size - 1 - index, moves + index + 1);
}

// Is the state a legal position for atari go starting from an empty position.
int is_atari_legal(state *s) {
    stones_t p;
    stones_t chain;
    stones_t libs;
    stones_t player = s->player;
    stones_t opponent = s->opponent;
    stones_t p_empty = s->playing_area & ~s->opponent;
    stones_t o_empty = s->playing_area & ~s->player;
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
            #define CHECK_CHAINS \
            chain = flood(p, player);\
            libs = liberties(chain, p_empty);\
            if (chain && !libs) {\
                /* Illegal.*/\
                return 0;\
            }\
            player ^= chain;\
            chain = flood(p, opponent);\
            if (!chain) {\
                continue;\
            }\
            libs = liberties(chain, o_empty);\
            if (!(libs & (libs - 1))) {\
                /* Illegal or there is a stone that can be captured.*/\
                return 0;\
            }\
            opponent ^= chain;
            CHECK_CHAINS
        }
        if (!(player | opponent)) {
            return 1;
        }
    }
    if (HEIGHT % 2) {
        for (int i = 0; i < WIDTH ; i += 2) {
            p = 3ULL << (i + (HEIGHT - 1) * V_SHIFT);  // Assumes that end bits don't matter.
            CHECK_CHAINS
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
            #define CHECK_CHAIN \
            chain = flood(p, opponent);\
            if (!chain) {\
                continue;\
            }\
            stones_t libs = liberties(chain, s->playing_area & ~s->player);\
            if (libs && !(libs & (libs - 1))) {\
                /* There is a stone that can be captured.*/\
                return 1;\
            }\
            opponent ^= chain;
            CHECK_CHAIN
        }
        if (!opponent) {
            return 0;
        }
    }
    if (HEIGHT % 2) {
        for (int i = 0; i < WIDTH ; i += 2) {
            p = 3ULL << (i + (HEIGHT - 1) * V_SHIFT);  // Assumes that end bits don't matter.
            CHECK_CHAIN
        }
    }
    return 0;
}

size_t atari_size(size_t num_stones, size_t size) {
    size_t num_o = (num_stones + 1) / 2;
    // return n_choose_k(size, num_o) * n_choose_k(size - num_o, num_stones / 2);
    return N_CHOOSE_K_TABLE[size + 26 * num_o] * N_CHOOSE_K_TABLE[size - num_o + 26 * (num_stones / 2)];
}

int from_atari_key(state *s, state_info *si, size_t key) {
    s->white_to_play = 0;
    for (size_t num_stones = 0; num_stones < si->size; num_stones++) {
        // size_t offset = atari_size(num_stones, si->size);
        size_t num_o = (num_stones + 1) / 2;
        size_t num_p = num_stones / 2;
        size_t m = N_CHOOSE_K_TABLE[si->size - num_o + 26 * num_p];
        size_t offset = N_CHOOSE_K_TABLE[si->size + 26 * num_o] * m;
        if (key < offset) {
            // size_t m = n_choose_k(si->size - num_o, num_p);
            size_t p_key = key % m;
            size_t o_key = key / m;
            s->opponent = combination_from_key(o_key, num_o, si->size, si->moves + 1);

            int num_moves = 0;
            stones_t p_moves[STATE_SIZE];
            for (int i = 1; i < si->num_moves; i++) {
                if (!(si->moves[i] & s->opponent)) {
                    p_moves[num_moves++] = si->moves[i];
                }
            }

            s->player = combination_from_key(p_key, num_p, si->size - num_o, p_moves);
        }
        key -= offset;
    }
    if (!is_canonical(s, si)) {
        return 0;
    }
    return is_atari_legal(s);
}

size_t to_atari_key(state *s, state_info *si) {
    state c_ = *s;
    state *c = &c_;
    canonize(c, si);
    size_t num_o = popcount(c->opponent);
    size_t num_p = popcount(c->player);
    size_t num_stones = num_o + num_p;
    size_t offset = 0;
    for (size_t i = 0; i < num_stones; i++) {
        offset += atari_size(i, si->size);
    }
    size_t o_key = combination_to_key(c->opponent, num_o, si->size, si->moves + 1);

    int num_moves = 0;
    stones_t p_moves[STATE_SIZE];
    for (int i = 1; i < si->num_moves; i++) {
        if (!(si->moves[i] & c->opponent)) {
            p_moves[num_moves++] = si->moves[i];
        }
    }

    size_t p_key = combination_to_key(c->player, num_p, si->size - num_o, p_moves);
    return offset + p_key + n_choose_k(si->size - num_o, num_p) * o_key;
}

size_t atari_key_size(state_info *si) {
    size_t offset = 0;
    // si->size, illegal
    // si->size - 1, chain in atari -> preleaf
    for (size_t num_stones = 0; num_stones <= si->size - 2; num_stones++) {
        offset += atari_size(num_stones, si->size);
    }
    return offset;
}

void test_atari_state() {
    stones_t moves[4] = {1, 2, 4, 8};
    print_stones(combination_from_key(4, 2, 4, moves));
    printf("%zu\n", combination_to_key(2 | 8, 2, 4, moves));


    state s_ = (state) {rectangle(4, 1)};
    state *s = &s_;
    state_info si_;
    state_info *si = &si_;
    init_state(s, si);
    size_t k_size = atari_key_size(si);
    for (size_t k = 0; k < k_size; k++) {
        int legal = from_atari_key(s, si, k);
        print_state(s);
        printf("%zu, %zu, %d\n", k, to_atari_key(s, si), legal);
    }

    s->playing_area = rectangle(5, 5);
    s->player = 0;
    s->opponent = 0;
    init_state(s, si);

    printf("%zu\n", atari_key_size(si));

    // Benchmark
    s->playing_area = rectangle(4, 4);
    s->player = 0;
    s->opponent = 0;
    init_state(s, si);
    k_size = atari_key_size(si);
    for (size_t k = 0; k < k_size; k++) {
        if (from_atari_key(s, si, k)) {
            to_atari_key(s, si);
        }
    }
}
