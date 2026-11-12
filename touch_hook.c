#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// SDL2 Event Types
#define SDL_QUIT            0x100
#define SDL_WINDOWEVENT     0x200
#define SDL_KEYDOWN         0x300
#define SDL_KEYUP           0x301
#define SDL_MOUSEMOTION     0x400
#define SDL_MOUSEBUTTONDOWN 0x401
#define SDL_MOUSEBUTTONUP   0x402
#define SDL_FINGERDOWN      0x700
#define SDL_FINGERUP        0x701
#define SDL_FINGERMOTION    0x702

#define SDL_SCANCODE_LCTRL  224
#define SDL_SCANCODE_SPACE  44
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_RIGHT    3

#pragma pack(push, 1)
typedef struct {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t  state;
    uint8_t  repeat;
    uint8_t  padding2;
    uint8_t  padding3;
    struct {
        int32_t  scancode;
        int32_t  sym;
        uint16_t mod;
        uint32_t unused;
    } keysym;
    uint8_t padding[16];
} FakeKeyboardEvent;

typedef struct {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint8_t  button;
    uint8_t  state;
    uint8_t  clicks;
    uint8_t  padding1;
    int32_t  x;
    int32_t  y;
    uint8_t  padding[24];
} FakeMouseButtonEvent;

typedef struct {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint32_t state;
    int32_t  x;
    int32_t  y;
    int32_t  xrel;
    int32_t  yrel;
    uint8_t  padding[20];
} FakeMouseMotionEvent;

typedef struct {
    uint32_t type;
    uint32_t timestamp;
    int64_t  touchId;
    int64_t  fingerId;
    float    x;
    float    y;
    float    dx;
    float    dy;
    float    pressure;
    uint8_t  padding[8];
} FakeTouchFingerEvent;

typedef struct {
    uint32_t type;
    uint8_t  raw[52];
} GenericSDLEvent;
#pragma pack(pop)

typedef int (*PFN_SDL_PollEvent)(void* event);
typedef void (*PFN_SDL_GetWindowSize)(void* window, int* w, int* h);

static HMODULE g_real_sdl = NULL;
static PFN_SDL_PollEvent g_real_PollEvent = NULL;
static PFN_SDL_GetWindowSize g_real_GetWindowSize = NULL;

// Configuration options
static int g_cfg_swipe_dist = 28;       // Pixels needed to trigger swipe
static int g_cfg_swipe_max_ms = 380;    // Max time for swipe attack
static int g_cfg_long_press_ms = 420;   // Time for right-click radial menu
static int g_cfg_combat_motion = 120;   // Synthetic mouse delta sent on swipe

// Gesture tracking states
typedef enum {
    TOUCH_IDLE,
    TOUCH_PENDING,       // Finger down, evaluating gesture (tap vs swipe vs long press)
    TOUCH_SWIPE_ATTACK,  // In active swipe attack
    TOUCH_LONG_PRESSED,  // Radial menu triggered
    TOUCH_DRAGGING       // Normal drag/walk
} TouchGestureState;

static TouchGestureState g_state = TOUCH_IDLE;
static int g_start_x = 0;
static int g_start_y = 0;
static int g_last_x = 0;
static int g_last_y = 0;
static DWORD g_start_time = 0;
static int g_ctrl_held = 0;
static uint32_t g_window_id = 0;

// Synthetic event FIFO queue
#define QUEUE_SIZE 64
static GenericSDLEvent g_queue[QUEUE_SIZE];
static int g_q_head = 0;
static int g_q_tail = 0;

static void queue_push(const GenericSDLEvent* ev) {
    int next = (g_q_tail + 1) % QUEUE_SIZE;
    if (next != g_q_head) {
        g_queue[g_q_tail] = *ev;
        g_q_tail = next;
    }
}

static int queue_pop(GenericSDLEvent* ev) {
    if (g_q_head == g_q_tail) return 0;
    *ev = g_queue[g_q_head];
    g_q_head = (g_q_head + 1) % QUEUE_SIZE;
    return 1;
}

static void emit_key(uint32_t type, int scancode) {
    GenericSDLEvent ev;
    memset(&ev, 0, sizeof(ev));
    FakeKeyboardEvent* ke = (FakeKeyboardEvent*)&ev;
    ke->type = type;
    ke->timestamp = GetTickCount();
    ke->windowID = g_window_id;
    ke->state = (type == SDL_KEYDOWN) ? 1 : 0;
    ke->repeat = 0;
    ke->keysym.scancode = scancode;
    ke->keysym.sym = scancode;
    queue_push(&ev);
}

