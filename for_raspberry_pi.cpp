#include <iostream>
#include <thread>
#include <chrono>
#include <sys/time.h>
#include <fstream>

using namespace std::chrono;

double get_time(){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int t1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return t1 / 1000.0;
}

typedef int_fast32_t T_INT;

const bool ASSERT = true;
const uint8_t MAX_DEPTH = 14;
const uint16_t CACHE_DEPTH_CHECK = 6;
const uint32_t CACHE_BUCKETS = 1000;
const uint32_t MAX_CACHE_SIZE = 1000000;


const T_INT QB_POINTS[] = {3848, 3802, 3552, 3480, 3453, 3309, 3279, 3167, 3143, 3075, 2962, 2893, 2893,
                           2825, 2783, 2767, 2692, 2671, 2623, 2620, 2613, 2597, 2515, 2451, 2441, 2435, 2411, 2358,
                           2256,
                           2239, 2190, 2064, 1035, 919};
const T_INT RB_POINTS[] = {3116, 2913, 2761, 2619, 2390, 2364, 2335, 2333, 2284, 2271, 2153, 2128, 2078,
                           2033, 1953, 1935, 1933, 1910, 1886, 1814, 1723, 1718, 1618, 1600, 1586, 1572, 1549, 1547,
                           1530,
                           1526, 1513, 1497, 1464, 1419, 1415, 1314, 1293, 1293, 1268, 1217, 1198, 1142, 1103, 1093,
                           1090,
                           1085, 1079, 1076, 1063, 1062, 1038, 1022, 1006,
                           1005, 997, 985, 935, 935, 913, 857, 838, 818, 808, 768, 746, 745, 739, 736};
const T_INT WR_POINTS[] = {2575, 2555, 2531, 2436, 2353, 2290, 2271, 2212, 2209, 2151, 2112, 2104, 2027,
                           1949, 1947, 1932, 1931, 1910, 1891, 1885, 1872, 1859, 1754, 1716, 1715, 1707, 1635, 1601,
                           1580,
                           1558, 1533, 1528, 1513, 1502, 1491, 1471, 1470, 1447, 1443, 1442, 1438, 1437, 1434, 1432,
                           1412,
                           1412, 1410, 1409, 1409, 1404, 1390, 1387, 1379,
                           1318, 1298, 1291, 1247, 1244, 1204, 1202, 1202, 1186, 1173, 1173, 1160, 1128, 1119, 1106,
                           1105,
                           1083, 1055, 1054, 1037, 1031, 1025, 993, 992, 992, 990, 980, 972, 966, 960, 942, 940, 927,
                           923,
                           911, 901, 871, 870, 861, 832, 831, 804, 791, 787, 782, 747, 713, 695};
const T_INT TE_POINTS[] = {2368, 2092, 1740, 1449, 1419, 1367, 1235, 1200, 1176, 1170, 1129, 1119, 1107,
                           1084, 1077, 1069, 1022, 1002, 993, 993, 990, 961, 954, 913, 865, 838, 819, 819,
                           785, 773, 767, 764, 759, 743};

const T_INT SNAKE_ORDER[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};


void assert_something(bool b, int64_t num) {
    if (ASSERT) {
        if (!b) {
            std::cout << "error : " << num << std::endl;
            exit(0);
        }
    }
}


constexpr uint8_t get_next_depth(uint8_t depth) {
    if (depth == 255) {
        return 0;
    }
    return depth + 1;
}

const T_INT MAX_QB = 1;
const T_INT MAX_WR = 3;
const T_INT MAX_RB = 2;
const T_INT MAX_TE = 1;
const T_INT TEAM_SIZE = MAX_QB + MAX_RB + MAX_WR + MAX_TE + 1;
const T_INT TREE_DEPTH = TEAM_SIZE * 10;


enum Position {
    QB = 0, RB = 1, WR = 2, TE = 3
};
const Position POSITIONS[4] = {QB, RB, WR, TE};


uint64_t NUM_STATES_EVALUATED = 0;
uint64_t HEURISTIC_CALLS = 0;
uint64_t CACHE_HITS = 0;
uint64_t CACHE_CHECKS = 0;

const uint32_t QB_MASK = 255u << 24u;
const uint32_t RB_MASK = 255u << 16u;
const uint32_t WR_MASK = 255u << 8u;
const uint32_t TE_MASK = 255;
const uint32_t INV_QB_MASK = ~(255u << 24u);
const uint32_t INV_RB_MASK = ~(255u << 16u);
const uint32_t INV_WR_MASK = ~(255u << 8u);
const uint32_t INV_TE_MASK = ~255;

struct PlayerState {
private:
    uint32_t data = 0;
//    uint_fast8_t num_QBs_ = 0;
//    uint_fast8_t num_RBs_ = 0;
//    uint_fast8_t num_WRs_ = 0;
//    uint_fast8_t num_TEs_ = 0;
    bool flex_open_ = true;


public:
    T_INT sum_ = 0;

    uint32_t hash32() const {
        const uint32_t flex_as_int = flex_open_;
        return data ^ sum_ ^ (flex_open_ << 31u);
    }

