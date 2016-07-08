#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// TODO <signal.h> catch Ctrl+C

#include <omp.h>

#define MAIN
#include "state.c"
#include "dict.c"
#include "node.c"
#include "solver_common_light.c"
#include "utils.c"

// gcc -std=gnu99 -Wall -O3 -fopenmp solver_light.c -o solver_light; solver_light 4 3

void iterate(solution *sol, char *filename) {
    size_t num_states = num_keys(sol->d);

    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t j = 0; j < sol->num_layers; j++) {
            size_t key;
            size_t i;
            int tid;
            int num_threads;
            #pragma omp parallel private(i, key, tid, num_threads)
            {
                num_threads = omp_get_num_threads();
                state s_;
                state *s = &s_;
                tid = omp_get_thread_num();
                key = sol->d->min_key;
                for (i = 0; i < num_states; i++) {
                    if (i % num_threads == tid) {
                        assert(from_key_s(sol, s, key, j));
                        light_value new_v = negamax_node(sol, s, key, j, 1);
                        assert(new_v.low >= sol->base_nodes[j][i].low);
                        assert(new_v.high <= sol->base_nodes[j][i].high);
                        changed = changed || !equal_light(sol->base_nodes[j][i], new_v);
                        sol->base_nodes[j][i] = new_v;
                    }
                    key = next_key(sol->d, key);
                }
                #pragma omp for
                for (i = 0; i < sol->ko_ld->num_keys; i++) {
                    key = sol->ko_ld->keys[i];
                    assert(from_key_ko(sol, s, key, j));
                    light_value new_v = negamax_node(sol, s, key, j, 1);
                    assert(new_v.low >= sol->ko_nodes[j][i].low);
                    assert(new_v.high <= sol->ko_nodes[j][i].high);
                    changed = changed || !equal_light(sol->ko_nodes[j][i], new_v);
                    sol->ko_nodes[j][i] = new_v;
                }
            }
        }
        int base_layer;
        size_t base_key = to_key_s(sol, sol->base_state, &base_layer);

        printf("Saving...\n");  // TODO: Prevent data corruption by catching Ctrl+C.
        FILE *f = fopen(filename, "wb");
        save_solution(sol, f);
        fclose(f);
        print_light(negamax_node(sol, sol->base_state, base_key, base_layer, 0));
        // Verify solution integrity. For debugging only. Leaks memory.
        // char *buffer = file_to_buffer(filename);
        // buffer = load_solution(sol, buffer, 0);
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
        else {
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

    sprintf(temp_filename, "%s_temp.ldat", sol_name);


    state_info si_;
    state_info *si = &si_;
    init_state(base_state, si);

    num_layers = abs(base_state->ko_threats) + 1;

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
        goto iterate_chinese_liberty;
    }

    size_t k_size = 2 * key_size(sol->si);
    init_dict(sol->d, k_size);

    size_t total_legal = 0;
    for (size_t k = 0; k < k_size; k++) {
        if (!from_key_s(sol, s, k, 0)){
            continue;
        }
        total_legal++;
        int layer;
        size_t key = to_key_s(sol, s, &layer);
        assert(layer == 0);
        add_key(sol->d, key);
    }
    finalize_dict(sol->d);
    num_states = num_keys(sol->d);

    printf("Total positions %zu\n", total_legal);
    printf("Total unique positions %zu\n", num_states);

    light_value **base_nodes = (light_value**) malloc(sol->num_layers * sizeof(light_value*));
    for (size_t i = 0; i < sol->num_layers; i++) {
        base_nodes[i] = (light_value*) malloc(num_states * sizeof(light_value));
    }

    lin_dict ko_ld_ = (lin_dict) {0, 0, 0, NULL};
    lin_dict *ko_ld = &ko_ld_;

    sol->base_nodes = base_nodes;
    sol->ko_ld = ko_ld;

    size_t child_key;
    size_t key = sol->d->min_key;
    for (size_t i = 0; i < num_states; i++) {
        assert(from_key_s(sol, s, key, 0));
        for (size_t k = 0; k < sol->num_layers; k++) {
            (sol->base_nodes[k])[i] = (light_value) {VALUE_MIN, VALUE_MAX};
        }
        for (int j = 0; j < STATE_SIZE; j++) {
            *child = *s;
            int prisoners;
            if (make_move(child, 1ULL << j, &prisoners)) {
                if (target_dead(child)) {
                    continue;
                }
                if (child->ko) {
                    int child_layer;
                    child_key = to_key_s(sol, child, &child_layer);
                    add_lin_key(sol->ko_ld, child_key);
                }
            }
        }
        key = next_key(sol->d, key);
    }

    finalize_lin_dict(sol->ko_ld);

    light_value **ko_nodes = (light_value**) malloc(sol->num_layers * sizeof(light_value*));
    sol->ko_nodes = ko_nodes;
    for (size_t i = 0; i < sol->num_layers; i++) {
        sol->ko_nodes[i] = (light_value*) malloc(sol->ko_ld->num_keys * sizeof(light_value));
    }
    printf("Unique positions with ko %zu\n", sol->ko_ld->num_keys);

    for (size_t i = 0; i < sol->ko_ld->num_keys; i++) {
        for (size_t k = 0; k < sol->num_layers; k++) {
            sol->ko_nodes[k][i] = (light_value) {VALUE_MIN, VALUE_MAX};
        }
    }

    iterate_chinese_liberty:
    printf("Negamax with Chinese liberty rules.\n");
    iterate(sol, temp_filename);

    sprintf(filename, "%s_chinese.ldat", sol_name);
    FILE *f = fopen(filename, "wb");
    save_solution(sol, f);
    fclose(f);

    frontend:
    /*
    *s = *sol->base_state;

    char coord1;
    int coord2;

    int total_prisoners = 0;
    int turn = 0;

    while (1) {
        size_t layer;
        size_t key = to_key_s(sol, s, &layer);
        light_value v = negamax_node(sol, s, key, layer, 0);
        print_state(s);
        if (turn) {
            print_light((light_value) {total_prisoners - v.high, total_prisoners - v.low, v.high_distance, v.low_distance});
        }
        else {
            print_light((light_value) {total_prisoners + v.low, total_prisoners + v.high, v.low_distance, v.high_distance});
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
                light_value child_v = negamax_node(sol, child, child_key, child_layer, 0);
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
    */

    return 0;
}
