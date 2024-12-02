#ifndef GFX_H
#define GFX_H

#include "ultra64.h"
#include "ultra64/gbi.h"
#include "sched.h"
#include "thga.h"
#include "versions.h"

// Texture memory size, 4 KiB
#define TMEM_SIZE 0x1000

typedef struct GfxPool {
    /* 0x00000 */ u16 headMagic; // GFXPOOL_HEAD_MAGIC
    /* 0x00008 */ Gfx polyOpaBuffer[0x17E0];
    /* 0x0BF08 */ Gfx polyXluBuffer[0x800];
    /* 0x0FF08 */ Gfx overlayBuffer[0x400];
    /* 0x11F08 */ Gfx workBuffer[0x80];
    /* 0x11308 */ Gfx unusedBuffer[0x20];
    /* 0x12408 */ u16 tailMagic; // GFXPOOL_TAIL_MAGIC
} GfxPool; // size = 0x12410

typedef struct GraphicsContext {
    /* 0x0000 */ Gfx* polyOpaBuffer; 
    // Pointer to the buffer for opaque polygons ("Zelda 0").
    // This is used to render solid objects, like walls, buildings, or Link himself, where no transparency is required.

    /* 0x0004 */ Gfx* polyXluBuffer; 
    // Pointer to the buffer for translucent polygons ("Zelda 1").
    // This is used for semi-transparent effects, such as water, fog, and magical effects.

    /* 0x0008 */ char unk_008[0x08]; 
    // Unknown or unused field.
    // Speculated to be reserved for additional rendering buffers or future features.

    /* 0x0010 */ Gfx* overlayBuffer; 
    // Pointer to the buffer for overlays ("Zelda 4").
    // Used for rendering elements like the HUD (health, minimap, text) or menus that appear on top of the gameplay.

    /* 0x0014 */ u32 unk_014;
    // Unknown purpose. Possibly a flag or reserved value for controlling rendering behavior.

    /* 0x0018 */ char unk_018[0x20]; 
    // Reserved space. Its exact use is unclear, possibly used as temporary storage or padding.

    /* 0x0038 */ OSMesg msgBuff[0x08]; 
    // Message buffer for communication.
    // This stores messages sent between the graphics thread and other parts of the game (e.g., scheduling commands).

    /* 0x0058 */ OSMesgQueue* schedMsgQueue; 
    // Pointer to the scheduler's message queue.
    // Used to coordinate tasks like sending rendering jobs or syncing frames.

    /* 0x005C */ OSMesgQueue queue; 
    // Internal message queue specific to the graphics context.
    // Handles graphics-related messages such as rendering commands and status updates.

    /* 0x0078 */ OSScTask task; 
    // Represents a task sent to the RCP (Reality Coprocessor) for rendering.
    // Includes data like the display list (set of graphics commands) to process.

    /* 0x00E0 */ char unk_0E0[0xD0]; 
    // Reserved or unused space.
    // Might be used for temporary data during rendering or padding for alignment.

    /* 0x01B0 */ Gfx* workBuffer; 
    // Pointer to a working buffer for temporary rendering data.
    // This might be used for effects or transitions that need extra space.

    /* 0x01B4 */ TwoHeadGfxArena work; 
    // Arena for managing temporary graphics data.
    // A "two-head" arena is a memory management system that efficiently allocates and deallocates temporary resources.

    /* 0x01C4 */ char unk_01C4[0xC0]; 
    // Reserved space. Purpose unknown.

    /* 0x0284 */ OSViMode* viMode; 
    // Pointer to the current video interface mode.
    // Controls aspects like resolution, scaling, and interlacing for the display output.

    /* 0x0288 */ char unk_0288[0x20]; 
    // Reserved space, speculated to be unused or for future features.

    /* 0x02A8 */ TwoHeadGfxArena overlay; 
    // Arena for managing overlay graphics ("Zelda 4").
    // Used for elements like HUDs or other 2D graphical elements.

    /* 0x02B8 */ TwoHeadGfxArena polyOpa; 
    // Arena for managing opaque polygons ("Zelda 0").
    // Stores and organizes memory for rendering solid, non-transparent objects.

    /* 0x02C8 */ TwoHeadGfxArena polyXlu; 
    // Arena for managing translucent polygons ("Zelda 1").
    // Used for rendering semi-transparent objects and effects like fog or magic.

    /* 0x02D8 */ u32 gfxPoolIdx; 
    // Index for managing the current graphics pool.
    // Graphics pools are memory regions used for rendering tasks.

    /* 0x02DC */ u16* curFrameBuffer; 
    // Pointer to the current framebuffer.
    // The framebuffer is where the final rendered image is stored before being sent to the display.

    /* 0x02E0 */ char unk_2E0[0x04]; 
    // Reserved space. Exact purpose unknown.

    /* 0x02E4 */ u32 viFeatures; 
    // Video interface features.
    // Controls display settings like anti-aliasing, scaling, or interlacing.

    /* 0x02E8 */ s32 fbIdx; 
    // Index of the framebuffer currently in use.
    // Games often alternate between multiple framebuffers to avoid visual tearing.

    /* 0x02EC */ void (*callback)(struct GraphicsContext*, void*); 
    // Pointer to a callback function.
    // This function is executed during certain stages of the graphics pipeline, allowing custom behavior.

    /* 0x02F0 */ void* callbackParam; 
    // Parameter passed to the callback function.
    // This could be additional data needed by the callback.

#if OOT_VERSION >= PAL_1_0
    /* 0x02F4 */ f32 xScale; 
    // Horizontal scaling factor for PAL versions.
    // Used to adjust the screen's width during rendering.

    /* 0x02F8 */ f32 yScale; 
    // Vertical scaling factor for PAL versions.
    // Used to adjust the screen's height during rendering.
#endif

    /* 0x02FC */ char unk_2FC[0x04]; 
    // Reserved space. Exact purpose unknown.
} GraphicsContext; // size = 0x300


