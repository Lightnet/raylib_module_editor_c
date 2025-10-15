#ifndef MODULE_ENET_H
#define MODULE_ENET_H

#include "flecs.h"

// Prevent Windows API conflicts
#ifdef _WIN32
    // #define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
    // #define NOVIRTUALKEYCODES // VK_*
    // #define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
    // #define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
    // #define NOSYSMETRICS      // SM_*

    // #define NOMENUS           // MF_*
    // #define NOICONS           // IDI_*
    // #define NOKEYSTATES       // MK_*
    // #define NOSYSCOMMANDS     // SC_*
    // #define NORASTEROPS       // Binary and Tertiary raster ops

    // #define NOSHOWWINDOW      // SW_*
    // #define OEMRESOURCE       // OEM Resource values
    // #define NOATOM            // Atom Manager routines
    // #define NOCLIPBOARD       // Clipboard routines
    // #define NOCOLOR           // Screen colors
    // #define NOCTLMGR          // Control and Dialog routines
    #define NODRAWTEXT        // DrawText() and DT_*
    #define NOGDI             // All GDI defines and routines
    // #define NOKERNEL          // All KERNEL defines and routines
    #define NOUSER            // All USER defines and routines
    //#define NONLS             // All NLS defines and routines

    // #define NOMB              // MB_* and MessageBox()
    // #define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
    // #define NOMETAFILE        // typedef METAFILEPICT
    // #define NOMINMAX          // Macros min(a,b) and max(a,b)
    // #define NOMSG             // typedef MSG and associated routines
    // #define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
    // #define NOSCROLL          // SB_* and scrolling routines
    // #define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
    // #define NOSOUND           // Sound driver routines
    // #define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines

    // #define NOWH              // SetWindowsHook and WH_*
    // #define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
    // #define NOCOMM            // COMM driver routines
    // #define NOKANJI           // Kanji support stuff.
    // #define NOHELP            // Help engine interface.
    // #define NOPROFILER        // Profiler interface.
    // #define NODEFERWINDOWPOS  // DeferWindowPos routines
    // #define NOMCX             // Modem Configuration Extensions
#endif

// Type required before windows.h inclusion
typedef struct tagMSG *LPMSG;

#include <enet.h>  // Include ENet after 
// #define CloseWindow Raylib_CloseWindow  // Before #include <raylib.h>
// #define ShowCursor Raylib_ShowCursor  // Before #include <raylib.h>
#include "raylib.h"  // Include raylib.h first to define Rectangle, etc.
// #undef CloseWindow
// #undef ShowCursor

// TransformGUI component
typedef struct {
    void *data;
} enet_packet_t;
extern ECS_COMPONENT_DECLARE(enet_packet_t);

typedef struct {
    ENetPeer *peer;             // Client peer (null for server)
} enet_client_t;
extern ECS_COMPONENT_DECLARE(enet_client_t);

// New: NetworkConfig component for server/client setup
typedef struct {
    bool isNetwork;             // True to enable network, false to disable
    bool isServer;              // True for server mode, false for client
    int port;                   // Port for server listen or client connect
    int maxPeers;               // Max connected peers (e.g., 32)
    const char *address;        // Client: IP to connect to (e.g., "localhost")
} NetworkConfig;
extern ECS_COMPONENT_DECLARE(NetworkConfig);

// New: NetworkState component for connection flags (prevents over-triggering)
typedef struct {
    ENetHost *host;             // ENet host (server or client)
    ENetPeer *peer;             // Client peer (null for server)
    bool serverStarted;         // True after server init (no re-init)
    bool clientConnected;       // True after successful connect (no re-connect)
    bool isConnected;           // Overall connection status
    bool isServer;              // True if server, false if client (mirrors NetworkConfig)
} NetworkState;
extern ECS_COMPONENT_DECLARE(NetworkState);

// Declare event entity as extern for global access
extern ecs_entity_t event_receive_packed;
extern ecs_entity_t event_connect_peer;
extern ecs_entity_t event_disconnect_peer;
extern ecs_entity_t event_disconnect_timeout;

void module_init_enet(ecs_world_t *world);

#endif // MODULE_ENET_H