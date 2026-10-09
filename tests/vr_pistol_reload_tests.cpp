#include "../L4D2VR/vr_pistol_reload.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(v) do { if (!(v)) { std::fprintf(stderr, "Pistol transaction failed at line %d\n", __LINE__); std::abort(); } } while(false)
static void CheckSingleGestureReload(int startingClip, bool infiniteReserve)
{
    using namespace l4d2vr_pistol_reload;
    Server server; server.Reset(123u);
    Client client; client.Offer(123u);
    l4d2vr_pistol::AmmoLedger ammo;
    Snapshot native{true, 0x3005u, 200u, {10u, 50u, 2u, 3u, startingClip, false}, 40, infiniteReserve};
    std::uint64_t now = 1000u;
    std::uint32_t sequence = 0u;
    auto act = [&](Action action)
    {
        now += 100u; ++native.command;
        Request request{};
        CHECK(client.Begin(native.handle, native.ammo.clip, native.reserve, Hand::Right,
            action, native.command, native.ammo.owner, now, request));
        int nextClip = native.ammo.clip, nextReserve = native.reserve;
        const auto reply = server.Apply(request, native, now, ammo,
            [&](int clip, int reserve) { nextClip = clip; nextReserve = reserve; return true; });
        client.Receive(reply);
        l4d2vr_pistol_sync::State state{};
        Reply selected{};
        if (reply.status == Status::Applied)
        {
            // A reply alone cannot finish the gesture before its native replica.
            CHECK(client.Update(native.ammo.owner, native.handle, now + 1u, state, true, selected) == Client::Poll::Waiting);
            native.ammo.clip = nextClip; native.reserve = nextReserve;
            int right = -1, left = -1;
            l4d2vr_pistol::MagazineState magazines{};
            CHECK(ammo.Counts(native.ammo, right, left) && ammo.MagazineInfo(native.ammo, magazines));
            state = {123u, ++sequence, native.handle, native.command, 15, nextClip,
                right, left, true, false, magazines.physical, magazines.attached,
                magazines.chambered, request.sequence};
            CHECK(state.ValidContents());
            CHECK(client.Update(native.ammo.owner, native.handle, now + 2u, state, false, selected) == Client::Poll::Waiting);
        }
        CHECK(client.Update(native.ammo.owner, native.handle, now + 3u, state, true, selected) == Client::Poll::Result);
        CHECK(selected.request == request);
        return selected;
    };
    auto reply = act(Action::Eject);
    CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::Ejected);
    CHECK(native.ammo.clip == (startingClip > 0 ? 1 : 0) && native.reserve == 40);
    const int removed = reply.result.removed;
    CHECK(removed == (startingClip > 0 ? startingClip - 1 : 0));
    reply = act(Action::Cycle); // No chambering without an attached magazine.
    CHECK(reply.status == Status::NoSpaceOrAmmo && SettleGesture(reply, Hand::Right) == GestureOutcome::RetryCycle);
    reply = act(Action::Reinsert);
    CHECK(reply.status == Status::Applied && reply.result.added == removed);
    CHECK(native.ammo.clip == startingClip && native.reserve == 40);
    CHECK(SettleGesture(reply, Hand::Right) == (startingClip > 0 ? GestureOutcome::MagazineReady : GestureOutcome::NeedsCycle));
    reply = act(Action::Eject);
    CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::Ejected);
    reply = act(Action::Insert);
    CHECK(reply.status == Status::Applied && reply.result.released == removed);
    CHECK(native.ammo.clip == 15 && native.reserve == (infiniteReserve ? 40 : 40 - reply.result.added));
    CHECK(SettleGesture(reply, Hand::Right) == (startingClip > 0 ? GestureOutcome::MagazineReady : GestureOutcome::NeedsCycle));
    CHECK(ammo.BlocksUnchambered(native.ammo, Hand::Right) == (startingClip == 0));
    const int clipBeforeCycle = native.ammo.clip, reserveBeforeCycle = native.reserve;
    reply = act(Action::Cycle);
    CHECK(native.ammo.clip == clipBeforeCycle && native.reserve == reserveBeforeCycle);
    if (startingClip == 0)
    {
        CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::Cycled);
        CHECK(!ammo.BlocksUnchambered(native.ammo, Hand::Right));
    }
    else CHECK(reply.status == Status::NoSpaceOrAmmo); // A loaded chamber needs no slide stroke.
    CHECK(SettleGesture(reply, Hand::Left) == GestureOutcome::Cancel);
    reply.status = Status::Backend;
    CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::Cancel);
    reply.status = Status::Stale;
    CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::RetryCycle);
    reply.request.action = Action::Insert;
    CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::RetryMagazine);
    reply.request.action = Action::Eject;
    CHECK(SettleGesture(reply, Hand::Right) == GestureOutcome::Cancel);
}
int main()
{
    using namespace l4d2vr_pistol_reload;
    CheckSingleGestureReload(0, false);
    CheckSingleGestureReload(7, false);
    CheckSingleGestureReload(0, true);
    CheckSingleGestureReload(7, true);
    CHECK(BlocksNativeReload(true, 1u));
    CHECK(BlocksNativeReload(true, 2u));
    CHECK(BlocksNativeReload(true, 3u));
    CHECK(!BlocksNativeReload(false, 3u));
    CHECK(!BlocksNativeReload(true, 0u));
    CHECK(!BlocksNativeReload(true, 4u));
    Server server; server.Reset(42u);
    l4d2vr_pistol::AmmoLedger ammo;
    Snapshot snapshot{true, 0x3005u, 100u, {10u, 50u, 2u, 3u, 30, true}, 30, false};
    Request request{42u, 1u, 0x3005u, 100u, 30, 30, Hand::Right, Action::Eject};
    int writes = 0;
    auto writer = [&](int, int) { ++writes; return true; };
    auto reply = server.Apply(request, snapshot, 1000u, ammo, writer);
    CHECK(reply.status == Status::Applied && writes == 1 && reply.result.clip == 16 && reply.result.removed == 14);
    CHECK(server.Apply(request, snapshot, 1010u, ammo, writer).status == Status::Applied && writes == 1);
    auto changed = request; changed.action = Action::Insert;
    CHECK(server.Apply(changed, snapshot, 1020u, ammo, writer).status == Status::Stale && writes == 1);
    snapshot.ammo.clip = 16; request.sequence = 2u; request.clip = 16; request.hand = Hand::Left;
    reply = server.Apply(request, snapshot, 1020u, ammo, writer);
    CHECK(reply.status == Status::Applied && writes == 2 && reply.result.clip == 2); // other hand not globally throttled
    snapshot.ammo.clip = 2; request.sequence = 3u; request.clip = 2; request.hand = Hand::Right; request.action = Action::Insert;
    CHECK(server.Apply(request, snapshot, 1040u, ammo, writer).status == Status::RateLimited && writes == 2);
    request.sequence = 4u;
    reply = server.Apply(request, snapshot, 1100u, ammo, writer);
    CHECK(reply.status == Status::Applied && writes == 3 && reply.result.clip == 16 && reply.result.reserve == 16);
    snapshot.ammo.clip = 16; snapshot.reserve = 16;
    request.sequence = 5u; request.clip = 16; request.reserve = 16; request.action = Action::Cycle;
    CHECK(server.Apply(request, snapshot, 1200u, ammo, writer).status == Status::NoSpaceOrAmmo); // already chambered
    request.sequence = 6u; request.action = Action::Eject;
    CHECK(server.Apply(request, snapshot, 1200u, ammo, [](int, int) { return false; }).status == Status::Backend);
    request.sequence = 7u; request.handle = 0x4005u;
    CHECK(server.Apply(request, snapshot, 1300u, ammo, writer).status == Status::Weapon);
    request.sequence = 8u; request.handle = 0x3005u; request.command = 1u;
    CHECK(server.Apply(request, snapshot, 1300u, ammo, writer).status == Status::Stale);
    request.sequence = 9u; request.command = 109u;
    CHECK(server.Apply(request, snapshot, 1300u, ammo, writer).status == Status::Stale);
    request.sequence = 10u; request.command = 101u;
    CHECK(server.Apply(request, snapshot, 1300u, ammo, writer).status == Status::Stale);
    request.sequence = 11u; request.command = 100u; request.reserve = 0;
    CHECK(server.Apply(request, snapshot, 1300u, ammo, writer).status == Status::Stale);

    Client client; client.Offer(42u);
    Request outgoing{};
    CHECK(client.Begin(0x3005u, 30, 30, Hand::Right, Action::Eject, 100u, 10u, 1000u, outgoing));
    CHECK(!client.Begin(0x3005u, 30, 30, Hand::Left, Action::Eject, 100u, 10u, 1000u, outgoing));
    Reply accepted{outgoing, Status::Applied, {16, 30, 14, 0, 0, {1u, 2u, 3u}}};
    auto forged = accepted; ++forged.request.sequence; client.Receive(forged);
    l4d2vr_pistol_sync::State state{42u, 1u, 0x3005u, 100u, 15, 16, 1, 15, true, true, 1u, 2u, 3u, 1u};
    Reply selected{};
    CHECK(client.Update(10u, 0x3005u, 1050u, state, true, selected) == Client::Poll::Waiting);
    forged = accepted; forged.result.clip = 17; client.Receive(forged);
    CHECK(client.Update(10u, 0x3005u, 1050u, state, true, selected) == Client::Poll::Waiting);
    client.Receive(accepted);
    CHECK(client.Update(10u, 0x3005u, 1050u, state, false, selected) == Client::Poll::Waiting);
    state.reloadSequence = 0u;
    CHECK(client.Update(10u, 0x3005u, 1050u, state, true, selected) == Client::Poll::Waiting);
    state.reloadSequence = 1u;
    CHECK(client.Update(10u, 0x3005u, 1050u, state, true, selected) == Client::Poll::Result && selected.result.removed == 14);
    CHECK(client.Begin(0x3005u, 16, 30, Hand::Left, Action::Eject, 101u, 10u, 1100u, outgoing));
    CHECK(client.Update(20u, 0x3005u, 1150u, state, true, selected) == Client::Poll::None);
    CHECK(client.Begin(0x3005u, 16, 30, Hand::Left, Action::Eject, 102u, 10u, 1200u, outgoing));
    CHECK(client.Update(10u, 0x3005u, 3700u, state, true, selected) == Client::Poll::Timeout);
    CHECK(client.Blocked(42u) && !client.Blocked(99u));
    client.Offer(42u);
    CHECK(!client.Begin(0x3005u, 16, 30, Hand::Left, Action::Eject, 102u, 10u, 3800u, outgoing));
    client.Offer(99u);
    CHECK(client.Begin(0x3005u, 16, 30, Hand::Left, Action::Eject, 102u, 10u, 3800u, outgoing));
    client.Disconnect(); CHECK(client.Update(10u, 0x3005u, 3800u, state, true, selected) == Client::Poll::None);
    std::puts("Independent pistol transaction deduplication and settlement checks passed");
}
