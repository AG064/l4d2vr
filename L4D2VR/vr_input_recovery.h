#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace l4d2vr_input
{
    class ControllerPairReadiness
    {
    public:
        void Reset() { *this = {}; }

        bool Update(bool enabled, bool bothValid, std::uint64_t capturedAtMs,
            std::uint32_t poseToken, std::uint64_t nowMs)
        {
            constexpr std::uint64_t kMaxAgeMs = 150u;
            if (!enabled || !bothValid || capturedAtMs == 0u || poseToken == 0u ||
                nowMs < capturedAtMs || nowMs - capturedAtMs > kMaxAgeMs)
            {
                Reset();
                return false;
            }

            if (m_LastPoseToken != 0u &&
                (capturedAtMs < m_LastCapturedAtMs || nowMs < m_LastCapturedAtMs ||
                    nowMs - m_LastCapturedAtMs > kMaxAgeMs))
            {
                if (poseToken == m_LastPoseToken)
                {
                    m_Ready = false;
                    m_StableSinceMs = 0u;
                    return false;
                }
                Reset();
            }

            // Repeated render calls and republished snapshots do not provide a
            // new tracking sample or extend the lifetime of the previous one.
            if (poseToken == m_LastPoseToken)
                return m_Ready;

            if (m_LastPoseToken == 0u)
                m_StableSinceMs = capturedAtMs;
            m_LastPoseToken = poseToken;
            m_LastCapturedAtMs = capturedAtMs;
            m_Ready = capturedAtMs - m_StableSinceMs >= 200u;
            return m_Ready;
        }

        std::uint64_t StableDurationMs() const
        {
            return m_StableSinceMs == 0u ? 0u : m_LastCapturedAtMs - m_StableSinceMs;
        }

    private:
        std::uint32_t m_LastPoseToken = 0u;
        std::uint64_t m_LastCapturedAtMs = 0u;
        std::uint64_t m_StableSinceMs = 0u;
        bool m_Ready = false;
    };

    struct HeldTarget
    {
        enum class Kind { None, VirtualKey, Command };
        Kind kind = Kind::None;
        std::uint16_t virtualKey = 0u;
        std::string pressCommand;
        std::string releaseCommand;

        bool operator==(const HeldTarget& other) const
        {
            return kind == other.kind && virtualKey == other.virtualKey &&
                pressCommand == other.pressCommand && releaseCommand == other.releaseCommand;
        }
        bool operator!=(const HeldTarget& other) const { return !(*this == other); }
    };

    class HeldCustomActions
    {
    public:
        void Reconcile(std::size_t index, const HeldTarget& target, bool held)
        {
            if (index >= m_Desired.size())
                return;
            m_Desired[index] = held ? target : HeldTarget{};
        }

        template<class Emit>
        void Flush(Emit&& emit)
        {
            // Decide from the complete frame so ownership may transfer between
            // slots without briefly releasing a key or gameplay command.
            for (std::size_t index = 0u; index < m_Owned.size(); ++index)
                if (m_Owned[index].kind != HeldTarget::Kind::None &&
                    !Contains(m_Desired, m_Owned[index], m_Desired.size()) &&
                    !Contains(m_Owned, m_Owned[index], index))
                    emit(m_Owned[index], false);
            for (std::size_t index = 0u; index < m_Desired.size(); ++index)
                if (m_Desired[index].kind != HeldTarget::Kind::None &&
                    !Contains(m_Owned, m_Desired[index], m_Owned.size()) &&
                    !Contains(m_Desired, m_Desired[index], index))
                    emit(m_Desired[index], true);
            m_Owned = m_Desired;
        }

        template<class Emit>
        void ReleaseAll(Emit&& emit)
        {
            for (std::size_t index = 0u; index < m_Owned.size(); ++index)
                Release(index, emit);
            m_Desired = {};
        }

    private:
        static bool Contains(const std::array<HeldTarget, 5u>& targets,
            const HeldTarget& target, std::size_t count)
        {
            for (std::size_t index = 0u; index < count; ++index)
                if (targets[index].kind != HeldTarget::Kind::None && targets[index] == target)
                    return true;
            return false;
        }

        template<class Emit>
        void Release(std::size_t index, Emit&& emit)
        {
            if (m_Owned[index].kind == HeldTarget::Kind::None)
                return;
            const HeldTarget previous = m_Owned[index];
            m_Owned[index] = {};
            if (!Contains(m_Owned, previous, m_Owned.size()))
                emit(previous, false);
        }

        std::array<HeldTarget, 5u> m_Owned{};
        std::array<HeldTarget, 5u> m_Desired{};
    };
}
