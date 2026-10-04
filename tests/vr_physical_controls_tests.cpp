#include "../L4D2VR/vr_physical_controls.h"
#include "../L4D2VR/vr_grip_release.h"
#include <cstdlib>
#include <cstdio>
#include <limits>

static void CheckImpl(bool result, int line)
{
    if (!result) { std::fprintf(stderr, "Check failed at line %d\n", line); std::abort(); }
}
#define Check(result) CheckImpl((result), __LINE__)

int main()
{
    using namespace l4d2vr_physical;
    Check(MeleeSweepSamples(0.0f, 0.0f, 40.0f) == 0);
    Check(MeleeSweepSamples(4.0f, 0.0f, 40.0f) == 5);
    Check(MeleeSweepSamples(0.0f, 80.0f, 40.0f) == 8);
    Check(MeleeSweepSamples(19.0f, 180.0f, 40.0f) == 12);
    Check(MeleeSweepSamples(21.0f, 30.0f, 40.0f) == 0);
    Check(MeleeSweepSamples(std::numeric_limits<float>::infinity(), 1.0f, 40.0f) == 0);
    ContactLatch contact;
    Check(!contact.Update(true, true, false));
    Check(contact.Update(true, true, true));
    Check(!contact.Update(true, true, true));
    Check(!contact.Update(false, true, true));
    Check(contact.Update(true, true, true));

    PickupIntent pickup;
    Check(pickup.Update(true, true, true, true, 100u, 10u) == PickupIntent::RequestUse);
    Check(pickup.Update(true, true, true, true, 110u, 10u) == 0u);
    Check(pickup.Update(true, true, true, false, 150u, 20u) == PickupIntent::AdoptWeapon);
    Check(pickup.Update(true, true, true, true, 160u, 20u) == 0u);
    Check(pickup.Update(true, true, false, true, 170u, 20u) == 0u);
    Check(pickup.Update(true, true, true, true, 180u, 20u) == PickupIntent::RequestUse);
    Check(pickup.Update(true, true, true, true, 900u, 30u) == 0u);
    Check(pickup.Update(false, true, true, true, 901u, 30u) == 0u);
    pickup.Begin(0xfffffff0u, 10u);
    Check(pickup.Update(true, true, true, false, 20u, 20u) == PickupIntent::AdoptWeapon);

    l4d2vr_grip::ReleaseLatch release;
    release.AdoptWeapon(20u);
    Check(!release.Update(true, true, true, 20u));
    Check(release.Update(true, true, false, 20u));
    Check(!release.Update(true, true, false, 20u));
    Check(!release.Update(true, true, true, 30u));
    Check(!release.Update(true, true, false, 30u));

    PumpCycles pumps;
    Check(!pumps.Observe(10u, 3, 8));
    Check(!pumps.Observe(10u, 3, 7)); // ammo corrections are not shot events
    pumps.NotifyShot(10u, 3, 7);
    Check(pumps.Observe(10u, 3, 7));
    Check(!pumps.Observe(20u, 8, 8));
    Check(pumps.Observe(10u, 3, 8)); // loading and switching cannot bypass cycling
    Check(!PumpCycles::StrokeComplete(false, 0.0f, 0.02f));
    Check(!PumpCycles::StrokeComplete(true, 0.08f, 0.02f));
    Check(PumpCycles::StrokeComplete(true, 0.01f, 0.02f));
    Check(!PumpCycles::StrokeComplete(true, std::numeric_limits<float>::quiet_NaN(), 0.02f));
    pumps.Complete(10u);
    Check(!pumps.Observe(10u, 3, 8));
    pumps.NotifyShot(10u, 3, 0);
    Check(pumps.Observe(10u, 3, 0));
    pumps.Reset();
    Check(!pumps.Observe(10u, 3, 0));

    pumps.ObserveOwner(true, 100u);
    pumps.NotifyShot(10u, 3, 7);
    pumps.ObserveOwner(true, 100u); // normal interaction reset on a weapon swap
    Check(!pumps.Observe(20u, 2, 50));
    pumps.ObserveOwner(true, 100u);
    Check(pumps.Observe(10u, 3, 7)); // drawing the shotgun again still requires a stroke
    pumps.ObserveOwner(false, 100u); // downed/dead/VR interaction unavailable
    Check(!pumps.Observe(10u, 3, 7));
    pumps.ObserveOwner(true, 100u);
    pumps.NotifyShot(10u, 3, 6);
    pumps.ObserveOwner(true, 200u); // a replacement player owns a different inventory
    Check(!pumps.Observe(10u, 3, 6));
    pumps.ObserveOwner(true, 200u);
    pumps.NotifyShot(10u, 8, 5);
    pumps.Complete(10u);
    Check(!pumps.Observe(10u, 8, 5));
}
