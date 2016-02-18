#include <stdio.h>
#include <ctype.h>
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
#define PRISONER_VALUE (1)

int sign(int x) {
    return (x > 0) - (x < 0);
}

int from_key_s(state *base_state, state *s, size_t key, size_t layer) {
    *s = *base_state;
    stones_t fixed = base_state->target | base_state->immortal;
    s->player &= fixed;
    s->opponent &= fixed;
    s->ko_threats -= sign(s->ko_threats) * layer;
    if (key % 2) {
        stones_t temp = s->player;
        s->player = s->opponent;
        s->opponent = temp;
        s->ko_threats = -s->ko_threats;
        return from_key(s, key / 2);
    }
    else {
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
    if (fixed) {
        if (fixed & ((base_state->player ^ s->player) | (base_state->opponent ^ s->opponent))) {
            parity = 1;
        }
    }
    else if (sign(s->ko_threats) != sign(base_state->ko_threats)) {
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
    assert(*layer <= abs(base_state->ko_threats));
    return key;
}


node_value negamax_node(
        dict *d, lin_dict *ko_ld,
        node_value **base_nodes, node_value **ko_nodes, value_t *leaf_nodes,
        state *base_state, state *s, size_t key, size_t layer, int leaf_rule, int japanese_rules, int depth
    ) {
    if (target_dead(s)) {
        value_t score = -TARGET_SCORE;
        return (node_value) {score, score, 0, 0};
    }
    else if (s->passes == 2) {
        value_t score;
        if (leaf_rule == 2) {
            score = 0;
        }
        else if (leaf_rule == 1) {
            score = liberty_score(s);
        }
        else {
            score = leaf_nodes[key_index(d, key)];
        }
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
            node_value child_v = negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, child, child_key, child_layer, leaf_rule, japanese_rules, depth - 1);
            if (japanese_rules) {
                int prisoners = (popcount(s->opponent) - popcount(child->player)) * PRISONER_VALUE;
                if (child_v.low > -TARGET_SCORE && child_v.low < TARGET_SCORE) {
                    child_v.low = child_v.low - prisoners;
                }
                if (child_v.high > -TARGET_SCORE && child_v.high < TARGET_SCORE) {
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
                node_value new_v = negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, s, key, j, 0, japanese_rules, 1);
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
        size_t base_layer;
        size_t base_key = to_key_s(base_state, base_state, &base_layer);
        print_node(negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, base_state, base_key, base_layer, 0, japanese_rules, 0));
    }
}

void endstate(
        dict *d, lin_dict *ko_ld,
        node_value **base_nodes, node_value **ko_nodes,
        state *base_state, state *s, node_value parent_v, int turn, int low_player
    ) {
    if (s->passes == 2 || target_dead(s)) {
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
            move = 1ULL << j;
        }
        if (make_move(child, move)) {
            size_t child_layer;
            size_t child_key = to_key_s(base_state, child, &child_layer);
            node_value child_v = negamax_node(d, ko_ld, base_nodes, ko_nodes, NULL, base_state, child, child_key, child_layer, 2, 1, 0);
            node_value child_v_p = child_v;
            int japanese_rules = 1;
            if (japanese_rules) {
                int prisoners = (popcount(s->opponent) - popcount(child->player)) * PRISONER_VALUE;
                if (child_v_p.low > -TARGET_SCORE && child_v_p.low < TARGET_SCORE) {
                    child_v_p.low = child_v_p.low - prisoners;
                }
                if (child_v_p.high > -TARGET_SCORE && child_v_p.high < TARGET_SCORE) {
                    child_v_p.high = child_v_p.high - prisoners;
                }
            }
            int is_best_child = (-child_v_p.high == parent_v.low && child_v_p.high_distance + 1 == parent_v.low_distance);
            is_best_child = low_player ? is_best_child : (-child_v_p.low == parent_v.high && child_v_p.low_distance + 1 == parent_v.high_distance);
            if (is_best_child) {
                *s = *child;
                endstate(
                    d, ko_ld,
                    base_nodes, ko_nodes,
                    base_state, s, child_v, !turn, !low_player
                );
                return;
            }
        }
    }
}

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
    int width = 4;
    int height = 3;

    #include "tsumego.c"

    state base_state_ = (state) {rectangle(width, height), 0, 0, 0, 0};
    state *base_state = &base_state_;
    // base_state->opponent = base_state->target = NORTH_WALL & base_state->playing_area;

    // *base_state = *corner_six_1;
    // *base_state = *bulky_ten;
    // *base_state = *cho3;
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
        leaf_nodes[i] = score * 0;
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

    #ifdef CHINESE
    printf("Negamax with Chinese rules.\n");
    iterate(
        d, ko_ld, num_layers,
        base_nodes, ko_nodes, leaf_nodes,
        base_state, key_min, 0
    );
    #endif

    // NOTE: Capture rules may refuse to kill stones when the needed nakade sacrifices exceed the number of stones killed.
    printf("Negamax with capture rules.\n");
    int japanese_rules = 1;
    iterate(
        d, ko_ld, num_layers,
        base_nodes, ko_nodes, leaf_nodes,
        base_state, key_min, 1
    );

    // Japanese leaf state calculation.
    size_t zero_layer = abs(base_state->ko_threats);
    state new_s_;
    state *new_s = &new_s_;
    key = key_min;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(base_state, s, key, zero_layer));

        *new_s = *s;
        endstate(
            d, ko_ld,
            base_nodes, ko_nodes,
            base_state, new_s, base_nodes[zero_layer][i], 0, 1
        );

        stones_t player_alive = s->player & new_s->player;
        stones_t opponent_alive = s->opponent & new_s->opponent;

        stones_t player_territory = 0;
        stones_t opponent_territory = 0;
        stones_t player_region_space = s->playing_area & ~s->player;
        stones_t opponent_region_space = s->playing_area & ~s->opponent;
        for (int j = 0; j < STATE_SIZE; j++) {
            stones_t p = 1ULL << j;
            stones_t region = flood(p, player_region_space);
            player_region_space ^= region;
            if (!(region & opponent_alive)) {
                player_territory |= region;
            }
            region = flood(p, opponent_region_space);
            opponent_region_space ^= region;
            if (!(region & player_alive)) {
                opponent_territory |= region;
            }
        }

        int score;
        if (player_territory & s->target) {
            score = TARGET_SCORE;
        }
        else if (opponent_territory & s->target) {
            score = -TARGET_SCORE;
        }
        else {
            score = popcount(player_territory) + popcount(player_territory & s->opponent) - popcount(opponent_territory) - popcount(opponent_territory & s->player);
        }

        leaf_nodes[i] = score;

        key = next_key(d, key);
    }

    // Clear the rest of the tree.
    for (size_t j = 0; j < num_layers; j++) {
        for (size_t i = 0; i < num_states; i++) {
            base_nodes[j][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
        for (size_t i = 0; i < ko_ld->num_keys; i++) {
            ko_nodes[j][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
    }

    printf("Negamax with Japanese rules.\n");
    iterate(
        d, ko_ld, num_layers,
        base_nodes, ko_nodes, leaf_nodes,
        base_state, key_min, 1
    );

    *s = *base_state;

    char coord1;
    int coord2;

    int parity = 0;
    while (1) {
        size_t layer;
        size_t key = to_key_s(base_state, s, &layer);
        node_value v = negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, s, key, layer, 0, japanese_rules, 0);
        if (parity) {
            state ps_;
            state *ps = &ps_;
            *ps = *s;
            ps->player = s->opponent;
            ps->opponent = s->player;
            ps->ko_threats = -s->ko_threats;
            print_state(ps);
            print_node((node_value){-v.high, -v.low, v.high_distance, v.low_distance});
        }
        else {
            print_state(s);
            print_node(v);
        }
        if (target_dead(s) || s->passes >= 2) {
            break;
        }
        for (int j = -1; j < STATE_SIZE; j++) {
            *child = *s;
            stones_t move;
            if (j == -1){
                move = 0;
            }
            else {
                move = 1UL << j;
            }
            char c1 = 'A' + (j % WIDTH);
            char c2 = '0' + (j / WIDTH);
            if (make_move(child, move)) {
                size_t child_layer;
                size_t child_key = to_key_s(base_state, child, &child_layer);
                node_value child_v = negamax_node(d, ko_ld, base_nodes, ko_nodes, leaf_nodes, base_state, child, child_key, child_layer, 0, japanese_rules, 0);
                if (japanese_rules) {
                    int prisoners = (popcount(s->opponent) - popcount(child->player)) * PRISONER_VALUE;
                    if (child_v.low > -TARGET_SCORE && child_v.low < TARGET_SCORE) {
                        child_v.low = child_v.low - prisoners;
                    }
                    if (child_v.high > -TARGET_SCORE && child_v.high < TARGET_SCORE) {
                        child_v.high = child_v.high - prisoners;
                    }
                }
                if (move) {
                    printf("%c%c", c1, c2);
                }
                else {
                    printf("pass");
                }
                if (-child_v.high == v.low) {
                    printf("-");
                    if (child_v.high_distance + 1 == v.low_distance) {
                        printf("L");
                    }
                    else {
                        printf("l");
                    }
                }
                if (-child_v.high == v.low) {
                    printf("-");
                    if (child_v.low_distance + 1 == v.high_distance) {
                        printf("H");
                    }
                    else {
                        printf("h");
                    }
                }
                printf(" ");
            }
        }
        printf("\n");
        printf("Enter coordinates:\n");
        assert(scanf("%c %d", &coord1, &coord2));
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        coord1 = tolower(coord1) - 'a';
        stones_t move;
        if (coord1 < 0 || coord1 >= WIDTH) {
            // printf("%d, %d\n", coord1, coord2);
            move = 0;
        }
        else {
            move = 1ULL << (coord1 + V_SHIFT * coord2);
        }
        if (make_move(s, move)) {
            parity = !parity;
        }
    }

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
    */

    /*
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
    */

    return 0;
}
