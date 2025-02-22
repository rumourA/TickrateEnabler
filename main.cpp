#ifdef _WIN32
#include <Windows.h>
#define PROTECT_MEMORY(addr, size, prot, oldProt) VirtualProtect(addr, size, prot, &oldProt)
#else
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>
#define PROTECT_MEMORY(addr, size, prot, oldProt) mprotect((void *)((uintptr_t)addr & ~(getpagesize() - 1)), size, prot)
#endif

#include <cstdio>
#include <cstdlib>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS002"

enum
{
    IFACE_OK = 0,
    IFACE_FAILED
};

typedef enum
{
    PLUGIN_CONTINUE = 0, // keep going
    PLUGIN_OVERRIDE,     // run the game dll function but use our return value instead
    PLUGIN_STOP,         // don't run the game dll function at all
} PLUGIN_RESULT;

typedef enum
{
    eQueryCvarValueStatus_ValueIntact = 0, // It got the value fine.
    eQueryCvarValueStatus_CvarNotFound = 1,
    eQueryCvarValueStatus_NotACvar = 2,     // There's a ConCommand, but it's not a ConVar.
    eQueryCvarValueStatus_CvarProtected = 3 // The cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the server is not allowed to have its value.
} EQueryCvarValueStatus;

class CCommand;
struct edict_t;
class KeyValues;

typedef void *(*CreateInterfaceFn)(const char *pName, int *pReturnCode);
typedef void *(*InstantiateInterfaceFn)();

typedef int QueryCvarCookie_t;

// Used internally to register classes.
class InterfaceReg
{
public:
    InterfaceReg(InstantiateInterfaceFn fn, const char *pName);

public:
    InstantiateInterfaceFn m_CreateFn;
    const char *m_pName;

    InterfaceReg *m_pNext; // For the global list.
    static InterfaceReg *s_pInterfaceRegs;
};

class IServerPluginCallbacks
{
public:
    // Initialize the plugin to run
    // Return false if there is an error during startup.
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) = 0;

    // Called when the plugin should be shutdown
    virtual void Unload(void) = 0;

    // called when a plugins execution is stopped but the plugin is not unloaded
    virtual void Pause(void) = 0;

    // called when a plugin should start executing again (sometime after a Pause() call)
    virtual void UnPause(void) = 0;

    // Returns string describing current plugin.  e.g., Admin-Mod.
    virtual const char *GetPluginDescription(void) = 0;

    // Called any time a new level is started (after GameInit() also on level transitions within a game)
    virtual void LevelInit(char const *pMapName) = 0;

    // The server is about to activate
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) = 0;

    // The server should run physics/think on all edicts
    virtual void GameFrame(bool simulating) = 0;

    // Called when a level is shutdown (including changing levels)
    virtual void LevelShutdown(void) = 0;

    // Client is going active
    virtual void ClientActive(edict_t *pEntity) = 0;

    // Client is disconnecting from server
    virtual void ClientDisconnect(edict_t *pEntity) = 0;

    // Client is connected and should be put in the game
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername) = 0;

    // Sets the client index for the client who typed the command into their console
    virtual void SetCommandClient(int index) = 0;

    // A player changed one/several replicated cvars (name etc)
    virtual void ClientSettingsChanged(edict_t *pEdict) = 0;

    // Client is connecting to server ( set retVal to false to reject the connection )
    //	You can specify a rejection message by writing it into reject
    virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) = 0;

    // The client has typed a command at the console
    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args) = 0;

    // A user has had their network id setup and validated
    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) = 0;

    // This is called when a query from IServerPluginHelpers::StartQueryCvarValue is finished.
    // iCookie is the value returned by IServerPluginHelpers::StartQueryCvarValue.
    // Added with version 2 of the interface.
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) = 0;
};

class IGameEventListener
{
public:
    virtual ~IGameEventListener(void) {};

    // FireEvent is called by EventManager if event just occured
    // KeyValue memory will be freed by manager if not needed anymore
    virtual void FireGameEvent(KeyValues *event) = 0;
};

class CEmptyServerPlugin : public IServerPluginCallbacks, public IGameEventListener
{
public:
    CEmptyServerPlugin();
    ~CEmptyServerPlugin();

    // IServerPluginCallbacks methods
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload(void);
    virtual void Pause(void);
    virtual void UnPause(void);
    virtual const char *GetPluginDescription(void);
    virtual void LevelInit(char const *pMapName);
    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
    virtual void GameFrame(bool simulating);
    virtual void LevelShutdown(void);
    virtual void ClientActive(edict_t *pEntity);
    virtual void ClientDisconnect(edict_t *pEntity);
    virtual void ClientPutInServer(edict_t *pEntity, char const *playername);
    virtual void SetCommandClient(int index);
    virtual void ClientSettingsChanged(edict_t *pEdict);
    virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args);
    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue);

    virtual void OnEdictAllocated(edict_t *edict);
    virtual void OnEdictFreed(const edict_t *edict);

    // IGameEventListener Interface
    virtual void FireGameEvent(KeyValues *event);

    virtual int GetCommandIndex() { return m_iClientCommandIndex; }

private:
    int m_iClientCommandIndex;
};

#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName)                            \
    static void *__Create##className##interfaceName##_interface() { return static_cast<interfaceName *>(&globalVarName); } \
    static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName);
