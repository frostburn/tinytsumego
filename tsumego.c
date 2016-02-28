state corner_six_ = (state) {
    rectangle(WIDTH, HEIGHT),
    rectangle(WIDTH, HEIGHT) ^ rectangle(4, 3),
    rectangle(4, 3) ^ rectangle(3, 2),
    0,
    rectangle(4, 3) ^ rectangle(3, 2),
    rectangle(WIDTH, HEIGHT) ^ rectangle(4, 3),
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
    rectangle(WIDTH, HEIGHT),
    rectangle(WIDTH, HEIGHT) ^ rectangle(5, 3),
    rectangle(5, 3) ^ rectangle(4, 2),
    0,
    rectangle(5, 3) ^ rectangle(4, 2),
    rectangle(WIDTH, HEIGHT) ^ rectangle(5, 3),
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
    rectangle(WIDTH, HEIGHT),
    rectangle(WIDTH, HEIGHT) ^ blob(bulk),
    blob(bulk) ^ bulk,
    0,
    blob(bulk) ^ bulk,
    rectangle(WIDTH, HEIGHT) ^ blob(bulk),
    0,
    0,
};
state *bulky_ten = &bulky_ten_;
bulky_ten->ko = 0;

state cho1_4_ = (state) {
    rectangle(9, 3),
    rectangle(9, 3) ^ rectangle(7, 2) ^ (3ULL << 7) ^ (1ULL << (2 * V_SHIFT)),
    south(east(rectangle(4, 1))),
    0,
    0,
    0,
    0,
    0
};
state *cho1_4 = &cho1_4_;
cho1_4->immortal = cho1_4->player;
cho1_4->target = cho1_4->opponent;

state cho1_ = cho1_4_;
state *cho1 = &cho1_;
cho1->opponent |= 1ULL << V_SHIFT;

state cho2_ = cho1_;
state *cho2 = &cho2_;
cho2->player |= 1ULL << 1;
cho2->opponent |= (1ULL << 7) | (3ULL << (5 + V_SHIFT));

state cho3_ = cho1_;
state *cho3 = &cho3_;
cho3->player |= 3UL << 5;
cho3->opponent |= 3UL << (5 + V_SHIFT);

state cho4_ = cho1_4_;
state *cho4 = &cho4_;
cho4->opponent |= 3UL << (5 + V_SHIFT);

state cho529_532_ = (state) {
    rectangle(6, 6),
    south(east(rectangle(5, 5))) & ~rectangle(4, 4),
    two(1, 3) | tvo(3, 1),
    0,
    0,
    0,
    0,
    0
};
state *cho529_532 = &cho529_532_;
cho529_532->immortal = cho529_532->player;
cho529_532->target = cho529_532->opponent;

state cho532_ = cho529_532_;
state *cho532 = &cho532_;
cho532->player |= one(1, 1);
cho532->opponent |= one(4, 0) | one(0, 1) | one(0, 4);

state cho534_ = (state) {
    rectangle(7, 5),
    south(south(south(rectangle(4, 1)))) | two(2, 0) | tvo(4, 1),
    rectangle(7, 5) ^ rectangle(5, 4) ^ one(4, 3) ^ two(5, 0),
    0,
    0,
    0,
    0,
    0,
};
state *cho534 = &cho534_;
cho534->target = cho534->player;
cho534->immortal = cho534->opponent;
cho534->opponent |= two(2, 1) | one(1, 2);

state cho535_537_ = (state) {
    rectangle(7, 6),
    south(east(rectangle(6, 5) ^ rectangle(3, 3))) | one(6, 0),
    tvo(3, 1) | tvo(4, 2) | one(5, 3) | two(1, 3) | one(2, 4),
};
state *cho535_537 = &cho535_537_;
cho535_537->player &= ~(two(4, 4) | one(6, 3) | cho535_537->opponent);
cho535_537->immortal = cho535_537->player;
cho535_537->target = cho535_537->opponent;

state cho535_ = cho535_537_;
state *cho535 = &cho535_;
cho535->player |= one(2, 1);
cho535->opponent |= one(1, 0) | one(0, 4);

state cho538_540_ = (state) {
    rectangle(6, 6),
    rectangle(6, 6) ^ rectangle(4, 4) ^ one(3, 3) ^ two(4, 0) ^ tvo(0, 4),
    tvo(3, 1) | two(1, 3),
    0,
    0,
    0,
    0,
    0,
};
state *cho538_540 = &cho538_540_;
cho538_540->immortal = cho538_540->player;
cho538_540->target = cho538_540->opponent;

state cho552_ = (state) {
    rectangle(5, 5),
    rectangle(4, 4) ^ rectangle(3, 3) ^ one(3, 3),
    rectangle(5, 5) ^ rectangle(4, 4),
    0,
    0,
    0,
    0,
    0,
};
state *cho552 = &cho552_;
cho552->target = cho552->player;
cho552->immortal = cho552->opponent;
cho552->opponent |= one(1, 1);

state cho553_ = (state) {
    rectangle(5, 6),
    east(rectangle(4, 4) ^ rectangle(3, 3)) | one(3, 0) | one(3, 2),
    (rectangle(5, 6) ^ rectangle(5, 5)) | two(0, 4) | one(0, 3),
    0,
    0,
    0,
    0,
    0,
};
state *cho553 = &cho553_;
cho553->target = cho553->player;
cho553->immortal = cho553->opponent;

cho553->opponent |= liberties(one(2, 1), cho553->playing_area);

state cho427_ = (state) {
    rectangle(8, 4),
    south(south(east(rectangle(3, 1)))),
    south(south(east(rectangle(7, 2) ^ rectangle(3, 1) ^ one(4, 0)))),
    0,
    0,
    0,
    0,
    0,
};
state *cho427 = &cho427_;
cho427->target = cho427->player;
cho427->immortal = cho427->opponent;

cho427->player |= tvo(7, 0);
cho427->immortal |= tvo(7, 0);
