#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAIN
#include "state.c"
#include "dict.c"
#include "node.c"
#include "solver_common.c"

char* file_to_mmap(char *filename) {
    struct stat sb;
    stat(filename, &sb);
    int fd = open(filename, O_RDONLY);
    assert(fd != -1);
    char *map;
    map = (char*) mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    madvise(map, sb.st_size, MADV_RANDOM);
    return map;
}

solution* mmap_solution(char *filename) {
    solution *sol = (solution*) malloc(sizeof(solution));
    char *buffer = file_to_mmap(filename);
    buffer = load_solution(sol, buffer, 1);
    return sol;
}

int main(int argc, char *argv[]) {
    int num_solutions = argc - 1;
    solution **sols = (solution**) malloc(num_solutions * sizeof(solution*));
    for (int i = 0; i < num_solutions; i++) {
        sols[i] = mmap_solution(argv[i + 1]);
        repr_state(sols[i]->base_state);
        printf("%zu\n", sols[i]->num_layers);
    }
    // sols[0] = mmap_solution("center_4x3_japanese.dat");
    // sols[1] = mmap_solution("corner_4x3_japanese.dat");
    // sols[2] = mmap_solution("3x3_japanese.dat");
    // sols[3] = mmap_solution("4x3_japanese.dat");
    // if () {
    //     for (int i = 0; i < NUM_SOLUTIONS; i++) {
    //         print_state(sols[i]->base_state);
    //         repr_state(sols[i]->base_state);
    //     }
    //     return 0;
    // }
    state s_;
    state *s = &s_;
    state child_;
    state *child = &child_;
    stones_t child_moves[STATE_SIZE + 1];
    node_value child_values[STATE_SIZE + 1];
    char in[512];

    printf("Solutions loaded.\n");

    while (1) {
        char* test = fgets(in, 512, stdin);
        if (test == NULL) {
            break;
        }
        sscanf_state(in, s);
        int solution_found = 0;
        for (int i = 0; i < num_solutions; i++) {
            // TODO: Check color of target and immortal too.
            if (
                (sols[i]->base_state->playing_area == s->playing_area) &&
                (sols[i]->base_state->target == s->target) &&
                (sols[i]->base_state->immortal == s->immortal)
            ) {
                size_t layer;
                size_t key = to_key_s(sols[i], s, &layer);
                int passes_valid = s->passes <= 2;
                int layer_valid = (layer >= 0 && layer < sols[i]->num_layers);
                int key_valid;
                if (s->ko) {
                    key_valid = test_lin_key(sols[i]->ko_ld, key);
                }
                else {
                    key_valid = test_key(sols[i]->d, key);
                }
                if (!layer_valid || !key_valid || !passes_valid) {
                    printf("1 -1 %d %d\n", layer_valid, key_valid);
                    printf("0\n");
                    solution_found = 1;
                    break;
                }
                node_value v = negamax_node(sols[i], s, key, layer, 0);
                printf("%d %d %d %d\n", v.low, v.high, v.low_distance, v.high_distance);

                int num_values = 0;
                if (s->passes < 2) {
                    for (int j = 0; j < sols[i]->si->num_moves; j++) {
                        *child = *s;
                        int prisoners;
                        stones_t move = sols[i]->si->moves[j];
                        if (make_move(child, move, &prisoners)) {
                            size_t child_layer;
                            size_t child_key = to_key_s(sols[i], child, &child_layer);
                            node_value child_v = negamax_node(sols[i], child, child_key, child_layer, 0);
                            child_moves[num_values] = move;
                            child_values[num_values++] = child_v;
                        }
                    }
                }
                printf("%d\n", num_values);
                for (int j = 0; j < num_values; j++) {
                    stones_t cm = child_moves[j];
                    node_value cv = child_values[j];
                    printf("%llu %d %d %d %d\n", cm, cv.low, cv.high, cv.low_distance, cv.high_distance);
                }
                solution_found = 1;
                break;
            }
        }
        if (!solution_found) {
            printf("1 -1 1 1\n");
            printf("0\n");
        }
    }
    // TODO: Cleanup.
    return 0;
}
