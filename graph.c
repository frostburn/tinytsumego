#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAIN
#include "state.c"
#include "dict.c"
#include "node.c"
#include "solver_common_light.c"
#include "utils.c"
#include "jkiss.c"
#include "monte_carlo.c"

typedef struct graph_node {
    state s;
    num_t num_children;
    size_t *children;
    heuristic_value v;
    score_bins bins;
} graph_node;

typedef struct graph
{
    state base_state;
    state_info si;
    size_t num_nodes;
    graph_node *nodes;
} graph;

int state_equal(const state a, const state b)
{
    return (
        (a.player == b.player) &&
        (a.opponent == b.opponent) &&
        (a.ko == b.ko) &&
        (a.passes == b.passes)
    );
}

void graph_node_save(const graph_node n, FILE *f)
{
    fwrite((void*) &n, sizeof(graph_node), 1, f);
    fwrite((void*) n.children, sizeof(size_t), n.num_children, f);
    int num_bins = 2 * n.bins.shift + 1;
    fwrite((void*) n.bins.bins, sizeof(bin_t), num_bins, f);
}

char* graph_node_load(graph_node *n, char *buffer) {
    *n = *((graph_node*) buffer);
    buffer += sizeof(graph_node);
    n->children = malloc(n->num_children * sizeof(size_t));
    for (int i = 0; i < n->num_children; i++) {
        n->children[i] = *((size_t*) buffer);
        buffer += sizeof(size_t);
    }
    int num_bins = 2 * n->bins.shift + 1;
    n->bins.bins = malloc(num_bins * sizeof(bin_t));
    for (int i = 0; i < num_bins; i++) {
        n->bins.bins[i] = *((bin_t*) buffer);
        buffer += sizeof(bin_t);
    }
    return buffer;
}

size_t graph_get_or_create(graph *g, state s)
{
    canonize(&s, &g->si);
    for (size_t i = 0; i < g->num_nodes; i++) {
        if (state_equal(g->nodes[i].s, s)) {
            return i;
        }
    }
    size_t index = g->num_nodes;
    g->num_nodes++;
    g->nodes = realloc(g->nodes, g->num_nodes * sizeof(graph_node));
    score_bins bins = score_bins_new(s);
    g->nodes[index] = (graph_node) {s, 0, NULL, {HEURISTIC_MIN, HEURISTIC_MAX}, bins};
    return index;
}

int add_child(graph *g, size_t parent_index, size_t child_index)
{
    graph_node *n = g->nodes + parent_index;
    for (int i = 0; i < n->num_children; i++) {
        if (n->children[i] == child_index) {
            return 0;
        }
    }
    num_t i = n->num_children;
    n->num_children++;
    n->children = realloc(n->children, n->num_children * sizeof(size_t));
    n->children[i] = child_index;
    return 1;
}

int graph_expand(graph *g, size_t index, int depth)
{
    if (depth == 0) {
        return 0;
    }
    for (int j = 0; j < g->si.num_moves; j++) {
        state child = g->nodes[index].s;
        stones_t move = g->si.moves[j];
        int prisoners;
        if (make_move(&child, move, &prisoners)) {
            size_t child_index = graph_get_or_create(g, child);
            add_child(g, index, child_index);
        }
    }
    for (int i = 0; i < g->nodes[index].num_children; i++) {
        graph_expand(g, g->nodes[index].children[i], depth - 1);
    }
    return 1;
}

void graph_heuristic_reset(graph g)
{
    for (size_t i = 0; i < g.num_nodes; i++) {
        g.nodes[i].v = (heuristic_value) {HEURISTIC_MIN, HEURISTIC_MAX};
    }
}

int graph_heuristic_iterate(graph g)
{
    int changed = 0;
    for (size_t i = 0; i < g.num_nodes; i++) {
        heuristic_value v = (heuristic_value) {HEURISTIC_MIN, HEURISTIC_MIN};
        if (!g.nodes[i].num_children) {
            float avg = score_bins_average(g.nodes[i].bins);
            v = (heuristic_value) {avg, avg};
        }
        else {
            for (int j = 0; j < g.nodes[i].num_children; j++) {
                v = negamax_heuristic(v, g.nodes[g.nodes[i].children[j]].v);
            }
        }
        changed = changed || !equal_heuristic(g.nodes[i].v, v);
        g.nodes[i].v = v;
    }
    return changed;
}

void graph_negamax(graph g)
{
    graph_heuristic_reset(g);
    while (graph_heuristic_iterate(g));
}

size_t graph_best_child(const graph g, const size_t index)
{
    graph_node parent = g.nodes[index];
    for (int i = 0; i < parent.num_children; i++) {
        graph_node child = g.nodes[parent.children[i]];
        if (parent.v.low == -child.v.high) {
            return parent.children[i];
        }
    }
    return ~0;
}

void graph_save(const graph g, FILE *f)
{
    fwrite((void*) &g, sizeof(graph), 1, f);
    for (size_t i = 0; i < g.num_nodes; i++) {
        graph_node_save(g.nodes[i], f);
    }
}

char* graph_load(graph *g, char *buffer) {
    *g = *((graph*) buffer);
    buffer += sizeof(graph);
    g->nodes = malloc(g->num_nodes * sizeof(graph_node));
    for (size_t i = 0; i < g->num_nodes; i++) {
        buffer = graph_node_load(g->nodes + i, buffer);
    }
    return buffer;
}

int main()
{
    graph g;
    state base_state = (state) {rectangle(7, 7), 0, 0, 0, 0, 0, 0};
    state_info si;
    init_state(&base_state, &si);
    g.base_state = base_state;
    g.si = si;
    g.num_nodes = 0;
    g.nodes = NULL;
    graph_get_or_create(&g, base_state);
    graph_expand(&g, 0, 2);

    printf("Loading...\n");
    char *buffer = file_to_buffer("graph.dump");
    graph_load(&g, buffer);

    while (1) {
        printf("Number of nodes %zu\n", g.num_nodes);
        for (size_t i = 0; i < g.num_nodes; i++) {
            monte_carlo(g.nodes[i].s, g.si, g.nodes[i].bins, 1000);
            // print_state(&g.nodes[i].s);
            // printf("Average %g (%zu)\n", score_bins_average(g.nodes[i].bins), score_bins_total(g.nodes[i].bins));
        }
        graph_negamax(g);

        size_t index = 0;
        size_t last_index = 0;
        int parity = 0;
        printf("Path");
        while (index != ~0) {
            printf(" %zu", index);
            last_index = index;
            index = graph_best_child(g, index);
            parity++;
        }
        printf("\n");
        state s = g.nodes[last_index].s;
        if (parity % 2 == 0) {
            s.white_to_play = 1;
        }
        print_state(&s);

        printf("Saving...\n");
        FILE *f = fopen("graph.dump", "wb");
        graph_save(g, f);
        fclose(f);
        printf("Root value: "); print_heuristic(g.nodes[0].v);

        graph_expand(&g, last_index, 1);
    }
    printf("no hei!\n");
}