Gfx* Gfx_SetFog(Gfx* gfx, s32 r, s32 g, s32 b, s32 a, s32 near, s32 far);
Gfx* Gfx_SetFogWithSync(Gfx* gfx, s32 r, s32 g, s32 b, s32 a, s32 near, s32 far);
Gfx* Gfx_SetFog2(Gfx* gfx, s32 r, s32 g, s32 b, s32 a, s32 near, s32 far);

Gfx* Gfx_BranchTexScroll(Gfx** gfxP, u32 x, u32 y, s32 width, s32 height);
Gfx* func_80094E78(GraphicsContext* gfxCtx, u32 x, u32 y);
Gfx* Gfx_TexScroll(GraphicsContext* gfxCtx, u32 x, u32 y, s32 width, s32 height);
Gfx* Gfx_TwoTexScroll(GraphicsContext* gfxCtx, s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2,
                      u32 y2, s32 width2, s32 height2);
Gfx* Gfx_TwoTexScrollEnvColor(GraphicsContext* gfxCtx, s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2,
                              u32 x2, u32 y2, s32 width2, s32 height2, s32 r, s32 g, s32 b, s32 a);
Gfx* Gfx_EnvColor(GraphicsContext* gfxCtx, s32 r, s32 g, s32 b, s32 a);
void Gfx_SetupFrame(GraphicsContext* gfxCtx, u8 r, u8 g, u8 b);
void func_80095974(GraphicsContext* gfxCtx);

void* Graph_Alloc(GraphicsContext* gfxCtx, size_t size);
void* Graph_Alloc2(GraphicsContext* gfxCtx, size_t size);

#define WORK_DISP       __gfxCtx->work.p
#define POLY_OPA_DISP   __gfxCtx->polyOpa.p
#define POLY_XLU_DISP   __gfxCtx->polyXlu.p
#define OVERLAY_DISP    __gfxCtx->overlay.p

#if DEBUG_FEATURES

void Graph_OpenDisps(Gfx** dispRefs, GraphicsContext* gfxCtx, const char* file, int line);
void Graph_CloseDisps(Gfx** dispRefs, GraphicsContext* gfxCtx, const char* file, int line);

// __gfxCtx shouldn't be used directly.
// Use the DISP macros defined above when writing to display buffers.
#define OPEN_DISPS(gfxCtx, file, line) \
    {                                  \
        GraphicsContext* __gfxCtx;     \
        Gfx* dispRefs[4];              \
        __gfxCtx = gfxCtx;             \
        (void)__gfxCtx;                \
        Graph_OpenDisps(dispRefs, gfxCtx, file, line)

#define CLOSE_DISPS(gfxCtx, file, line)                     \
        do {                                                \
            Graph_CloseDisps(dispRefs, gfxCtx, file, line); \
        } while (0);                                        \
    }                                                       \
    (void)0

#define GRAPH_ALLOC(gfxCtx, size) Graph_Alloc(gfxCtx, size)

#else

#define OPEN_DISPS(gfxCtx, file, line)      \
    {                                       \
        GraphicsContext* __gfxCtx = gfxCtx; \
        s32 __dispPad

#define CLOSE_DISPS(gfxCtx, file, line) \
        do {} while (0);                \
    }                                   \
    (void)0

#define GRAPH_ALLOC(gfxCtx, size) ((void*)((gfxCtx)->polyOpa.d = (Gfx*)((u8*)(gfxCtx)->polyOpa.d - ALIGN16(size))))

#endif

#endif
