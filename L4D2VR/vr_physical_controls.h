#pragma once

#include <array>
#include <cstdint>
#include <cmath>

namespace l4d2vr_physical
{
    inline int MeleeSweepSamples(float translation, float angleDegrees, float unitsPerMeter)
    {
        if (!std::isfinite(translation) || !std::isfinite(angleDegrees) ||
            !std::isfinite(unitsPerMeter) || unitsPerMeter <= 0.001f ||
            translation < 0.0f || translation > 0.5f * unitsPerMeter)
            return 0;
        if (translation <= 0.001f && std::fabs(angleDegrees) <= 0.01f)
            return 0;
        const float byDistance = translation / (0.02f * unitsPerMeter);
        const float byAngle = std::fabs(angleDegrees) / 10.0f;
        const float wanted = std::fmax(byDistance, byAngle);
        // Avoid an extra sample from roundoff at an exact 2 cm boundary.
        return wanted >= 12.0f ? 12 : static_cast<int>(std::fmax(1.0f, std::ceil(wanted - 0.00001f)));
    }

    // A contact can consume a held grip once. Leaving and entering a different
    // body slot while still holding the same item must not switch it repeatedly.
    class ContactLatch
    {
    public:
        bool Update(bool active, bool down, bool contact)
        {
            if (!active || !down)
            {
                m_Consumed = false;
                return false;
            }
            if (!contact || m_Consumed)
                return false;
            m_Consumed = true;
            return true;
        }
    private:
        bool m_Consumed = false;
    };

    class PickupIntent
    {
    public:
        enum : unsigned { RequestUse = 1u, AdoptWeapon = 2u };

        void Begin(std::uint32_t now, std::uintptr_t weapon)
        {
            m_Pending = true;
            m_Started = now;
            m_PreviousWeapon = weapon;
        }

        unsigned Update(bool eligible, bool active, bool down, bool contact,
            std::uint32_t now, std::uintptr_t weapon)
        {
            if (!eligible || !active || !down)
            {
                Reset();
                return 0u;
            }
            if (m_Pending)
            {
                if (now - m_Started <= 650u && weapon && weapon != m_PreviousWeapon)
                {
                    m_Pending = false;
                    m_Consumed = true;
                    return AdoptWeapon;
                }
                if (now - m_Started <= 650u)
                    return 0u;
                m_Pending = false;
                m_Consumed = true;
            }
            if (!contact || m_Consumed)
                return 0u;
            Begin(now, weapon);
            return RequestUse;
        }

        void Reset()
        {
            m_Pending = false;
            m_Consumed = false;
            m_Started = 0u;
            m_PreviousWeapon = 0u;
        }
    private:
        bool m_Pending = false;
        bool m_Consumed = false;
        std::uint32_t m_Started = 0u;
        std::uintptr_t m_PreviousWeapon = 0u;
    };

    // Keep cycling state per weapon, including while the player draws another
    // item. Clip increases are loading, not shots. No ammo is created here.
    class PumpCycles
    {
    public:
        void ObserveOwner(bool active, std::uintptr_t owner)
        {
            if (!active || !owner) { Reset(); return; }
            if (owner != m_Owner) { Reset(); m_Owner = owner; }
        }
        bool Observe(std::uintptr_t weapon, int weaponId, int clip)
        {
            if (!weapon || clip < 0)
                return false;
            Entry* entry = nullptr;
            for (auto& candidate : m_Entries)
            {
                if (candidate.weapon == weapon && candidate.weaponId == weaponId)
                {
                    entry = &candidate;
                    break;
                }
            }
            if (!entry)
            {
                entry = &m_Entries[m_Next++ % m_Entries.size()];
                *entry = { weapon, weaponId, clip, false };
            }
            entry->clip = clip;
            return entry->needsCycle;
        }

        void NotifyShot(std::uintptr_t weapon, int weaponId, int clip)
        {
            if (!weapon || (weaponId != 3 && weaponId != 8) || clip < 0)
                return;
            Observe(weapon, weaponId, clip);
            for (auto& entry : m_Entries)
                if (entry.weapon == weapon && entry.weaponId == weaponId)
                    entry.needsCycle = true;
        }

        void Complete(std::uintptr_t weapon)
        {
            for (auto& entry : m_Entries)
                if (entry.weapon == weapon)
                    entry.needsCycle = false;
        }

        void Reset() { m_Entries = {}; m_Next = 0u; m_Owner = 0u; }

        static bool StrokeComplete(bool reachedRear, float distance, float returnDistance)
        {
            return reachedRear && std::isfinite(distance) &&
                std::isfinite(returnDistance) && distance >= 0.0f &&
                distance <= returnDistance;
        }
    private:
        struct Entry
        {
            std::uintptr_t weapon = 0u;
            int weaponId = 0;
            int clip = -1;
            bool needsCycle = false;
        };
        std::array<Entry, 16> m_Entries{};
        std::uintptr_t m_Owner = 0u;
        std::size_t m_Next = 0u;
    };
}
