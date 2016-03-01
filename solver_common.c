enum rule {
    precalculated,
    chinese_liberty,
    japanese_double_liberty,
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

#define TARGET_SCORE (63)
#define PRISONER_VALUE (1)

int sign(int x) {
    return (x > 0) - (x < 0);
}


// TODO *sol
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
        else if (sol->leaf_rule == chinese_liberty) {
            score = chinese_liberty_score(s);
        }
        else if (sol->leaf_rule == japanese_double_liberty) {
            score = 2 * japanese_liberty_score(s);
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
                // TODO: assert no overflows.
                if (child_v.low > VALUE_MIN && child_v.low < VALUE_MAX) {
                    child_v.low = child_v.low - prisoners;
                }
                if (child_v.high > VALUE_MIN && child_v.high < VALUE_MAX) {
                    child_v.high = child_v.high - prisoners;
                }
            }
            v = negamax(v, child_v);
        }
    }
    return v;
}
