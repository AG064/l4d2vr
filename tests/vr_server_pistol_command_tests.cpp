#include "../L4D2VR/vr_server_pistol_commands.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
static void Check(bool okay, int line)
{
    if (!okay) { std::fprintf(stderr, "Server pistol command check failed at line %d\n", line); std::exit(1); }
}
#define CHECK(value) Check((value), __LINE__)
int main()
{
    using namespace l4d2vr_dual;
    l4d2vr_server_pistol::Commands commands;
    Shot left{}; left.command = 100; left.hand = Hand::Left; left.position = {1.0f, 2.0f, 3.0f};
    left.angles = {10.0f, 20.0f, 30.0f}; left.capturedAtMs = 1000u;
    Shot right = left; right.command = 101; right.hand = Hand::Right; right.position[0] = 9.0f;
    CHECK(commands.Store(10u, 3u, left));
    CHECK(commands.Store(10u, 3u, right));
    Shot selected{};
    CHECK(commands.Get(10u, 3u, 100, 1100u, selected) && selected.hand == Hand::Left && selected.position[0] == 1.0f);
    CHECK(commands.Get(10u, 3u, 101, 1100u, selected) && selected.hand == Hand::Right && selected.position[0] == 9.0f);
    // Decoding a newer/right packet does not change the older/left command
    // that Source is about to simulate. Modified duplicates cannot replace it.
    auto altered = left; altered.hand = Hand::Right; altered.position[0] = 100.0f;
    CHECK(!commands.Store(10u, 3u, altered));
    CHECK(commands.Get(10u, 3u, 100, 1200u, selected) && selected.hand == Hand::Left && selected.position[0] == 1.0f);
    CHECK(!commands.Get(11u, 3u, 100, 1200u, selected));
    CHECK(!commands.Get(10u, 4u, 100, 1200u, selected));
    CHECK(!commands.Get(10u, 3u, 100, 3501u, selected));
    CHECK(!commands.Get(10u, 3u, 100, 999u, selected));
    Shot ordinary = left; ordinary.command = 102; ordinary.hand = Hand::None;
    CHECK(commands.Store(10u, 3u, ordinary));
    CHECK(!commands.Get(10u, 3u, 102, 1100u, selected));
    Shot newer = right; newer.command = 250;
    CHECK(commands.Store(10u, 3u, newer));
    CHECK(!commands.Get(10u, 3u, 100, 1100u, selected));
    CHECK(!commands.Store(10u, 3u, left)); // too old for the current ring
    CHECK(commands.Store(10u, 4u, left)); // an edict generation has its own history
    CHECK(!commands.Get(10u, 3u, 100, 1100u, selected));
    CHECK(commands.Get(10u, 4u, 100, 1100u, selected));
    auto bad = right; bad.position[1] = std::numeric_limits<float>::quiet_NaN();
    CHECK(!commands.Store(10u, 4u, bad));
    CHECK(!commands.Get(10u, 4u, 101, 1100u, selected));
    commands.Reset(); CHECK(!commands.Get(10u, 4u, 100, 1100u, selected));
    CHECK(!commands.Store(0u, 4u, left));
    std::puts("Server pistol command replay checks passed");
}