    T_INT sum() const {
        return sum_;
    }

    uint_fast8_t num_QBs() const {
        return (data & QB_MASK) >> 24u;
    }

    uint_fast8_t num_RBs() const {
        return (data & RB_MASK) >> 16u;
    }

    uint_fast8_t num_WRs() const {
        return (data & WR_MASK) >> 8u;
    }

    uint_fast8_t num_TEs() const {
        return (data & TE_MASK);
    }

    bool flex_open() const {
        return flex_open_;
    }

    void set_num_QBs(const uint_fast8_t n) {
        data = data & INV_QB_MASK;
        data = data | (n << 24u);
    }

    void set_num_RBs(const uint_fast8_t n) {
        data = data & INV_RB_MASK;
        data = data | (n << 16u);
    }

    void set_num_WRs(const uint_fast8_t n) {
        data = data & INV_WR_MASK;
        data = data | (n << 8u);
    }

    void set_num_TEs(const uint_fast8_t n) {
        data = data & INV_TE_MASK;
        data = data | n;
    }

    void assign(const PlayerState *ps) {
        sum_ = ps->sum_;
//        set_num_QBs(ps->num_QBs());
//        set_num_RBs(ps->num_RBs());
//        set_num_WRs(ps->num_WRs());
//        set_num_TEs(ps->num_TEs());
        data = ps->data;
        flex_open_ = ps->flex_open_;
    }

    bool equals(const PlayerState *ps) const {
        return sum_ == ps->sum_ && data == ps->data && flex_open_ == ps->flex_open_;
    }

    void print() const {
        std::cout << (int) sum() << " " << (int) num_QBs() << " " << (int) num_RBs() << " " << (int) num_WRs() << " "
                  << (int) num_TEs() << " "
                  << (int) flex_open()
                  << " num_players=" << (int) num_players() << std::endl;
    }

    T_INT num_players() const {
        return num_QBs() + num_RBs() + num_WRs() + num_TEs() + 1 - flex_open();
    }

    const uint32_t QB_ADDER = 1ul << 24u;
    void add_one_to_QB(){
//        set_num_QBs(num_QBs()+1);
        data += QB_ADDER;
    }

    const uint32_t RB_ADDER = 1ul << 16u;
    void add_one_to_RB(){
//        set_num_RBs(num_RBs()+1);
        data += RB_ADDER;
    }

    const uint32_t WR_ADDER = 1ul << 8u;
    void add_one_to_WR(){
//        set_num_WRs(num_WRs()+1);
        data += WR_ADDER;
    }

    void add_one_to_TE(){
//        set_num_TEs(num_TEs()+1);
        data++;
    }

    template<Position p>
    void add_to_position() {
        if (p == QB) {
            add_one_to_QB();
            assert_something(num_QBs() <= MAX_QB, 0);
        } else if (p == RB) {
            if (num_RBs() >= MAX_RB) {
                assert_something(flex_open_, 123321);
                flex_open_ = false;
            } else {
                add_one_to_RB();
                assert_something(num_RBs() <= MAX_RB, 1);
            }
        } else if (p == WR) {
            if (num_WRs() >= MAX_WR) {
                assert_something(flex_open_, 543413);
                flex_open_ = false;
            } else {
                add_one_to_WR();
                assert_something(num_WRs() <= MAX_WR, 123123123);
            }
        } else if (p == TE) {
            if (num_TEs() >= MAX_TE) {
                assert_something(flex_open_, 594934);
                flex_open_ = false;
            } else {
                add_one_to_TE();
                assert_something(num_TEs() <= MAX_TE, 12390);
            }
        }
    }
};

uint16_t fold32into16(const uint32_t n) {
    return (n >> 16u) ^ n;
}

struct GameState {
    PlayerState player_states_[10];

    uint_fast16_t num_QBs_taken_ = 0;
    uint_fast16_t num_RBs_taken_ = 0;
    uint_fast16_t num_WRs_taken_ = 0;
    uint_fast16_t num_TEs_taken_ = 0;

    bool equals(const GameState *gs) const {
        if (num_QBs_taken_ != gs->num_QBs_taken_ ||
            num_RBs_taken_ != gs->num_RBs_taken_ ||
            num_WRs_taken_ != gs->num_WRs_taken_ ||
            num_TEs_taken_ != gs->num_TEs_taken_) {
            return false;
        }
        for (uint_fast8_t i = 0; i < 10; ++i) {
            if (!player_states_[i].equals(&gs->player_states_[i])) {
                return false;
            }
        }
        return true;
    }

    uint32_t hash32() const {
        uint32_t hash = num_QBs_taken_ ^
                        num_RBs_taken_ ^
                        (num_WRs_taken_ << 16u) ^
                        (num_TEs_taken_ << 16u);

        for (uint8_t i = 0; i < 10; ++i) {
            hash ^= player_states_[i].hash32();
        }
        return hash;
    }

    uint16_t hash16() const {
        return fold32into16(hash32());
    }

    T_INT num_QBs_taken() const {
        return num_QBs_taken_;
    }