static void emit_mouse_button(uint32_t type, uint8_t button, int x, int y) {
    GenericSDLEvent ev;
    memset(&ev, 0, sizeof(ev));
    FakeMouseButtonEvent* me = (FakeMouseButtonEvent*)&ev;
    me->type = type;
    me->timestamp = GetTickCount();
    me->windowID = g_window_id;
    me->button = button;
    me->state = (type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
    me->clicks = 1;
    me->x = x;
    me->y = y;
    queue_push(&ev);
}

static void emit_mouse_motion(int x, int y, int xrel, int yrel) {
    GenericSDLEvent ev;
    memset(&ev, 0, sizeof(ev));
    FakeMouseMotionEvent* mm = (FakeMouseMotionEvent*)&ev;
    mm->type = SDL_MOUSEMOTION;
    mm->timestamp = GetTickCount();
    mm->windowID = g_window_id;
    mm->state = g_ctrl_held ? 0 : 1;
    mm->x = x;
    mm->y = y;
    mm->xrel = xrel;
    mm->yrel = yrel;
    queue_push(&ev);
}

static void execute_swipe_combat(int dx, int dy) {
    // Determine primary direction
    int abs_x = abs(dx);
    int abs_y = abs(dy);
    int motion_x = 0;
    int motion_y = 0;

    if (abs_x > abs_y) {
        // Horizontal: Left or Right slash
        motion_x = (dx > 0) ? g_cfg_combat_motion : -g_cfg_combat_motion;
    } else {
        // Vertical: Up (Thrust) or Down (Shield block / defend)
        motion_y = (dy > 0) ? g_cfg_combat_motion : -g_cfg_combat_motion;
    }

    // 1. Enter combat stance (Ctrl down)
    emit_key(SDL_KEYDOWN, SDL_SCANCODE_LCTRL);
    g_ctrl_held = 1;

    // 2. Synthesize mouse motion in attack direction
    emit_mouse_motion(g_start_x + motion_x, g_start_y + motion_y, motion_x, motion_y);

    // 3. Trigger attack action
    emit_mouse_button(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, g_start_x + motion_x, g_start_y + motion_y);
    emit_mouse_button(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, g_start_x + motion_x, g_start_y + motion_y);
}

static void load_config(void) {
    char ini[MAX_PATH];
    GetModuleFileNameA(NULL, ini, MAX_PATH);
    char* slash = strrchr(ini, '\\');
    if (slash) *slash = '\0';
    strcat(ini, "\\silver_touch.ini");

    g_cfg_swipe_dist = GetPrivateProfileIntA("Touch", "SwipeThreshold", 28, ini);
    g_cfg_swipe_max_ms = GetPrivateProfileIntA("Touch", "SwipeMaxTimeMs", 380, ini);
    g_cfg_long_press_ms = GetPrivateProfileIntA("Touch", "LongPressTimeMs", 420, ini);
    g_cfg_combat_motion = GetPrivateProfileIntA("Touch", "CombatMotionDelta", 120, ini);
}

static void check_long_press(void) {
    if (g_state == TOUCH_PENDING) {
        DWORD now = GetTickCount();
        if ((now - g_start_time) >= (DWORD)g_cfg_long_press_ms) {
            int dx = g_last_x - g_start_x;
            int dy = g_last_y - g_start_y;
            if (sqrt((double)(dx * dx + dy * dy)) < 15.0) {
                // Long press completed -> Trigger Right Click (Radial Menu)
                g_state = TOUCH_LONG_PRESSED;
                emit_mouse_button(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, g_start_x, g_start_y);
                emit_mouse_button(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, g_start_x, g_start_y);
            }
        }
    }
}

__declspec(dllexport) int Hook_SDL_PollEvent(void* event) {
    // Check if long press timer matured
    check_long_press();

    // 1. Deliver queued synthetic events first
    if (queue_pop((GenericSDLEvent*)event)) {
        return 1;
    }

    // 2. Resolve real SDL_PollEvent from sdl2_orig.dll
    if (!g_real_PollEvent) {
        if (!g_real_sdl) {
            g_real_sdl = LoadLibraryA("sdl2_orig.dll");
            load_config();
        }
        if (g_real_sdl) {
            g_real_PollEvent = (PFN_SDL_PollEvent)GetProcAddress(g_real_sdl, "SDL_PollEvent");
            g_real_GetWindowSize = (PFN_SDL_GetWindowSize)GetProcAddress(g_real_sdl, "SDL_GetWindowSize");
        }
    }

    if (!g_real_PollEvent) return 0;

    int res = g_real_PollEvent(event);
    if (!res) {
        // While no physical events are coming, keep checking long press timer
        check_long_press();
        if (queue_pop((GenericSDLEvent*)event)) {
            return 1;
        }
        return 0;
    }

    GenericSDLEvent* ge = (GenericSDLEvent*)event;

    // Filter and transform touch / mouse events
    if (ge->type == SDL_MOUSEBUTTONDOWN) {
        FakeMouseButtonEvent* me = (FakeMouseButtonEvent*)ge;
        g_window_id = me->windowID;
        if (me->button == SDL_BUTTON_LEFT) {
            g_state = TOUCH_PENDING;
            g_start_x = me->x;
            g_start_y = me->y;
            g_last_x = me->x;
            g_last_y = me->y;
            g_start_time = GetTickCount();

            // Suppress raw MOUSEBUTTONDOWN from the game until we classify it
            // (Avoids accidental walking before swipe attack or long press is determined)
            return Hook_SDL_PollEvent(event);
        }
    }
    else if (ge->type == SDL_MOUSEMOTION) {
        FakeMouseMotionEvent* mme = (FakeMouseMotionEvent*)ge;
        g_window_id = mme->windowID;
        g_last_x = mme->x;
        g_last_y = mme->y;

        if (g_state == TOUCH_PENDING) {
            int dx = mme->x - g_start_x;
            int dy = mme->y - g_start_y;
            double dist = sqrt((double)(dx * dx + dy * dy));
            DWORD elapsed = GetTickCount() - g_start_time;

            if (dist >= (double)g_cfg_swipe_dist) {
                if (elapsed <= (DWORD)g_cfg_swipe_max_ms) {
                    // FAST SWIPE -> Combat attack!
                    g_state = TOUCH_SWIPE_ATTACK;
                    execute_swipe_combat(dx, dy);
                    return Hook_SDL_PollEvent(event);
                } else {
                    // SLOW DRAG -> Continuous walk
                    g_state = TOUCH_DRAGGING;
                    emit_mouse_button(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, g_start_x, g_start_y);
                    emit_mouse_motion(mme->x, mme->y, mme->xrel, mme->yrel);
                    return Hook_SDL_PollEvent(event);
                }
            } else {
                // Within threshold, keep evaluating
                return Hook_SDL_PollEvent(event);
            }
        }
        else if (g_state == TOUCH_SWIPE_ATTACK) {
            // Already attacking, suppress raw motion
            return Hook_SDL_PollEvent(event);
        }
    }
    else if (ge->type == SDL_MOUSEBUTTONUP) {
        FakeMouseButtonEvent* me = (FakeMouseButtonEvent*)ge;
        g_window_id = me->windowID;
        if (me->button == SDL_BUTTON_LEFT) {
            if (g_state == TOUCH_PENDING) {
                // Short tap without significant motion -> Regular Left Click (Walk / Talk / Interact)
                g_state = TOUCH_IDLE;
                emit_mouse_button(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, g_start_x, g_start_y);
                emit_mouse_button(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, me->x, me->y);
                return Hook_SDL_PollEvent(event);
            }
            else if (g_state == TOUCH_SWIPE_ATTACK) {
                // Finger released after swipe: Release Ctrl
                g_state = TOUCH_IDLE;
                if (g_ctrl_held) {
                    emit_key(SDL_KEYUP, SDL_SCANCODE_LCTRL);
                    g_ctrl_held = 0;
                }
                return Hook_SDL_PollEvent(event);
            }
            else if (g_state == TOUCH_LONG_PRESSED) {
                // Finger released after long press: reset state without emitting left click
                g_state = TOUCH_IDLE;
                return Hook_SDL_PollEvent(event);
            }
            else if (g_state == TOUCH_DRAGGING) {
                // Drag ended
                g_state = TOUCH_IDLE;
            }
        }
    }

    return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
