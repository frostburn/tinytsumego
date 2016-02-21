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

enum rule {
    precalculated,
    liberty,
    none
};

typedef struct solution {
    state *base_state;
    state_info *si;
    dict *d;
    lin_dict *ko_ld;
    size_t num_layers;
    node_value **base_nodes;
    node_value **ko_nodes;
    value_t *leaf_nodes;
    enum rule leaf_rule;
    int count_prisoners;
} solution;

void save_solution(solution *sol, char *filename) {
    FILE *f = fopen(filename, "wb");
    size_t state_size = sizeof(state);
    fwrite((void*) &state_size, sizeof(size_t), 1, f);
    fwrite((void*) sol->base_state, state_size, 1, f);
    fwrite((void*) &(sol->d->num_slots), sizeof(size_t), 1, f);
    fwrite((void*) sol->d->slots, sizeof(slot_t), sol->d->num_slots, f);
    fwrite((void*) &(sol->ko_ld->num_keys), sizeof(size_t), 1, f);
    fwrite((void*) sol->ko_ld->keys, sizeof(size_t), sol->ko_ld->num_keys, f);
    fwrite((void*) &(sol->num_layers), sizeof(size_t), 1, f);
    size_t num_states = num_keys(sol->d);
    for (int i = 0;i < sol->num_layers; i++) {
        fwrite((void*) sol->base_nodes[i], sizeof(node_value), num_states, f);
        fwrite((void*) sol->ko_nodes[i], sizeof(node_value), sol->ko_ld->num_keys, f);
    }
    fwrite((void*) sol->leaf_nodes, sizeof(value_t), num_states, f);
    fwrite((void*) &(sol->leaf_rule), sizeof(enum rule), 1, f);
    fwrite((void*) &(sol->count_prisoners), sizeof(int), 1, f);
    fclose(f);
}

// TODO: Allocations
void load_solution(solution *sol, char *filename) {
    FILE *f = fopen(filename, "rb");
    size_t state_size;
    fread((void*) &state_size, sizeof(size_t), 1, f);
    fread((void*) sol->base_state, state_size, 1, f);
    fread((void*) &(sol->d->num_slots), sizeof(size_t), 1, f);
    fread((void*) sol->d->slots, sizeof(slot_t), sol->d->num_slots, f);
    fread((void*) &(sol->ko_ld->num_keys), sizeof(size_t), 1, f);
    fread((void*) sol->ko_ld->keys, sizeof(size_t), sol->ko_ld->num_keys, f);
    fread((void*) &(sol->num_layers), sizeof(size_t), 1, f);
    finalize_dict(sol->d);
    size_t min_key = 0;
    for (size_t i = 0; i < sol->d->num_slots; i++) {
        if (sol->d->slots[i]) {
            for (size_t j = 0; j < 64; i++) {
                if (test_key(sol->d, min_key)) {
                    break;
                }
                else {
                    min_key++;
                }
            }
        }
        else {
            min_key += 64;
        }
    }
    sol->d->min_key = min_key;
    sol->d->max_key = 64 * sol->d->num_slots;

    size_t num_states = num_keys(sol->d);
    // sol->base_nodes = (node_value**) malloc(sizeof(node_value*) * sol->num_layers);
    // sol->ko_nodes = (node_value**) malloc(sizeof(node_value*) * sol->num_layers);
    for (int i = 0;i < sol->num_layers; i++) {
        fread((void*) sol->base_nodes[i], sizeof(node_value), num_states, f);
        fread((void*) sol->ko_nodes[i], sizeof(node_value), sol->ko_ld->num_keys, f);
    }
    fread((void*) sol->leaf_nodes, sizeof(value_t), num_states, f);
    fread((void*) &(sol->leaf_rule), sizeof(enum rule), 1, f);
    fread((void*) &(sol->count_prisoners), sizeof(int), 1, f);
    fclose(f);
}

int sign(int x) {
    return (x > 0) - (x < 0);
}

