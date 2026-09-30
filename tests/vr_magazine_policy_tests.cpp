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
    Check(ChamberRoundsAfterEject(-1) == 0, "Unknown clip data must not create ammunition");
    Check(ChamberRoundsAfterEject(0) == 0, "An empty pistol must not gain an infinite-reserve chambered round");
    Check(ChamberRoundsAfterEject(1) == 1, "Removing a magazine must preserve the last existing chambered round");
    Check(ChamberRoundsAfterEject(15) == 1, "A tactical reload must preserve exactly one existing round");
    Check(ChamberRoundsAfterEject(30) == 1, "Clip capacity must not increase the retained chamber count");
}
