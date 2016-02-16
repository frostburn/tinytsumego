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
