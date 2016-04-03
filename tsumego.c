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

state cho5_;
state *cho5 = &cho5_;
sscanf_state("9223372036854775807 2185562305871872 206427915264 0 0 0 0 0 0", cho5);
cho5->opponent ^= one(2, 2);
cho5->playing_area = rectangle(6, 6);
cho5->target = cho5->opponent;
cho5->immortal = cho5->player;

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

state cho538_;
state *cho538 = &cho538_;
sscanf_state("9223372036854775807 4268136333312 807407616 0 0 0 0 0 0", cho538);
cho538->playing_area = rectangle(6, 5);
cho538->target = cho538->opponent;
cho538->immortal = cho538->player;

state cho551_;
state *cho551 = &cho551_;
sscanf_state("9223372036854775807 2101256 1092574320271360 0 0 0 0 0 0", cho551);
cho551->playing_area = rectangle(5, 6);
cho551->target = cho551->player;
cho551->immortal = cho551->opponent;

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

state cho554_;
state *cho554 = &cho554_;
sscanf_state("9223372036854775807 4267058413616 1886396416 0 0 0 0 0 0", cho554);
cho554->playing_area = rectangle(6, 5);
cho554->target = cho554->opponent;
cho554->immortal = cho554->player;

state cho558_;
state *cho558 = &cho558_;
sscanf_state("9223372036854775807 1881149440 2185146228826112 0 0 0 0 0 0", cho558);
cho558->playing_area = rectangle(6, 6);
cho558->target = cho558->player;
cho558->immortal = cho558->opponent;

state cho571_;
state *cho571 = &cho571_;
sscanf_state("9223372036854775807 139320111104 2185008789864448 0 0 0 0 0 0", cho571);
cho571->playing_area = rectangle(6, 6);
cho571->target = cho571->player;
cho571->immortal = cho571->opponent;

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

state rabbity_ = (state) {
    rectangle(5, 4),
    rectangle(4, 4) & ~(rectangle(2, 2) | south(east(rectangle(2, 2)))),
    rectangle(2, 2) | one(1, 2),
    0,
    0,
    rectangle(1, 4) << (4 * H_SHIFT)
};
state *rabbity = &rabbity_;
rabbity->opponent |= rabbity->immortal;

state super_ko_seki_ = (state) {
    rectangle(4, 3),
    two(1, 0) | one(0, 1),
    south(east(rectangle(3, 2)))
};
state *super_ko_seki = &super_ko_seki_;
super_ko_seki->target = super_ko_seki->opponent;

state wut_ = (state) {
    rectangle(5, 4) ^ one(4, 3),
    rectangle(1, 4) | cross(one(3, 2)) | one(4, 1),
    east(rectangle(1, 4) | rectangle(3, 1))
};
state *wut = &wut_;
// wut->target = wut->opponent;
wut->immortal = rectangle(1, 4);

state rabbity2_ = (state) {
    rectangle(5, 5),
    tvo(2, 0) | (rectangle(5, 5) ^ rectangle(5, 4)),
    rectangle(5, 4) ^ cross(tvo(2, 0)),
};
state *rabbity2 = &rabbity2_;
stones_t im = rectangle(5, 5) ^ rectangle(5, 4) ^ one(0, 3) ^ one(4, 3);
rabbity2->player |= im;
rabbity2->opponent &= ~im;
// rabbity2->target = rabbity2->opponent;
rabbity2->immortal = im;

state test_ = (state) {
    rectangle(3, 2),
    rectangle(1, 2)
};
state *test = &test_;
test->immortal = test->player;

state corner_3x3_ = (state) {
    rectangle(6, 6),
    rectangle(6, 6) ^ rectangle(5, 5) ^ south(south(east(east(rectangle(3, 3) ^ rectangle(2, 2))))),
    rectangle(4, 4) ^ rectangle(3, 3),
};
state *corner_3x3 = &corner_3x3_;
corner_3x3->target = corner_3x3->opponent;
corner_3x3->immortal = corner_3x3->player;

state corner_4x3_ = (state) {
    rectangle(7, 5),
    rectangle(7, 5) ^ rectangle(6, 4),
    rectangle(5, 4) ^ rectangle(4, 3),
};
state *corner_4x3 = &corner_4x3_;
corner_4x3->target = corner_4x3->opponent;
corner_4x3->immortal = corner_4x3->player;

state edge_4x3_ = (state) {
    rectangle(8, 6),
    rectangle(8, 6) ^ east(rectangle(6, 4)) ^ two(3, 4),
    east(rectangle(6, 4) ^ east(rectangle(4, 3))),
};
state *edge_4x3 = &edge_4x3_;
edge_4x3->target = edge_4x3->opponent;
edge_4x3->immortal = edge_4x3->player;

