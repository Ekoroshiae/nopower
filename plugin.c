// NoPower: hides the power button from the system and adds Select+Circle shortcuts.
// Short press (under 1 second)  -> screen off
// Hold (2 seconds or more)      -> shut down
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/power.h>
#include <taihen.h>

#define NUM_HOOKS 6
static SceUID hook_ids[NUM_HOOKS];
static tai_hook_ref_t refs[NUM_HOOKS];
static int hook_count = 0;
static volatile int running = 1;
static SceUID thread_id = -1;

static void filter(SceCtrlData *pad, int n) {
  for (int i = 0; i < n; i++)
    pad[i].buttons &= ~SCE_CTRL_POWER;   // pretend the power button is never pressed
}

#define MAKE_HOOK(idx, fname)                                              \
  static int fname(int port, SceCtrlData *pad, int count) {                \
    int ret = TAI_CONTINUE(int, refs[idx], port, pad, count);              \
    if (ret > 0) filter(pad, ret);                                         \
    return ret;                                                            \
  }

MAKE_HOOK(0, hook_kpeek)
MAKE_HOOK(1, hook_kread)
MAKE_HOOK(2, hook_upeek)
MAKE_HOOK(3, hook_uread)
MAKE_HOOK(4, hook_upeek2)
MAKE_HOOK(5, hook_uread2)

static void add_hook(int idx, uint32_t nid, void *func) {
  hook_ids[idx] = taiHookFunctionExport(&refs[idx], "SceCtrl", TAI_ANY_LIBRARY, nid, func);
}

static int combo_thread(SceSize args, void *argp) {
  SceCtrlData pad;
  uint64_t start = 0;
  int holding = 0, shutdown_done = 0;

  while (running) {
    ksceKernelDelayThread(20 * 1000);
    if (ksceCtrlPeekBufferPositive(0, &pad, 1) < 1) continue;

    int combo = (pad.buttons & SCE_CTRL_SELECT) && (pad.buttons & SCE_CTRL_CIRCLE);
    uint64_t now = ksceKernelGetSystemTimeWide();

    if (combo && !holding) {
      holding = 1; shutdown_done = 0; start = now;
    } else if (combo && holding) {
      if (!shutdown_done && (now - start) >= 2000000ULL) {
        shutdown_done = 1;
        kscePowerRequestStandby();
      }
    } else if (!combo && holding) {
      holding = 0;
      if (!shutdown_done && (now - start) < 1000000ULL)
        kscePowerRequestDisplayOff();
    }
  }
  return 0;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {
  add_hook(0, 0xEA1D3A34, hook_kpeek);   // kernel Peek
  add_hook(1, 0x9B96A1AA, hook_kread);   // kernel Read
  add_hook(2, 0xA9C3CED6, hook_upeek);   // user Peek
  add_hook(3, 0x67E7AB83, hook_uread);   // user Read
  add_hook(4, 0x15F81E8C, hook_upeek2);  // user Peek2
  add_hook(5, 0xC4226A3E, hook_uread2);  // user Read2
  hook_count = NUM_HOOKS;

  thread_id = ksceKernelCreateThread("nopower_combo", combo_thread, 0x3C, 0x1000, 0, 0, NULL);
  if (thread_id >= 0) ksceKernelStartThread(thread_id, 0, NULL);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  running = 0;
  if (thread_id >= 0) {
    ksceKernelWaitThreadEnd(thread_id, NULL, NULL);
    ksceKernelDeleteThread(thread_id);
  }
  for (int i = 0; i < hook_count; i++)
    if (hook_ids[i] >= 0) taiHookReleaseForKernel(hook_ids[i], refs[i]);
  return SCE_KERNEL_STOP_SUCCESS;
}
