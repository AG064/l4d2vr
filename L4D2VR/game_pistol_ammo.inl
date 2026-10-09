namespace
{
    bool PistolAmmoBackendReady(const Game* game)
    {
        return game && game->m_VR && game->m_VR->m_DualPistolsIndependentHandsEnabled &&
            game->m_ServerPluginHelpers && Hooks::hkPistolPlayerRunCommand.isEnabled && Hooks::hkPistolGunFire.isEnabled &&
            game->m_Offsets && game->m_Offsets->PhysicalShellActiveHandle.valid &&
            game->m_Offsets->GetActiveWeapon.valid && game->m_Offsets->CBaseEntity_entindex.valid;
    }
    bool ReadPistolAmmoRecipient(Game* game, int index, const l4d2vr_pistol::AmmoSnapshot& snapshot,
        edict_t*& entity, uint32_t& handle)
    {
        if (!PistolAmmoBackendReady(game) || index <= 0 || !game->IsValidPlayerIndex(index) || !snapshot.Valid()) return false;
#ifdef _MSC_VER
        __try
        {
#endif
            auto* owner = reinterpret_cast<unsigned char*>(snapshot.owner);
            auto* weapon = reinterpret_cast<unsigned char*>(snapshot.weapon);
            entity = *reinterpret_cast<edict_t**>(owner + 0x28);
            auto* weaponEntity = *reinterpret_cast<edict_t**>(weapon + 0x28);
            using Active = void* (__thiscall*)(void*);
            if (!entity || !weaponEntity || (entity->m_fStateFlags & 2u) != 0u ||
                (weaponEntity->m_fStateFlags & 2u) != 0u || entity->m_EdictIndex != index ||
                static_cast<unsigned short>(entity->m_NetworkSerialNumber) != snapshot.ownerSerial ||
                static_cast<unsigned short>(weaponEntity->m_NetworkSerialNumber) != snapshot.weaponSerial ||
                reinterpret_cast<Active>(game->m_Offsets->GetActiveWeapon.address)(owner) != weapon) return false;
            const uint32_t nativeHandle = *reinterpret_cast<uint32_t*>(owner + 0x19d4);
            handle = ((nativeHandle >> 12) & 0x3ffu) << 12 | (nativeHandle & 0xfffu);
            return nativeHandle != 0xffffffffu && l4d2vr_shell::ValidHandle(handle) &&
                (handle & 0xfffu) == static_cast<unsigned>(weaponEntity->m_EdictIndex);
#ifdef _MSC_VER
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    }
}

void Game::ResetPistolAmmoServerClients()
{
    std::lock_guard<std::mutex> lock(m_PistolAmmoServerMutex);
    m_PistolAmmoServerClients.fill(PistolAmmoServerClient{});
}
void Game::OfferPistolAmmoProtocol(edict_t* entity)
{
    if (!PistolAmmoBackendReady(this)) return;
    int index = -1; std::int16_t serial = 0;
    if (!VRPoseRelayReadEdictIdentity(entity, index, serial)) return;
    uint32_t token = 0u;
    {
        std::lock_guard<std::mutex> lock(m_PistolAmmoServerMutex);
        auto& client = m_PistolAmmoServerClients[index];
        if (client.entity != entity || client.serial != serial || !client.sender.Token())
        {
            static std::atomic<uint32_t> next{(static_cast<uint32_t>(GetTickCount64()) ^ GetCurrentProcessId()) | 1u};
            token = next.fetch_add(1u, std::memory_order_relaxed);
            if (!token) token = next.fetch_add(1u, std::memory_order_relaxed);
            client = {}; client.entity = entity; client.serial = serial; client.sender.Reset(token);
        }
        token = client.sender.Token();
    }
    char command[96]{};
    std::snprintf(command, sizeof(command), "l4d2vr_pistol_ammo_cap %u %u\n", l4d2vr_pistol_sync::kVersion, token);
    VRPoseRelaySendClientCommand(m_ServerPluginHelpers, entity, command);
}
void Game::PublishPistolAmmoState(int index, const l4d2vr_pistol::AmmoSnapshot& snapshot, int command)
{
    if (command <= 0) return;
    edict_t* entity = nullptr; uint32_t handle = 0u;
    if (!ReadPistolAmmoRecipient(this, index, snapshot, entity, handle)) return;
    l4d2vr_pistol_sync::State state{};
    state.handle = handle; state.command = static_cast<uint32_t>(command);
    state.capacity = snapshot.capacity; state.clip = snapshot.clip; state.dual = snapshot.dual;
    {
        std::lock_guard<std::mutex> lock(m_PistolAmmoMutex);
        state.known = m_PistolAmmo[index].Counts(snapshot, state.right, state.left);
    }
    {
        std::lock_guard<std::mutex> lock(m_PistolAmmoServerMutex);
        auto& client = m_PistolAmmoServerClients[index];
        if (client.entity != entity || static_cast<unsigned short>(client.serial) != snapshot.ownerSerial ||
            !client.sender.Prepare(state, GetTickCount64(), state)) return;
    }
    char response[192]{};
    std::snprintf(response, sizeof(response), "l4d2vr_pistol_ammo_state %u %u %u %u %d %d %d %d %u %u\n",
        state.token, state.sequence, state.handle, state.command, state.capacity, state.clip, state.right, state.left,
        state.known ? 1u : 0u, state.dual ? 1u : 0u);
    VRPoseRelaySendClientCommand(m_ServerPluginHelpers, entity, response);
}