    T_INT num_RBs_taken() const {
        return num_RBs_taken_;
    }

    T_INT num_WRs_taken() const {
        return num_WRs_taken_;
    }

    T_INT num_TEs_taken() const {
        return num_TEs_taken_;
    }

    void assign(const GameState *gs) {
#pragma clang loop unroll(full)
//#pragma clang loop vectorize(enable) interleave(enable)
        for (uint_fast8_t i = 0; i < 10; ++i) {
            player_states_[i].assign(&gs->player_states_[i]);
        }
        num_QBs_taken_ = gs->num_QBs_taken_;
        num_RBs_taken_ = gs->num_RBs_taken_;
        num_WRs_taken_ = gs->num_WRs_taken_;
        num_TEs_taken_ = gs->num_TEs_taken_;
    }

    void print() const {
        std::cout << "---------" << std::endl;
        std::cout << "num_QBs_taken=" << num_QBs_taken_ << std::endl;
        std::cout << "num_RBs_taken=" << num_RBs_taken_ << std::endl;
        std::cout << "num_WRs_taken=" << num_WRs_taken_ << std::endl;
        std::cout << "num_TEs_taken=" << num_TEs_taken_ << std::endl;
        for (uint_fast8_t i = 0; i < 10; ++i) {
            player_states_[i].print();
        }
    }


};


void print_position(Position p, std::ofstream* myfile) {
    if (p == QB) {
        std::cout << "QB" << std::endl;
        (*myfile) << "QB" << std::endl;
    } else if (p == RB) {
        std::cout << "RB" << std::endl;
        (*myfile) << "RB" << std::endl;
    } else if (p == WR) {
        std::cout << "WR" << std::endl;
        (*myfile) << "WR" << std::endl;
    } else if (p == TE) {
        std::cout << "TE" << std::endl;
        (*myfile) << "TE" << std::endl;
    }
}

struct GameResult {
    Position position_chosen;
    T_INT player_sums[10];

    void assign(const GameResult *gr) {
        position_chosen = gr->position_chosen;
#pragma clang loop unroll(full)
//#pragma clang loop vectorize(enable) interleave(enable)
        for (uint8_t i = 0; i < 10; ++i) {
            player_sums[i] = gr->player_sums[i];
        }
    }

};

struct CacheEntry {
    GameState gs;
    GameResult gr;
    CacheEntry *next = nullptr;

    int num_entries() {
        if (next == nullptr) {
            return 1;
        }
        return 1 + next->num_entries();
    }

    CacheEntry* get_end(){
        if(next == nullptr){
            return this;
        }
        return next->get_end();
    }
};

struct Cache {
    CacheEntry *cache_[CACHE_BUCKETS] = {nullptr};

    void add_cache(Cache* other){
        for (int i = 0; i < CACHE_BUCKETS; ++i) {
            if(cache_[i] == nullptr){
                cache_[i] = other->cache_[i];
            } else {
                CacheEntry* last = cache_[i]->get_end();
                last->next = other->cache_[i];
            }
        }
    }

    // this assumes gs is not in the cache
    void put(const GameState *gs, const GameResult *gr) {
//        std::cout << "in put" << std::endl;
        const uint32_t index = gs->hash32() % CACHE_BUCKETS;
        if (cache_[index] == nullptr) {
            cache_[index] = new CacheEntry;
        }
        CacheEntry *entry = cache_[index];
        while (entry->next != nullptr) {
            entry = entry->next;
        }
        // so now entry.next == nullptr

        CacheEntry *new_entry = new CacheEntry;
        new_entry->gs.assign(gs);
        new_entry->gr.assign(gr);
        entry->next = new_entry;
    }

    void get(const GameState *gs, GameResult *result, bool *found) {
//        std::cout << "in get" << std::endl;
        const uint32_t index = gs->hash32() % CACHE_BUCKETS;
        CacheEntry *entry = cache_[index];
        while (entry != nullptr) {
            if (entry->gs.equals(gs)) {
                result->assign(&entry->gr);
                *found = true;
                return;
            }
            entry = entry->next;
        }
        // now entry == nullptr
        *found = false;
    }

    double avg_entries() {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < CACHE_BUCKETS; ++i) {
            CacheEntry *entry = cache_[i];
            if (entry != nullptr) {
                sum += entry->num_entries();
            }
        }
        return sum * 1.0 / CACHE_BUCKETS;
    }

    int max_entries() {
        int max = 0;
        for (uint32_t i = 0; i < CACHE_BUCKETS; ++i) {
            CacheEntry *entry = cache_[i];
            if (entry != nullptr) {
                int count = entry->num_entries();
//                if(count == 25) {
//                    while(entry != nullptr){
//                        entry->gs.print();
//                        entry = entry->next;
//                    }
//                }
                if (count > max) {
                    max = count;
                }
            }
        }
        return max;
    }

};

