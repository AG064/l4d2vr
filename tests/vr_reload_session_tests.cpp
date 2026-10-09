#include "../L4D2VR/vr_magazine_policy.h"
#include "../L4D2VR/vr_physical_controls.h"
#include <cstdio>
#include <cstdlib>

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
    using namespace l4d2vr_magazine;
    SessionTracker pumpSession;
    l4d2vr_physical::PumpCycles pump;
    pumpSession.Observe(true, 501u, 601u, 3, 3u);
    pump.ObserveOwner(true, pumpSession.OwnerTag());
    pump.NotifyShot(601u, 3, 7);
    Check(pumpSession.Observe(true, 501u, 602u, 2, 3u), "Drawing an SMG resets only current hand interaction");
    pump.ObserveOwner(true, pumpSession.OwnerTag());
    pumpSession.Observe(true, 501u, 601u, 3, 3u);
    pump.ObserveOwner(true, pumpSession.OwnerTag());
    Check(pump.Observe(601u, 3, 7), "Interaction recovery must preserve the shotgun's outstanding pump cycle");
    pumpSession.Observe(false, 501u, 601u, 3, 3u);
    pump.ObserveOwner(false, pumpSession.OwnerTag());
    Check(!pump.Observe(601u, 3, 7), "Death or incapacitation must clear expired pump ownership");
    SessionTracker session;
    Check(!session.Observe(false, 0u, 0u, 0, 0u), "A disconnected session must remain idle");
    Check(session.Observe(true, 100u, 200u, 1, 3u), "Entering gameplay must discard state from the previous session");
    const unsigned int queuedGeneration = 1u;
    for (int reload = 0; reload < 10; ++reload)
    {
        Check(!session.Observe(true, 100u, 200u, 1, 3u), "Normal reload completion must not invalidate its pending ammunition update");
        Check(MayCommitAmmo(1, 1, 1, queuedGeneration, queuedGeneration), "A current reload may settle its actual ammunition");
    }
    Check(session.Observe(true, 100u, 201u, 1, 3u), "A same-type weapon swap must reset held magazine and support-hand ownership");
    Check(!MayCommitAmmo(1, 1, 1, queuedGeneration, 2u), "An old reload must not reach a different pistol of the same type");
    Check(session.Observe(false, 100u, 201u, 1, 3u), "Incapacitation must end the interaction session even while grip stays held");
    Check(!session.Observe(false, 100u, 201u, 1, 3u), "Remaining incapacitated must not repeatedly create new sessions");
    Check(session.Observe(true, 100u, 201u, 1, 3u), "Revival must begin with new interaction ownership");
    Check(session.Observe(true, 101u, 201u, 1, 3u), "Respawn or player replacement must invalidate old interactions");
    Check(session.Observe(true, 101u, 201u, 2, 3u), "A changed weapon type at the same address must reset ownership");
    Check(session.Observe(true, 101u, 201u, 2, 2u), "Disabling manual reload or changing hand mode must clear prior suppression");
    Check(session.Observe(true, 101u, 201u, 2, 3u), "Re-enabling manual reload must start a new session");
    Check(session.Observe(true, 101u, 0u, 2, 3u), "A missing active weapon must end the session");
    Check(!session.Observe(true, 101u, 0u, 2, 3u), "An invalid weapon must never become ready");
    Check(!session.Observe(true, 0u, 201u, 2, 3u), "An invalid player must never become ready");
    Check(!session.Observe(true, 101u, 201u, 0, 3u), "An unknown weapon must never become ready");

    Check(!MayCommitAmmo(1, 5, 1, 1u, 1u), "A matching client pistol must not authorize writing pistol ammo to a server rifle");
    Check(!MayCommitAmmo(1, 1, 5, 1u, 1u), "A matching server pistol must not authorize an update after the client switched weapons");
    Check(!MayCommitAmmo(0, 1, 1, 1u, 1u), "An update with no known weapon owner must fail");
    Check(!MayCommitAmmo(3, 0, 3, 1u, 1u), "An unresolved server weapon must never receive a shell update");
    Check(MayCommitAmmo(3, 3, 3, 4u, 4u), "A current shotgun shell update must remain valid");
    Check(!MayCommitAmmo(3, 8, 3, 4u, 4u), "A pump-shotgun update must not affect a Chrome shotgun");

    SessionTracker ammoSession;
    Check(ammoSession.Observe(true, 1u, 2u, 2, 3u), "A legacy local reload starts with its own session");
    const unsigned authoritativeMode = 3u | kAuthoritativeMagazineInputMode;
    Check(UseAuthoritativeMagazine(true, true, false, false), "A supported single gun uses native transactions on host and guest");
    Check(ammoSession.Observe(true, 1u, 2u, 2, authoritativeMode), "A late ammo capability must fence the old predicted reload");
    Check(!ammoSession.Observe(true, 1u, 2u, 2, authoritativeMode), "Local hook heartbeats must not move an authoritative reload back to prediction");
    Check(!MayCommitAmmo(2, 2, 2, 7u, 8u), "A previously queued local clip write cannot cross the backend boundary");
    const unsigned pistolMode = 3u | kAuthoritativePistolInputMode;
    Check(ammoSession.Observe(true, 1u, 2u, 2, pistolMode), "Pistol transactions fence the older magazine protocol");
    Check(!ammoSession.Observe(true, 1u, 2u, 2, pistolMode), "A pending pistol gesture retains its backend session");
    Check((kAuthoritativePistolInputMode & (kAuthoritativeMagazineInputMode | kAuthoritativeShellInputMode)) == 0u,
        "Pistol, magazine and shell session capabilities remain distinct");
    Check(ammoSession.Observe(true, 1u, 2u, 2, 3u), "Losing the transaction lease must cancel its pending hand state");
    Check(!UseAuthoritativeMagazine(false, true, false, false), "An absent capability cannot invent transaction support");
    Check(!UseAuthoritativeMagazine(true, true, true, false), "Native dual pistols keep their explicit shared reload fallback");
    Check(!UseAuthoritativeMagazine(true, true, false, true), "A pistol retained in the left hand keeps its native fallback");
    Check(!UseAuthoritativeMagazine(true, false, false, false), "A shell-fed gun cannot enter the detachable-magazine path");
    SessionTracker shellSession;
    Check(shellSession.Observe(true, 1u, 3u, 3, 3u), "An unsupported shotgun begins with native controls");
    const unsigned shellMode = 3u | kAuthoritativeShellInputMode;
    Check(shellSession.Observe(true, 1u, 3u, 3, shellMode), "Shell capability starts fresh physical reload ownership");
    Check(!shellSession.Observe(true, 1u, 3u, 3, shellMode), "Local server hooks do not change the native shell route");
    Check(shellSession.Observe(true, 1u, 3u, 3, 3u), "A shell timeout clears held-ammo and pump interaction state");
    Check((kAuthoritativeShellInputMode & kAuthoritativeMagazineInputMode) == 0u, "Shell and magazine session capabilities remain distinct");

    NativeFallbackPulse nativePulse;
    Check(!nativePulse.Update(true, false, 10u, 100u), "Unsupported servers must not reload until a button request");
    Check(nativePulse.Update(true, true, 10u, 200u), "Magazine Release requests stock reload without writing ammo");
    Check(nativePulse.Update(true, false, 10u, 549u), "The request must survive an input frame until Source samples it");
    Check(!nativePulse.Update(true, false, 10u, 550u), "A stock reload request has a bounded button hold");
    Check(nativePulse.Update(true, true, 10u, 600u), "Another deliberate press starts a new reload request");
    Check(!nativePulse.Update(true, false, 11u, 610u), "A gun/player/backend change cannot reload the newly selected gun");
    Check(nativePulse.Update(true, true, 11u, 620u), "The new session can accept its own button request");
    Check(!nativePulse.Update(false, false, 11u, 630u), "Menus and unavailable control cancel native fallback input");
    Check(!nativePulse.Update(true, false, 11u, 640u), "Returning to gameplay cannot resume a stale native reload pulse");
    Check(nativePulse.Update(true, true, 11u, 0xfffffff0u), "The pulse can start before a tick-count wrap");
    Check(nativePulse.Update(true, false, 11u, 20u), "Tick-count wrap does not cancel a current short request");
    nativePulse.Reset();
    Check(!nativePulse.Update(true, false, 11u, 30u), "Session reset requires a fresh request even with the same generation");

    Check(IsLocalPlayerCommand(1, 1), "The host's command may settle the host's reload");
    for (int player = 2; player <= 32; ++player)
        Check(!IsLocalPlayerCommand(player, 1), "Other players must never consume the host's queued ammunition update");
    Check(!IsLocalPlayerCommand(0, 0) && !IsLocalPlayerCommand(-1, -1), "Invalid player slots must not be treated as a local host");
}
