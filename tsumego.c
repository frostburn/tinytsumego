state corner_six_ = (state) {
    rectangle(8, 8),
    rectangle(8, 8) ^ rectangle(4, 3),
    rectangle(4, 3) ^ rectangle(3, 2),
    0,
    rectangle(4, 3) ^ rectangle(3, 2),
    rectangle(8, 8) ^ rectangle(4, 3),
    0,
    0
};
state *corner_six = &corner_six_;
corner_six->ko = 0;  // shush my little compiler

state corner_six_1_ = corner_six_;
state *corner_six_1 = &corner_six_1_;
corner_six_1->player = corner_six_1->immortal ^= 1ULL << 4;

state corner_six_2_ = corner_six_1_;
state *corner_six_2 = &corner_six_2_;
corner_six_2->player = corner_six_2->immortal ^= 1ULL << (4 + V_SHIFT);

state corner_eight_ = (state) {
    rectangle(8, 8),
    rectangle(8, 8) ^ rectangle(5, 3),
    rectangle(5, 3) ^ rectangle(4, 2),
    0,
    rectangle(5, 3) ^ rectangle(4, 2),
    rectangle(8, 8) ^ rectangle(5, 3),
    0,
    0
};
state *corner_eight = &corner_eight_;
corner_eight->ko = 0;

state corner_eight_1_ = corner_eight_;
state *corner_eight_1 = &corner_eight_1_;
corner_eight_1->player = corner_eight_1->immortal ^= 1ULL << 5;

stones_t bulk = rectangle(3, 3) | (1ULL << 3);
state bulky_ten_ = (state) {
    rectangle(8, 8),
    rectangle(8, 8) ^ blob(bulk),
    blob(bulk) ^ bulk,
    0,
    blob(bulk) ^ bulk,
    rectangle(8, 8) ^ blob(bulk),
    0,
    0,
};
state *bulky_ten = &bulky_ten_;
bulky_ten->ko = 0;
