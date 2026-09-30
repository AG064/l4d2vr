#pragma once

namespace l4d2vr_magazine
{
    inline bool ShouldEject(bool buttonRequired, bool buttonPressed,
        bool suppressEmptyAutoReload, int clip, bool detachable)
    {
        if (!detachable)
            return false;
        return buttonRequired ? buttonPressed : (suppressEmptyAutoReload && clip == 0);
    }

    inline int ChamberRoundsAfterEject(int clip)
    {
        // Reserve ammunition cannot add a round to an empty chamber or
        // remove the round already loaded in a nonempty weapon.
        return clip > 0 ? 1 : 0;
    }
}
