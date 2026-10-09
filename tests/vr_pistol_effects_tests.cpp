#include "../L4D2VR/vr_pistol_effects.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

static void Check(bool value, const char* message)
{ if (!value) { std::fprintf(stderr, "%s\n", message); std::exit(1); } }
static l4d2vr_dual::Matrix3x4 Pose(float x, float y)
{
    l4d2vr_dual::Matrix3x4 m{};
    m.m[0][0] = m.m[1][1] = m.m[2][2] = 1.0f;
    m.m[0][3] = x; m.m[1][3] = y;
    return m;
}
int main()
{
    using namespace l4d2vr_pistol_fx;
    using l4d2vr_dual::Hand;
    Muzzles cache;
    const Key dual{100u, 200u, 0x1001u, 3u};
    cache.Observe(dual);
    const auto right = Pose(10.0f, 0.0f), left = Pose(-10.0f, 0.0f);
    Check(cache.Capture(dual, Hand::Right, right, Pose(14.0f, 1.0f), 40.0f, 100u), "Capture right muzzle");
    Check(cache.Capture(dual, Hand::Left, left, Pose(-6.0f, -1.0f), 40.0f, 100u), "Capture left muzzle independently");
    auto executing = Pose(-20.0f, 5.0f), result = Pose(0.0f, 0.0f);
    executing.m[0][0] = 0.0f; executing.m[0][1] = -1.0f;
    executing.m[1][0] = 1.0f; executing.m[1][1] = 0.0f;
    Check(cache.Resolve(dual, Hand::Left, executing, 150u, result), "Resolve the left muzzle in the executing command frame");
    Check(std::fabs(result.m[0][3] + 19.0f) < 0.0001f && std::fabs(result.m[1][3] - 9.0f) < 0.0001f,
        "Room-scale translation and rotation must preserve the left gun's own muzzle offset");
    Check(cache.Resolve(dual, Hand::Right, executing, 150u, result) && std::fabs(result.m[0][3] + 21.0f) < 0.0001f,
        "A right shot must not borrow the left attachment");
    Check(!cache.Resolve({101u,200u,0x1001u,3u}, Hand::Left, executing, 150u, result), "Reject another player");
    Check(!cache.Resolve({100u,201u,0x1001u,3u}, Hand::Left, executing, 150u, result), "Reject another weapon");
    Check(!cache.Resolve({100u,200u,0x2001u,3u}, Hand::Left, executing, 150u, result), "Reject a reused entity address with a new serial");
    Check(!cache.Resolve(dual, Hand::Left, executing, 251u, result), "Reject a stale attachment");
    Check(!cache.Resolve(dual, Hand::Left, executing, 99u, result), "Reject a future sample");
    Check(!cache.Resolve(dual, Hand::None, executing, 150u, result), "Never choose a hand without a shot marker");
    Check(!cache.Capture(dual, Hand::Left, left, Pose(-10.0f,100000.0f),40.0f,150u), "Hidden gun branches cannot publish a muzzle");
    auto bad = left; bad.m[0][0] = std::numeric_limits<float>::quiet_NaN();
    Check(!cache.Capture(dual, Hand::Left, bad, left,40.0f,150u), "Reject non-finite tracking");
    const Key retained{100u,200u,0x1001u,2u};
    cache.Observe(retained);
    Check(!cache.Resolve(retained, Hand::Left, executing, 150u, result), "Changing handed inventory invalidates the old model sample");
    Check(!cache.Capture(dual, Hand::Left, left,left,40.0f,150u), "Reject a queued draw from the previous inventory mode");
    Check(cache.Capture(retained, Hand::Left, left,Pose(-6.0f,-1.0f),40.0f,150u), "Retained left pistol can publish its transformed primary attachment");
    Check(!cache.Resolve(retained, Hand::Right, right,150u,result), "A dropped pistol cannot emit effects");
    cache.Observe({}); cache.Observe(retained);
    Check(!cache.Resolve(retained,Hand::Left,left,150u,result), "Gameplay loss clears previous attachments");
}
