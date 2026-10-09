#include "../L4D2VR/vr_pistol_ammo.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(value) do { if (!(value)) { std::fprintf(stderr, "Pistol magazine failed at line %d\n", __LINE__); std::abort(); } } while(false)

int main()
{
    using namespace l4d2vr_pistol;
    AmmoLedger ledger;
    AmmoSnapshot gun{10u, 50u, 2u, 3u, 30, true};
    int clip = 30, reserve = 30, writes = 0;
    auto writer = [&](int nextClip, int nextReserve) { clip = nextClip; reserve = nextReserve; ++writes; return true; };
    MagazineResult result{};
    CHECK(ledger.Observe(gun));
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Eject, reserve, false, writer, result));
    CHECK(clip == 16 && reserve == 30 && result.removed == 14);
    CHECK(result.state.physical == 1u && result.state.attached == 2u && result.state.chambered == 3u);
    gun.clip = clip;
    CHECK(!ledger.Magazine(gun, Hand::Right, MagazineAction::Eject, reserve, false, writer, result) && writes == 1);
    auto after = gun; --after.clip;
    CHECK(ledger.Shot(gun, after, 1, Hand::Right)); gun = after; clip = gun.clip;
    CHECK(ledger.BlocksUnchambered(gun, Hand::Right));
    CHECK(!ledger.BlocksUnchambered(gun, Hand::Left));
    CHECK(ledger.Magazine(gun, Hand::Left, MagazineAction::Eject, reserve, false, writer, result));
    CHECK(clip == 1 && result.removed == 14 && result.state.leftDetached == 14);
    gun.clip = clip;
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Insert, reserve, false, writer, result));
    CHECK(clip == 16 && reserve == 15 && result.added == 15 && result.released == 14);
    gun.clip = clip;
    CHECK(ledger.BlocksUnchambered(gun, Hand::Right)); // insertion cannot chamber a round
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Cycle, reserve, false, writer, result));
    CHECK(clip == 16 && reserve == 15 && !ledger.BlocksUnchambered(gun, Hand::Right));
    CHECK(!ledger.Magazine(gun, Hand::Right, MagazineAction::Cycle, reserve, false, writer, result));
    CHECK(ledger.Magazine(gun, Hand::Left, MagazineAction::Reinsert, reserve, false, writer, result));
    CHECK(clip == 30 && reserve == 15 && result.added == 14);
    gun.clip = clip;
    int right = -1, left = -1;
    CHECK(ledger.Counts(gun, right, left) && right == 15 && left == 15);

    // A failed native writer cannot change either pistol's magazine state.
    MagazineState before{}, unchanged{};
    CHECK(ledger.MagazineInfo(gun, before));
    CHECK(!ledger.Magazine(gun, Hand::Left, MagazineAction::Eject, reserve, false,
        [](int, int) { return false; }, result));
    CHECK(ledger.MagazineInfo(gun, unchanged) && unchanged.attached == before.attached &&
        unchanged.chambered == before.chambered && unchanged.leftDetached == before.leftDetached);
    CHECK(ledger.Counts(gun, right, left) && right == 15 && left == 15);

    // Per-hand chambers and removed contents survive retaining that single gun.
    CHECK(ledger.Magazine(gun, Hand::Left, MagazineAction::Eject, reserve, false, writer, result));
    gun.clip = clip;
    auto single = gun; single.dual = false; single.clip = 1;
    CHECK(ledger.Single(single, Hand::Left));
    after = single; after.clip = 0;
    CHECK(ledger.Shot(single, after, 2, Hand::Left)); single = after; clip = 0;
    CHECK(ledger.BlocksUnchambered(single, Hand::Left));
    CHECK(ledger.Magazine(single, Hand::Left, MagazineAction::Reinsert, reserve, false, writer, result));
    CHECK(clip == 14 && result.added == 14 && reserve == 15);
    single.clip = clip;
    CHECK(ledger.BlocksUnchambered(single, Hand::Left));
    CHECK(ledger.Magazine(single, Hand::Left, MagazineAction::Cycle, reserve, false, writer, result));
    CHECK(!ledger.BlocksUnchambered(single, Hand::Left));
    after = single; --after.clip;
    CHECK(ledger.Shot(single, after, 3, Hand::Left));
    CHECK(!ledger.BlocksUnchambered(after, Hand::Left)); // attached magazine auto feeds after firing
    CHECK(!ledger.Magazine(after, Hand::Right, MagazineAction::Eject, reserve, false, writer, result));

    // Native/external ammo changes revoke inferred physical state, and unknown
    // partial pairs never become a guessed exact partition through a reload.
    ledger.Reset(); gun.clip = 10;
    CHECK(!ledger.Magazine(gun, Hand::Right, MagazineAction::Eject, 30, false, writer, result));
    gun.clip = 30; CHECK(ledger.Observe(gun));
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Eject, 0, true, writer, result));
    gun.clip = clip;
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Insert, 0, true, writer, result));
    CHECK(clip == 30 && reserve == 0 && result.added == 14);
    gun.clip = 29; CHECK(ledger.Observe(gun));
    CHECK(!ledger.BlocksUnchambered(gun, Hand::Right));

    ledger.Reset(); gun.capacity = 8; gun.clip = 16; reserve = 3;
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Eject, reserve, false, writer, result));
    CHECK(clip == 9 && result.removed == 7); gun.clip = clip;
    CHECK(ledger.Magazine(gun, Hand::Right, MagazineAction::Insert, reserve, false, writer, result));
    CHECK(clip == 12 && reserve == 0 && result.added == 3);
    gun.clip = clip;
    CHECK(!ledger.Magazine(gun, Hand::Left, static_cast<MagazineAction>(4u), reserve, false, writer, result));
    CHECK(!ledger.Magazine(gun, Hand::Left, MagazineAction::Eject, -1, false, writer, result));
    CHECK(!ledger.Magazine(gun, Hand::None, MagazineAction::Eject, reserve, false, writer, result));
    ++gun.weaponSerial;
    CHECK(!ledger.Magazine(gun, Hand::Left, MagazineAction::Eject, reserve, false, writer, result));
    std::puts("Independent pistol magazine/chamber conservation checks passed");
}
