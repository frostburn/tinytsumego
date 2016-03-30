#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// TODO <signal.h> catch Ctrl+C

// No headers? Are you serious?!
#define MAIN
#include "state.c"
#include "dict.c"
#include "node.c"
#include "solver_common.c"
#include "utils.c"

// Where's the makefile? Oh, you gotta be kidding me.
// gcc -std=gnu99 -Wall -O3 solver.c -o solver; solver 4 3

void iterate(solution *sol, char *filename) {
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
                    assert(from_key_s(sol, s, key, j));
                }
                else {
                    key = sol->ko_ld->keys[i - num_states];
                    assert(from_key_ko(sol, s, key, j));
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
        size_t base_key = to_key_s(sol, sol->base_state, &base_layer);
        print_node(negamax_node(sol, sol->base_state, base_key, base_layer, 0));

        FILE *f = fopen(filename, "wb");
        save_solution(sol, f);
        fclose(f);
        // Verify solution integrity. For debugging only. Leaks memory.
        // char *buffer = file_to_buffer(filename);
        // buffer = load_solution(sol, buffer, 0);
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
            size_t child_key = to_key_s(sol, child, &child_layer);
            node_value child_v = negamax_node(sol, child, child_key, child_layer, 0);
            node_value child_v_p = add_prisoners(sol, child_v, prisoners);
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

static const char* tsumego_name = NULL;
static int board_width = 0;
static int board_height = 0;
static int ko_threats = 0;
static int num_layers = -1;

static int string_is_number(const char* string) {
    const size_t length = strlen(string);
    size_t i;

    if (!length) {
        return 0;
    }
    for (i = 0; i < length; ++i) {
        if (!isdigit(string[i])) {
            return 0;
        }
    }

    return 1;
}

static void parse_args(const int argc, char** argv) {
    int i;

    for (i = 0; i < argc; ++i) {
        const char* argument = argv[i];

        if (*argument != '-') {
            if (string_is_number(argument)) {
                if (!board_width) {
                    board_width = atoi(argument);
                }
                else if (!board_height) {
                    board_height = atoi(argument);
                } else {
                    assert(0);
                }
            }
            else if (!tsumego_name) {
                tsumego_name = argument;
            } else {
                assert(0);
            }
            continue;
        }
        else if (argument[1] == 'k') {
            ko_threats = atoi(argument + 2);
        }
        else if (argument[1] == 'l') {
            num_layers = atoi(argument + 2);
        } else {
            assert(0);
        }
    }
    if (board_width) {
        assert(board_height);
        assert(!tsumego_name);
    }
    else if (!tsumego_name) {
        assert(0);
    }
}

typedef struct tsumego_info {
    const char* name;
    state* state;
} tsumego_info;

int main(int argc, char *argv[]) {
    int load_sol = 0;
    int resume_sol = 0;
    if (strcmp(argv[argc - 1], "load") == 0) {
        load_sol = 1;
        argc--;
    }
    if (strcmp(argv[argc - 1], "resume") == 0) {
        resume_sol = 1;
        argc--;
    }
    parse_args(argc - 1, argv + 1);

    int width = board_width;
    int height = board_height;
    if (board_width >= 10) {
        fprintf(stderr, "Width must be less than 10.\n");
        exit(EXIT_FAILURE);
    }
    if (board_height >= 8) {
        fprintf(stderr, "Height must be less than 8.\n");
        exit(EXIT_FAILURE);
    }

    #include "tsumego.c"

    state base_state_;
    state *base_state = &base_state_;

    char sol_name[64] = "unknown";
    char temp_filename[128];
    char filename[128];
    if (board_width > 0) {
        *base_state = (state) {rectangle(width, height), 0, 0, 0, 0};
        sprintf(sol_name, "%dx%d", width, height);
    }
    else {
        int i;
        int found = 0;

        for (i = 0; tsumego_infos[i].name; ++i) {
            if (!strcmp(tsumego_name, tsumego_infos[i].name)) {
                *base_state = *(tsumego_infos[i].state);
                strcpy(sol_name, tsumego_name);
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "unknown tsumego: `%s'\n", tsumego_name);
            exit(EXIT_FAILURE);
        }
    }
    base_state->ko_threats = ko_threats;

    sprintf(temp_filename, "%s_temp.dat", sol_name);


    state_info si_;
    state_info *si = &si_;
    init_state(base_state, si);

    if (si->color_symmetry) {
        num_layers = 2 * abs(base_state->ko_threats) + 1;
    }
    else if (num_layers <= 0) {
        num_layers = abs(base_state->ko_threats) + 1;
    }
    else {
        assert(num_layers >= abs(base_state->ko_threats) + 1);
    }

    print_state(base_state);
    for (int i = 0; i < si->num_external; i++) {
        print_stones(si->externals[i]);
    }
    printf(
        "width=%d height=%d c=%d v=%d h=%d d=%d\n",
        si->width,
        si->height,
        si->color_symmetry,
        si->mirror_v_symmetry,
        si->mirror_h_symmetry,
        si->mirror_d_symmetry
    );

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
    size_t num_states;

    // Re-used at frontend. TODO: Allocate a different pointer.
    state child_;
    state *child = &child_;

    if (load_sol) {
        goto frontend;
    }

    if (resume_sol) {
        char *buffer = file_to_buffer(temp_filename);
        buffer = load_solution(sol, buffer, 1);
        num_states = num_keys(sol->d);
        if (sol->leaf_rule == japanese_double_liberty) {
            goto iterate_capture;
        }
        else {
            goto iterate_japanese;
        }
    }

    size_t k_size = key_size(sol->si);
    if (!sol->si->color_symmetry) {
        k_size *= 2;
    }
    init_dict(sol->d, k_size);

    size_t total_legal = 0;
    for (size_t k = 0; k < k_size; k++) {
        if (!from_key_s(sol, s, k, 0)){
            continue;
        }
        total_legal++;
        size_t layer;
        size_t key = to_key_s(sol, s, &layer);
        assert(layer == 0);
        add_key(sol->d, key);
    }
    finalize_dict(sol->d);
    num_states = num_keys(sol->d);

    printf("Total positions %zu\n", total_legal);
    printf("Total unique positions %zu\n", num_states);

    node_value **base_nodes = (node_value**) malloc(sol->num_layers * sizeof(node_value*));
    for (size_t i = 0; i < sol->num_layers; i++) {
        base_nodes[i] = (node_value*) malloc(num_states * sizeof(node_value));
    }
    value_t *leaf_nodes = (value_t*) malloc(num_states * sizeof(value_t));

    lin_dict ko_ld_ = (lin_dict) {0, 0, 0, NULL};
    lin_dict *ko_ld = &ko_ld_;

    sol->base_nodes = base_nodes;
    sol->leaf_nodes = leaf_nodes;
    sol->ko_ld = ko_ld;

    size_t child_key;
    size_t key = sol->d->min_key;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(sol, s, key, 0));
        // size_t layer;
        // assert(to_key_s(sol, s, &layer) == key);
        sol->leaf_nodes[i] = 0;
        for (size_t k = 0; k < sol->num_layers; k++) {
            (sol->base_nodes[k])[i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
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
                    child_key = to_key_s(sol, child, &child_layer);
                    add_lin_key(sol->ko_ld, child_key);
                }
            }
        }
        key = next_key(sol->d, key);
    }

    finalize_lin_dict(sol->ko_ld);

    node_value **ko_nodes = (node_value**) malloc(sol->num_layers * sizeof(node_value*));
    sol->ko_nodes = ko_nodes;
    for (size_t i = 0; i < sol->num_layers; i++) {
        sol->ko_nodes[i] = (node_value*) malloc(sol->ko_ld->num_keys * sizeof(node_value));
    }
    printf("Unique positions with ko %zu\n", sol->ko_ld->num_keys);

    for (size_t i = 0; i < sol->ko_ld->num_keys; i++) {
        for (size_t k = 0; k < sol->num_layers; k++) {
            sol->ko_nodes[k][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
    }

    #ifdef CHINESE
    printf("Negamax with Chinese rules.\n");
    sol->count_prisoners = 0;
    sol->leaf_rule = chinese_liberty;
    iterate(sol, temp_filename);
    #endif

    // NOTE: Capture rules may refuse to kill stones when the needed nakade sacrifices exceed triple the number of stones killed.
    printf("Negamax with capture rules.\n");
    sol->count_prisoners = 1;
    sol->leaf_rule = japanese_double_liberty;
    iterate_capture:
    iterate(sol, temp_filename);

    sprintf(filename, "%s_capture.dat", sol_name);
    FILE *f = fopen(filename, "wb");
    save_solution(sol, f);
    fclose(f);

    // Japanese leaf state calculation.
    // Store territory for UI. (Or maybe not. Takes too much space.)
    // f = fopen("territory.dat", "wb");
    size_t zero_layer = abs(sol->base_state->ko_threats);
    state new_s_;
    state *new_s = &new_s_;
    key = sol->d->min_key;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(sol, s, key, zero_layer));

        *new_s = *s;
        endstate(sol, new_s, sol->base_nodes[zero_layer][i], 0, 1);

        // Use a flood of life so that partially dead nakade won't give extra points.
        // Note while this won't mark dead groups as alive, it can treat living nakade stones as dead.
        stones_t player_alive = flood(new_s->player, s->player);
        stones_t opponent_alive = flood(new_s->opponent, s->opponent);

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
            // Subtract friendly stones on the board from territory.
            player_territory &= ~s->player;
            opponent_territory &= ~s->opponent;
            score = popcount(player_territory) + popcount(player_territory & s->opponent) - popcount(opponent_territory) - popcount(opponent_territory & s->player);
        }

        sol->leaf_nodes[i] = score;

        // stones_t territory = player_territory | opponent_territory;
        // fwrite((void*) &territory, sizeof(stones_t), 1, f);

        key = next_key(sol->d, key);
    }
    // fclose(f);

    // Clear the rest of the tree.
    for (size_t j = 0; j < sol->num_layers; j++) {
        for (size_t i = 0; i < num_states; i++) {
            sol->base_nodes[j][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
        for (size_t i = 0; i < sol->ko_ld->num_keys; i++) {
            sol->ko_nodes[j][i] = (node_value) {VALUE_MIN, VALUE_MAX, DISTANCE_MAX, DISTANCE_MAX};
        }
    }

    printf("Negamax with Japanese rules.\n");
    sol->count_prisoners = 1;
    sol->leaf_rule = precalculated;
    iterate_japanese:
    iterate(sol, temp_filename);

    sprintf(filename, "%s_japanese.dat", sol_name);
    f = fopen(filename, "wb");
    save_solution(sol, f);
    fclose(f);

    frontend:
    if (load_sol) {
        sprintf(filename, "%s_japanese.dat", sol_name);
        char *buffer = file_to_buffer(filename);
        buffer = load_solution(sol, buffer, 1);
    }

    *s = *sol->base_state;

    char coord1;
    int coord2;

    int total_prisoners = 0;
    int turn = 0;

    while (1) {
        size_t layer;
        size_t key = to_key_s(sol, s, &layer);
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
                move = 1ULL << j;
            }
            char c1 = 'A' + (j % WIDTH);
            char c2 = '0' + (j / WIDTH);
            int prisoners;
            if (make_move(child, move, &prisoners)) {
                size_t child_layer;
                size_t child_key = to_key_s(sol, child, &child_layer);
                node_value child_v = negamax_node(sol, child, child_key, child_layer, 0);
                if (sol->count_prisoners) {
                    if (child_v.low > VALUE_MIN && child_v.low < VALUE_MAX) {
                        child_v.low = child_v.low - prisoners;
                    }
                    if (child_v.high > VALUE_MIN && child_v.high < VALUE_MAX) {
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

    return 0;
}
