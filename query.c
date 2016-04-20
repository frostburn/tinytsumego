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
#include "connect.c"

#define SOLUTION_FOUND (0)
#define ERROR_BAD_STATE (-101)
#define ERROR_SOLUTION_NOT_FOUND (-102)
#define ERROR_INVALID_ACTION (-103)

#define ACTION_LIST_SOLUTIONS (1)
#define ACTION_QUERY_STATE (2)

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

// Unfortunately the solver takes a shortcut and uses color information to figure out parity.
// We don't want that here.

int has_substate(solution *sol, state *s, int *colors_swapped) {
    state *b = sol->base_state;
    if (b->playing_area != s->playing_area) {
        return 0;
    }
    if (b->target != s->target) {
        return 0;
    }
    if (b->immortal != s->immortal) {
        return 0;
    }
    stones_t b_pt = b->target & b->player;
    stones_t b_ot = b->target & b->opponent;
    stones_t b_pi = b->immortal & b->player;
    stones_t b_oi = b->immortal & b->opponent;

    stones_t s_pt = s->target & s->player;
    stones_t s_ot = s->target & s->opponent;
    stones_t s_pi = s->immortal & s->player;
    stones_t s_oi = s->immortal & s->opponent;

    if (b_pt == s_pt && b_ot == s_ot && b_pi == s_pi && b_oi == s_oi) {
        *colors_swapped = b->white_to_play != s->white_to_play;
        return 1;
    }
    if (b_pt == s_ot && b_ot == s_pt && b_pi == s_oi && b_oi == s_pi) {
        *colors_swapped = b->white_to_play == s->white_to_play;
        return 1;
    }
    return 0;
}

static solution **sols;
static int num_solutions;
static char** solution_names;

#define WRITE(client, buf, count) if (write(client, buf, count) != count) {return WRITE_ERROR;}

int query_state(const int client, state *s) {
    state child_;
    state *child = &child_;
    stones_t child_moves[STATE_SIZE + 1];
    node_value child_values[STATE_SIZE + 1];

    int solution_found = 0;
    int return_code;
    for (int i = 0; i < num_solutions; i++) {
        int colors_swapped;
        if (has_substate(sols[i], s, &colors_swapped)) {
            if (colors_swapped) {
                s->white_to_play = !s->white_to_play;
            }
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
                return_code = ERROR_BAD_STATE;
                WRITE(client, &return_code, sizeof(int));
                WRITE(client, &layer_valid, sizeof(int));
                WRITE(client, &key_valid, sizeof(int));
                return return_code;
            }
            node_value v = negamax_node(sols[i], s, key, layer, 0);
            return_code = SOLUTION_FOUND;
            WRITE(client, &return_code, sizeof(int));
            WRITE(client, &v, SIZE_OF_NODE_VALUE);

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
            WRITE(client, &num_values, sizeof(int));
            for (int j = 0; j < num_values; j++) {
                stones_t cm = child_moves[j];
                node_value cv = child_values[j];
                WRITE(client, &cm, sizeof(stones_t));
                WRITE(client, &cv, SIZE_OF_NODE_VALUE);
            }
            solution_found = 1;
            break;
        }
    }
    if (!solution_found) {
        return_code = ERROR_SOLUTION_NOT_FOUND;
        WRITE(client, &return_code, sizeof(int));
        return return_code;
    }
    return 0;
}

int serve_client(const int client) {
    state s_;
    state *s = &s_;
    int action = -1;
    if (read(client, (void*) &action, sizeof(int)) != sizeof(int)) {
        return READ_ERROR;
    }
    if (action == ACTION_LIST_SOLUTIONS) {
        WRITE(client, &num_solutions, sizeof(int));
        for (int i = 0; i < num_solutions; i++) {
            WRITE(client, sols[i]->base_state, SIZE_OF_STATE);
            int num_layers = (int)(sols[i]->num_layers);
            WRITE(client, &num_layers, sizeof(int));
            int name_len = strlen(solution_names[i]);
            WRITE(client, &name_len, sizeof(int));
            WRITE(client,solution_names[i], name_len * sizeof(char));
        }
        return SOLUTION_FOUND;
    }
    else if (action == ACTION_QUERY_STATE) {
        if (read(client, (void*) s, SIZE_OF_STATE) != SIZE_OF_STATE) {
            return READ_ERROR;
        }
        #ifdef DEBUG
            print_state(s);
            repr_state(s);
        #endif
        return query_state(client, s);
    }
    return ERROR_INVALID_ACTION;
}

int main(int argc, char *argv[]) {
    num_solutions = argc - 2;
    solution_names = argv + 2;
    sols = (solution**) malloc(num_solutions * sizeof(solution*));
    for (int i = 0; i < num_solutions; i++) {
        sols[i] = mmap_solution(solution_names[i]);
        repr_state(sols[i]->base_state);
        printf("%zu\n", sols[i]->num_layers);
    }

    printf("Solutions loaded.\n");

    serve(argv[1]);
    // TODO: Cleanup.
    return 0;
}
