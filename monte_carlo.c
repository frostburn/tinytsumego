#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <omp.h>

#ifndef MAIN
    #define MAIN
    #include "state.c"
    #include "dict.c"
    #include "node.c"
    #include "solver_common_light.c"
    #include "utils.c"
    #include "jkiss.c"
    #undef MAIN
#endif

#define NUM_TRIES (12)
#define NUM_MAX_KILLS (12)

typedef size_t bin_t;
typedef unsigned char num_t;

typedef struct score_bins {
    bin_t *bins;
    num_t shift;
} score_bins;

score_bins score_bins_new(const state s)
{
    score_bins sb;
    int shift = popcount(s.playing_area);
    sb.shift = shift;
    sb.bins = calloc(2 * shift + 1, sizeof(bin_t));
    return sb;
}

void score_bins_destroy(score_bins sb)
{
    free(sb.bins);
}

void score_bins_reset(score_bins sb)
{
    for (int i = 0; i < 2 * sb.shift + 1; i++) {
        sb.bins[i] = 0;
    }
}

void score_bins_add(score_bins sb, int score)
{
    sb.bins[score + sb.shift]++;
}

void score_bins_merge(score_bins a, score_bins b)
{
    for (int i = 0; i < 2 * a.shift + 1; i++) {
        a.bins[i] += b.bins[i];
    }
}

bin_t score_bins_total(score_bins sb)
{
    int num_bins = 2 * sb.shift + 1;
    bin_t total = 0;
    for (bin_t i = 0; i < num_bins; i++) {
        total += sb.bins[i];
    }
    return total;
}

double score_bins_average(score_bins sb)
{
    int num_bins = 2 * sb.shift + 1;
    bin_t total = 0;
    bin_t expectation = 0;
    for (bin_t i = 0; i < num_bins; i++) {
        bin_t bin = sb.bins[i];
        total += bin;
        expectation += i * bin;
    }
    return expectation / (double) total - sb.shift;
}

void print_score_bins(const score_bins sb, int resolution)
{
    int num_bins = 2 * sb.shift + 1;
    bin_t total = 0;
    bin_t expectation = 0;
    bin_t max = 0;
    for (bin_t i = 0; i < num_bins; i++) {
        bin_t bin = sb.bins[i];
        total += bin;
        expectation += i * bin;
        if (bin > max) {
            max = bin;
        }
    }
    for (int i = 0; i < num_bins; i++) {
        for (size_t j = 0; j * max < resolution * sb.bins[i]; j++) {
            printf("#");
        }
        printf("\n");
    }
    printf("Average %g\n", expectation / (double) total - sb.shift);
}

int uniform_move(state *s, const state_info si, jkiss *jk)
{
    int index;
    stones_t move;
    int prisoners;
    for (int i = 0; i < NUM_TRIES; i++) {
        index = jkiss_step(jk) % si.num_moves;
        move = si.moves[index];
        if (make_move(s, move, &prisoners)) {
            return prisoners;
        }
    }
    return -1;
}

int playout(state *s, const state_info si, jkiss *jk)
{
    int num_empty = popcount(s->playing_area ^ s->player ^ s->opponent);
    for (int i = 0; i < num_empty; i++) {
        if (uniform_move(s, si, jk) < 0) {
            return 0;
        }
    }
    return 1;
}

// Ignores target and immortal
int kill_group(state *s)
{
    stones_t chain, p;
    stones_t opponent = s->opponent;
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j += 2) {
            p = (1ULL | (1ULL << V_SHIFT)) << (i + j * V_SHIFT);
            chain = flood(p, opponent);
            opponent ^= chain;
            stones_t libs = liberties(chain, s->playing_area & ~s->player);
            if (popcount(libs) == 1) {
                int prisoners;
                make_move(s, libs, &prisoners);
                return 1;
            }
        }
    }
    return 0;
}

void kill_groups(state *s)
{
    int prisoners;
    int misses = 0;
    int i = 0;
    while (misses < 2 && i++ < NUM_MAX_KILLS) {
        if (kill_group(s)) {
            misses = 0;
        }
        else {
            make_move(s, 0, &prisoners);
            misses++;
        }
    }
}

int clean_playout(state s, const state_info si, jkiss *jk)
{
    playout(&s, si, jk);
    kill_groups(&s);
    int score = chinese_liberty_score(&s);
    if (s.white_to_play) {
        return -score;
    }
    return score;
}

void monte_carlo(state s, const state_info si, score_bins sb, size_t num_iter)
{
    int num_threads = omp_get_max_threads();
    jkiss *jks = malloc(num_threads * sizeof(jkiss));
    score_bins *sbs = malloc(num_threads * sizeof(score_bins));
    for (int i = 0; i < num_threads; i++) {
        sbs[i] = score_bins_new(s);
        jkiss_init(jks + i);
    }
    size_t i;
    int tid;
    int finished = 0;
    #pragma omp parallel private(i, tid)
    {
        tid = omp_get_thread_num();
        for (i = 0; i < num_iter; i++) {
            score_bins_add(sbs[tid], clean_playout(s, si, jks + tid));
            // Bailout once one thread finishes.
            if (finished) {
                break;
            }
        }
        finished = 1;
    }

    for (i = 0; i < num_threads; i++) {
        score_bins_merge(sb, sbs[i]);
        score_bins_destroy(sbs[i]);
    }
}

#ifndef MAIN
int main()
{
    size_t i;
    size_t num_iter_single = 1000000;
    size_t num_iter_parallel = 10000000;

    state base_state = (state) {rectangle(7, 7), 0, 0, 0, 0, 0, 0};
    int asdf;
    make_move(&base_state, one(3, 3), &asdf);
    state_info si;
    init_state(&base_state, &si);

    state s = base_state;
    jkiss jk;
    jkiss_init(&jk);
    printf("%d\n", playout(&s, si, &jk));
    print_state(&s);
    kill_groups(&s);
    print_state(&s);
    printf("%d\n", chinese_liberty_score(&s));

    score_bins sb = score_bins_new(base_state);

    for (i = 0; i < num_iter_single; i++) {
        score_bins_add(sb, clean_playout(base_state, si, &jk));
    }
    print_score_bins(sb, 50);
    printf("-----------------------------\n");

    score_bins_reset(sb);
    monte_carlo(base_state, si, sb, num_iter_parallel);
    print_score_bins(sb, 50);

    /*
    int num_threads = omp_get_max_threads();
    jkiss *jks = malloc(num_threads * sizeof(jkiss));
    bin_t **binss = malloc(num_threads * sizeof(bin_t*));
    for (int i = 0; i < num_threads; i++) {
        binss[i] = calloc(num_bins, sizeof(bin_t));
        jkiss_init(jks + i);
    }
    int tid;
    int finished = 0;
    #pragma omp parallel private(i, s, tid)
    {
        tid = omp_get_thread_num();
        for (i = 0; i < num_iter_parallel; i++) {
            s = base_state;
            playout(&s, si, jks + tid);
            kill_groups(&s);
            int score = chinese_liberty_score(&s);
            if (s.white_to_play) {
                score = -score;
            }
            binss[tid][si.size + score]++;
            // Bailout once one thread finishes.
            if (finished) {
                break;
            }
        }
        finished = 1;
    }

    for (i = 0; i < num_bins; i++) {
        bins[i] = 0;
    }
    for (i = 0; i < num_threads; i++) {
        for (int j = 0; j < num_bins; j++) {
            bins[j] += binss[i][j];
        }
    }
    print_bins(bins, num_bins, 128);
    */

    return 0;
}
#endif
