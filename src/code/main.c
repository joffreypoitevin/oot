#include "ultra64.h"
#include "versions.h"

// External variables declared for use throughout the code
extern uintptr_t gSegments[NUM_SEGMENTS]; // Memory segments used for loading and processing game data.

#pragma increment_block_number "gc-eu:252 ..." // Version-specific configurations. Used for ensuring the correct build setup.

// Declares pointers to major managers in the game system (e.g., rendering, input, interrupts)
extern struct PreNmiBuff* gAppNmiBufferPtr; // Pointer to a buffer used when a non-maskable interrupt (NMI) occurs.
extern struct Scheduler gScheduler;        // Scheduler for managing tasks like rendering and audio updates.
extern struct PadMgr gPadMgr;              // Manager for handling player input (e.g., controllers).
extern struct IrqMgr gIrqMgr;              // Interrupt manager for handling hardware-level events.

#include "global.h"
#include "fault.h"              // Handles game crashes and error reporting.
#include "segmented_address.h"  // Provides utilities for working with memory segments.
#include "stack.h"              // Tools for managing thread stacks.
#include "terminal.h"           // Provides debugging and logging utilities.
#include "versions.h"
#if PLATFORM_N64
#include "cic6105.h"            // Specific to N64 systems for boot and memory management.
#include "n64dd.h"              // Nintendo 64 Disk Drive support, if applicable.
#endif

#pragma increment_block_number "gc-eu:160 ..." // Another version-specific configuration block.

// Declares an external buffer address used during memory allocation.
extern u8 _buffersSegmentEnd[];

s32 gScreenWidth = SCREEN_WIDTH;  // Global variable for the screen width.
s32 gScreenHeight = SCREEN_HEIGHT; // Global variable for the screen height.
u32 gSystemHeapSize = 0;           // Size of the system heap (dynamic memory for the game).

// Definitions of global variables
PreNmiBuff* gAppNmiBufferPtr; // Buffer for handling NMIs.
Scheduler gScheduler;         // Task scheduler.
PadMgr gPadMgr;               // Player input manager.
IrqMgr gIrqMgr;               // Interrupt manager.
uintptr_t gSegments[NUM_SEGMENTS]; // Array of memory segment addresses.

// Threads and their stacks
OSThread sGraphThread;          // Thread responsible for graphics processing.
STACK(sGraphStack, 0x1800);     // Stack memory for the graphics thread.
#if OOT_VERSION < PAL_1_0
STACK(sSchedStack, 0x400);      // Smaller scheduler stack for earlier versions.
#else
STACK(sSchedStack, 0x600);      // Larger scheduler stack for PAL versions.
#endif
STACK(sAudioStack, 0x800);      // Stack for the audio thread.
STACK(sPadMgrStack, 0x500);     // Stack for the controller (input) manager thread.
STACK(sIrqMgrStack, 0x500);     // Stack for the interrupt manager thread.

// Stack entry metadata (used for debugging and safety checks)
StackEntry sGraphStackInfo;
StackEntry sSchedStackInfo;
StackEntry sAudioStackInfo;
StackEntry sPadMgrStackInfo;
StackEntry sIrqMgrStackInfo;

// Other system components
AudioMgr sAudioMgr;               // Audio manager responsible for sound effects and music.
OSMesgQueue sSerialEventQueue;    // Queue for serial events (e.g., controller inputs).
OSMesg sSerialMsgBuf[1];          // Buffer for serial messages.

#if DEBUG_FEATURES
// Debugging function to log the system heap's state
void Main_LogSystemHeap(void) {
    PRINTF(VT_FGCOL(GREEN)); // Change text output color to green.
    PRINTF(
        T("システムヒープサイズ %08x(%dKB) 開始アドレス %08x\n", "System heap size %08x (%dKB) Start address %08x\n"),
        gSystemHeapSize, gSystemHeapSize / 1024, _buffersSegmentEnd); // Log heap size and address.
    PRINTF(VT_RST); // Reset text color.
}
#endif