template<uint8_t depth>
void heuristic(const GameState *node, GameResult *gr) {
//    std::cout << "IN HEURISTIC" << std::endl;
    HEURISTIC_CALLS++;

    GameState state{};
    state.assign(node);
    for (uint_fast8_t i = 0; i < 10; ++i) {
//        state.player_states_[i] = node.player_states_[i];
        assert_something(state.player_states_[i].equals(&node->player_states_[i]), 167386473);
    }

#pragma clang loop unroll(full)
    for (T_INT pick = depth; pick < TREE_DEPTH; pick++) {
//        std::cout << pick << std::endl;
        const T_INT player_index = SNAKE_ORDER[pick % 20];
        PlayerState *current_player = &state.player_states_[player_index];
//        node.print();
//        std::cout << " -- " << std::endl;
//        current_player->print();
//        std::cout << " -- " << std::endl;
        assert_something(current_player->num_players() < TEAM_SIZE, 1233333);

        T_INT max = -200;
        Position best_position;
        if (current_player->num_QBs() < MAX_QB) {
            T_INT points = QB_POINTS[state.num_QBs_taken_];
            max = points;
            best_position = QB;
        }
        if (current_player->num_RBs() < MAX_RB || current_player->flex_open()) {
            T_INT points = RB_POINTS[state.num_RBs_taken_];
            if (points >= max) {
                max = points;
                best_position = RB;
            }
        }
        if (current_player->num_WRs() < MAX_WR || current_player->flex_open()) {
            T_INT points = WR_POINTS[state.num_WRs_taken_];
            if (points >= max) {
                max = points;
                best_position = WR;
            }
        }
        if (current_player->num_TEs() < MAX_TE || current_player->flex_open()) {
            T_INT points = TE_POINTS[state.num_TEs_taken_];
            if (points >= max) {
                max = points;
                best_position = TE;
            }
        }

        assert_something(max != -200, 124355555);


        if (best_position == QB) {
            current_player->add_to_position<QB>();
            current_player->sum_ += QB_POINTS[state.num_QBs_taken_];
            state.num_QBs_taken_++;
        } else if (best_position == RB) {
            current_player->add_to_position<RB>();
            current_player->sum_ += RB_POINTS[state.num_RBs_taken_];
            state.num_RBs_taken_++;
        } else if (best_position == WR) {
            current_player->add_to_position<WR>();
            current_player->sum_ += WR_POINTS[state.num_WRs_taken_];
            state.num_WRs_taken_++;
        } else if (best_position == TE) {
            current_player->add_to_position<TE>();
            current_player->sum_ += TE_POINTS[state.num_TEs_taken_];
            state.num_TEs_taken_++;
        }
    }

//#pragma clang loop vectorize(enable) interleave(enable)
    for (uint_fast8_t i = 0; i < 10; ++i) {
        gr->player_sums[i] = state.player_states_[i].sum_;
        if (ASSERT) {
            const PlayerState *ps = &state.player_states_[i];
            assert_something(ps->num_players() == TEAM_SIZE, 20);
            if (state.player_states_[i].num_QBs() != MAX_QB || state.player_states_[i].num_RBs() != MAX_RB ||
                state.player_states_[i].num_WRs() != MAX_WR || state.player_states_[i].num_TEs() != MAX_TE) {

                node->print();
                state.player_states_[i].print();
                std::cout << "HEURISTIC IS WRONG" << std::endl;
                exit(0);
            }
        }
    }

}

