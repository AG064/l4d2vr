#pragma once
#include "vr_dual_pistols.h"

namespace l4d2vr_server_pistol
{
    class Commands
    {
    public:
        void Reset() { *this = {}; }
        bool Store(std::uintptr_t owner, unsigned serial, const l4d2vr_dual::Shot& shot)
        {
            if (!owner || shot.command <= 0) return false;
            if (owner != m_Owner || serial != m_Serial)
            { Reset(); m_Owner = owner; m_Serial = serial; }
            for (unsigned axis = 0; axis < 3; ++axis)
                if (!std::isfinite(shot.position[axis]) || !std::isfinite(shot.angles[axis])) return false;
            if (static_cast<std::int64_t>(m_Latest) - shot.command >= static_cast<std::int64_t>(m_Shots.size()))
                return false;
            auto& entry = m_Shots[static_cast<unsigned>(shot.command) % m_Shots.size()];
            if (entry.command == shot.command) return false; // first decoded backup owns its exact pose
            entry = shot;
            if (shot.command > m_Latest) m_Latest = shot.command;
            return true;
        }
        bool Get(std::uintptr_t owner, unsigned serial, int command, std::uint64_t now,
            l4d2vr_dual::Shot& result) const
        {
            if (!owner || owner != m_Owner || serial != m_Serial || command <= 0) return false;
            const auto& shot = m_Shots[static_cast<unsigned>(command) % m_Shots.size()];
            if (shot.command != command || shot.hand == l4d2vr_dual::Hand::None ||
                now < shot.capturedAtMs || now - shot.capturedAtMs > 2500u) return false;
            result = shot; return true;
        }
    private:
        std::uintptr_t m_Owner = 0;
        unsigned m_Serial = 0;
        int m_Latest = 0;
        std::array<l4d2vr_dual::Shot, 150> m_Shots{};
    };
}
