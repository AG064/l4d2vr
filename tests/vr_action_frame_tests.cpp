#include "../L4D2VR/vr_input_recovery.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "../L4D2VR/sdk/vector.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "../L4D2VR/vr_interaction_geometry.h"
#include "../L4D2VR/vr_magazine_policy.h"
#include "../L4D2VR/vr_pistol_detachment.h"
#include "../L4D2VR/vr_remote_shells.h"
#include "openvr.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "vr_action_frame_types.inl"

static int checks = 0;
static void Require(bool value, const char* message)
{
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(EXIT_FAILURE); }
}
struct ScriptedInput
{
    vr::EVRInputError updateError = vr::VRInputError_None, readError = vr::VRInputError_None;
    std::unordered_map<vr::VRActionHandle_t, vr::InputDigitalActionData_t> digital;
    vr::InputAnalogActionData_t analog{};
    vr::InputSkeletalActionData_t skeleton{};
    vr::VRSkeletalSummaryData_t summary{};
    int reads = 0, updates = 0;
    std::function<void()> onRead;
    vr::EVRInputError UpdateActionState(vr::VRActiveActionSet_t*, uint32_t size, uint32_t count)
    {
        Require(size == sizeof(vr::VRActiveActionSet_t) && count == 2u, "action sets retain OpenVR ABI");
        ++updates; return updateError;
    }
    vr::EVRInputError GetDigitalActionData(vr::VRActionHandle_t action, vr::InputDigitalActionData_t* out,
        uint32_t, vr::VRInputValueHandle_t)
    { ++reads; *out = digital[action]; if (onRead) onRead(); return readError; }
    vr::EVRInputError GetAnalogActionData(vr::VRActionHandle_t, vr::InputAnalogActionData_t* out,
        uint32_t, vr::VRInputValueHandle_t)
    { ++reads; *out = analog; if (onRead) onRead(); return readError; }
    vr::EVRInputError GetInputSourceHandle(const char*, vr::VRInputValueHandle_t* out)
    { *out = 100u; return vr::VRInputError_None; }
    vr::EVRInputError GetActionHandle(const char*, vr::VRActionHandle_t* out)
    { *out = 9u; return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalActionData(vr::VRActionHandle_t, vr::InputSkeletalActionData_t* out, uint32_t)
    { ++reads; *out = skeleton; return readError; }
};
static bool TryReadOpenVRSkeletalSummary(ScriptedInput* input, vr::VRActionHandle_t,
    vr::EVRSummaryType, vr::VRSkeletalSummaryData_t& out, const char*)
{ out = input->summary; if (input->onRead) input->onRead(); return true; }
#include "vr_action_frame_curls.inl"

struct C_WeaponCSBase
{
    enum class WeaponID { NONE, PISTOL };
    WeaponID GetWeaponID() const { return WeaponID::PISTOL; }
};
struct C_BasePlayer
{
    int team = 2;
    unsigned char life = 0;
    int observer = 0;
    C_WeaponCSBase weapon;
    void* GetActiveWeapon() { return &weapon; }
};
static bool SafeReadInt(const unsigned char* base, int offset, int& out)
{ std::memcpy(&out, base+offset, sizeof(out)); return true; }
static bool SafeReadU8(const unsigned char* base, int offset, unsigned char& out)
{ out=base[offset]; return true; }
struct Engine
{
    bool inGame = true;
    int playerIndex = 1;
    int GetLocalPlayer() const { return playerIndex; }
    bool IsInGame() const { return inGame; }
};
struct Game
{
    Engine engine;
    Engine* m_EngineClient = &engine;
    C_BasePlayer player;
    int entityLookups = 0;
    bool dualPistol = false;
    std::vector<std::string> commands;
    void* GetClientEntity(int index) { Require(index>0,"entity zero is never cast to player"); ++entityLookups; return &player; }
    bool IsDualPistolWeapon(C_WeaponCSBase*) const { return dualPistol; }
    void ClientCmd_Unrestricted(const char* command) { commands.emplace_back(command); }
    static void logMsg(const char*, ...) {}
};
enum class ObjectPullClientPhase { Idle, Active };
struct VRWorldPoseTrackingSnapshot
{
    bool hmdValid = true, leftHandValid = true, rightHandValid = true;
    uint64_t capturedAtMs = 100u;
    Vector hmdPosition{0,0,0}, leftHandPosition{0,0,0}, rightHandPosition{0,0,0};
    QAngle hmdAngles{0,0,0}, leftHandAngles{0,0,0}, rightHandAngles{0,0,0};
};
static uint64_t GetTickCount64() { return 100u; }

struct VR
{
    static constexpr int kTeamNumOffset = offsetof(C_BasePlayer,team);
    static constexpr int kLifeStateOffset = offsetof(C_BasePlayer,life);
    static constexpr int kObserverModeOffset = offsetof(C_BasePlayer,observer);
    bool m_IsVREnabled = true, m_DualPistolsIndependentHandsEnabled = true, m_MouseModeEnabled = false;
    bool m_MagazineInteractionEnabled = true, m_VrHandsEnabled = true, m_NativeViewmodelHandsOnly = false;
    bool m_DualPistolsNativeReloadFallbackEnabled = false, m_PhysicalReloadObservedEmpty = false;
    bool m_MagazineInteractionSuppressEmptyClipAutoReload = false, m_ManualThrowEnabled = true;
    bool m_VrHandsTwoHandedGripActive = false;
    int m_VrHandsTwoHandedGripWeaponId = 0;
    uintptr_t m_VrHandsTwoHandedGripWeaponTag = 0;
    std::atomic<bool> m_LeftHandPistolActive{false}, m_DualPistolsActive{false};
    std::atomic<unsigned> m_PistolOwnershipMask{2u}, m_ServerPhysicalInteractionVersion{1u};
    l4d2vr_magazine::SessionTracker m_MagazineInteractionSession;
    std::atomic<unsigned> m_MagazineInteractionSessionGeneration{0u};
    bool m_MagazineInteractionLeftHandHolding = true, m_MagazineInteractionQuickReloadMode = false;
    std::atomic<unsigned> m_MagazineInteractionLeftHandPoseActive{0u};
    int shellWorldUpdates = 0;
    ScriptedInput input;
    ScriptedInput* m_Input = &input;
    vr::IVRSystem* m_System = nullptr;
    Game game;
    Game* m_Game = &game;
    l4d2vr_input::ActionFrame m_InputActionFrame;
    vr::EVRInputError m_LastInputActionStateError = vr::VRInputError_None;
    std::array<vr::VRActiveActionSet_t,2> m_ActiveActionSets{};
    uint64_t m_InputActionStateFrameSerial = 0u;
    bool m_LeftHanded = false, m_LeftHandedSwapInputActions = false, m_VrHandsDebugLog = false;
    vr::VRActionHandle_t m_ActionWalk = 10u, m_ActionTeleport = 1u, m_ActionWeaponGrip = 1u;
    std::unordered_map<vr::VRActionHandle_t,std::vector<LeftHandedMirroredActionSource>> m_LeftHandedDigitalActionSwapMap, m_LeftHandedAnalogActionSwapMap;
    std::unordered_set<vr::VRActionHandle_t> m_LeftHandedDigitalHandBoundActions, m_LeftHandedAnalogHandBoundActions;
    std::unordered_map<vr::VRActionHandle_t,LeftHandedDigitalActionFrameCache> m_LeftHandedDigitalActionFrameCache;
    std::unordered_map<vr::VRActionHandle_t,LeftHandedAnalogActionFrameCache> m_LeftHandedAnalogActionFrameCache;
    bool m_SuppressPlayerInput = false, m_PrimaryAttackCmdOwned = false, m_SecondaryAttackCmdOwned = false;
    bool m_JumpCmdOwned = false, m_UseCmdOwned = false, m_ReloadCmdOwned = false, m_DuckCmdOwned = false;
    bool m_ServerUseControllerAimActive = false, m_SpeechToTextCaptureActive = false;
    bool m_VoiceRecordCmdOwned = false, m_AutoVoiceRecordRequested = false, m_SpeechVoiceBroadcastActive = false;
    bool m_PrimaryAttackDown = false, m_MagazineInteractionThumbIndexCurlDownPrev = false, m_MagazineInteractionThreeFingerCurlDownPrev = false;
    bool m_GripReleaseDropEnabled = true, m_BodyGripInventoryEnabled = false, leftPistolPhysicalReload = false;
    std::chrono::steady_clock::time_point m_ServerUseControllerAimUntil{}, m_JumpGestureHoldUntil{}, m_ReloadGestureHoldUntil{}, m_SecondaryAttackGestureHoldUntil{};
    ObjectPullClientPhase m_ObjectPullPhase = ObjectPullClientPhase::Idle;
    int cancellations = 0, teleportCommits = 0, customReleases = 0, settlementPolls = 0, settlements = 0, freshGestures = 0;
    bool targeting = false, pendingMagazine = false, pendingShell = false, acknowledgement = false, chamberRequired = true;
    bool gripDown = false, gripPressed = false, curlPressed = false;
    bool IsInputActionFrameAvailable() const { return l4d2vr_input::ActionFrame::Ready(m_InputActionFrame.Read()); }
    bool ReadWorldPoseTrackingSnapshot(VRWorldPoseTrackingSnapshot&) const { return true; }
    void RefreshLeftHandedInputActionSwapMaps(bool) {}
    void UpdateInputActionState(const l4d2vr_input::ActionFrame::Attempt&);
    bool PressedDigitalAction(vr::VRActionHandle_t&, bool = false);
    bool GetLeftHandedMirroredDigitalActionData(vr::VRActionHandle_t, vr::InputDigitalActionData_t&);
    bool GetLeftHandedMirroredAnalogActionData(vr::VRActionHandle_t, vr::InputAnalogActionData_t&);
    bool GetDigitalActionData(vr::VRActionHandle_t&, vr::InputDigitalActionData_t&);
    bool GetAnalogActionData(vr::VRActionHandle_t&, vr::InputAnalogActionData_t&);
    bool GetWalkAxis(float&, float&);
    bool HasFreshMagazineInteractionTracking() const;
    l4d2vr_dual::Hand GetPrimaryPistolHand() const;
    void Frame(vr::EVRInputError error = vr::VRInputError_None)
    { input.updateError = error; UpdateInputActionState(m_InputActionFrame.Begin()); }
    void ReleaseCustomActionInputs() { ++customReleases; }
    void EndSpeechToTextCapture(bool) {}
    void StopVoiceRecordCommandNow() {}
    void ResetObjectPullInput(bool) {}
    void CancelMagazineInteractionManual() { ++cancellations; pendingMagazine = pendingShell = false; chamberRequired = false; }
    void ResetMagazineInteractionSession()
    {
#include "vr_action_frame_session_reset.inl"
    }
    void BeginTeleportTargeting() { targeting = true; }
    void EndTeleportTargeting(bool commit) { if (commit) ++teleportCommits; targeting = false; }
    void CancelTeleportTargeting() { targeting = false; }
    bool IsGameplayHandLeftPhysical(bool left) const { return left; }
    bool UpdateMagazineInteraction(C_BasePlayer* player, bool a, bool b, bool c, bool d, bool e)
    {
        Require(!a && !b && !c && !d && !e, "unavailable frame supplies neutral controls");
        C_BasePlayer* localPlayer = player;
        C_WeaponCSBase* activeWeapon = player ? &player->weapon : nullptr;
        const bool vrHandsInteractionAvailable = player != nullptr;
        const int activeWeaponIdInt = 1;
        const uint32_t activeWeaponHandle = 0x1001u;
        const bool authoritativeMagazine = false, authoritativeShell = false, authoritativePistol = true;
#include "vr_action_frame_session.inl"
        if (player)
        {
            Require(player == &game.player, "settlement retains the real owner");
            ++settlementPolls;
            if (acknowledgement)
            {
                settlements += int(pendingMagazine) + int(pendingShell);
                pendingMagazine = pendingShell = false;
            }
            if (HasFreshMagazineInteractionTracking()) ++freshGestures;
        }
        return false;
    }
    void Cleanup()
    {
#include "vr_action_frame_ownership.inl"
#include "vr_action_frame_cleanup.inl"
    }
    void updateFreshDetachedMagazineFromLeftHand() { ++shellWorldUpdates; }
    void ShellPending(bool leftGripDown)
    {
#include "vr_action_frame_shell_hold.inl"
    }
    void Teleport()
    {
#include "vr_action_frame_teleport.inl"
    }
    void Grip()
    {
        uint32_t weaponGripInputState = 0u;
#include "vr_action_frame_grip.inl"
        gripDown = rightGripDown; gripPressed = rightGripJustPressed;
        (void)rightGripValid; (void)weaponGripInputState;
    }
    void CurlEdges(bool held)
    {
        const bool magazineButtonGripInput = false, magazineButtonGripDown = false, magazineButtonGripJustPressed = false;
        const bool thumbIndexGripDown = held, threeFingerGripDown = held;
        const bool thumbIndexWasDown = m_MagazineInteractionThumbIndexCurlDownPrev;
        const bool threeFingerWasDown = m_MagazineInteractionThreeFingerCurlDownPrev;
#include "vr_action_frame_curl_edges.inl"
        curlPressed = magazineGripJustPressed || supportHandJustPressed;
        (void)magazineGripDown; (void)supportHandDown;
    }
};
#include "vr_action_frame_functions.inl"
#include "vr_action_frame_tracking.inl"
#include "vr_action_frame_primary.inl"

struct ThrowFixture
{
    VR vr;
    l4d2vr_input::ReleaseInputGate s_manualThrowInputGate;
    uintptr_t s_manualCarryThrowWeapon = 0;
    bool s_manualCarryThrowPhysicalAttackDown = false, s_manualInventoryThrowPhysicalUseDown = false;
    bool s_manualInventoryThrowArmed = false;
    struct Command { int buttons=0,command_number=1; unsigned char impulse=0; } command;
    void Step(bool carriedProp, bool attack, bool use)
    {
        vr.m_PrimaryAttackDown=attack; vr.m_UseCmdOwned=use;
        VR* m_VR=&vr;
        Command* cmd=&command;
        const bool s_ServerUnderstandsVR=true, throwBackendReady=true, emptyHandsPlaceholderActive=false;
        const bool localUseButtonDownForAutoActions=use, gripDropRequested=false;
        constexpr uint32_t kManualCarryThrowIN_ATTACK=1u, kManualInventoryThrowIN_USE=1u<<5;
        constexpr uint32_t kManualCarryThrowWeaponShift=26u, kManualCarryThrowWeaponMask=0x3fu<<26;
        constexpr int carryWeaponId=3, activeWeaponId=4;
        constexpr uintptr_t activeWeaponTag=100u;
        const char* activeWeaponName="fixture"; const char* activeWeaponNetClass="fixture";
        const auto pistolDropHand=l4d2vr_dual::Hand::None;
        auto pistolInteractionHand=l4d2vr_dual::Hand::None;
#include "vr_action_frame_throw_gate.inl"
#include "vr_action_frame_throw_release.inl"
        (void)pistolInteractionHand;
    }
    bool Emitted() const { return (static_cast<uint32_t>(command.buttons) & (0x3fu<<26)) != 0u || command.impulse != 0u; }
};

static void ThrowFailureTests(bool carried)
{
    ThrowFixture f; f.vr.Frame();
    f.Step(carried,false,!carried);
    f.Step(carried,true,!carried);
    Require(carried || f.s_manualInventoryThrowArmed,"inventory arms on genuine use-then-trigger press");
    f.vr.Frame(vr::VRInputError_IPCError);
    // CreateMove misses the failed and edge-suppressed recovery Presents.
    f.vr.Frame(); f.vr.Frame();
    f.Step(carried,false,false);
    Require(!f.Emitted() && !f.s_manualInventoryThrowArmed,"missed failure cancels the old release state");
    f.Step(carried,true,!carried);
    Require(!f.Emitted() && !f.s_manualInventoryThrowArmed,"held recovery cannot rearm throw");
    f.Step(carried,false,false);
    Require(!f.Emitted(),"neutral recovery is only a baseline");
    f.Step(carried,false,!carried); f.Step(carried,true,!carried); f.Step(carried,false,!carried);
    Require(f.Emitted(),"fresh deliberate press and release still emits");

    ThrowFixture direct; direct.vr.Frame(); direct.Step(carried,false,!carried); direct.Step(carried,true,!carried);
    direct.vr.Frame(vr::VRInputError_IPCError);
    direct.command.buttons=static_cast<int>(0x3fu<<26); direct.command.impulse=l4d2vr_grip::kReleaseImpulse;
    direct.Step(carried,false,false);
    Require(!direct.Emitted(),"unavailable frame suppresses encoded throw/drop output");
    direct.vr.Frame(); direct.Step(carried,true,!carried);
    Require(!direct.Emitted() && !direct.s_manualInventoryThrowArmed,"first recovered held sample stays disarmed");
}

static vr::InputDigitalActionData_t Digital(bool active, bool held, bool changed)
{ vr::InputDigitalActionData_t data{}; data.bActive=active; data.bState=held; data.bChanged=changed; data.activeOrigin=100u; return data; }

int main()
{
    VR vr;
    vr.input.digital[1] = Digital(true,true,true);
    vr.input.analog.bActive = true; vr.input.analog.x = 0.8f;
    vr.Frame();
    vr.Cleanup();
    vr.UpdateMagazineInteraction(&vr.game.player,false,false,false,false,false);
    vr.cancellations=vr.settlementPolls=vr.freshGestures=0; vr.chamberRequired=true;
    const auto sessionGeneration=vr.m_MagazineInteractionSessionGeneration.load();
    Require(vr.GetPrimaryPistolHand()==l4d2vr_dual::Hand::Left,"single-pistol session starts with physical left ownership");
    float x=0, y=0;
    Require(vr.GetWalkAxis(x,y) && x == 0.8f, "successful action frame provides movement");
    Require(vr.PressedDigitalAction(vr.m_ActionTeleport,true), "healthy fresh press remains available");
    vr.targeting = true;
    vr.Frame(vr::VRInputError_IPCError);
    const int reads = vr.input.reads;
    Require(!vr.GetWalkAxis(x,y) && x == 0.0f, "failed update cannot reuse stale movement");
    Require(!vr.PressedDigitalAction(vr.m_ActionTeleport,true), "failed update cannot replay button edge");
    vr.Grip(); Require(!vr.gripDown, "direct grip read is gated");
    Require(vr.input.reads == reads, "failed frames never query stale action data");
    vr.pendingMagazine = vr.pendingShell = true;
    vr.m_PrimaryAttackCmdOwned = vr.m_SecondaryAttackCmdOwned = vr.m_JumpCmdOwned = true;
    vr.m_UseCmdOwned = vr.m_ReloadCmdOwned = vr.m_DuckCmdOwned = true;
    vr.Cleanup();
    Require(vr.GetPrimaryPistolHand()==l4d2vr_dual::Hand::Left &&
        vr.m_MagazineInteractionSessionGeneration.load()==sessionGeneration,"failed-frame neutral poll keeps real session and generation");
    Require(vr.game.commands.size()==6u && !vr.targeting, "failed frame releases owned controls and cancels targeting");
    Require(vr.pendingMagazine && vr.pendingShell && vr.chamberRequired && vr.cancellations==0, "transient failure preserves submitted ammo and chamber state");
    Require(vr.settlementPolls==1 && vr.freshGestures==0, "acknowledgements poll while gestures pause");
    vr.acknowledgement = true; vr.Cleanup();
    Require(vr.settlements==2 && vr.chamberRequired && vr.game.commands.size()==6u, "magazine and shell settlement remain possible during failure");
    vr.Frame();
    Require(vr.PressedDigitalAction(vr.m_ActionTeleport) && !vr.PressedDigitalAction(vr.m_ActionTeleport,true), "recovery restores held state without one-shot replay");
    vr.Grip(); Require(vr.gripDown && !vr.gripPressed, "grip recovery does not replay pressed edge");
    vr.CurlEdges(true); Require(!vr.curlPressed, "recovered curl baseline does not manufacture gesture edge");
    vr.Teleport(); Require(vr.teleportCommits==0 && !vr.targeting, "recovery does not replay teleport press");
    vr.Frame(); vr.CurlEdges(true); Require(!vr.curlPressed, "held curl remains baselined on following frame");
    vr.input.digital[1] = Digital(false,true,true);
    Require(!vr.PressedDigitalAction(vr.m_ActionTeleport), "inactive direct payload cannot press buttons");
    vr.targeting = true; vr.Teleport();
    Require(!vr.targeting && vr.teleportCommits==0, "inactive teleport cancels without commit");

    VR mirror; mirror.m_LeftHanded = mirror.m_LeftHandedSwapInputActions = true;
    mirror.m_LeftHandedDigitalHandBoundActions.insert(1);
    mirror.m_LeftHandedDigitalActionSwapMap[1] = {{2,100},{3,101}};
    mirror.input.digital[2] = Digital(false,true,true);
    mirror.input.digital[3] = Digital(true,false,false);
    mirror.Frame();
    vr::InputDigitalActionData_t output{};
    Require(mirror.GetDigitalActionData(mirror.m_ActionTeleport,output) && output.bActive && !output.bState && !output.bChanged, "inactive mirrored press cannot contaminate active neutral source");
    mirror.input.readError = vr::VRInputError_InvalidDevice; mirror.Frame();
    Require(!mirror.GetDigitalActionData(mirror.m_ActionTeleport,output) && !output.bActive && !output.bChanged, "all mirrored errors stay failed without release event");
    const int failedReads = mirror.input.reads;
    Require(!mirror.GetDigitalActionData(mirror.m_ActionTeleport,output) && mirror.input.reads == failedReads, "cached mirrored failure retains failure status");
    mirror.input.readError = vr::VRInputError_None; mirror.input.digital[3] = Digital(true,true,true); mirror.Frame();
    Require(mirror.GetDigitalActionData(mirror.m_ActionTeleport,output) && output.bState && !output.bChanged, "source recovery exposes held state without synthetic edge");
    mirror.m_LeftHandedDigitalActionSwapMap[1].clear(); mirror.Frame();
    Require(!mirror.GetDigitalActionData(mirror.m_ActionTeleport,output) && !output.bActive, "empty mirrored source list cannot fabricate success");

    VR concurrent; concurrent.Frame(); concurrent.input.digital[1]=Digital(true,true,true);
    concurrent.input.onRead = [&] { concurrent.m_InputActionFrame.Begin(); };
    Require(!concurrent.GetDigitalActionData(concurrent.m_ActionTeleport,output) && !output.bState, "frame changes during query reject old payload");
    ScriptedInput skeleton; l4d2vr_input::ActionFrame frames;
    vr::VRActionHandle_t action=9u; std::array<float,5> curls{};
    skeleton.skeleton.bActive=true; skeleton.skeleton.activeOrigin=100u;
    for(float& curl : skeleton.summary.flFingerCurl) curl=0.9f;
    Require(!ReadOpenVRSkeletalFingerCurls(&skeleton,frames,action,"/actions/base/in/skeleton_lefthand",curls,"test") && skeleton.reads==0, "unavailable skeletal frame never reads curls");
    frames.Complete(frames.Begin(),true);
    Require(ReadOpenVRSkeletalFingerCurls(&skeleton,frames,action,"/actions/base/in/skeleton_lefthand",curls,"test") && curls[0]==0.9f, "successful skeletal frame supplies held curls");
    skeleton.onRead = [&] { frames.Begin(); };
    Require(!ReadOpenVRSkeletalFingerCurls(&skeleton,frames,action,"/actions/base/in/skeleton_lefthand",curls,"test") && curls[0]==0.0f, "changed skeletal frame neutralizes curls");
    VR direct; direct.Frame(); vr::InputAnalogActionData_t axes{};
    direct.input.analog.bActive=false; direct.input.analog.x=0.9f; direct.input.analog.y=std::numeric_limits<float>::infinity();
    Require(direct.GetAnalogActionData(direct.m_ActionWalk,axes) && !axes.bActive && axes.x==0.0f && axes.y==0.0f,"inactive direct analog values are neutralized");
    direct.input.analog.bActive=true;
    Require(!direct.GetAnalogActionData(direct.m_ActionWalk,axes) && axes.x==0.0f && !axes.bActive,"nonfinite active direct analog values fail neutral");
    direct.input.analog.y=0.25f;
    Require(direct.GetAnalogActionData(direct.m_ActionWalk,axes) && axes.bActive && axes.x==0.9f,"valid direct analog values remain available");

    VR missingPlayer; missingPlayer.Frame(vr::VRInputError_IPCError); missingPlayer.game.engine.inGame=false; missingPlayer.Cleanup();
    Require(missingPlayer.game.entityLookups==0 && !missingPlayer.m_LeftHandPistolActive.load(),"not-in-game settlement never casts world entity");
    missingPlayer.game.engine.inGame=true; missingPlayer.game.engine.playerIndex=0; missingPlayer.Cleanup();
    Require(missingPlayer.game.entityLookups==0,"zero local player index remains unavailable");
    VR changedOwner; changedOwner.Frame(); changedOwner.Cleanup();
    changedOwner.UpdateMagazineInteraction(&changedOwner.game.player,false,false,false,false,false);
    const auto oldGeneration=changedOwner.m_MagazineInteractionSessionGeneration.load();
    changedOwner.pendingMagazine=true; changedOwner.m_PistolOwnershipMask.store(1u);
    changedOwner.Frame(vr::VRInputError_IPCError); changedOwner.Cleanup();
    Require(changedOwner.GetPrimaryPistolHand()==l4d2vr_dual::Hand::Right &&
        changedOwner.m_MagazineInteractionSessionGeneration.load()>oldGeneration && !changedOwner.pendingMagazine,
        "genuine pistol ownership change still fences the real session");

    VR shell; shell.Frame(vr::VRInputError_IPCError); shell.ShellPending(false);
    Require(shell.m_MagazineInteractionLeftHandHolding && shell.shellWorldUpdates==0,"unknown shell input preserves held flag without world update");
    shell.Frame(); shell.ShellPending(true);
    Require(shell.m_MagazineInteractionLeftHandHolding && shell.shellWorldUpdates==1,"still-held recovery resumes shell world update");
    shell.ShellPending(false); shell.ShellPending(true);
    Require(!shell.m_MagazineInteractionLeftHandHolding && shell.shellWorldUpdates==1,"genuine released shell cannot be resurrected");
    ThrowFailureTests(true); ThrowFailureTests(false);
    std::printf("PASS: action frames (%d checks)\n", checks);
    return EXIT_SUCCESS;
}