template<const bool backtrace, const uint8_t depth>
void minmaxkinda(const GameState *state, GameResult *result, Cache* cache, std::ofstream* myfile) {
    NUM_STATES_EVALUATED++;
    if (ASSERT) {
        for (uint_fast8_t i = 0; i < 10; ++i) {
            const PlayerState *ps = &state->player_states_[i];
            assert_something(ps->num_players() < TEAM_SIZE, 54353222);
        }
    }

    if (depth == CACHE_DEPTH_CHECK && !backtrace) {
        CACHE_CHECKS++;
        bool b = false;
        cache->get(state, result, &b);
        if (b) {
//            std::cout << "CACHE USED" << std::endl;
//            state->print();
            CACHE_HITS++;
            return;
        }
    }

    if (depth >= MAX_DEPTH) {
        heuristic<depth>(state, result);
        return;
    }
    constexpr uint8_t next_depth = get_next_depth(depth);

    const T_INT player_index = SNAKE_ORDER[depth % 20];
    const PlayerState *current_player = &state->player_states_[player_index];

    GameResult pick_QB_score{};
    GameState qb_state{};
    if (current_player->num_QBs() < MAX_QB) {
        if (backtrace && depth == 0) {
            std::cout << "in QB depth=" << (int) depth << std::endl;
            (*myfile) << "in QB depth=" << (int) depth << std::endl;
        }
        qb_state.assign(state);
        const T_INT next_QB = state->num_QBs_taken_;
        qb_state.player_states_[player_index].sum_ += QB_POINTS[next_QB];
        qb_state.player_states_[player_index].add_to_position<QB>();
        qb_state.num_QBs_taken_++;
        minmaxkinda<false, next_depth>(&qb_state, &pick_QB_score, cache, myfile);
    } else {
        pick_QB_score.player_sums[player_index] = -100;
    }
    pick_QB_score.position_chosen = QB;

    GameResult pick_RB_score{};
    GameState rb_state{};
    if (current_player->num_RBs() < MAX_RB || current_player->flex_open()) {
        if (backtrace && depth == 0) {
            std::cout << "in RB depth=" << (int) depth << std::endl;
            (*myfile) << "in RB depth=" << (int) depth << std::endl;
        }
        rb_state.assign(state);
        const T_INT next_RB = state->num_RBs_taken_;
        rb_state.player_states_[player_index].sum_ += RB_POINTS[next_RB];
        rb_state.player_states_[player_index].add_to_position<RB>();
        rb_state.num_RBs_taken_++;
        minmaxkinda<false, next_depth>(&rb_state, &pick_RB_score, cache, myfile);
    } else {
        pick_RB_score.player_sums[player_index] = -100;
    }
    pick_RB_score.position_chosen = RB;

    GameResult pick_WR_score{};
    GameState wr_state{};
    if (current_player->num_WRs() < MAX_WR || current_player->flex_open()) {
        if (backtrace && depth == 0) {
            std::cout << "in WR depth=" << (int) depth << std::endl;
            (*myfile) << "in WR depth=" << (int) depth << std::endl;
        }
        wr_state.assign(state);
        const T_INT next_WR = state->num_WRs_taken_;
        wr_state.player_states_[player_index].sum_ += WR_POINTS[next_WR];
        wr_state.player_states_[player_index].add_to_position<WR>();
        wr_state.num_WRs_taken_++;


        minmaxkinda<false, next_depth>(&wr_state, &pick_WR_score, cache, myfile);
    } else {
        pick_WR_score.player_sums[player_index] = -100;
    }
    pick_WR_score.position_chosen = WR;

    GameResult pick_TE_score{};
    GameState te_state{};
    if (current_player->num_TEs() < MAX_TE || current_player->flex_open()) {
        if (backtrace && depth == 0) {
            std::cout << "in TE depth=" << (int) depth << std::endl;
            (*myfile) << "in TE depth=" << (int) depth << std::endl;
        }
//        te_state = state.copy();
        te_state.assign(state);

        const T_INT next_TE = state->num_TEs_taken_;
        te_state.player_states_[player_index].sum_ += TE_POINTS[next_TE];
//        std::cout << "adding TE in first minmax thing" << std::endl;
        te_state.player_states_[player_index].add_to_position<TE>();
        te_state.num_TEs_taken_++;
        minmaxkinda<false, next_depth>(&te_state, &pick_TE_score, cache, myfile);
    } else {
        pick_TE_score.player_sums[player_index] = -100;
    }
    pick_TE_score.position_chosen = TE;


    GameResult temp_result1{};
    if (pick_QB_score.player_sums[player_index] > pick_RB_score.player_sums[player_index]) {
        temp_result1 = pick_QB_score;
    } else {
        temp_result1 = pick_RB_score;
    }

    GameResult temp_result2{};
    if (pick_WR_score.player_sums[player_index] > pick_TE_score.player_sums[player_index]) {
        temp_result2 = pick_WR_score;
    } else {
        temp_result2 = pick_TE_score;
    }

    GameResult winning_move{};
    if (temp_result1.player_sums[player_index] > temp_result2.player_sums[player_index]) {
        winning_move = temp_result1;
    } else {
        winning_move = temp_result2;
    }

    if (backtrace) {
        print_position(winning_move.position_chosen, myfile);
        GameState winning_state{};
        if (winning_move.position_chosen == QB) {
            winning_state.assign(&qb_state);
        } else if (winning_move.position_chosen == RB) {
            winning_state.assign(&rb_state);
        } else if (winning_move.position_chosen == WR) {
            winning_state.assign(&wr_state);
        } else if (winning_move.position_chosen == TE) {
            winning_state.assign(&te_state);
        }

//        qb_state.print();
//        rb_state.print();
//        wr_state.print();
//        te_state.print();
//        winning_state.print();
//        std::cout << "here" << std::endl;

        GameResult temp{};
        minmaxkinda<true, next_depth>(&winning_state, &temp, cache, myfile);
    }

    result->assign(&winning_move);
    if (depth == CACHE_DEPTH_CHECK && !backtrace) {
        cache->put(state, result);
    }
}