#else
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName)                                      \
    namespace _SUBSYSTEM                                                                                                             \
    {                                                                                                                                \
        static void *__Create##className##interfaceName##_interface() { return static_cast<interfaceName *>(&globalVarName); }       \
        static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName); \
    }
#endif

#if defined(_WIN32)
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

InterfaceReg *InterfaceReg::s_pInterfaceRegs = NULL;

InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char *pName) : m_pName(pName)
{
    m_CreateFn = fn;
    m_pNext = s_pInterfaceRegs;
    s_pInterfaceRegs = this;
}

DLL_EXPORT void *CreateInterface(const char *pName, int *pReturnCode)
{
    InterfaceReg *pCur;

    for (pCur = InterfaceReg::s_pInterfaceRegs; pCur; pCur = pCur->m_pNext)
    {
        if (strcmp(pCur->m_pName, pName) == 0)
        {
            if (pReturnCode)
            {
                *pReturnCode = IFACE_OK;
            }
            return pCur->m_CreateFn();
        }
    }

    if (pReturnCode)
    {
        *pReturnCode = IFACE_FAILED;
    }
    return NULL;
}

CEmptyServerPlugin g_EmtpyServerPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_EmtpyServerPlugin);

CEmptyServerPlugin::CEmptyServerPlugin()
{
    m_iClientCommandIndex = 0;
}

CEmptyServerPlugin::~CEmptyServerPlugin()
{
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::GameFrame(bool simulating)
{
}

class IServerGameDLL
{
};

IServerGameDLL *gamedll = NULL;
void *tier0 = NULL;

void HookVTable(void **vtable, size_t index, void *function, void **original)
{
#ifdef _WIN32
    DWORD old;
    int prot = PAGE_EXECUTE_READWRITE;
#else
    int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    int old;
#endif

    PROTECT_MEMORY(&vtable[index], sizeof(void *), prot, old);

    *original = vtable[index];

    vtable[index] = function;

    PROTECT_MEMORY(&vtable[index], sizeof(void *), old, old);
}

typedef float (*GetTickInterval_t)(int64_t instance);
GetTickInterval_t GetTickInterval = nullptr;

#ifdef _WIN32
const char *Plat_GetCommandLineA()
{
    return reinterpret_cast<char *(*)()>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCommandLineA"))();
}
void Warning(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    reinterpret_cast<void (*)(const char *, va_list)>(GetProcAddress(GetModuleHandleA("tier0.dll"), "Warning"))(fmt, va);
    va_end(va);
}
#else
const char *(*Plat_GetCommandLineA)();
void (*Warning)(const char *, ...);
void GetTier0Exports()
{
    if (!tier0)
    {
        tier0 = dlopen("bin/linux64/libtier0.so", RTLD_NOW);

        if (!tier0)
            tier0 = dlopen("bin/libtier0_srv.so", RTLD_NOW);

        if (!tier0)
            tier0 = dlopen("bin/linux64/libtier0_srv.so", RTLD_NOW);
    }

    if (tier0)
    {
        *(void **)(&Plat_GetCommandLineA) = dlsym(tier0, "Plat_GetCommandLineA");
        *(void **)(&Warning) = dlsym(tier0, "Warning");
    }
}
#endif

float GetTickIntervalHook(int64_t instance)
{
    const char *GetCommandLineA = Plat_GetCommandLineA();

    int tick_rate = 0;
    if (auto tickrate_str = strstr(GetCommandLineA, "-tickrate"); tickrate_str)
    {
        tickrate_str += 10;

        if (tickrate_str && tickrate_str[0] != '0')
        {
            tick_rate = atoi(tickrate_str);
            if (tick_rate > 10)
            {
                return 1.f / tick_rate;
            }
        }
    }

    return 0.015f;
}

bool CEmptyServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
#ifdef __unix__
    GetTier0Exports();

    if (!tier0)
    {
        return false;
    }
#endif

    int num = 1;
    char buf[256];
    while (!gamedll && num < 100)
    {
        snprintf(buf, sizeof(buf), "ServerGameDLL0%d", num);
        gamedll = (IServerGameDLL *)gameServerFactory(buf, NULL);
        num++;
    }

    if (!gamedll)
    {
        Warning("Failed to get a pointer to ServerGameDLL.\n");
        return false;
    }

    HookVTable(*(void ***)gamedll, 10, (void *)GetTickIntervalHook, (void **)&GetTickInterval);

    return true;
}

void CEmptyServerPlugin::Unload(void)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Pause(void)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::UnPause(void)
{
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CEmptyServerPlugin::GetPluginDescription(void)
{
    return "TickrateEnabler";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelInit(char const *pMapName)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelShutdown(void) // !!!!this can get called multiple times per map change
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientActive(edict_t *pEntity)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientDisconnect(edict_t *pEntity)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientPutInServer(edict_t *pEntity, char const *playername)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::SetCommandClient(int index)
{
    m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientSettingsChanged(edict_t *pEdict)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
    return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientCommand(edict_t *pEntity, const CCommand &args)
{
    return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID)
{
    return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue)
{
}
void CEmptyServerPlugin::OnEdictAllocated(edict_t *edict)
{
}
void CEmptyServerPlugin::OnEdictFreed(const edict_t *edict)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::FireGameEvent(KeyValues *event)
{
}