int from_key_s(state *base_state, state_info *si, state *s, size_t key, size_t layer) {
    *s = *base_state;
    stones_t fixed = base_state->target | base_state->immortal;
    s->player &= fixed;
    s->opponent &= fixed;
    s->ko_threats -= sign(s->ko_threats) * layer;

    if (!si->symmetry) {
        if (key % 2) {
            stones_t temp = s->player;
            s->player = s->opponent;
            s->opponent = temp;
            s->ko_threats = -s->ko_threats;
            s->white_to_play = !s->white_to_play;
        }
        key /= 2;
    }
    return from_key(s, si, key);
}

int from_key_ko(state *base_state, state_info *si, state *s, size_t key, size_t layer) {
    size_t ko_index = key % si->size;
    key /= si->size;
    int legal = from_key_s(base_state, si, s, key, layer);
    s->ko = si->moves[ko_index + 1];
    if (!legal || s->ko & (s->player | s->opponent)) {
        return 0;
    }
    return 1;
}

size_t to_key_s(state *base_state, state_info *si, state *s, size_t *layer) {
    size_t key;
    if (si->symmetry) {
        *layer = abs(s->ko_threats - base_state->ko_threats);
        state c_ = *s;
        state *c = &c_;
        canonize(c, si);
        key = to_key(c, si);
        if (c->ko) {
            int i;
            for (i = 0; i < si->size; i++) {
                if (c->ko == si->moves[i + 1]) {
                    break;
                }
            }
            key = si->size * key + i;
        }
    }
    else {
        key = 2 * to_key(s, si);
        if (s->white_to_play != base_state->white_to_play) {
            key += 1;
            *layer = abs(s->ko_threats + base_state->ko_threats);
        }
        else {
            *layer = abs(s->ko_threats - base_state->ko_threats);
        }
        if (s->ko) {
            int i;
            for (i = 0; i < si->size; i++) {
                if (s->ko == si->moves[i + 1]) {
                    break;
                }
            }
            key = si->size * key + i;
        }
    }
    return key;
}

