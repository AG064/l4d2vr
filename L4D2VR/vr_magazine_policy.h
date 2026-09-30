#pragma once

namespace l4d2vr_magazine
{
    inline int ChamberRoundsAfterEject(int clip)
    {
        // Reserve ammunition cannot add a round to an empty chamber or
        // remove the round already loaded in a nonempty weapon.
        return clip > 0 ? 1 : 0;
    }
}