template<const bool backtrace, const uint8_t depth>
void minmaxkinda_1depth(const GameState *state, GameResult *result, Cache* cache, std::ofstream* myfile) {
    NUM_STATES_EVALUATED++;
    if (ASSERT) {
        for (uint_fast8_t i = 0; i < 10; ++i) {
            const PlayerState *ps = &state->player_states_[i];
            assert_something(ps->num_players() < TEAM_SIZE, 54353222);
        }
    }

    if (depth == CACHE_DEPTH_CHECK && !backtrace) {
        CACHE_CHECKS++;
        bool b = false;
        cache->get(state, result, &b);
        if (b) {
//            std::cout << "CACHE USED" << std::endl;
//            state->print();
            CACHE_HITS++;
            return;
        }
    }

    if (depth >= MAX_DEPTH) {
        heuristic<depth>(state, result);
        return;
    }
    constexpr uint8_t next_depth = get_next_depth(depth);

    const T_INT player_index = SNAKE_ORDER[depth % 20];
    const PlayerState *current_player = &state->player_states_[player_index];

    GameResult pick_QB_score{};
    GameState qb_state{};
    std::thread QB_thread;
    bool QB_thread_used = false;
    Cache QB_cache{};
    if (current_player->num_QBs() < MAX_QB) {
        if (backtrace && depth == 0) {
            std::cout << "in QB depth=" << (int) depth << std::endl;
            (*myfile) << "in QB depth=" << (int) depth << std::endl;
        }
        qb_state.assign(state);
        const T_INT next_QB = state->num_QBs_taken_;
        qb_state.player_states_[player_index].sum_ += QB_POINTS[next_QB];
        qb_state.player_states_[player_index].add_to_position<QB>();
        qb_state.num_QBs_taken_++;

        QB_thread = std::thread(minmaxkinda<false, next_depth>, &qb_state, &pick_QB_score, &QB_cache, myfile);
        QB_thread_used = true;
    } else {
        pick_QB_score.player_sums[player_index] = -100;
    }


    GameResult pick_RB_score{};
    GameState rb_state{};
    std::thread RB_thread;
    bool RB_thread_used = false;
    Cache RB_cache{};
    if (current_player->num_RBs() < MAX_RB || current_player->flex_open()) {
        if (backtrace && depth == 0) {
            std::cout << "in RB depth=" << (int) depth << std::endl;
            (*myfile) << "in RB depth=" << (int) depth << std::endl;
        }
        rb_state.assign(state);
        const T_INT next_RB = state->num_RBs_taken_;
        rb_state.player_states_[player_index].sum_ += RB_POINTS[next_RB];
        rb_state.player_states_[player_index].add_to_position<RB>();
        rb_state.num_RBs_taken_++;
        RB_thread = std::thread(minmaxkinda<false, next_depth>, &rb_state, &pick_RB_score, &RB_cache, myfile);
        RB_thread_used = true;
    } else {
        pick_RB_score.player_sums[player_index] = -100;
    }



    GameResult pick_WR_score{};
    GameState wr_state{};
    std::thread WR_thread;
    bool WR_thread_used = false;
    Cache WR_cache{};
    if (current_player->num_WRs() < MAX_WR || current_player->flex_open()) {
        if (backtrace && depth == 0) {
            std::cout << "in WR depth=" << (int) depth << std::endl;
            (*myfile) << "in WR depth=" << (int) depth << std::endl;
        }
        wr_state.assign(state);
        const T_INT next_WR = state->num_WRs_taken_;
        wr_state.player_states_[player_index].sum_ += WR_POINTS[next_WR];
        wr_state.player_states_[player_index].add_to_position<WR>();
        wr_state.num_WRs_taken_++;


        WR_thread = std::thread(minmaxkinda<false, next_depth>, &wr_state, &pick_WR_score, &WR_cache, myfile);
        WR_thread_used = true;
    } else {
        pick_WR_score.player_sums[player_index] = -100;
    }


    GameResult pick_TE_score{};
    GameState te_state{};
    std::thread TE_thread;
    bool TE_thread_used = false;
    Cache TE_cache{};
    if (current_player->num_TEs() < MAX_TE || current_player->flex_open()) {
        if (backtrace && depth == 0) {
            std::cout << "in TE depth=" << (int) depth << std::endl;
            (*myfile) << "in TE depth=" << (int) depth << std::endl;
        }
//        te_state = state.copy();
        te_state.assign(state);

        const T_INT next_TE = state->num_TEs_taken_;
        te_state.player_states_[player_index].sum_ += TE_POINTS[next_TE];
//        std::cout << "adding TE in first minmax thing" << std::endl;
        te_state.player_states_[player_index].add_to_position<TE>();
        te_state.num_TEs_taken_++;

        TE_thread = std::thread(minmaxkinda<false, next_depth>, &te_state, &pick_TE_score, &TE_cache, myfile);
        TE_thread_used = true;
    } else {
        pick_TE_score.player_sums[player_index] = -100;
    }

    if(QB_thread_used){
        QB_thread.join();
        cache->add_cache(&QB_cache);
    }
    if(RB_thread_used){
        RB_thread.join();
        cache->add_cache(&RB_cache);
    }
    if(WR_thread_used){
        WR_thread.join();
        cache->add_cache(&WR_cache);
    }
    if(TE_thread_used){
        TE_thread.join();
        cache->add_cache(&TE_cache);
    }

    pick_QB_score.position_chosen = QB;
    pick_RB_score.position_chosen = RB;
    pick_WR_score.position_chosen = WR;
    pick_TE_score.position_chosen = TE;


    GameResult temp_result1{};
    if (pick_QB_score.player_sums[player_index] > pick_RB_score.player_sums[player_index]) {
        temp_result1 = pick_QB_score;
    } else {
        temp_result1 = pick_RB_score;
    }

    GameResult temp_result2{};
    if (pick_WR_score.player_sums[player_index] > pick_TE_score.player_sums[player_index]) {
        temp_result2 = pick_WR_score;
    } else {
        temp_result2 = pick_TE_score;
    }

    GameResult winning_move{};
    if (temp_result1.player_sums[player_index] > temp_result2.player_sums[player_index]) {
        winning_move = temp_result1;
    } else {
        winning_move = temp_result2;
    }

    if (backtrace) {
        print_position(winning_move.position_chosen, myfile);
        GameState winning_state{};
        if (winning_move.position_chosen == QB) {
            winning_state.assign(&qb_state);
        } else if (winning_move.position_chosen == RB) {
            winning_state.assign(&rb_state);
        } else if (winning_move.position_chosen == WR) {
            winning_state.assign(&wr_state);
        } else if (winning_move.position_chosen == TE) {
            winning_state.assign(&te_state);
        }

//        qb_state.print();
//        rb_state.print();
//        wr_state.print();
//        te_state.print();
//        winning_state.print();
//        std::cout << "here" << std::endl;

        GameResult temp{};
        minmaxkinda<true, next_depth>(&winning_state, &temp, cache, myfile);
    }

    result->assign(&winning_move);
    if (depth == CACHE_DEPTH_CHECK && !backtrace) {
        cache->put(state, result);
    }
}

