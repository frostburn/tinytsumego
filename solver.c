#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// No headers? Are you serious?!
#define MAIN
#include "state.c"
#include "dict.c"
#include "node.c"

// Where's the makefile? Oh, you gotta be kidding me.
// gcc -std=gnu99 -Wall -O3 solver.c -o solver; solver 4 3


#define TARGET_SCORE (126)
#define PRISONER_VALUE (2)

int sign(int x) {
    return (x > 0) - (x < 0);
}

int from_key_s(state *base_state, state *s, size_t key, size_t layer) {
    if (key % 2) {
        *s = *base_state;
        s->ko_threats -= sign(s->ko_threats) * layer;
        stones_t temp = s->player;
        s->player = s->opponent;
        s->opponent = temp;
        s->ko_threats = -s->ko_threats;
        return from_key(s, key / 2);
    }
    else {
        *s = *base_state;
        s->ko_threats -= sign(s->ko_threats) * layer;
        return from_key(s, key / 2);
    }
}

int from_key_ko(state *base_state, state *s, size_t key, size_t layer) {
    size_t ko_pos = key % STATE_SIZE;
    key /= STATE_SIZE;
    int legal = from_key_s(base_state, s, key, layer);
    s->ko = 1ULL << ko_pos;
    if (!legal || s->ko & (s->player | s->opponent)) {
        return 0;
    }
    return 1;
}

size_t to_key_s(state *base_state, state *s, size_t *layer) {
    stones_t fixed = s->target | s->immortal;
    size_t parity = 0;
    if (fixed & ((base_state->player ^ s->player) | (base_state->opponent ^ s->opponent))) {
        parity = 1;
    }
    size_t key = 2 * to_key(s) + parity;
    if (s->ko) {
        key = STATE_SIZE * key + bitscan(s->ko);
    }
    if (parity) {
        *layer = abs(s->ko_threats + base_state->ko_threats);
    }
    else {
        *layer = abs(s->ko_threats - base_state->ko_threats);
    }
    return key;
}


node_value negamax_node(
        dict *d, lin_dict *ko_ld,
        node_value **base_nodes, node_value **ko_nodes, value_t *leaf_nodes,
        state *base_state, state *s, size_t key, size_t layer, int japanese_rules, int depth
    ) {
    if (target_dead(s)) {
        value_t score = -TARGET_SCORE;
        return (node_value) {score, score, 0, 0};
    }
    else if (s->passes == 2) {
        value_t score = leaf_nodes[key_index(d, key)];
        return (node_value) {score, score, 0, 0};
    }
    else if (s->passes == 1) {
        depth++;
    }
    if (depth == 0) {
        if (s->ko) {
            return ko_nodes[layer][lin_key_index(ko_ld, key)];
        }
        else {
            return base_nodes[layer][key_index(d, key)];
        }
    }

    state child_;
    state *child = &child_;
    node_value v = (node_value) {VALUE_MIN, VALUE_MIN, DISTANCE_MAX, 0};
    for (int j = -1; j < STATE_SIZE; j++) {
        *child = *s;
        stones_t move;
        if (j == -1){
            move = 0;
        }
        else {
            move = 1ULL << j;
        }
        if (make_move(child, move)) {
            size_t child_layer;
            size_t child_key = to_key_s(base_state, child, &child_layer);
            node_value child_v = negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, child, child_key, child_layer, japanese_rules, depth - 1);
            if (japanese_rules) {
                int prisoners = (popcount(s->opponent) - popcount(child->player)) * PRISONER_VALUE;
                if (child_v.low > -TARGET_SCORE) {
                    child_v.low = child_v.low - prisoners;
                }
                if (child_v.high < TARGET_SCORE) {
                    child_v.high = child_v.high - prisoners;
                }
            }
            v = negamax(v, child_v);
        }
    }
    return v;
}


