#include "../L4D2VR/vr_remote_magazines.h"
#include <cstdio>
#include <cstdlib>

static void CheckImpl(bool okay, int line)
{
    if (!okay) { std::fprintf(stderr, "Remote magazine check failed at line %d\n", line); std::abort(); }
}
#define Check(value) CheckImpl((value), __LINE__)

int main()
{
    using namespace l4d2vr_remote_mag;
    const std::uint32_t handle = (3u << 12) | 27u;
    ServerLedger server; server.Reset(10u);
    Snapshot state{true, handle, 200u, 2, 23, 7, 50, false};
    Request eject{10u, 1u, handle, 200u, 2, 23, 7, Action::Eject};
    int writes = 0;
    auto writer = [&](int clip, int reserve) { ++writes; state.clip = clip; state.reserve = reserve; return true; };
    auto out = server.Apply(eject, state, 1000u, writer);
    Check(out.status == Status::Applied && state.clip == 1 && state.reserve == 7 && writes == 1);
    Check(server.Apply(eject, state, 1200u, writer).clip == 1 && writes == 1);
    Request altered = eject; altered.action = Action::Insert;
    Check(server.Apply(altered, state, 1300u, writer).status == Status::Stale && writes == 1);
    // A fresh magazine with only seven reserve rounds is partially loaded.
    Request insert{10u, 2u, handle, 200u, 2, 1, 7, Action::Insert};
    out = server.Apply(insert, state, 1400u, writer);
    Check(out.status == Status::Applied && state.clip == 8 && state.reserve == 0 && writes == 2);
    Check(server.Apply(insert, state, 1600u, writer).status == Status::Applied && writes == 2);
    insert.sequence = 3; insert.clip = state.clip; insert.reserve = state.reserve;
    Check(server.Apply(insert, state, 1700u, writer).status == Status::Weapon && writes == 2);

    // The listen-host uses this same path even while its local weapon hooks
    // are active. A shot spending the retained chamber is real native ammo.
    server.Reset(40u); state = {true, handle, 300u, 2, 20, 2, 50, false};
    eject = {40u, 1u, handle, 300u, 2, 20, 2, Action::Eject};
    Check(server.Apply(eject, state, 1800u, writer).status == Status::Applied && state.clip == 1);
    state.clip = 0; // native shot between ejection and insertion
    insert = {40u, 2u, handle, 300u, 2, 1, 2, Action::Insert};
    const int beforeShot = writes;
    Check(server.Apply(insert, state, 2000u, writer).status == Status::Stale && writes == beforeShot);
    insert.sequence = 3; insert.clip = 0;
    Check(server.Apply(insert, state, 2200u, writer).status == Status::Applied && state.clip == 2 && state.reserve == 0);

    // An empty gun keeps no chambered round. Infinite ammo comes from the host,
    // not from the weapon ID or a client-controlled request flag.
    server.Reset(11u); state = {true, handle, 200u, 1, 0, 0, 15, false};
    eject = {11u, 1u, handle, 200u, 1, 0, 0, Action::Eject};
    Check(server.Apply(eject, state, 2000u, writer).status == Status::Applied && state.clip == 0);
    insert = {11u, 2u, handle, 200u, 1, 0, 0, Action::Insert};
    Check(server.Apply(insert, state, 2200u, writer).status == Status::NoSpaceOrAmmo && state.clip == 0);
    state.infinite = true; insert.sequence = 3;
    out = server.Apply(insert, state, 2400u, writer);
    Check(out.status == Status::Applied && state.clip == 15 && state.reserve == 0);

    // Custom capacities, including M60-sized clips, use native limits.
    server.Reset(12u); state = {true, handle, 200u, 37, 120, 149, 150, false};
    eject = {12u, 1u, handle, 200u, 37, 120, 149, Action::Eject};
    Check(server.Apply(eject, state, 3000u, writer).status == Status::Applied);
    insert = {12u, 2u, handle, 200u, 37, 1, 149, Action::Insert};
    Check(server.Apply(insert, state, 3200u, writer).status == Status::Applied && state.clip == 150 && state.reserve == 0);

    // Native changes, stale commands, recycled handles and invalid owners
    // cannot be overwritten by an insertion from an earlier physical gesture.
    server.Reset(13u); state = {true, handle, 200u, 32, 0, 16, 8, false};
    eject = {13u, 1u, handle, 200u, 32, 0, 16, Action::Eject};
    Check(server.Apply(eject, state, 4000u, writer).status == Status::Applied);
    insert = {13u, 2u, handle, 200u, 32, 0, 16, Action::Insert};
    state.reserve = 15;
    Check(server.Apply(insert, state, 4200u, writer).status == Status::Stale);
    insert.sequence = 3; insert.reserve = 15; insert.command = 151u;
    Check(server.Apply(insert, state, 4300u, writer).status == Status::Stale);
    insert.sequence = 4; insert.command = 209u;
    Check(server.Apply(insert, state, 4400u, writer).status == Status::Stale);
    insert.sequence = 5; insert.command = 200u; insert.handle += 4096u;
    Check(server.Apply(insert, state, 4500u, writer).status == Status::Weapon);
    insert.sequence = 6; insert.handle = handle; state.eligible = false;
    Check(server.Apply(insert, state, 4600u, writer).status == Status::Weapon);
    state.eligible = true; insert.sequence = 7;
    const int beforeFailed = writes;
    Check(server.Apply(insert, state, 4700u, [](int, int) { return false; }).status == Status::Backend);
    Check(server.Apply(insert, state, 4800u, writer).status == Status::Backend && writes == beforeFailed);
    insert.sequence = 8;
    Check(server.Apply(insert, state, 4900u, writer).status == Status::Applied && state.clip == 8 && state.reserve == 7);

    // Leaving and redrawing an ejected weapon can repeat ejection safely.
    eject.sequence = 9; eject.clip = 8; eject.reserve = 7;
    Check(server.Apply(eject, state, 5100u, writer).status == Status::Applied && state.clip == 1);
    eject.sequence = 10; eject.clip = 1;
    Check(server.Apply(eject, state, 5300u, writer).status == Status::Applied && state.clip == 1 && state.reserve == 7);
    insert.sequence = 11; insert.clip = 1; insert.reserve = 7;
    Check(server.Apply(insert, state, 5350u, writer).status == Status::RateLimited);
    insert.sequence = 12;
    Check(server.Apply(insert, state, 5500u, writer).status == Status::Applied && state.clip == 8 && state.reserve == 0);

    // ACK delivery and Source replication can arrive in either order. Neither
    // one alone advances the physical reload to its next phase.
    ClientRequest client; Reply reply{}; Request request{};
    client.Offer(kVersion, 20u);
    Check(client.Begin(handle, 2, 23, 7, Action::Eject, 200u, 100u, 4u, 6000u, request));
    Check(!client.Begin(handle, 2, 23, 7, Action::Eject, 200u, 100u, 4u, 6001u, request));
    Check(client.Update(handle, 2, 100u, 4u, 6100u, reply, 1, 7) == ClientRequest::Poll::Waiting);
    Reply ack{request, Status::Applied, 1, 7};
    auto wrong = ack; ++wrong.request.sequence; client.Receive(wrong);
    Check(client.Update(handle, 2, 100u, 4u, 6200u, reply, 1, 7) == ClientRequest::Poll::Waiting);
    client.Receive(ack);
    Check(client.Update(handle, 2, 100u, 4u, 6300u, reply, 23, 7) == ClientRequest::Poll::Waiting);
    Check(client.Update(handle, 2, 100u, 4u, 6400u, reply, 1, 7) == ClientRequest::Poll::Result);
    Check(client.Begin(handle, 2, 1, 7, Action::Insert, 210u, 100u, 4u, 6500u, request));
    ack = {request, Status::Applied, 8, 0}; client.Receive(ack);
    Check(client.Update(handle, 2, 100u, 4u, 6600u, reply, 8, 7) == ClientRequest::Poll::Waiting);
    Check(client.Update(handle, 2, 100u, 4u, 6700u, reply, 8, 0) == ClientRequest::Poll::Result);
    // Invalid transfers cannot advance the client. Infinite reserve remains
    // unchanged only when the host's applied result confirms it.
    Check(client.Begin(handle, 1, 0, 0, Action::Insert, 211u, 100u, 4u, 6800u, request));
    ack = {request, Status::Applied, 15, 1}; client.Receive(ack);
    Check(client.Update(handle, 1, 100u, 4u, 6900u, reply, 15, 1) == ClientRequest::Poll::Waiting);
    ack.reserve = 0; client.Receive(ack);
    Check(client.Update(handle, 1, 100u, 4u, 7000u, reply, 15, 0) == ClientRequest::Poll::Result);
    Check(client.Begin(handle, 2, 0, 10, Action::Insert, 212u, 100u, 4u, 7100u, request));
    Check(client.Update(handle, 2, 101u, 4u, 7200u, reply, 0, 10) == ClientRequest::Poll::None && !client.Pending());
    Check(client.Begin(handle, 2, 0, 10, Action::Insert, 213u, 100u, 4u, 7300u, request));
    Check(client.Update(handle, 2, 100u, 5u, 7400u, reply, 0, 10) == ClientRequest::Poll::None);
    Check(client.Begin(handle, 2, 0, 10, Action::Insert, 214u, 100u, 5u, 7500u, request));
    Check(client.Update(handle, 2, 100u, 5u, 10000u, reply, 0, 10) == ClientRequest::Poll::Timeout && !client.Supported());
    client.Offer(kVersion, 20u); Check(!client.Supported());
    client.Offer(kVersion, 21u); Check(client.Supported());
    Check(client.Begin(handle, 2, 0, 10, Action::Insert, 215u, 100u, 5u, 10100u, request));
    ack = {request, Status::Unsupported, 0, 0}; client.Receive(ack);
    Check(client.Update(handle, 2, 100u, 5u, 10200u, reply, 0, 10) == ClientRequest::Poll::Result && !client.Supported());
    client.Disconnect(); client.Offer(kVersion + 1u, 22u); Check(!client.Supported());
    // Bound the server's remembered dropped guns without allowing insertion
    // into an evicted entry. Redrawing and ejecting it restores that phase.
    server.Reset(30u);
    for (unsigned i = 0; i < 17u; ++i)
    {
        state = {true, handle + i, 300u, 2, 0, 10, 50, false};
        eject = {30u, i + 1u, state.handle, 300u, 2, 0, 10, Action::Eject};
        Check(server.Apply(eject, state, 11000u + i * 200u, writer).status == Status::Applied);
    }
    state = {true, handle, 300u, 2, 0, 10, 50, false};
    insert = {30u, 18u, handle, 300u, 2, 0, 10, Action::Insert};
    Check(server.Apply(insert, state, 15000u, writer).status == Status::Weapon);
    eject = {30u, 19u, handle, 300u, 2, 0, 10, Action::Eject};
    Check(server.Apply(eject, state, 15200u, writer).status == Status::Applied);
    insert.sequence = 20u;
    Check(server.Apply(insert, state, 15400u, writer).status == Status::Applied && state.clip == 10 && state.reserve == 0);
    std::puts("Remote magazine regression checks passed");
}
