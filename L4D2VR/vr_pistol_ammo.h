#pragma once
#include "vr_pistol_detachment.h"
#include <array>

namespace l4d2vr_pistol
{
    struct AmmoSnapshot
    {
        std::uintptr_t owner = 0u, weapon = 0u;
        unsigned ownerSerial = 0u, weaponSerial = 0u;
        int clip = -1;
        bool dual = false;
        int capacity = 15;
        bool Valid() const { return owner && weapon && capacity > 0 && capacity <= 15 &&
            clip >= 0 && clip <= (dual ? capacity * 2 : capacity); }
        bool SameGun(const AmmoSnapshot& other) const
        {
            return owner == other.owner && ownerSerial == other.ownerSerial &&
                weapon == other.weapon && weaponSerial == other.weaponSerial;
        }
    };
    inline Hand Opposite(Hand hand)
    { return hand == Hand::Right ? Hand::Left : hand == Hand::Left ? Hand::Right : Hand::None; }

    // Native ammunition remains authoritative. A partial pair without a known
    // pickup partition uses the existing balanced split, never invented rounds.
    class AmmoLedger
    {
    public:
        void Reset() { *this = {}; }
        bool Observe(const AmmoSnapshot& snapshot)
        {
            if (!snapshot.Valid()) return false;
            Scope(snapshot);
            Entry& entry = Find(snapshot);
            if (entry.dual != snapshot.dual || entry.capacity != snapshot.capacity || entry.right + entry.left != snapshot.clip)
            {
                const bool wasDual = entry.dual;
                entry.dual = snapshot.dual;
                entry.capacity = snapshot.capacity;
                entry.exact = !snapshot.dual || snapshot.clip == 0 || snapshot.clip == snapshot.capacity * 2;
                if (snapshot.dual)
                { entry.right = snapshot.clip / 2; entry.left = snapshot.clip - entry.right; }
                else
                {
                    if (wasDual) entry.singleHand = Hand::Right;
                    entry.right = entry.singleHand == Hand::Right ? snapshot.clip : 0;
                    entry.left = entry.singleHand == Hand::Left ? snapshot.clip : 0;
                }
            }
            return true;
        }
        bool Single(const AmmoSnapshot& snapshot, Hand hand)
        {
            if (snapshot.dual || Opposite(hand) == Hand::None || !Observe(snapshot)) return false;
            Entry& entry = Find(snapshot);
            entry.singleHand = hand;
            entry.right = hand == Hand::Right ? snapshot.clip : 0;
            entry.left = hand == Hand::Left ? snapshot.clip : 0;
            entry.exact = true;
            return true;
        }
        bool Joined(const AmmoSnapshot& pair, int held, int incoming, Hand pickup)
        {
            int total = -1;
            if (!pair.dual || !pair.Valid() || !JoinAmmo(held, incoming, total) || held > pair.capacity ||
                incoming > pair.capacity || pair.clip != total ||
                (pickup != Hand::None && Opposite(pickup) == Hand::None)) return false;
            Scope(pair);
            Entry& entry = Find(pair);
            const Hand heldHand = pickup == Hand::None ? entry.singleHand : Opposite(pickup);
            entry.right = heldHand == Hand::Right ? held : incoming;
            entry.left = heldHand == Hand::Left ? held : incoming;
            entry.dual = entry.exact = true;
            entry.capacity = pair.capacity;
            return true;
        }
        bool Shot(const AmmoSnapshot& before, const AmmoSnapshot& after, int command, Hand hand,
            int rounds = 1, unsigned ordinal = 0u)
        {
            if (!before.Valid() || !after.Valid() || !before.SameGun(after) || before.dual != after.dual ||
                before.capacity != after.capacity ||
                command <= 0 || Opposite(hand) == Hand::None || rounds <= 0 || rounds > before.capacity ||
                before.clip - after.clip != rounds)
                return false;
            Scope(before);
            Entry& entry = Find(before);
            if (command < entry.lastShot || (command == entry.lastShot && ordinal <= entry.lastOrdinal))
            { Observe(after); return false; }
            Observe(before);
            entry.lastShot = command;
            entry.lastOrdinal = ordinal;
            if (!before.dual)
                return Single(after, hand);
            int& selected = hand == Hand::Right ? entry.right : entry.left;
            if (!entry.exact || selected < rounds) { Observe(after); return false; }
            selected -= rounds;
            return entry.right + entry.left == after.clip;
        }
        bool ObserveFire(const AmmoSnapshot& before, const AmmoSnapshot& after, int command, Hand hand,
            unsigned firstBullet, unsigned lastBullet)
        {
            if (!before.Valid() || !after.Valid() || !before.SameGun(after)) return false;
            if (lastBullet > firstBullet && lastBullet - firstBullet <= static_cast<unsigned>(before.capacity) &&
                Shot(before, after, command, hand, static_cast<int>(lastBullet - firstBullet), firstBullet + 1u))
                return true;
            Observe(after);
            return false;
        }
        bool Split(const AmmoSnapshot& pair, Hand drop, AmmoSplit& result, bool& exact)
        {
            exact = false;
            if (!pair.dual || Opposite(drop) == Hand::None || !Observe(pair)) return false;
            Entry& entry = Find(pair);
            if (!entry.exact) return SplitAmmo(pair.clip, result);
            result = drop == Hand::Right ? AmmoSplit{entry.left, entry.right} : AmmoSplit{entry.right, entry.left};
            exact = true;
            return result.retained + result.dropped == pair.clip;
        }
    private:
        struct Entry
        {
            std::uintptr_t weapon = 0u;
            unsigned serial = 0u;
            unsigned lastOrdinal = 0u;
            int right = 0, left = 0, lastShot = 0, capacity = 15;
            bool dual = false, exact = true;
            Hand singleHand = Hand::Right;
        };
        void Scope(const AmmoSnapshot& snapshot)
        {
            if (snapshot.owner != m_Owner || snapshot.ownerSerial != m_Serial)
            { Reset(); m_Owner = snapshot.owner; m_Serial = snapshot.ownerSerial; }
        }
        Entry& Find(const AmmoSnapshot& snapshot)
        {
            for (auto& entry : m_Entries)
                if (entry.weapon == snapshot.weapon && entry.serial == snapshot.weaponSerial) return entry;
            Entry& entry = m_Entries[m_Next++ % m_Entries.size()];
            entry = {}; entry.weapon = snapshot.weapon; entry.serial = snapshot.weaponSerial;
            return entry;
        }
        std::uintptr_t m_Owner = 0u;
        unsigned m_Serial = 0u, m_Next = 0u;
        std::array<Entry, 16> m_Entries{};
    };
}
