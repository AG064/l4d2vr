#pragma once
#include "vr_dual_pistols.h"

namespace l4d2vr_pistol_fx
{
    struct Key
    {
        std::uintptr_t owner = 0u, weapon = 0u;
        std::uint32_t handle = 0u;
        unsigned hands = 0u;
        bool Valid() const { return owner && weapon && handle && handle != 0xffffffffu && hands > 0u && hands <= 3u; }
        bool operator==(const Key& other) const
        { return owner == other.owner && weapon == other.weapon && handle == other.handle && hands == other.hands; }
    };

    inline unsigned HandBit(l4d2vr_dual::Hand hand)
    { return hand == l4d2vr_dual::Hand::Right ? 1u : hand == l4d2vr_dual::Hand::Left ? 2u : 0u; }

    class Muzzles
    {
    public:
        using Matrix = l4d2vr_dual::Matrix3x4;
        void Observe(const Key& key)
        {
            const Key next = key.Valid() ? key : Key{};
            if (!(next == m_Key)) { m_Key = next; m_Samples = {}; }
        }
        Key Current() const { return m_Key; }
        bool Capture(const Key& key, l4d2vr_dual::Hand hand, const Matrix& controller,
            const Matrix& muzzle, float unitsPerMeter, std::uint64_t now)
        {
            const unsigned bit = HandBit(hand);
            if (!m_Key.Valid() || !(key == m_Key) || !(m_Key.hands & bit) || !now ||
                !std::isfinite(unitsPerMeter) || unitsPerMeter <= 0.001f) return false;
            Matrix identity{}, local{};
            identity.m[0][0] = identity.m[1][1] = identity.m[2][2] = 1.0f;
            if (!l4d2vr_dual::Retarget(controller, identity, muzzle, identity, local)) return false;
            float distanceSquared = 0.0f;
            for (unsigned row = 0; row < 3; ++row)
            {
                const float component = local.m[row][3] / unitsPerMeter;
                distanceSquared += component * component;
            }
            // Reject hidden branches and corrupted native attachment matrices.
            if (!std::isfinite(distanceSquared) || distanceSquared > 0.81f) return false;
            m_Samples[bit == 1u ? 0u : 1u] = {local, now};
            return true;
        }
        bool Resolve(const Key& key, l4d2vr_dual::Hand hand, const Matrix& controller,
            std::uint64_t now, Matrix& result) const
        {
            const unsigned bit = HandBit(hand);
            if (!m_Key.Valid() || !(key == m_Key) || !(m_Key.hands & bit) ||
                !l4d2vr_dual::Finite(controller)) return false;
            const auto& sample = m_Samples[bit == 1u ? 0u : 1u];
            if (!sample.tick || now < sample.tick || now - sample.tick > 150u) return false;
            result = l4d2vr_dual::Multiply(controller, sample.local);
            return l4d2vr_dual::Finite(result);
        }
    private:
        struct Sample { Matrix local{}; std::uint64_t tick = 0u; };
        Key m_Key{};
        std::array<Sample, 2> m_Samples{};
    };
}
