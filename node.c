#define VALUE_MIN (-127)
#define VALUE_MAX (127)
#define DISTANCE_MAX (254)
#define HEURISTIC_MIN (-1e12)
#define HEURISTIC_MAX (1e12)

typedef signed char value_t;
typedef unsigned char distance_t;

typedef struct node_value
{
    value_t low;
    value_t high;
    distance_t low_distance;
    distance_t high_distance;
} node_value;

typedef struct light_value
{
    value_t low;
    value_t high;
} light_value;

typedef struct heuristic_value
{
    float low;
    float high;
} heuristic_value;

// This is for the binary interface (no padding)
#define SIZE_OF_NODE_VALUE (2 * sizeof(value_t) + 2 * sizeof(distance_t))

void print_node(node_value nv) {
    printf("node_value(%d, %d, %d, %d)\n", nv.low, nv.high, nv.low_distance, nv.high_distance);
}

void print_light(const light_value lv) {
    printf("light_value(%d, %d)\n", lv.low, lv.high);
}

void print_heuristic(const heuristic_value lv) {
    printf("heuristic_value(%g, %g)\n", lv.low, lv.high);
}

node_value negamax(node_value parent, node_value child) {
    /*
    assert(parent.low <= parent.high);
    assert(child.low <= child.high);
    assert(parent.low != VALUE_MAX);
    assert(parent.high != VALUE_MIN);
    assert(child.low != VALUE_MAX);
    assert(child.high != VALUE_MIN);
    print_node(parent);
    print_node(child);
    */
    if (-child.high > parent.low) {
        parent.low = -child.high;
        parent.low_distance = child.high_distance + 1;
    }
    else if (-child.high == parent.low && child.high_distance + 1 < parent.low_distance) {
        parent.low_distance = child.high_distance + 1;
    }
    if (-child.low > parent.high) {
        parent.high = -child.low;
        parent.high_distance = child.low_distance + 1;
    }
    else if (-child.low == parent.high && child.low_distance + 1 > parent.high_distance) {
        parent.high_distance = child.low_distance + 1;
    }
    /*
    if (parent.low < VALUE_MIN) {
        parent.low = VALUE_MIN;
    }
    if (parent.high > VALUE_MAX) {
        parent.high = VALUE_MAX;
    }
    */
    if (parent.low_distance > DISTANCE_MAX) {
        parent.low_distance = DISTANCE_MAX;
    }
    if (parent.high_distance > DISTANCE_MAX) {
        parent.high_distance = DISTANCE_MAX;
    }
    assert(parent.low <= parent.high);
    // if (parent.low > VALUE_MIN){
    //     assert(parent.low_distance < DISTANCE_MAX);
    // }
    return parent;
}

light_value negamax_light(light_value parent, const light_value child) {
    if (-child.high > parent.low) {
        parent.low = -child.high;
    }
    if (-child.low > parent.high) {
        parent.high = -child.low;
    }
    assert(parent.low <= parent.high);
    return parent;
}

heuristic_value negamax_heuristic(heuristic_value parent, const heuristic_value child) {
    if (-child.high > parent.low) {
        parent.low = -child.high;
    }
    if (-child.low > parent.high) {
        parent.high = -child.low;
    }
    assert(parent.low <= parent.high);
    return parent;
}

int equal(node_value a, node_value b) {
    if (a.low != b.low) {
        return 0;
    }
    if (a.high != b.high) {
        return 0;
    }
    if (a.low_distance != b.low_distance) {
        return 0;
    }
    return a.high_distance == b.high_distance;
}

int equal_light(const light_value a, const light_value b) {
    return a.low == b.low && a.high == b.high;
}

int equal_heuristic(const heuristic_value a, const heuristic_value b) {
    return a.low == b.low && a.high == b.high;
}