void iterate(
        dict *d, lin_dict *ko_ld, size_t num_layers,
        node_value **base_nodes, node_value **ko_nodes, value_t *leaf_nodes,
        state *base_state, size_t key_min, int japanese_rules
    ) {
    state s_;
    state *s = &s_;

    size_t num_states = num_keys(d);

    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t j = 0; j < num_layers; j++) {
            size_t key = key_min;
            for (size_t i = 0; i < num_states + ko_ld->num_keys; i++) {
                if (i < num_states) {
                    assert(from_key_s(base_state, s, key, j));
                }
                else {
                    key = ko_ld->keys[i - num_states];
                    assert(from_key_ko(base_state, s, key, j));
                }
                node_value new_v = negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, s, key, j, japanese_rules, 1);
                if (i < num_states) {
                    assert(new_v.low >= base_nodes[j][i].low);
                    assert(new_v.high <= base_nodes[j][i].high);
                    changed = changed || !equal(base_nodes[j][i], new_v);
                    base_nodes[j][i] = new_v;
                    key = next_key(d, key);
                }
                else {
                    changed = changed || !equal(ko_nodes[j][i - num_states], new_v);
                    ko_nodes[j][i - num_states] = new_v;
                }
            }
        }
        print_node(base_nodes[0][0]);
    }
}

/*
void endstate(
        dict *d, lin_dict *ko_ld,
        node_value *base_nodes, node_value *pass_nodes, node_value *ko_nodes,
        state *s, node_value parent_v, int turn, int low_player
    ) {
    if (s->passes == 2) {
        if (turn) {
            stones_t temp = s->player;
            s->player = s->opponent;
            s->opponent = temp;
        }
        return;
    }
    state child_;
    state *child = &child_;
    for (int j = -1; j < STATE_SIZE; j++) {
        *child = *s;
        stones_t move;
        if (j == -1){
            move = 0;
        }
        else {
            move = 1UL << j;
        }
        node_value child_v;
        if (make_move(child, move)) {
            canonize(child);
            size_t child_key = to_key(child);
            if (child->passes == 2){
                value_t score = liberty_score(child);
                child_v = (node_value) {score, score, 0, 0};
            }
            else if (child->passes == 1) {
                child_v = pass_nodes[key_index(d, child_key)];
            }
            else if (child->ko) {
                child_key = child_key * STATE_SIZE + bitscan(child->ko);
                child_v = ko_nodes[lin_key_index(ko_ld, child_key)];
            }
            else {
                child_v = base_nodes[key_index(d, child_key)];
            }
            int is_best_child = (-child_v.high == parent_v.low && child_v.high_distance + 1 == parent_v.low_distance);
            is_best_child = low_player ? is_best_child : (-child_v.low == parent_v.high && child_v.low_distance + 1 == parent_v.high_distance);
            if (is_best_child) {
                *s = *child;
                endstate(
                    d, ko_ld,
                    base_nodes, pass_nodes, ko_nodes,
                    s, child_v, !turn, !low_player
                );
            }
        }
    }
}
*/


