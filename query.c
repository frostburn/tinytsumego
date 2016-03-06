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

#define NUM_SOLUTIONS (2)

int main() {
    solution* sols[NUM_SOLUTIONS];
    sols[0] = mmap_solution("4x3_japanese.dat");
    sols[1] = mmap_solution("4x4_japanese.dat");
    // for (int i = 0; i < NUM_SOLUTIONS; i++) {
    //     repr_state(sols[i]->base_state);
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
        char* test = gets(in);
        if (test == NULL) {
            break;
        }
        sscanf_state(in, s);
        for (int i = 0; i < NUM_SOLUTIONS; i++) {
            if (
                (sols[i]->base_state->playing_area == s->playing_area) &&
                (sols[i]->base_state->target == s->target) &&
                (sols[i]->base_state->immortal == s->immortal)
            ) {
                size_t layer;
                size_t key = to_key_s(sols[i], s, &layer);
                node_value v = negamax_node(sols[i], s, key, layer, 0);
                printf("%d %d %d %d\n", v.low, v.high, v.low_distance, v.high_distance);

                int num_values = 0;
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
                printf("%d\n", num_values);
                for (int j = 0; j < num_values; j++) {
                    stones_t cm = child_moves[j];
                    node_value cv = child_values[j];
                    printf("%llu %d %d %d %d\n", cm, cv.low, cv.high, cv.low_distance, cv.high_distance);
                }
                break;
            }
        }
    }
    // TODO: Cleanup.
    return 0;
}