template<const bool backtrace, const uint8_t depth>
void minmaxkinda_0depth(const GameState *state, GameResult *result, std::ofstream* myfile) {
    NUM_STATES_EVALUATED++;
    if (ASSERT) {
        for (uint_fast8_t i = 0; i < 10; ++i) {
            const PlayerState *ps = &state->player_states_[i];
            assert_something(ps->num_players() < TEAM_SIZE, 54353222);
        }
    }

//    if (depth == CACHE_DEPTH_CHECK && !backtrace) {
//        CACHE_CHECKS++;
//        bool b = false;
//        cache->get(state, result, &b);
//        if (b) {
////            std::cout << "CACHE USED" << std::endl;
////            state->print();
//            CACHE_HITS++;
//            return;
//        }
//    }

    if (depth >= MAX_DEPTH) {
        heuristic<depth>(state, result);
        return;
    }
    constexpr uint8_t next_depth = get_next_depth(depth);

    const T_INT player_index = SNAKE_ORDER[depth % 20];
    const PlayerState *current_player = &state->player_states_[player_index];


    GameResult pick_QB_score{};
    GameState qb_state{};
    Cache QB_cache{};
    if (backtrace && depth == 0) {
        std::cout << "in QB depth=" << (int) depth << std::endl;
        (*myfile) << "in QB depth=" << (int) depth << std::endl;
    }
    qb_state.assign(state);
    const T_INT next_QB = state->num_QBs_taken_;
    qb_state.player_states_[player_index].sum_ += QB_POINTS[next_QB];
    qb_state.player_states_[player_index].add_to_position<QB>();
    qb_state.num_QBs_taken_++;
    std::thread QB_thread(minmaxkinda_1depth<false, next_depth>, &qb_state, &pick_QB_score, &QB_cache, myfile);


    GameResult pick_RB_score{};
    GameState rb_state{};
    Cache RB_cache{};
    if (backtrace && depth == 0) {
        std::cout << "in RB depth=" << (int) depth << std::endl;
        (*myfile) << "in RB depth=" << (int) depth << std::endl;
    }
    rb_state.assign(state);
    const T_INT next_RB = state->num_RBs_taken_;
    rb_state.player_states_[player_index].sum_ += RB_POINTS[next_RB];
    rb_state.player_states_[player_index].add_to_position<RB>();
    rb_state.num_RBs_taken_++;
    std::thread RB_thread(minmaxkinda_1depth<false, next_depth>, &rb_state, &pick_RB_score, &RB_cache, myfile);


    GameResult pick_WR_score{};
    GameState wr_state{};
    Cache WR_cache{};
    if (backtrace && depth == 0) {
        std::cout << "in WR depth=" << (int) depth << std::endl;
        (*myfile) << "in WR depth=" << (int) depth << std::endl;
    }
    wr_state.assign(state);
    const T_INT next_WR = state->num_WRs_taken_;
    wr_state.player_states_[player_index].sum_ += WR_POINTS[next_WR];
    wr_state.player_states_[player_index].add_to_position<WR>();
    wr_state.num_WRs_taken_++;
    std::thread WR_thread(minmaxkinda_1depth<false, next_depth>, &wr_state, &pick_WR_score, &WR_cache, myfile);


    GameResult pick_TE_score{};
    GameState te_state{};
    Cache TE_cache{};
    if (backtrace && depth == 0) {
        std::cout << "in TE depth=" << (int) depth << std::endl;
        (*myfile) << "in TE depth=" << (int) depth << std::endl;
    }
//        te_state = state.copy();
    te_state.assign(state);

    const T_INT next_TE = state->num_TEs_taken_;
    te_state.player_states_[player_index].sum_ += TE_POINTS[next_TE];
//        std::cout << "adding TE in first minmax thing" << std::endl;
    te_state.player_states_[player_index].add_to_position<TE>();
    te_state.num_TEs_taken_++;
    std::thread TE_thread(minmaxkinda_1depth<false, next_depth>, &te_state, &pick_TE_score, &TE_cache, myfile);


    QB_thread.join();
    std::cout << "QB thread done" << std::endl;
    (*myfile) << "QB thread done" << std::endl;
    RB_thread.join();
    std::cout << "RB thread done" << std::endl;
    (*myfile) << "RB thread done" << std::endl;
    WR_thread.join();
    std::cout << "WR thread done" << std::endl;
    (*myfile) << "WR thread done" << std::endl;
    TE_thread.join();
    std::cout << "TE thread done" << std::endl;
    (*myfile) << "TE thread done" << std::endl;

    pick_QB_score.position_chosen = QB;
    pick_RB_score.position_chosen = RB;
    pick_WR_score.position_chosen = WR;
    pick_TE_score.position_chosen = TE;


    GameResult temp_result1{};
    if (pick_QB_score.player_sums[player_index] > pick_RB_score.player_sums[player_index]) {
        temp_result1 = pick_QB_score;
    } else {
        temp_result1 = pick_RB_score;
    }

    GameResult temp_result2{};
    if (pick_WR_score.player_sums[player_index] > pick_TE_score.player_sums[player_index]) {
        temp_result2 = pick_WR_score;
    } else {
        temp_result2 = pick_TE_score;
    }

    GameResult winning_move{};
    if (temp_result1.player_sums[player_index] > temp_result2.player_sums[player_index]) {
        winning_move = temp_result1;
    } else {
        winning_move = temp_result2;
    }

    if (backtrace) {
        print_position(winning_move.position_chosen, myfile);
        GameState winning_state{};
        if (winning_move.position_chosen == QB) {
            winning_state.assign(&qb_state);
        } else if (winning_move.position_chosen == RB) {
            winning_state.assign(&rb_state);
        } else if (winning_move.position_chosen == WR) {
            winning_state.assign(&wr_state);
        } else if (winning_move.position_chosen == TE) {
            winning_state.assign(&te_state);
        }

        QB_cache.add_cache(&RB_cache);
        QB_cache.add_cache(&WR_cache);
        QB_cache.add_cache(&TE_cache);

//        qb_state.print();
//        rb_state.print();
//        wr_state.print();
//        te_state.print();
//        winning_state.print();
//        std::cout << "here" << std::endl;

        GameResult temp{};
        minmaxkinda<true, next_depth>(&winning_state, &temp, &QB_cache, myfile);
    }

    result->assign(&winning_move);
//    if (depth == CACHE_DEPTH_CHECK && !backtrace) {
//        cache->put(state, result);
//    }
}


