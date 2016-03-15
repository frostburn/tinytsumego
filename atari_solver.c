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
#include "utils.c"

// Where's the makefile? Oh, you gotta be kidding me.
// gcc -std=gnu99 -Wall -O3 atari_solver.c -o atari_solver; atari_solver 4 3

typedef unsigned char array_t;

void set_value(array_t *a, size_t index, array_t value) {
    array_t mask = 1 << (index % 8);
    index /= 8;
    a[index] &= ~mask;
    if (value) {
        a[index] |= mask;
    }
}

array_t get_value(array_t *a, size_t index) {
    array_t mask = 1 << (index % 8);
    index /= 8;
    return !!(a[index] & mask);
}

int main(int argc, char *argv[]) {
    int width = -1;
    int height = -1;
    int load_sol = 0;
    if (strcmp(argv[argc - 1], "load") == 0) {
        load_sol = 1;
        argc--;
    }
    if (argc < 2) {
        printf("Using predifined board\n");
    }
    else if (argc == 3) {
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

    state base_state_;
    state *base_state = &base_state_;

    char sol_name[64] = "default";
    char node_name[64];
    char gd_name[64];
    if (width > 0) {
        *base_state = (state) {rectangle(width, height), 0, 0, 0, 0};
        sprintf(sol_name, "%dx%d", width, height);
    }
    else {
        *base_state = (state) {cross(south(east(rectangle(2, 2)))), 0, 0, 0, 0};
        // *base_state = *corner_six_1;
        // *base_state = *bulky_ten;
        // *base_state = *cho3;
        // *base_state = *cho534;
    }
    sprintf(gd_name, "atari_gd_%s.dat", sol_name);
    sprintf(node_name, "atari_nodes_%s.dat", sol_name);

    print_state(base_state);

    state_info si_;
    state_info *si = &si_;
    init_state(base_state, si);

    g_dict gd_;
    g_dict *gd = &gd_;

    state s_;
    state *s = &s_;

    *s = *base_state;

    int is_member(size_t key) {
        state s_ = *base_state;
        state *s = &s_;
        return from_atari_key(s, si, key);
    }

    FILE *f;
    if (load_sol) {
        char *buffer = file_to_buffer(gd_name);
        buffer = load_g_dict(gd, buffer);
        gd->is_member = is_member;
    }
    else {
        init_g_dict(gd, is_member, atari_key_size(si), DEFAULT_G_CONSTANT);
        f = fopen(gd_name, "wb");
        save_g_dict(gd, f);
        fclose(f);
    }

    size_t num_states = gd->num_keys;

    printf("Total unique positions %zu\n", num_states);
    printf("Number of checkpoints %zu\n", gd->num_checkpoints);


    size_t node_array_size = ceil_div(num_states, 8);
    array_t *nodes = (array_t*) malloc(node_array_size);

    state child_;
    state *child = &child_;

    if (load_sol) {
        goto frontend;
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        size_t key = gd->min_key;
        for (size_t i = 0; i < num_states; i++) {
            assert(from_atari_key(s, si, key));
            array_t win = 0;
            for (int j = 1; j < si->num_moves; j++) {
                *child = *s;
                int prisoners;
                if (make_move(child, si->moves[j], &prisoners)) {
                    assert(!prisoners);
                    if (is_atari_preleaf(child)) {
                        continue;
                    }
                    size_t child_key = to_atari_key(child, si);
                    if (!get_value(nodes, g_key_index(gd, child_key))) {
                        win = 1;
                        break;
                    }
                }
            }
            if (get_value(nodes, i) != win) {
                changed = 1;
                set_value(nodes, i, win);
            }
            key = g_next_key(gd, key);
        }
        if (get_value(nodes, 0)) {
            printf("Wins\n");
        }
        else {
            printf("Loses\n");
        }
        // print_state(base_state);
        // for (int j = 1; j < si->num_moves; j++) {
        //     *child = *base_state;
        //     int prisoners;
        //     if (make_move(child, si->moves[j], &prisoners)) {
        //         size_t child_key = to_atari_key(child, si);
        //         printf("%d %d\n", j, get_value(nodes, g_key_index(gd, child_key)));
        //     }
        //     else {
        //         print_stones(si->moves[j]);
        //     }
        // }

        f = fopen(node_name, "wb");
        fwrite((void*) nodes, sizeof(array_t), node_array_size, f);
        fclose(f);
    }

    return 0;

    frontend:
    if (load_sol) {
        f = fopen(node_name, "rb");
        assert(fread((void*) nodes, sizeof(array_t), node_array_size, f));
        fclose(f);
    }

    *s = *base_state;

    char coord1;
    int coord2;
    int turn = 0;
    while (1) {
        size_t key = to_atari_key(s, si);
        printf("%zu\n", g_key_index(gd, key));
        printf("%d\n", get_value(nodes, g_key_index(gd, key)));
        print_state(s);
        if (is_atari_preleaf(s)) {
            break;
        }
        if (get_value(nodes, g_key_index(gd, key)) ^ turn) {
            printf("Black wins\n");
        }
        else {
            printf("White wins\n");
        }
        assert(is_atari_legal(s));
        for (int j = 0; j < STATE_SIZE; j++) {
            *child = *s;
            stones_t move = 1ULL << j;
            char c1 = 'A' + (j % WIDTH);
            char c2 = '0' + (j / WIDTH);
            int prisoners;
            if (make_move(child, move, &prisoners)) {
                array_t child_v;
                if (is_atari_preleaf(child)) {
                    child_v = 1;
                }
                else {
                    size_t child_key = to_atari_key(child, si);
                    child_v = get_value(nodes, g_key_index(gd, child_key));
                }
                printf("%c%c", c1, c2);
                if (!child_v) {
                    printf("-W");
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
            printf("Illegal move\n");
            return 1;
        }
        else {
            move = 1ULL << (coord1 + V_SHIFT * coord2);
        }
        int prisoners;
        if (make_move(s, move, &prisoners)) {
            turn = !turn;
        }
        else {
            printf("Illegal move\n");
            return 1;
        }
    }
}