// Main function: the game's primary entry point.
void Main(void* arg) {
    // Declares variables for managing interrupts, memory, and framebuffers.
    IrqMgrClient irqClient;
    OSMesgQueue irqMgrMsgQueue;
    OSMesg irqMgrMsgBuf[60];
    uintptr_t systemHeapStart;
    uintptr_t fb;

    PRINTF(T("mainproc 実行開始\n", "mainproc Start running\n")); // Log that the main process is starting.

    // Initialize screen dimensions.
    gScreenWidth = SCREEN_WIDTH;
    gScreenHeight = SCREEN_HEIGHT;

    // Initialize the NMI buffer for handling system resets.
    gAppNmiBufferPtr = (PreNmiBuff*)osAppNMIBuffer;
    PreNmiBuff_Init(gAppNmiBufferPtr);

    // Initialize the error-handling system (used for crashes and debug logs).
    Fault_Init();

#if PLATFORM_N64
    // Platform-specific initialization for the N64.
    func_800AD410();
    if (D_80121211 != 0) {
        // If a certain condition is met, use a specific memory segment for the heap.
        systemHeapStart = (uintptr_t)_n64ddSegmentEnd;
        SysCfb_Init(1); // Advanced framebuffer mode.
    } else {
        // Default memory segment and basic framebuffer mode.
        func_800AD488();
        systemHeapStart = (uintptr_t)_buffersSegmentEnd;
        SysCfb_Init(0);
    }
#else
    // For non-N64 platforms, use the default buffer segment.
    SysCfb_Init(0);
    systemHeapStart = (uintptr_t)_buffersSegmentEnd;
#endif

    // Set up the system heap (memory allocated for dynamic game data).
    fb = (uintptr_t)SysCfb_GetFbPtr(0); // Get the first framebuffer pointer.
    gSystemHeapSize = fb - systemHeapStart; // Calculate heap size as the memory between the heap start and framebuffer.
    PRINTF(T("システムヒープ初期化 %08x-%08x %08x\n", "System heap initialization %08x-%08x %08x\n"), systemHeapStart,
           fb, gSystemHeapSize); // Log the heap range and size.
    SystemHeap_Init((void*)systemHeapStart, gSystemHeapSize);

#if DEBUG_FEATURES
    {
        // Debugging: Initialize an arena for debug-specific memory allocations.
        void* debugHeapStart;
        u32 debugHeapSize;

        if (osMemSize >= 0x800000) {
            debugHeapStart = SysCfb_GetFbEnd();
            debugHeapSize = PHYS_TO_K0(0x600000) - (uintptr_t)debugHeapStart;
        } else {
            debugHeapSize = 0x400; // Small debug heap for low-memory systems.
            debugHeapStart = SYSTEM_ARENA_MALLOC(debugHeapSize, "../main.c", 565);
        }

        PRINTF("debug_InitArena(%08x, %08x)\n", debugHeapStart, debugHeapSize); // Log debug heap info.
        DebugArena_Init(debugHeapStart, debugHeapSize); // Set up debug heap.
    }
#endif

    // Initialize system-wide registers and disable arena debugging.
    Regs_Init();
    R_ENABLE_ARENA_DBG = 0;

    // Create a message queue for serial events.
    osCreateMesgQueue(&sSerialEventQueue, sSerialMsgBuf, ARRAY_COUNT(sSerialMsgBuf));
    osSetEventMesg(OS_EVENT_SI, &sSerialEventQueue, NULL);

#if DEBUG_FEATURES
    Main_LogSystemHeap(); // Log the system heap (debug-only).
#endif

    // Initialize the interrupt manager and related resources.
    osCreateMesgQueue(&irqMgrMsgQueue, irqMgrMsgBuf, ARRAY_COUNT(irqMgrMsgBuf));
    StackCheck_Init(&sIrqMgrStackInfo, sIrqMgrStack, STACK_TOP(sIrqMgrStack), 0, 0x100, "irqmgr");
    IrqMgr_Init(&gIrqMgr, STACK_TOP(sIrqMgrStack), THREAD_PRI_IRQMGR, 1);

    PRINTF(T("タスクスケジューラの初期化\n", "Initialize the task scheduler\n")); // Log task scheduler initialization.
    StackCheck_Init(&sSchedStackInfo, sSchedStack, STACK_TOP(sSchedStack), 0, 0x100, "sched");
    Sched_Init(&gScheduler, STACK_TOP(sSchedStack), THREAD_PRI_SCHED, gViConfigModeType, 1, &gIrqMgr);

    // Platform-specific setup for N64 systems.
#if PLATFORM_N64
    CIC6105_AddFaultClient();
    func_80001640();
#endif

    // Add the interrupt manager client to handle system interrupts.
    IrqMgr_AddClient(&gIrqMgr, &irqClient, &irqMgrMsgQueue);

    // Initialize audio management.
    StackCheck_Init(&sAudioStackInfo, sAudioStack, STACK_TOP(sAudioStack), 0, 0x100, "audio");
    AudioMgr_Init(&sAudioMgr, STACK_TOP(sAudioStack), THREAD_PRI_AUDIOMGR, THREAD_ID_AUDIOMGR, &gScheduler, &gIrqMgr);

    // Initialize the controller (input) manager.
    StackCheck_Init(&sPadMgrStackInfo, sPadMgrStack, STACK_TOP(sPadMgrStack), 0, 0x100, "padmgr");
    PadMgr_Init(&gPadMgr, &sSerialEventQueue, &gIrqMgr, THREAD_ID_PADMGR, THREAD_PRI_PADMGR, STACK_TOP(sPadMgrStack));

    // Wait for audio to finish initializing.
    AudioMgr_WaitForInit(&sAudioMgr);

    // Initialize the graphics thread.
    StackCheck_Init(&sGraphStackInfo, sGraphStack, STACK_TOP(sGraphStack), 0, 0x100, "graph");
    osCreateThread(&sGraphThread, THREAD_ID_GRAPH, Graph_ThreadEntry, arg, STACK_TOP(sGraphStack), THREAD_PRI_GRAPH);
    osStartThread(&sGraphThread);

#if OOT_VERSION >= PAL_1_0
    osSetThreadPri(NULL, THREAD_PRI_MAIN); // Lower the priority of the main thread (PAL versions only).
#endif

    // Main loop: Handle interrupts and clean up on reset.
    while (true) {
        s16* msg = NULL;

        // Wait for a message in the interrupt manager's queue.
        osRecvMesg(&irqMgrMsgQueue, (OSMesg*)&msg, OS_MESG_BLOCK);
        if (msg == NULL) {
            break; // Exit loop if no message is received.
        }
        switch (*msg) {
            case OS_SC_PRE_NMI_MSG: // Handle pre-NMI messages (system reset).
                PRINTF(T("main.c: リセットされたみたいだよ\n", "main.c: Looks like it's been reset\n"));
#if OOT_VERSION < PAL_1_0
                StackCheck_Check(NULL); // Debug stack check (older versions).
#endif
                PreNmiBuff_SetReset(gAppNmiBufferPtr); // Mark reset in the NMI buffer.
                break;
        }
    }

    PRINTF(T("mainproc 後始末\n", "mainproc Cleanup\n")); // Log cleanup.
    osDestroyThread(&sGraphThread); // Destroy the graphics thread.
    RcpUtils_Reset(); // Reset the Reality Coprocessor (graphics system).
#if PLATFORM_N64
    CIC6105_RemoveFaultClient(); // Clean up CIC fault clients (N64-specific).
#endif
    PRINTF(T("mainproc 実行終了\n", "mainproc End of execution\n")); // Log process end.
}
