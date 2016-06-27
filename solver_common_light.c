#define TARGET_SCORE (126)

typedef struct solution {
    state *base_state;
    state_info *si;
    dict *d;
    lin_dict *ko_ld;
    int num_layers;
    light_value **base_nodes;
    light_value **ko_nodes;
} solution;

void save_solution(solution *sol, FILE *f) {
    fwrite((void*) sol, sizeof(solution), 1, f);
    fwrite((void*) sol->base_state, sizeof(state), 1, f);
    save_dict(sol->d, f);
    save_lin_dict(sol->ko_ld, f);
    size_t num_states = num_keys(sol->d);
    for (int i = 0;i < sol->num_layers; i++) {
        fwrite((void*) sol->base_nodes[i], sizeof(light_value), num_states, f);
        fwrite((void*) sol->ko_nodes[i], sizeof(light_value), sol->ko_ld->num_keys, f);
    }
}

char* load_solution(solution *sol, char *buffer, int do_alloc) {
    *sol = *((solution*) buffer);
    buffer += sizeof(solution);
    if (do_alloc) {
        sol->base_state = malloc(sizeof(state));
        sol->si = malloc(sizeof(state_info));
        sol->d = (dict*) malloc(sizeof(dict));
        sol->ko_ld = (lin_dict*) malloc(sizeof(lin_dict));
        sol->base_nodes = (light_value**) malloc(sizeof(light_value*) * sol->num_layers);
        sol->ko_nodes = (light_value**) malloc(sizeof(light_value*) * sol->num_layers);
    }
    *(sol->base_state) = *((state*) buffer);
    buffer += sizeof(state);
    init_state(sol->base_state, sol->si);
    buffer = load_dict(sol->d, buffer);
    buffer = load_lin_dict(sol->ko_ld, buffer);
    size_t num_states = num_keys(sol->d);
    for (int i = 0;i < sol->num_layers; i++) {
        sol->base_nodes[i] = (light_value*) buffer;
        buffer += num_states * sizeof(light_value);
        sol->ko_nodes[i] = (light_value*) buffer;
        buffer += sol->ko_ld->num_keys * sizeof(light_value);
    }
    return buffer;
}

int from_key_s(solution *sol, state *s, size_t key, int layer) {
    *s = *(sol->base_state);
    stones_t fixed = sol->base_state->target | sol->base_state->immortal;
    s->player &= fixed;
    s->opponent &= fixed;
    s->ko_threats = layer - (sol->num_layers / 2);

    if (key % 2) {
        stones_t temp = s->player;
        s->player = s->opponent;
        s->opponent = temp;
        s->ko_threats = -s->ko_threats;
        s->white_to_play = !s->white_to_play;
    }
    key /= 2;

    return from_key(s, sol->si, key);
}

int from_key_ko(solution *sol, state *s, size_t key, int layer) {
    size_t ko_index = key % sol->si->size;
    key /= sol->si->size;
    int legal = from_key_s(sol, s, key, layer);
    s->ko = sol->si->moves[ko_index + 1];
    if (!legal || s->ko & (s->player | s->opponent)) {
        return 0;
    }
    return 1;
}

size_t to_key_s(solution *sol, state *s, int *layer) {
    state c_ = *s;
    state *c = &c_;
    canonize(c, sol->si);
    size_t key = to_key(c, sol->si);
    key *= 2;

    /*
    state *b = sol->base_state;
    stones_t b_pt = b->target & b->player;
    stones_t b_ot = b->target & b->opponent;
    stones_t b_pi = b->immortal & b->player;
    stones_t b_oi = b->immortal & b->opponent;

    stones_t c_pt = c->target & c->player;
    stones_t c_ot = c->target & c->opponent;
    stones_t c_pi = c->immortal & c->player;
    stones_t c_oi = c->immortal & c->opponent;

    if (!(b_pt == c_pt && b_ot == c_ot && b_pi == c_pi && b_oi == c_oi)) {
        key += 1;
    }
    */
    if (c->white_to_play != sol->base_state->white_to_play) {
        key += 1;
    }

    *layer = c->ko_threats + sol->base_state->ko_threats;
    if (c->ko) {
        int i;
        for (i = 0; i < sol->si->size; i++) {
            if (c->ko == sol->si->moves[i + 1]) {
                break;
            }
        }
        key = sol->si->size * key + i;
    }
    return key;
}

light_value negamax_node(solution *sol, state *s, size_t key, int layer, int depth) {
    if (target_dead(s)) {
        value_t score = -TARGET_SCORE;
        return (light_value) {score, score};
    }
    else if (s->passes == 2) {
        value_t score = chinese_liberty_score(s);
        return (light_value) {score, score};
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
    light_value v = (light_value) {VALUE_MIN, VALUE_MIN};
    for (int j = 0; j < sol->si->num_moves; j++) {
        *child = *s;
        stones_t move = sol->si->moves[j];
        int prisoners;
        if (make_move(child, move, &prisoners)) {
            int child_layer;
            size_t child_key = to_key_s(sol, child, &child_layer);
            light_value child_v = negamax_node(sol, child, child_key, child_layer, depth - 1);
            v = negamax_light(v, child_v);
        }
    }
    return v;
}
