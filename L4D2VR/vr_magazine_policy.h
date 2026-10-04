#pragma once

#include <cstdint>

namespace l4d2vr_magazine
{
    class NativeReloadLedger
    {
    public:
        void Observe(int command, std::uintptr_t weapon, bool block, bool preserve = false, std::uintptr_t owner = 0u)
        {
            if (owner != m_Owner)
            {
                m_Owner = owner; m_Command = 0; m_Weapon = 0u;
            }
            if (command <= m_Command) return;
            m_Command = command;
            if (preserve)
            {
                if (m_Weapon != weapon) m_Weapon = 0u;
                return;
            }
            m_Weapon = block ? weapon : 0u;
        }
        bool Blocks(std::uintptr_t weapon) const { return weapon != 0u && m_Weapon == weapon; }
    private:
        int m_Command = 0;
        std::uintptr_t m_Weapon = 0u;
        std::uintptr_t m_Owner = 0u;
    };
    inline bool IsLocalPlayerCommand(int playerIndex, int localPlayerIndex)
    {
        return playerIndex > 0 && localPlayerIndex > 0 && playerIndex == localPlayerIndex;
    }

    inline bool MayCommitAmmo(int expectedWeaponId, int serverWeaponId, int clientWeaponId,
        std::uint32_t queuedGeneration, std::uint32_t currentGeneration)
    {
        return expectedWeaponId > 0 && expectedWeaponId == serverWeaponId &&
            expectedWeaponId == clientWeaponId && queuedGeneration == currentGeneration;
    }

    class SessionTracker
    {
    public:
        // Compare opaque identities without dereferencing objects that may
        // already have been deleted. Completion of a reload is not a boundary.
        bool Observe(bool ready, std::uintptr_t player, std::uintptr_t weapon,
            int weaponId, unsigned int inputMode)
        {
            ready = ready && player != 0u && weapon != 0u && weaponId > 0;
            if (!ready)
            {
                const bool changed = m_Ready;
                m_Ready = false;
                m_Player = m_Weapon = 0u;
                m_WeaponId = 0;
                m_InputMode = 0u;
                return changed;
            }
            const bool changed = !m_Ready || player != m_Player || weapon != m_Weapon ||
                weaponId != m_WeaponId || inputMode != m_InputMode;
            m_Ready = true;
            m_Player = player;
            m_Weapon = weapon;
            m_WeaponId = weaponId;
            m_InputMode = inputMode;
            return changed;
        }

    private:
        bool m_Ready = false;
        std::uintptr_t m_Player = 0u;
        std::uintptr_t m_Weapon = 0u;
        int m_WeaponId = 0;
        unsigned int m_InputMode = 0u;
    };

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
