#include "../L4D2VR/vr_magazine_policy.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

static void Check(bool result, const char* message)
{
    if (!result)
    {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

int main()
{
    using l4d2vr_magazine::ChamberRoundsAfterEject;
    using l4d2vr_magazine::ShouldEject;
    Check(!ShouldEject(true, false, true, 0, true), "Button mode must not automatically eject an empty magazine");
    Check(ShouldEject(true, true, true, 15, true), "Explicit release must eject a loaded magazine");
    Check(ShouldEject(true, true, true, 0, true), "Explicit release must eject an empty magazine");
    Check(!ShouldEject(true, true, true, 8, false), "A tube-fed shotgun must not eject a detachable magazine");
    Check(ShouldEject(false, false, true, 0, true), "Legacy empty-magazine handling remains available");
    Check(ChamberRoundsAfterEject(-1) == 0, "Unknown clip data must not create ammunition");
    Check(ChamberRoundsAfterEject(0) == 0, "An empty pistol must not gain an infinite-reserve chambered round");
    Check(ChamberRoundsAfterEject(1) == 1, "Removing a magazine must preserve the last existing chambered round");
    Check(ChamberRoundsAfterEject(15) == 1, "A tactical reload must preserve exactly one existing round");
    Check(ChamberRoundsAfterEject(30) == 1, "Clip capacity must not increase the retained chamber count");
    Check(!l4d2vr_magazine::ChamberReadyAfterEject(true, 1), "An ejection ACK cannot cycle a previously empty physical chamber");
    Check(l4d2vr_magazine::ChamberReadyAfterEject(false, 1), "An existing loaded chamber survives a tactical ejection ACK");
    Check(!l4d2vr_magazine::ChamberReadyAfterEject(false, 0), "The native empty result cannot preserve a chambered round");
    Check(!l4d2vr_magazine::ChamberReadyAfterEject(false, -1), "An unknown native count cannot ready the chamber");
    Check(l4d2vr_magazine::MayCatchEjectedMagazine(true, true, 0.04f, 0.05f), "Grip held under the well can catch an ejected magazine");
    Check(!l4d2vr_magazine::MayCatchEjectedMagazine(false, true, 0.0f, 0.05f), "Ejection alone cannot attach a magazine to the support hand");
    Check(!l4d2vr_magazine::MayCatchEjectedMagazine(true, false, 0.0f, 0.05f), "Lost tracking cannot catch an ejected magazine");
    Check(!l4d2vr_magazine::MayCatchEjectedMagazine(true, true, 0.06f, 0.05f), "A distant support hand cannot catch ammunition");
    Check(!l4d2vr_magazine::MayCatchEjectedMagazine(true, true, std::numeric_limits<float>::quiet_NaN(), 0.05f), "Invalid pose distances cannot attach ammunition");
    l4d2vr_magazine::RetainedInsertGate retained;
    Check(!retained.Update(true, 0.0f, 0.03f), "Catching at the well must not automatically reinsert the magazine");
    Check(!retained.Update(false, 0.01f, 0.03f), "A small overlap change is not a withdrawal gesture");
    Check(!retained.Update(true, 0.04f, 0.03f), "Travel alone must not arm reinsertion while the magazine remains in the well");
    Check(!retained.Update(false, 0.04f, 0.03f), "Withdrawing arms a later return without loading ammunition yet");
    Check(retained.Update(true, 0.01f, 0.03f), "A withdrawn magazine can return to its socket");
    retained.Reset();
    Check(!retained.Update(true, 0.04f, 0.03f), "Another caught magazine needs its own withdrawal gesture");
    using namespace l4d2vr_magazine;
    ChamberHistory chambers;
    chambers.ObserveOwner(true, 100u);
    Check(chambers.Observe(10u, 0x1001u, 1, 0), "Firing the last round must leave an empty chamber");
    Check(chambers.Observe(10u, 0x1001u, 1, 15), "Inserting or predicting a full magazine must not cycle its slide");
    Check(!chambers.Observe(20u, 0x1002u, 2, 50), "Drawing another loaded gun must not inherit that empty chamber");
    Check(chambers.Observe(10u, 0x1001u, 1, 15), "Drawing the pistol again must retain the unfinished cycle");
    chambers.Complete(20u);
    Check(chambers.Observe(10u, 0x1001u, 1, 15), "Cycling another gun cannot ready the pistol");
    chambers.Complete(10u);
    Check(!chambers.Observe(10u, 0x1001u, 1, 15), "A completed cycle readies only the current weapon");
    Check(chambers.Observe(10u, 0x1001u, 1, 0), "Spending the chamber again requires a new cycle");
    Check(!chambers.Observe(10u, 0x2001u, 1, 15), "A new native entity serial must not inherit deleted-object state");
    Check(chambers.Observe(30u, 0u, 1, 0), "A missing serial must still support the current physical reload");
    Check(chambers.Observe(30u, 0u, 1, 15), "Missing serial data must not automatically chamber an inserted magazine");
    chambers.Observe(40u, 0u, 19, -1);
    Check(!chambers.Observe(30u, 0u, 1, 15), "Unknown entity identities must not survive an intervening draw");
    Check(chambers.Observe(20u, 0x1002u, 2, 0), "The other weapon keeps its own chamber history");
    chambers.Forget(20u);
    Check(!chambers.Observe(20u, 0x1002u, 2, 50), "Native reload fallback must not retain a physical-only latch");
    chambers.Observe(10u, 0x2001u, 1, 0);
    chambers.ObserveOwner(true, 200u);
    Check(!chambers.Observe(10u, 0x2001u, 1, 15), "Replacing the player must release the previous inventory history");
    chambers.Observe(10u, 0x2001u, 1, 0);
    chambers.ObserveOwner(false, 200u);
    chambers.ObserveOwner(true, 200u);
    Check(!chambers.Observe(10u, 0x2001u, 1, 15), "Death, incap or disabled physical reload must clear ownership");

    // A tactical top-up can end after one shell. The cumulative number of
    // inserted shells does not keep firing blocked for the rest of the session.
    Check(ShotgunBlocksFire(false, true, false, false, false), "Holding a shell at the loading stage blocks firing");
    Check(ShotgunBlocksFire(false, false, false, false, ShellSettlementPending(100u, 1100u)),
        "A queued shell must settle before a shot can spend the predicted clip");
    Check(!ShotgunBlocksFire(false, false, false, false, ShellSettlementPending(200u, 0u)),
        "One confirmed shell in a partially filled tube must allow firing from the existing chamber");
    Check(ShotgunBlocksFire(true, false, true, false, false), "The resulting shot must wait for the pump cycle");
    Check(!ShotgunBlocksFire(false, false, false, false, false), "Completing the cycle must not require filling the tube");
    Check(ShotgunBlocksFire(true, false, false, false, false), "Loading a shell into an empty gun does not chamber it");
    Check(ShotgunBlocksFire(false, false, false, true, false), "An early slide cycle must still wait for backend ammo");
    Check(!ShellSettlementPending(1100u, 1100u), "A lost server settlement cannot retain a permanent firing lock");
    Check(!ShellSettlementPending(1200u, 1100u), "Expired settlements must remain unlocked");
    l4d2vr_magazine::NativeReloadLedger local, teammate;
    local.Observe(100, 10u, true);
    Check(local.Blocks(10u) && !teammate.Blocks(10u), "Physical reload suppression belongs only to its sender");
    local.Observe(101, 10u, false);
    local.Observe(100, 10u, true);
    Check(!local.Blocks(10u), "An older backup command must not cancel an authorized reload");
    local.Observe(102, 20u, true);
    Check(local.Blocks(20u) && !local.Blocks(10u), "Changing weapons must release the old weapon's reload gate");
    local.Observe(103, 0u, true);
    Check(!local.Blocks(20u) && !local.Blocks(0u), "Empty inventory must not retain a reload gate");
    local.Observe(104, 20u, true, false, 1u);
    local.Observe(105, 20u, false, true, 1u);
    Check(local.Blocks(20u), "A remote pull or grip action must not silently permit automatic reload");
    local.Observe(106, 30u, false, true, 1u);
    Check(!local.Blocks(20u) && !local.Blocks(30u), "A transient action on a new weapon must not inherit another weapon's gate");
    local.Observe(107, 30u, true, false, 1u);
    local.Observe(1, 40u, true, false, 2u);
    Check(local.Blocks(40u) && !local.Blocks(30u), "A replaced player must get a new command sequence and ownership");
    local.Observe(2, 40u, false, false, 2u);
    local.Observe(1, 40u, true, false, 2u);
    Check(!local.Blocks(40u), "Backup commands from the current player must still be rejected");
    local.Observe(200, 40u, true, false, 2u, 1u);
    local.Observe(1, 40u, false, false, 2u, 2u);
    Check(!local.Blocks(40u), "Reusing a player address with a new edict serial must clear the old reload gate");
    local.Observe(2, 50u, true, false, 2u, 2u);
    Check(local.Blocks(50u), "The replacement edict must accept its own command sequence");
}
