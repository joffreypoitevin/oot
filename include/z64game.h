#ifndef Z64GAME_H
#define Z64GAME_H
// This is an include guard. It ensures that the contents of this header file
// are included only once in the program, even if it's included multiple times
// in different files.

#include "ultra64/ultratypes.h" // Includes basic type definitions (e.g., u32, s32, etc.).
#include "padmgr.h"             // Includes functionality for handling controllers (Pad Manager).
#include "tha.h"                // Includes tools for managing memory arenas (TwoHeadArena).

struct GraphicsContext; // Declares a structure for managing rendering resources (defined elsewhere).

// Structure for managing a single memory allocation in a linked list.
typedef struct GameAllocEntry {
    /* 0x00 */ struct GameAllocEntry* next; // Pointer to the next allocation in the linked list.
    /* 0x04 */ struct GameAllocEntry* prev; // Pointer to the previous allocation in the list.
    /* 0x08 */ u32 size; // Size of the allocated memory block (in bytes).
    /* 0x0C */ u32 unk_0C; // Unknown purpose (possibly flags or metadata about the allocation).
} GameAllocEntry; // This structure has a size of 16 bytes (0x10).

// Structure for managing all memory allocations as a linked list.
typedef struct GameAlloc {
    /* 0x00 */ GameAllocEntry base; // The base (starting point) of the allocation list.
    /* 0x10 */ GameAllocEntry* head; // Pointer to the head of the list (most recent allocation).
} GameAlloc; // This structure has a size of 20 bytes (0x14).

// These macros help define game states in the game. They allow mapping between
// game state types (like "menu" or "gameplay") and their corresponding identifiers.
#define DEFINE_GAMESTATE_INTERNAL(typeName, enumName) enumName,
#define DEFINE_GAMESTATE(typeName, enumName, name) DEFINE_GAMESTATE_INTERNAL(typeName, enumName)

// Enum for identifying different game states (like menus or levels).
typedef enum GameStateId {
#include "tables/gamestate_table.h" // This file defines the list of game states (e.g., MENU, GAMEPLAY).
    GAMESTATE_ID_MAX // A special value representing the maximum number of game states.
} GameStateId;

// Undefine the macros after use to avoid conflicts elsewhere in the program.
#undef DEFINE_GAMESTATE
#undef DEFINE_GAMESTATE_INTERNAL

struct GameState; // Declares the GameState structure (defined later).

// Defines a function pointer type for functions that operate on a GameState.
// This is used for functions like `main` (update loop), `init` (initialization),
// and `destroy` (cleanup) in each game state.
typedef void (*GameStateFunc)(struct GameState* gameState);

// Structure representing the current state of the game.
typedef struct GameState {
    /* 0x00 */ struct GraphicsContext* gfxCtx; // Pointer to the graphics context (manages rendering).
    /* 0x04 */ GameStateFunc main;    // Function pointer for the main loop of this game state.
    /* 0x08 */ GameStateFunc destroy; // Function pointer for cleaning up this game state.
    /* 0x0C */ GameStateFunc init;    // Function pointer for initializing this game state.
    /* 0x10 */ u32 size;              // Size of this game state (used for memory allocation).
    /* 0x14 */ Input input[MAXCONTROLLERS]; // Array of inputs (one for each controller).
    /* 0x74 */ TwoHeadArena tha;      // TwoHeadArena for managing temporary allocations for this state.
    /* 0x84 */ GameAlloc alloc;       // Allocator for dynamic memory in this state.
    /* 0x98 */ u32 running;           // Flag indicating if this state is running (true or false).
    /* 0x9C */ u32 frames;            // Frame counter (increments every frame in this state).
    /* 0xA0 */ u32 inPreNMIState;     // Flag for handling the "Pre-NMI" (reset or power-off) state.
} GameState; // This structure has a size of 164 bytes (0xA4).

#endif // End of include guard.