int main() {
    PlayerState ps{};
    ps.set_num_QBs(123);
    if (ps.num_QBs() != 123) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }
    ps.add_one_to_QB();
    if (ps.num_QBs() != 124) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }

    ps.set_num_RBs(33);
    if (ps.num_RBs() != 33) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }
    ps.add_one_to_RB();
    if (ps.num_RBs() != 34) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }

    ps.set_num_WRs(12);
    if (ps.num_WRs() != 12) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }
    ps.add_one_to_WR();
    if (ps.num_WRs() != 13) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }

    ps.set_num_TEs(155);
    if (ps.num_TEs() != 155) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }
    ps.add_one_to_TE();
    if (ps.num_TEs() != 156) {
        std::cout << "failed initial test" << std::endl;
        exit(0);
    }

    std::ofstream myfile;
    myfile.open ("output.txt");

//    Cache cache{};

    std::cout << "depth=" << (int) MAX_DEPTH << std::endl;
    myfile << "depth=" << (int) MAX_DEPTH << std::endl;
    GameState start_state{};

    double t1 = get_time();
    GameResult result{};


    minmaxkinda_0depth<true, 0>(&start_state, &result, &myfile);


    double t2 = get_time();
    std::cout << (t2 - t1) << " seconds" << std::endl;
    myfile << (t2 - t1) << " seconds" << std::endl;

    std::cout << "num_states_evaluated=" << NUM_STATES_EVALUATED << std::endl;
    myfile<< "num_states_evaluated=" << NUM_STATES_EVALUATED << std::endl;
    std::cout << "heuristic_calls     =" << HEURISTIC_CALLS << std::endl;
    myfile<< "heuristic_calls     =" << HEURISTIC_CALLS << std::endl;
    std::cout << "cache_checks      =" << CACHE_CHECKS << std::endl;
    myfile<< "cache_checks      =" << CACHE_CHECKS << std::endl;
    std::cout << "cache_hit_rate      =" << (CACHE_HITS * 100.0 / CACHE_CHECKS) << "%" << std::endl;
    myfile<< "cache_hit_rate      =" << (CACHE_HITS * 100.0 / CACHE_CHECKS) << "%" << std::endl;

//    std::cout << "cache average entries per bucket=" << cache.avg_entries() << std::endl;
//    std::cout << "cache max entries per bucket=" << cache.max_entries() << std::endl;
    myfile.close();
}











