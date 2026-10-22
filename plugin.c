// NoPower (hide-only version): makes the system think the power button is never pressed.
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/ctrl.h>
#include <taihen.h>

#ifndef KERNEL_PID
#define KERNEL_PID 0x10005
#endif

static SceUID hook_ids[2] = {-1, -1};
static tai_hook_ref_t refs[2];

static int hook_peek(int port, SceCtrlData *pad, int count) {
  int ret = TAI_CONTINUE(int, refs[0], port, pad, count);
  if (ret > 0 && ret <= 64)
    for (int i = 0; i < ret; i++)
      pad[i].buttons &= ~SCE_CTRL_POWER;
  return ret;
}

static int hook_read(int port, SceCtrlData *pad, int count) {
  int ret = TAI_CONTINUE(int, refs[1], port, pad, count);
  if (ret > 0 && ret <= 64)
    for (int i = 0; i < ret; i++)
      pad[i].buttons &= ~SCE_CTRL_POWER;
  return ret;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {
  hook_ids[0] = taiHookFunctionExportForKernel(KERNEL_PID, &refs[0], "SceCtrl", TAI_ANY_LIBRARY, 0xEA1D3A34, hook_peek);
  hook_ids[1] = taiHookFunctionExportForKernel(KERNEL_PID, &refs[1], "SceCtrl", TAI_ANY_LIBRARY, 0x9B96A1AA, hook_read);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  if (hook_ids[0] >= 0) taiHookReleaseForKernel(hook_ids[0], refs[0]);
  if (hook_ids[1] >= 0) taiHookReleaseForKernel(hook_ids[1], refs[1]);
  return SCE_KERNEL_STOP_SUCCESS;
}
