
#include <assert.h>
#include "../../src/host/window_sdl.c"
static RuntimeGamepadState observed;
int deck_imu_start(void) { return 0; }
void runtime_gamepad_set(const RuntimeGamepadState *s) { observed = *s; }
void runtime_motion_live(int on) { (void)on; }
int runtime_motion_axes(const char *s) { (void)s; return 0; }
int main(void) {
 SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
 assert(SDL_Init(SDL_INIT_GAMEPAD));
 pad_lock = SDL_CreateMutex(); assert(pad_lock);
 SDL_VirtualJoystickDesc desc; SDL_INIT_INTERFACE(&desc);
 desc.type = SDL_JOYSTICK_TYPE_GAMEPAD; desc.naxes = 6; desc.nbuttons = 15;
 desc.button_mask = (1u << 15) - 1; desc.axis_mask = (1u << 6) - 1;
 desc.name = "Idle receiver test"; SDL_JoystickID first = SDL_AttachVirtualJoystick(&desc); assert(first);
 desc.name = "Wired controller test"; SDL_JoystickID second = SDL_AttachVirtualJoystick(&desc); assert(second);
 open_pad(first); assert(SDL_GetGamepadID(pad) == first);
 SDL_Joystick *j = SDL_OpenJoystick(second); assert(j);
 select_pad(second); assert(SDL_GetGamepadID(pad) == second);
 setenv("TOTK_CONTROLLER_LAYOUT", "xbox-letters", 1);
 for (int button = 0; button < 4; button++) {
  assert(SDL_SetJoystickVirtualButton(j, button, true)); SDL_UpdateJoysticks(); pad_update();
  assert(observed.buttons == (1u << button));
  assert(SDL_SetJoystickVirtualButton(j, button, false)); SDL_UpdateJoysticks();
 }
 SDL_CloseJoystick(j); SDL_CloseGamepad(pad); pad = NULL;
 SDL_DetachVirtualJoystick(first); SDL_DetachVirtualJoystick(second);
 SDL_DestroyMutex(pad_lock); SDL_Quit();
 return 0;
}
