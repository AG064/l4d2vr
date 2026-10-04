#include "../L4D2VR/vr_magazine_policy.h"
#include <cstdio>
#include <cstdlib>

static void Check(bool result, const char* message)
{
    if (!result)
    {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

int main()
{
    using l4d2vr_magazine::ChamberRoundsAfterEject;
    using l4d2vr_magazine::ShouldEject;
    Check(!ShouldEject(true, false, true, 0, true), "Button mode must not automatically eject an empty magazine");
    Check(ShouldEject(true, true, true, 15, true), "Explicit release must eject a loaded magazine");
    Check(ShouldEject(true, true, true, 0, true), "Explicit release must eject an empty magazine");
    Check(!ShouldEject(true, true, true, 8, false), "A tube-fed shotgun must not eject a detachable magazine");
    Check(ShouldEject(false, false, true, 0, true), "Legacy empty-magazine handling remains available");
    Check(ChamberRoundsAfterEject(-1) == 0, "Unknown clip data must not create ammunition");
    Check(ChamberRoundsAfterEject(0) == 0, "An empty pistol must not gain an infinite-reserve chambered round");
    Check(ChamberRoundsAfterEject(1) == 1, "Removing a magazine must preserve the last existing chambered round");
    Check(ChamberRoundsAfterEject(15) == 1, "A tactical reload must preserve exactly one existing round");
    Check(ChamberRoundsAfterEject(30) == 1, "Clip capacity must not increase the retained chamber count");
    l4d2vr_magazine::NativeReloadLedger local, teammate;
    local.Observe(100, 10u, true);
    Check(local.Blocks(10u) && !teammate.Blocks(10u), "Physical reload suppression belongs only to its sender");
    local.Observe(101, 10u, false);
    local.Observe(100, 10u, true);
    Check(!local.Blocks(10u), "An older backup command must not cancel an authorized reload");
    local.Observe(102, 20u, true);
    Check(local.Blocks(20u) && !local.Blocks(10u), "Changing weapons must release the old weapon's reload gate");
    local.Observe(103, 0u, true);
    Check(!local.Blocks(20u) && !local.Blocks(0u), "Empty inventory must not retain a reload gate");
}
