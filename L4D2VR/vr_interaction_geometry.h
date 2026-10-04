#pragma once
#include "sdk/vector.h"
#include <cmath>

namespace l4d2vr_interaction
{
    inline bool Finite(const Vector& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    inline float WrapYaw(float value)
    {
        return value - 360.0f * std::floor((value + 180.0f) / 360.0f);
    }

    inline float BodyYaw(const Vector& worldHeadForward, float previous, float turnDelta, bool valid)
    {
        const float turned = WrapYaw(previous + turnDelta);
        // Looking straight down must not invalidate the waist reference.
        if (worldHeadForward.x * worldHeadForward.x + worldHeadForward.y * worldHeadForward.y < 0.000001f)
            return turned;
        const float head = std::atan2(worldHeadForward.y, worldHeadForward.x) * 180.0f / 3.14159265358979323846f;
        return !valid || std::fabs(WrapYaw(head - turned)) > 65.0f ? head : turned;
    }

    inline Vector BodyOrigin(const Vector& head, const Vector& forward, const Vector& right,
        const Vector& offsetMeters, float scale)
    {
        return head + forward * (offsetMeters.x * scale) + right * (offsetMeters.y * scale) +
            Vector(0.0f, 0.0f, offsetMeters.z * scale);
    }

    inline bool LocalHandPosition(const Vector& hand, const Vector& weapon,
        const Vector& weaponForward, const Vector& referenceUp, Vector& result)
    {
        if (!Finite(hand) || !Finite(weapon) || !Finite(weaponForward) || !Finite(referenceUp))
            return false;
        Vector forward = weaponForward;
        const float length = forward.Length();
        if (!std::isfinite(length) || length < 0.0001f)
            return false;
        forward *= 1.0f / length;
        Vector right{};
        CrossProduct(forward, referenceUp, right);
        const float rightLength = right.Length();
        if (!std::isfinite(rightLength) || rightLength < 0.0001f)
            return false;
        right *= 1.0f / rightLength;
        Vector up{};
        CrossProduct(right, forward, up);
        const Vector delta = hand - weapon;
        result = Vector(DotProduct(delta, forward), DotProduct(delta, right), DotProduct(delta, up));
        return Finite(result);
    }

    inline bool ShellTouchesPort(const Vector& shell, const Vector& port, float radius)
    {
        return Finite(shell) && Finite(port) && std::isfinite(radius) && radius > 0.0f &&
            (shell - port).LengthSqr() <= radius * radius;
    }
    inline float PointBoxDistance(const Vector& point, const Vector& mins, const Vector& maxs)
    {
        if (!Finite(point) || !Finite(mins) || !Finite(maxs) || mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z)
            return INFINITY;
        const Vector closest(std::clamp(point.x, mins.x, maxs.x), std::clamp(point.y, mins.y, maxs.y),
            std::clamp(point.z, mins.z, maxs.z));
        return (point - closest).Length();
    }
}