node_value negamax_node(solution *sol, state *s, size_t key, size_t layer, int depth) {
    if (target_dead(s)) {
        value_t score = -TARGET_SCORE;
        return (node_value) {score, score, 0, 0};
    }
    else if (s->passes == 2) {
        value_t score;
        if (sol->leaf_rule == none) {
            score = 0;
        }
        else if (sol->leaf_rule == liberty) {
            score = liberty_score(s);
        }
        else if (sol->leaf_rule == precalculated) {
            score = sol->leaf_nodes[key_index(sol->d, key)];
        }
        else {
            assert(0);
        }
        return (node_value) {score, score, 0, 0};
    }
    else if (s->passes == 1) {
        depth++;
    }
    if (depth == 0) {
        if (s->ko) {
            return sol->ko_nodes[layer][lin_key_index(sol->ko_ld, key)];
        }
        else {
            return sol->base_nodes[layer][key_index(sol->d, key)];
        }
    }

    state child_;
    state *child = &child_;
    node_value v = (node_value) {VALUE_MIN, VALUE_MIN, DISTANCE_MAX, 0};
    for (int j = 0; j < sol->si->num_moves; j++) {
        *child = *s;
        stones_t move = sol->si->moves[j];
        int prisoners;
        if (make_move(child, move, &prisoners)) {
            size_t child_layer;
            size_t child_key = to_key_s(sol->base_state, sol->si, child, &child_layer);
            node_value child_v = negamax_node(sol, child, child_key, child_layer, depth - 1);
            if (sol->count_prisoners) {
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


void iterate(solution *sol) {
    state s_;
    state *s = &s_;

    size_t num_states = num_keys(sol->d);

    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t j = 0; j < sol->num_layers; j++) {
            size_t key = sol->d->min_key;
            for (size_t i = 0; i < num_states + sol->ko_ld->num_keys; i++) {
                if (i < num_states) {
                    assert(from_key_s(sol->base_state, sol->si, s, key, j));
                }
                else {
                    key = sol->ko_ld->keys[i - num_states];
                    assert(from_key_ko(sol->base_state, sol->si, s, key, j));
                }
                node_value new_v = negamax_node(sol, s, key, j, 1);
                if (i < num_states) {
                    assert(new_v.low >= sol->base_nodes[j][i].low);
                    assert(new_v.high <= sol->base_nodes[j][i].high);
                    changed = changed || !equal(sol->base_nodes[j][i], new_v);
                    sol->base_nodes[j][i] = new_v;
                    key = next_key(sol->d, key);
                }
                else {
                    changed = changed || !equal(sol->ko_nodes[j][i - num_states], new_v);
                    sol->ko_nodes[j][i - num_states] = new_v;
                }
            }
        }
        size_t base_layer;
        size_t base_key = to_key_s(sol->base_state, sol->si, sol->base_state, &base_layer);
        print_node(negamax_node(sol, sol->base_state, base_key, base_layer, 0));
    }
}

void endstate(solution *sol, state *s, node_value parent_v, int turn, int low_player) {
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
    for (int j = 0; j < sol->si->num_moves; j++) {
        *child = *s;
        stones_t move = sol->si->moves[j];
        int prisoners;
        if (make_move(child, move, &prisoners)) {
            size_t child_layer;
            size_t child_key = to_key_s(sol->base_state, sol->si, child, &child_layer);
            node_value child_v = negamax_node(sol, child, child_key, child_layer, 0);
            node_value child_v_p = child_v;
            if (sol->count_prisoners) {
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
                endstate(sol, s, child_v, !turn, !low_player);
                return;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int width = -1;
    int height = -1;
    int ko_threats = 0;
    int load_sol = 0;
    if (strcmp(argv[argc - 1], "load") == 0) {
        load_sol = 1;
        argc--;
    }
    if (argc < 3) {
        if (argc == 2) {
            ko_threats = atoi(argv[1]);
        }
        printf("Using predifined tsumego\n");
    }
    else {
        if (argc == 4) {
            ko_threats = atoi(argv[3]);
        }
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        if (width <= 0) {
            printf("Width must be larger than 0.\n");
            return 0;
        }
        if (width >= 10) {
            printf("Width must be less than 10.\n");
            return 0;
        }
        if (height <= 0) {
            printf("Height must be larger than 0.\n");
            return 0;
        }
        if (height >= 8) {
            printf("Height must be less than 8.\n");
            return 0;
        }
    }

    #include "tsumego.c"

    state base_state_;
    state *base_state = &base_state_;

    if (width > 0) {
        *base_state = (state) {rectangle(width, height), 0, 0, 0, 0};
    }
    else {
        *base_state = *corner_six_1;
        // *base_state = *bulky_ten;
        // *base_state = *cho3;
        // *base_state = *cho534;
    }
    base_state->ko_threats = ko_threats;


    state_info si_;
    state_info *si = &si_;
    init_state(base_state, si);

    size_t num_layers;
    if (si->symmetry) {
        num_layers = 2 * abs(base_state->ko_threats) + 1;
    }
    else {
        num_layers = abs(base_state->ko_threats) + 1;
    }

    print_state(base_state);
    state s_;
    state *s = &s_;

    dict d_;
    dict *d = &d_;

    solution sol_;
    solution *sol = &sol_;
    sol->base_state = base_state;
    sol->si = si;
    sol->d = d;
    sol->num_layers = num_layers;

    size_t max_k = max_key(base_state);
    if (!si->symmetry) {
        max_k *= 2;
    }
    init_dict(d, max_k);

    size_t key_max = 0;

    size_t total_legal = 0;
    for (size_t k = 0; k < max_k; k++) {
        if (!from_key_s(base_state, si, s, k, 0)){
            continue;
        }
        total_legal++;
        size_t layer;
        size_t key = to_key_s(base_state, si, s, &layer);
        assert(layer == 0);
        add_key(d, key);
        if (key > key_max) {
            key_max = key;
        }
    }
    resize_dict(d, key_max);
    finalize_dict(d);
    size_t num_states = num_keys(d);

    printf("Total positions %zu\n", total_legal);
    printf("Total unique positions %zu\n", num_states);

    node_value **base_nodes = (node_value**) malloc(num_layers * sizeof(node_value*));
    for (size_t i = 0; i < num_layers; i++) {
        base_nodes[i] = (node_value*) malloc(num_states * sizeof(node_value));
    }
    value_t *leaf_nodes = (value_t*) malloc(num_states * sizeof(value_t));

    lin_dict ko_ld_ = (lin_dict) {0, 0, 0, NULL};
    lin_dict *ko_ld = &ko_ld_;

    sol->base_nodes = base_nodes;
    sol->leaf_nodes = leaf_nodes;
    sol->ko_ld = ko_ld;

    state child_;
    state *child = &child_;
    size_t child_key;
    size_t key = d->min_key;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(base_state, si, s, key, 0));
        value_t score = liberty_score(s);
        leaf_nodes[i] = score;
        for (size_t k = 0; k < num_layers; k++) {
            (base_nodes[k])[i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
        for (int j = 0; j < STATE_SIZE; j++) {
            *child = *s;
            int prisoners;
            if (make_move(child, 1ULL << j, &prisoners)) {
                if (target_dead(child)) {
                    continue;
                }
                if (child->ko) {
                    size_t child_layer;
                    child_key = to_key_s(base_state, si, child, &child_layer);
                    add_lin_key(ko_ld, child_key);
                }
            }
        }
        key = next_key(d, key);
    }

    finalize_lin_dict(ko_ld);

    node_value **ko_nodes = (node_value**) malloc(num_layers * sizeof(node_value*));
    sol->ko_nodes = ko_nodes;
    for (size_t i = 0; i < num_layers; i++) {
        ko_nodes[i] = (node_value*) malloc(ko_ld->num_keys * sizeof(node_value));
    }
    printf("Unique positions with ko %zu\n", ko_ld->num_keys);

    for (size_t i = 0; i < ko_ld->num_keys; i++) {
        for (size_t k = 0; k < num_layers; k++) {
            ko_nodes[k][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
    }

    if (load_sol) {
        goto frontend;
    }

    #ifdef CHINESE
    printf("Negamax with Chinese rules.\n");
    sol->count_prisoners = 0;
    sol->leaf_rule = precalculated;
    iterate(sol);
    #endif

    // NOTE: Capture rules may refuse to kill stones when the needed nakade sacrifices exceed the number of stones killed.
    // This can probably be remedied by setting the value of the stones on the board high enough compared to the stones that come later.
    // That doesn't sit well with tablebases though.
    printf("Negamax with capture rules.\n");
    sol->count_prisoners = 1;
    sol->leaf_rule = none;
    iterate(sol);

    save_solution(sol, "capture.dat");

    // Japanese leaf state calculation.
    size_t zero_layer = abs(base_state->ko_threats);
    state new_s_;
    state *new_s = &new_s_;
    key = d->min_key;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(base_state, si, s, key, zero_layer));

        *new_s = *s;
        endstate(sol, new_s, base_nodes[zero_layer][i], 0, 1);

        stones_t player_alive = s->player & new_s->player;
        stones_t opponent_alive = s->opponent & new_s->opponent;

        stones_t player_territory = 0;
        stones_t opponent_territory = 0;
        stones_t player_region_space = s->playing_area & ~player_alive;
        stones_t opponent_region_space = s->playing_area & ~opponent_alive;
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
    sol->leaf_rule = precalculated;
    iterate(sol);

    frontend:
    if (load_sol) {
        load_solution(sol, "japanese.dat");
    }
    else {
        save_solution(sol, "japanese.dat");
    }

    *s = *base_state;

    char coord1;
    int coord2;

    int total_prisoners = 0;
    int turn = 0;

    while (1) {
        size_t layer;
        size_t key = to_key_s(base_state, si, s, &layer);
        node_value v = negamax_node(sol, s, key, layer, 0);
        print_state(s);
        if (turn) {
            print_node((node_value) {total_prisoners - v.high, total_prisoners - v.low, v.high_distance, v.low_distance});
        }
        else {
            print_node((node_value) {total_prisoners + v.low, total_prisoners + v.high, v.low_distance, v.high_distance});
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
            int prisoners;
            if (make_move(child, move, &prisoners)) {
                size_t child_layer;
                size_t child_key = to_key_s(base_state, si, child, &child_layer);
                node_value child_v = negamax_node(sol, child, child_key, child_layer, 0);
                if (sol->count_prisoners) {
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
        int prisoners;
        if (make_move(s, move, &prisoners)) {
            if (turn) {
                total_prisoners -= prisoners;
            }
            else {
                total_prisoners += prisoners;
            }
            turn = !turn;
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