state center_4x3_ = (state) {
    rectangle(8, 7),
    rectangle(8, 7) ^ south(east(rectangle(6, 5))) ^ one(0, 3) ^ one(7, 3),
    south(east(rectangle(6, 5) ^ south(east(rectangle(4, 3))))),
};
state *center_4x3 = &center_4x3_;
center_4x3->target = center_4x3->opponent;
center_4x3->immortal = center_4x3->player;

state edge_semeai_4x3_ = (state) {
    rectangle(9, 7),
    rectangle(6, 4) ^ east(rectangle(4, 3)),
    rectangle(7, 5) ^ rectangle(6, 4),
};
state *edge_semeai_4x3 = &edge_semeai_4x3_;
edge_semeai_4x3->target = edge_semeai_4x3->opponent | edge_semeai_4x3->player;
stones_t es43_temp = rectangle(9, 7) ^ rectangle(8, 6) ^ one(7, 5);
edge_semeai_4x3->player |= es43_temp;
edge_semeai_4x3->immortal |= es43_temp;

state corner_14_;
state *corner_14 = &corner_14_;
sscanf_state("9223372036854775807 2292057341031383104 484798636048 0 0 0 0 0 0", corner_14);
corner_14->playing_area = rectangle(7, 7);
corner_14->target = corner_14->opponent;
corner_14->immortal = corner_14->player;

state edge_14_;
state *edge_14 = &edge_14_;
sscanf_state("9223372036854775807 4600541437287793152 4136876147778 0 0 0 0 0 0", edge_14);
edge_14->playing_area = rectangle(8, 7);
edge_14->target = edge_14->opponent;
edge_14->immortal = edge_14->player;

state corner_4x4_ = (state) {
    rectangle(7, 7),
    rectangle(7, 7) ^ rectangle(5, 5),
    rectangle(5, 5) ^ rectangle(4, 4),
};
state *corner_4x4 = &corner_4x4_;
// corner_4x4->player ^= two(0,5) | tvo(5, 0);
corner_4x4->player ^= one(4, 5) | one(5, 4);
corner_4x4->target = corner_4x4->opponent;
corner_4x4->immortal = corner_4x4->player;

state edge_4x4_ = (state) {
    rectangle(8, 7),
    rectangle(8, 7) ^ east(rectangle(6, 6)) ^ one(1, 5) ^ one(6, 5),
    east(rectangle(6, 5) ^ east(rectangle(4, 4))),
};
state *edge_4x4 = &edge_4x4_;
edge_4x4->target = edge_4x4->opponent;
edge_4x4->immortal = edge_4x4->player;

// state center_4x4_ = (state) {
//     rectangle(8, 6),
//     (rectangle(8, 6) ^ east(rectangle(6, 6))) & ~south(south(rectangle(8, 2))),
//     east(rectangle(6, 6) ^ south(east(rectangle(4, 4)))),
// };
// state *center_4x4 = &center_4x4_;
// center_4x4->target = center_4x4->opponent;
// center_4x4->immortal = center_4x4->player;

state center_4x4_ = (state) {
    rectangle(6, 6),
    rectangle(6, 6) ^ south(east(rectangle(4, 4))),
};
state *center_4x4 = &center_4x4_;
center_4x4->target = center_4x4->player;

state target_test_ = (state) {
    rectangle(5, 1),
    one(2, 0),
    one(0, 0) | one(4, 0),
};
state *target_test = &target_test_;
target_test->target = target_test->player;
target_test->immortal = target_test->opponent;

state nakade_test_ = (state) {
    rectangle(5, 3),
    rectangle(5, 3) ^ rectangle(4, 2),
    rectangle(4, 2) ^ rectangle(3, 1),
};
state *nakade_test = &nakade_test_;
nakade_test->target = nakade_test->opponent;
nakade_test->immortal = nakade_test->player;

const tsumego_info tsumego_infos[] = {
    {"cho1", cho1_4},
    {"cho5", cho5},
    {"cho538", cho538},
    {"cho551", cho551},
    {"cho552", cho552},
    {"cho553", cho553},
    {"cho554", cho554},
    {"cho558", cho558},
    {"cho571", cho571},
    {"bulky_ten", bulky_ten},
    {"wut", wut},
    {"corner_3x3", corner_3x3},
    {"corner_4x3", corner_4x3},
    {"corner_4x4", corner_4x4},
    {"edge_4x3", edge_4x3},
    {"edge_semeai_4x3", edge_semeai_4x3},
    {"center_4x3", center_4x3},
    {"corner_14", corner_14},
    {"edge_14", edge_14},
    {"center_4x4", center_4x4},
    {"target_test", target_test},
    {"nakade_test", nakade_test},
    {NULL, NULL}
};
