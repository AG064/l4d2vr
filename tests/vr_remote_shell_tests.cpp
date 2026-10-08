#include "../L4D2VR/vr_remote_shells.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

static void CheckImpl(bool okay, int line)
{
    if (!okay) { std::fprintf(stderr, "Remote shell check failed at line %d\n", line); std::abort(); }
}
#define Check(value) CheckImpl((value), __LINE__)

int main()
{
    using namespace l4d2vr_shell;
    uint32_t number = 0u;
    Check(ParseNumber("4294967295", 0xffffffffu, number) && number == 0xffffffffu);
    for (const char* bad : {"", "-1", "+1", "1.0", " 1", "1;quit", "4294967296", "00000000000"})
        Check(!ParseNumber(bad, 0xffffffffu, number));
    Check(!ParseNumber("129", 128u, number));
    const uint32_t handle = (7u << 12) | 25u;
    Check(HandleMatches(handle, 25, 7u));
    Check(HandleMatches(handle, 25, 0x4007u)); // native entity-list serial is truncated by the send proxy
    Check(!HandleMatches(handle, 25, 8u));
    Check(!ValidHandle(0u) && !ValidHandle(0xffffffffu) && !ValidHandle(2048u));
    ServerLedger ledger;
    ledger.Reset(10u);
    int clip = 3, reserve = 20, writes = 0;
    Snapshot state{true, handle, 200u, 3, clip, reserve, 8};
    Request request{10u, 1u, handle, 200u, 3, clip, reserve};
    auto writer = [&](int nextClip, int nextReserve) { clip = nextClip; reserve = nextReserve; ++writes; return true; };
    const auto result = ledger.Apply(request, state, 1000u, writer);
    Check(result.status == Status::Applied && result.clip == 4 && result.reserve == 19);
    state.clip = clip; state.reserve = reserve;
    auto duplicate = ledger.Apply(request, state, 1100u, writer);
    Check(duplicate.status == Status::Applied && writes == 1 && clip + reserve == 23);
    Request altered = request; altered.reserve = 19;
    Check(ledger.Apply(altered, state, 1200u, writer).status == Status::Stale && writes == 1);
    Request next{10u, 2u, handle, 200u, 3, 4, 19};
    Check(ledger.Apply(next, state, 1050u, writer).status == Status::RateLimited && writes == 1);
    next.sequence = 3u;
    Check(ledger.Apply(next, state, 1200u, writer).status == Status::Applied && writes == 2);
    state.clip = clip; state.reserve = reserve;
    next.sequence = 4u; // an old predicted baseline must not overwrite a shot or another reload
    Check(ledger.Apply(next, state, 1400u, writer).status == Status::Stale && writes == 2);
    next.sequence = 5u; next.clip = clip; next.reserve = reserve; next.command = 100u;
    Check(ledger.Apply(next, state, 1500u, writer).status == Status::Stale && writes == 2);
    next.sequence = 6u; next.command = 209u;
    Check(ledger.Apply(next, state, 1600u, writer).status == Status::Stale && writes == 2);
    next.sequence = 7u; next.command = 200u; next.handle += 4096u;
    Check(ledger.Apply(next, state, 1700u, writer).status == Status::Weapon && writes == 2);
    next.sequence = 8u; next.handle = handle; state.eligible = false;
    Check(ledger.Apply(next, state, 1800u, writer).status == Status::Weapon && writes == 2);
    state.eligible = true; state.clip = 8; next.clip = 8; next.sequence = 9u;
    Check(ledger.Apply(next, state, 1900u, writer).status == Status::NoSpaceOrAmmo && writes == 2);
    state.clip = 0; state.reserve = 0; next.clip = next.reserve = 0; next.sequence = 10u;
    Check(ledger.Apply(next, state, 2000u, writer).status == Status::NoSpaceOrAmmo && writes == 2);
    // Custom script capacity comes from the server and still moves exactly one existing round.
    state.clip = 11; state.reserve = 3; state.capacity = 12;
    next.clip = 11; next.reserve = 3; next.sequence = 11u;
    Check(ledger.Apply(next, state, 2100u, writer).status == Status::Applied && clip == 12 && reserve == 2);
    next.sequence = 12u; state.clip = next.clip = 2; state.reserve = next.reserve = 7;
    int failedWrites = 0;
    auto fail = [&](int, int) { ++failedWrites; return false; };
    Check(ledger.Apply(next, state, 2300u, fail).status == Status::Backend);
    Check(ledger.Apply(next, state, 2400u, writer).status == Status::Backend && failedWrites == 1 && writes == 3);
    ledger.Reset(20u);
    Check(ledger.Apply(next, state, 2500u, writer).status == Status::Session && writes == 3);
    next.token = 20u; next.sequence = 1u;
    Check(ledger.Unsupported(next).status == Status::Unsupported);
    Check(ledger.Apply(next, state, 2600u, writer).status == Status::Unsupported && writes == 3);

    ClientRequest client;
    client.Offer(2u, 10u); Check(!client.Supported());
    client.Offer(kVersion, 10u); Check(client.Supported());
    Request outgoing{};
    Check(client.Begin(handle, 3, 3, 20, 200u, 100u, 1u, 1000u, outgoing));
    Request extra{};
    Check(!client.Begin(handle, 3, 3, 20, 200u, 100u, 1u, 1001u, extra));
    Reply reply{outgoing, Status::Applied, 5, 18}; // forbid an acknowledgement that mints two shells
    client.Receive(reply);
    Reply received{};
    Check(client.Update(handle, 3, 100u, 1u, 1100u, received) == ClientRequest::Poll::Waiting);
    reply.clip = 4; reply.reserve = 19; client.Receive(reply);
    Check(client.Update(handle, 3, 100u, 2u, 1100u, received) == ClientRequest::Poll::None); // weapon/session reset
    Check(!client.Pending());
    Check(client.Begin(handle, 3, 3, 20, 201u, 100u, 2u, 1200u, outgoing));
    reply = {outgoing, Status::Applied, 4, 19}; client.Receive(reply);
    Check(client.Update(handle, 3, 100u, 2u, 1250u, received, 3, 20) == ClientRequest::Poll::Waiting);
    Check(client.Update(handle, 3, 100u, 2u, 1270u, received, 4, 20) == ClientRequest::Poll::Waiting);
    Check(client.Update(handle, 3, 100u, 2u, 1300u, received, 4, 19) == ClientRequest::Poll::Result);
    Check(received.clip == 4 && !client.Pending());
    client.Receive(reply); // late duplicate must not complete the next insertion
    Check(client.Begin(handle, 3, 4, 19, 202u, 100u, 2u, 1400u, outgoing));
    client.Receive(reply);
    Check(client.Update(handle, 3, 100u, 2u, 1500u, received) == ClientRequest::Poll::Waiting);
    Check(client.Update(handle, 3, 100u, 2u, 1400u + kReplyTimeoutMs, received) == ClientRequest::Poll::Timeout);
    Check(!client.Pending() && !client.Supported());
    client.Offer(kVersion, 10u); Check(!client.Supported()); // repeated offer cannot revive an unresolved lease
    client.Offer(kVersion, 20u); Check(client.Supported());
    Check(!client.Begin(handle, 1, 4, 19, 202u, 100u, 2u, 5000u, outgoing)); // no pistol authority in this protocol
    Check(client.Begin(handle, 3, 4, 19, 202u, 100u, 2u, 5000u, outgoing));
    Check(client.Update(handle, 3, 200u, 2u, 5010u, received) == ClientRequest::Poll::None);
    client.Disconnect(); Check(!client.Supported() && !client.Pending());
    client.Offer(kVersion, 30u);
    Check(client.Begin(handle, 3, 4, 19, 202u, 100u, 2u, 6000u, outgoing));
    reply = {outgoing, Status::Unsupported, 0, 0}; client.Receive(reply);
    Check(client.Update(handle, 3, 100u, 2u, 6010u, received) == ClientRequest::Poll::Result);
    Check(!client.Supported() && !client.Pending());
}
