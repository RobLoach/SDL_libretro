# Events

SDL_libretro reports what happens through the SDL event queue, as `SDL_UserEvent`s based at `SDL_EVENT_LIBRETRO`. `event->user.data1` is always the `SDL_Libretro*` that sent the event. Every SDL_libretro event falls between `SDL_EVENT_LIBRETRO` and `SDL_EVENT_LIBRETRO_LAST`, a fixed range in the SDL user events. The first `SDL_Libretro_Create()` reserves it through [`SDL_RegisterEvents()`](https://wiki.libsdl.org/SDL3/SDL_RegisterEvents), so events the application registers afterwards never collide with it; events registered before then should stay below `SDL_EVENT_LIBRETRO`, or a warning is logged.

- Environment commands the core calls that SDL_libretro doesn't handle itself arrive as `SDL_EVENT_LIBRETRO_ENVIRONMENT(RETRO_ENVIRONMENT_*)`, for example `SDL_EVENT_LIBRETRO_ENVIRONMENT(RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE)`. Experimental and private commands get event types of their own, so they don't collide with plain commands that share their number, and `SDL_LIBRETRO_ENVIRONMENT_COMMAND(event->type)` turns the type back into the command. Every environment event falls between `SDL_EVENT_LIBRETRO` and `SDL_EVENT_LIBRETRO_ENVIRONMENT_LAST`. The data pointer the core passed rides along in `event->user.data2`.
- `SDL_EVENT_LIBRETRO_CORE_LOADED` / `SDL_EVENT_LIBRETRO_GAME_LOADED`: A core or game finished loading, whether directly or through the menu. `data2` is the core or game name.
- `SDL_EVENT_LIBRETRO_CORE_UNLOADED` / `SDL_EVENT_LIBRETRO_GAME_UNLOADED`: The core or game was unloaded; unloading a core reports the game first.
- `SDL_EVENT_LIBRETRO_SHUTDOWN`: The core requested shutdown, mirroring `SDL_Libretro_ShouldQuit()`.
- `SDL_EVENT_LIBRETRO_GEOMETRY_CHANGED`: The video size, aspect ratio, or timing changed mid-game; query `SDL_Libretro_GetSize()` and friends for the new values.
- `SDL_EVENT_LIBRETRO_OPTIONS_CHANGED`: The core options, their values, or their visibility changed; bursts coalesce into one queued event.
- `SDL_EVENT_LIBRETRO_MESSAGE`: The core queued an on-screen message; `data2` is the text, valid while the message is queued; the event is withdrawn if the message expires first.
- `SDL_EVENT_LIBRETRO_MENU_OPENED` / `SDL_EVENT_LIBRETRO_MENU_CLOSED`: The menu became visible (the game pauses) or was dismissed (the game resumes). `data2` is the `SDL_LibretroMenu*`.

```c
while (SDL_PollEvent(&event)) {
    switch (event.type) {
        case SDL_EVENT_LIBRETRO_GAME_LOADED:
            SDL_Log("Loaded: %s", (const char*)event.user.data2);
            break;
        case SDL_EVENT_LIBRETRO_ENVIRONMENT(RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE):
            SDL_Log("Core asked for location services");
            break;
    }
}
```

An environment command's `data2` pointer is only valid while the core waits inside the environment call, which has already returned by the time the event is polled. To *implement* an environment command, handle the event from an [`SDL_AddEventWatch()`](https://wiki.libsdl.org/SDL3/SDL_AddEventWatch) callback instead — watches run synchronously while the core waits. Set `event->user.code` to a non-zero value there to tell the core the command succeeded; a claimed command is consumed by the watch and doesn't also reach the event queue, while earlier unclaimed copies of the same command stay queued. An [`SDL_SetEventFilter()`](https://wiki.libsdl.org/SDL3/SDL_SetEventFilter) filter can claim a command the same way, and may return `false` to drop the event.

```c
static bool SDLCALL MyEventWatch(void* userdata, SDL_Event* event) {
    if (event->type == SDL_EVENT_LIBRETRO_ENVIRONMENT(RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE)) {
        struct retro_camera_callback* camera = event->user.data2;
        // ... fill in the camera interface ...
        event->user.code = 1; // Tell the core the command succeeded.
    }
    return true;
}

SDL_AddEventWatch(MyEventWatch, NULL);
```