int main(int argc, char *argv[]) {
    /*
    if (argc != 3) {
        printf("Usage: %s width height\n", argv[0]);
        return 0;
    }
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    if (width <= 0) {
        printf("Width must be larger than 0.\n");
        return 0;
    }
    if (width >= 7) {
        printf("Width must be less than 7.\n");
        return 0;
    }
    if (height <= 0) {
        printf("Height must be larger than 0.\n");
        return 0;
    }
    if (height >= 6) {
        printf("Height must be less than 6.\n");
        return 0;
    }
    */
    int width = 3;
    int height = 3;

    #include "tsumego.c"

    state base_state_ = (state) {rectangle(width, height), 0, 0, 0, 0};
    state *base_state = &base_state_;
    base_state->opponent = base_state->target = NORTH_WALL & base_state->playing_area;

    *base_state = *bulky_ten;
    // base_state->opponent = base_state->target |= 1ULL << 9;
    // base_state->ko_threats = 1;
    size_t num_layers = abs(base_state->ko_threats) + 1;

    print_state(base_state);
    state s_;
    state *s = &s_;

    dict d_;
    dict *d = &d_;

    size_t max_k = 2 * max_key(base_state);
    init_dict(d, max_k);

    size_t key_min = ~0;
    size_t key_max = 0;

    size_t total_legal = 0;
    for (size_t k = 0; k < max_k; k++) {
        if (!from_key_s(base_state, s, k, 0)){
            continue;
        }
        total_legal++;
        size_t key = k;
        add_key(d, key);
        if (key < key_min) {
            key_min = key;
        }
        if (key > key_max) {
            key_max = key;
        }
    }
    resize_dict(d, key_max);
    finalize_dict(d);
    size_t num_states = num_keys(d);

    printf("Total positions %zu\n", num_states);

    node_value **base_nodes = (node_value**) malloc(num_layers * sizeof(node_value*));
    for (size_t i = 0; i < num_layers; i++) {
        base_nodes[i] = (node_value*) malloc(num_states * sizeof(node_value));
    }
    value_t *leaf_nodes = (value_t*) malloc(num_states * sizeof(value_t));

    lin_dict ko_ld_ = (lin_dict) {0, 0, 0, NULL};
    lin_dict *ko_ld = &ko_ld_;

    state child_;
    state *child = &child_;
    size_t child_key;
    size_t key = key_min;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(base_state, s, key, 0));
        value_t score = liberty_score(s);
        leaf_nodes[i] = score;
        for (size_t k = 0; k < num_layers; k++) {
            (base_nodes[k])[i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
        for (int j = 0; j < STATE_SIZE; j++) {
            *child = *s;
            if (make_move(child, 1ULL << j)) {
                if (target_dead(child)) {
                    continue;
                }
                if (child->ko) {
                    size_t dummy;
                    child_key = to_key_s(base_state, child, &dummy);
                    add_lin_key(ko_ld, child_key);
                }
            }
        }
        key = next_key(d, key);
    }

    finalize_lin_dict(ko_ld);

    node_value **ko_nodes = (node_value**) malloc(num_layers * sizeof(node_value*));
    for (size_t i = 0; i < num_layers; i++) {
        ko_nodes[i] = (node_value*) malloc(ko_ld->num_keys * sizeof(node_value));
    }
    printf("Positions with ko %zu\n", ko_ld->num_keys);

    for (size_t i = 0; i < ko_ld->num_keys; i++) {
        for (size_t k = 0; k < num_layers; k++) {
            ko_nodes[k][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
    }

    #ifndef PRELOAD
    printf("Negamax with Chinese rules.\n");
    iterate(
        d, ko_ld, num_layers,
        base_nodes, ko_nodes, leaf_nodes,
        base_state, key_min, 0
    );
    #endif

    /*
    char dir_name[16];
    sprintf(dir_name, "%dx%d", width, height);
    struct stat sb;
    if (stat(dir_name, &sb) == -1) {
        mkdir(dir_name, 0700);
    }
    assert(chdir(dir_name) == 0);
    FILE *f;

    #ifndef PRELOAD
    f = fopen("d_slots.dat", "wb");
    fwrite((void*) d->slots, sizeof(slot_t), d->num_slots, f);
    fclose(f);
    f = fopen("d_checkpoints.dat", "wb");
    fwrite((void*) d->checkpoints, sizeof(size_t), (d->num_slots >> 4) + 1, f);
    fclose(f);
    f = fopen("ko_ld_keys.dat", "wb");
    fwrite((void*) ko_ld->keys, sizeof(size_t), ko_ld->num_keys, f);
    fclose(f);

    f = fopen("base_nodes.dat", "wb");
    fwrite((void*) base_nodes, sizeof(node_value), num_states, f);
    fclose(f);
    f = fopen("pass_nodes.dat", "wb");
    fwrite((void*) pass_nodes, sizeof(node_value), num_states, f);
    fclose(f);
    f = fopen("leaf_nodes.dat", "wb");
    fwrite((void*) leaf_nodes, sizeof(value_t), num_states, f);
    fclose(f);
    f = fopen("ko_nodes.dat", "wb");
    fwrite((void*) ko_nodes, sizeof(node_value), ko_ld->num_keys, f);
    fclose(f);
    #endif

    #ifdef PRELOAD
    f = fopen("base_nodes.dat", "rb");
    fread((void*) base_nodes, sizeof(node_value), num_states, f);
    fclose(f);
    f = fopen("pass_nodes.dat", "rb");
    fread((void*) pass_nodes, sizeof(node_value), num_states, f);
    fclose(f);
    f = fopen("leaf_nodes.dat", "rb");
    fread((void*) leaf_nodes, sizeof(value_t), num_states, f);
    fclose(f);
    f = fopen("ko_nodes.dat", "rb");
    fread((void*) ko_nodes, sizeof(node_value), ko_ld->num_keys, f);
    fclose(f);
    #endif

    // Japanese leaf state calculation.
    state new_s_;
    state *new_s = &new_s_;
    key = key_min;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key(s, key));
        stones_t empty = s->playing_area & ~(s->player | s->opponent);

        *new_s = *s;
        endstate(
            d, ko_ld,
            base_nodes, pass_nodes, ko_nodes,
            new_s, base_nodes[i], 0, 1
        );

        stones_t player_controlled = new_s->player | liberties(new_s->player, new_s->playing_area & ~new_s->opponent);
        stones_t opponent_controlled = new_s->opponent | liberties(new_s->opponent, new_s->playing_area & ~new_s->player);
        value_t score = popcount(player_controlled & empty) - popcount(opponent_controlled & empty);
        score += 2 * (popcount(player_controlled & s->opponent) - popcount(opponent_controlled & s->player));
        leaf_nodes[i] = score;

        *new_s = *s;
        endstate(
            d, ko_ld,
            base_nodes, pass_nodes, ko_nodes,
            new_s, base_nodes[i], 0, 0
        );

        player_controlled = new_s->player | liberties(new_s->player, new_s->playing_area & ~new_s->opponent);
        opponent_controlled = new_s->opponent | liberties(new_s->opponent, new_s->playing_area & ~new_s->player);
        score = popcount(player_controlled & empty) - popcount(opponent_controlled & empty);
        score += 2 * (popcount(player_controlled & s->opponent) - popcount(opponent_controlled & s->player));
        leaf_nodes[i] += score;

        key = next_key(d, key);
    }

    // Clear the rest of the tree.
    for (size_t i = 0; i < num_states; i++) {
        pass_nodes[i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        base_nodes[i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
    }
    for (size_t i = 0; i < ko_ld->num_keys; i++) {
        ko_nodes[i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
    }

    printf("Negamax with Japanese rules.\n");
    iterate(
        d, ko_ld,
        base_nodes, pass_nodes, ko_nodes, leaf_nodes,
        s, key_min, 1
    );

    f = fopen("base_nodes_j.dat", "wb");
    fwrite((void*) base_nodes, sizeof(node_value), num_states, f);
    fclose(f);
    f = fopen("pass_nodes_j.dat", "wb");
    fwrite((void*) pass_nodes, sizeof(node_value), num_states, f);
    fclose(f);
    f = fopen("leaf_nodes_j.dat", "wb");
    fwrite((void*) leaf_nodes, sizeof(value_t), num_states, f);
    fclose(f);
    f = fopen("ko_nodes_j.dat", "wb");
    fwrite((void*) ko_nodes, sizeof(node_value), ko_ld->num_keys, f);
    fclose(f);

    return 0;
    */
}
