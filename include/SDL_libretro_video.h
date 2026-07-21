/**
 * SDL_libretro - Video System
 *
 * @file SDL_libretro_video.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VIDEO_IMPL_ONCE)
#define SDL_LIBRETRO_VIDEO_IMPL_ONCE

/**
 * Maps a libretro pixel format over to an SDL_PixelFormat.
 *
 * @internal
 */
static SDL_PixelFormat SDL_Libretro_GetTextureFormat(enum retro_pixel_format fmt) {
    switch (fmt) {
        case RETRO_PIXEL_FORMAT_XRGB8888: return SDL_PIXELFORMAT_XRGB8888;
        case RETRO_PIXEL_FORMAT_RGB565: return SDL_PIXELFORMAT_RGB565;
        case RETRO_PIXEL_FORMAT_0RGB1555: return SDL_PIXELFORMAT_XRGB1555;
        default: return SDL_PIXELFORMAT_RGB565;
    }
}

/**
 * Releases any in-flight software-framebuffer texture lock.
 *
 * Safe to call when no lock is held. Called before re-locking and before the texture is destroyed/recreated, so a core that acquires a buffer can't leave the texture locked or `softwareFramebufferPixels` dangling.
 *
 * @internal
 */
static void SDL_Libretro_ReleaseSoftwareFramebuffer(SDL_Libretro* lr) {
    if (!lr->core.softwareFramebufferPixels) {
        return;
    }
    if (lr->core.texture) {
        SDL_UnlockTexture(lr->core.texture);
    }
    lr->core.softwareFramebufferPixels = NULL;
}

/**
 * Initialize the libretro video system.
 *
 * @internal
 */
static bool SDL_Libretro_InitVideo(SDL_Libretro* lr) {
    if (!lr || !lr->renderer) return false;

    // Make sure we're starting a clean video context.
    SDL_Libretro_CloseVideo(lr);

    // If there is no desired width/height, select an arbitrary one.
    if (lr->core.width == 0 || lr->core.height == 0) {
        lr->core.width = 320;
        lr->core.height = 240;
    }

    // Build the Texture.
    lr->core.texture = SDL_CreateTexture(lr->renderer, SDL_Libretro_GetTextureFormat(lr->core.pixelFormat), SDL_TEXTUREACCESS_STREAMING, lr->core.width, lr->core.height);

    if (!lr->core.texture) {
        SDL_SetError("[SDL_Libretro] Failed to create texture: %s", SDL_GetError());
        return false;
    }
    lr->core.videoReinitPending = false;

    // Scale Mode: nearest upgrades to pixelart when available.
    SDL_ScaleMode scaleMode = lr->scaleMode;
#if SDL_VERSION_ATLEAST(3, 4, 0)
    if (scaleMode == SDL_SCALEMODE_NEAREST)
        scaleMode = SDL_SCALEMODE_PIXELART; // SDL >= 3.4
#endif
    SDL_SetTextureScaleMode(lr->core.texture, scaleMode);
    return true;
}

/**
 * Cleans up any video memory reserved for the core.
 *
 * @internal
 */
static void SDL_Libretro_CloseVideo(SDL_Libretro* lr) {
    if (!lr || !lr->core.texture) return;

    SDL_Libretro_ReleaseSoftwareFramebuffer(lr);
    SDL_DestroyTexture(lr->core.texture);
    lr->core.texture = NULL;
}

static void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return;

    // Software framebuffer (RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER)
    if (lr->core.softwareFramebufferPixels) {
        void* swfb = lr->core.softwareFramebufferPixels;
        SDL_Libretro_ReleaseSoftwareFramebuffer(lr);
        if (data == swfb) return;
    }

    if (!data) return;

    // A hardware-rendered core signals its frame via this sentinel rather than a real pointer; we're software-only, so there's nothing to copy.
    if (data == RETRO_HW_FRAME_BUFFER_VALID) return;

    // Rebuild the texture when the core's frame dimensions change.
    if (width != lr->core.width || height != lr->core.height) {
        lr->core.width = width;
        lr->core.height = height;
        if (!SDL_Libretro_InitVideo(lr)) return;
    }

    // Make sure the texture is workable.
    if (!lr->core.texture) return;

    // Copy straight into the texture's backing memory to avoid the extra staging copy SDL_UpdateTexture() makes.
    void* pixels = NULL;
    int lockPitch = 0;
    if (!SDL_LockTexture(lr->core.texture, NULL, &pixels, &lockPitch)) {
        return;
    }

    // The core's `pitch` and the texture's locked pitch can differ, so copy row by row.
    const Uint8* src = (const Uint8*)data;
    Uint8* dst = (Uint8*)pixels;
    size_t rowBytes = pitch < (size_t)lockPitch ? pitch : (size_t)lockPitch;
    if (pitch == (size_t)lockPitch) {
        // The strides match, so we can do a straight copy.
        SDL_memcpy(dst, src, rowBytes * height);
    }
    else {
        for (size_t y = 0; y < height; y++) {
            SDL_memcpy(dst + y * lockPitch, src + y * pitch, rowBytes);
        }
    }

    SDL_UnlockTexture(lr->core.texture);
}

/**
 * Set the renderer the libretro context draws into.
 *
 * If a game is already loaded, setting the renderer builds the video texture immediately.
 *
 * @param lr the libretro context.
 * @param renderer the renderer to draw into; must not be NULL.
 * @returns true on success, false on invalid arguments or if the texture (re)build fails.
 */
bool SDL_Libretro_SetRenderer(SDL_Libretro* lr, SDL_Renderer* renderer) {
    if (!lr || !renderer) {
        SDL_SetError("[SDL_Libretro] Invalid context");
        return false;
    }

    bool changed = (lr->renderer != renderer);
    lr->renderer = renderer;
    lr->window = SDL_GetRenderWindow(renderer);

    // Build the texture if a game is loaded but has none yet (deferred init after
    // a renderer-less LoadGame), or rebuild it against the new renderer on a swap
    // (a texture is bound to the renderer that created it). With no game loaded,
    // LoadGame's InitVideo handles it once the geometry is known.
    if (lr->core.gameLoaded && (!lr->core.texture || changed)) {
        return SDL_Libretro_InitVideo(lr);
    }

    return true;
}

/**
 * Get the renderer the context draws into.
 *
 * @param lr the libretro context.
 * @returns the renderer set via SDL_Libretro_SetRenderer(), or NULL if none.
 */
SDL_Renderer* SDL_Libretro_GetRenderer(const SDL_Libretro* lr) {
    return lr ? lr->renderer : NULL;
}

SDL_Texture* SDL_Libretro_GetTexture(const SDL_Libretro* lr) {
    return (lr && lr->core.texture) ? lr->core.texture : NULL;
}

/**
 * Creates a new surface from the current libretro context.
 *
 * The Surface must be destroyed after use with SDL_DestroySurface().
 */
SDL_Surface* SDL_Libretro_CreateSurface(const SDL_Libretro* lr) {
    if (!lr || !lr->core.texture || !lr->renderer) return NULL;

    // Blit the current frame 1:1 into an offscreen target texture
    // and read it back.
    SDL_Renderer* renderer = lr->renderer;
    int w = (int)lr->core.width;
    int h = (int)lr->core.height;

    SDL_Texture* target = SDL_CreateTexture(renderer, SDL_Libretro_GetTextureFormat(lr->core.pixelFormat), SDL_TEXTUREACCESS_TARGET, w, h);
    if (!target) return NULL;

    SDL_Texture* prevTarget = SDL_GetRenderTarget(renderer);
    SDL_Surface* out = NULL;
    if (SDL_SetRenderTarget(renderer, target)) {
        // Draw the frame unscaled and unrotated into the target, then read it.
        if (SDL_RenderTexture(renderer, lr->core.texture, NULL, NULL)) {
            out = SDL_RenderReadPixels(renderer, NULL);
        }
        SDL_SetRenderTarget(renderer, prevTarget);
    }

    SDL_DestroyTexture(target);
    return out;
}

/**
 * Shrink `*rect` from the available area to the on-screen rectangle.
 *
 * Letterboxed to the aspect ratio, inverted for a 90/270 turn when
 * `rotated`. Considers the integer-scale mode, snapped to a whole
 * multiple of the native size.
 *
 * @internal
 */
static void SDL_Libretro_FitRect(const SDL_Libretro* lr, SDL_FRect* rect, bool rotated) {
    SDL_FRect avail = *rect;

    // Nothing to fit into a zero-size area, or when we're stretching it.
    if (avail.w <= 0.0f || avail.h <= 0.0f || lr->fitMode == SDL_LIBRETRO_FIT_STRETCH) return;

    // Fix the aspect ratio.
    float srcAspect = lr->core.aspectRatio;
    if (srcAspect <= 0.0f && lr->core.width > 0 && lr->core.height > 0) {
        srcAspect = (float)lr->core.width / (float)lr->core.height;
    }

    // Letterbox to the effective aspect ratio within the available area.
    float aspect = (rotated && srcAspect > 0.0f) ? 1.0f / srcAspect : srcAspect;
    if (aspect > 0.0f) {
        if (aspect > avail.w / avail.h) {
            rect->h = avail.w / aspect;
            rect->y = avail.y + (avail.h - rect->h) * 0.5f;
        }
        else {
            rect->w = avail.h * aspect;
            rect->x = avail.x + (avail.w - rect->w) * 0.5f;
        }
    }

    // Integer scaling: a quarter turn maps the core's width to the vertical
    // extent and its height to the horizontal.
    if (lr->fitMode == SDL_LIBRETRO_FIT_INTEGER && lr->core.width > 0 && lr->core.height > 0) {
        float coreW = rotated ? (float)lr->core.height : (float)lr->core.width;
        float coreH = rotated ? (float)lr->core.width : (float)lr->core.height;
        int scale = SDL_max(1, SDL_min((int)(rect->w / coreW), (int)(rect->h / coreH)));
        float w = coreW * (float)scale;
        float h = coreH * (float)scale;
        rect->x += (rect->w - w) * 0.5f;
        rect->y += (rect->h - h) * 0.5f;
        rect->w = w;
        rect->h = h;
    }
}

/**
 * Render the libretro context in the given renderer.
 *
 * @param renderer the renderer to draw into; must not be NULL.
 * @param lr the libretro context.
 * @param dstRect the destination rectangle, or NULL to fit within the full width and height of the renderer.
 * @returns true on success, false on invalid arguments or if there is nothing to draw yet.
 *
 * @see SDL_Libretro_SetRenderer()
 */
bool SDL_Libretro_Render(SDL_Renderer* renderer, SDL_Libretro* lr, const SDL_FRect* dstRect) {
    if (!lr || !renderer) return false;

    // Adopt the renderer if it changed or isn't initialized.
    if (renderer != lr->renderer && !SDL_Libretro_SetRenderer(lr, renderer)) return false;

    // Requires a texture to render.
    if (!lr->core.texture) return false;

    // Determine the destination area: the given rect, or the full render output.
    if (dstRect) {
        lr->core.renderDstRect = *dstRect;
    }
    else {
        int w, h;
        if (!SDL_GetRenderOutputSize(renderer, &w, &h)) return false;
        lr->core.renderDstRect = (SDL_FRect){0.0f, 0.0f, (float)w, (float)h};
    }

    // A 90/270 turn swaps the image's on-screen extents.
    bool rotated = (lr->core.rotation & 1) != 0;

    // Fit the destination to optimally fit within the renderer.
    SDL_Libretro_FitRect(lr, &lr->core.renderDstRect, rotated);

    // libretro SET_ROTATION is counter-clockwise, but SDL_RenderTextureRotated's
    // angle is clockwise, so negate it.
    double angle = -(lr->core.rotation * 90.0);

    if (rotated) {
        SDL_FRect dst = lr->core.renderDstRect;
        dst.w = lr->core.renderDstRect.h;
        dst.h = lr->core.renderDstRect.w;
        dst.x = lr->core.renderDstRect.x + (lr->core.renderDstRect.w - dst.w) * 0.5f;
        dst.y = lr->core.renderDstRect.y + (lr->core.renderDstRect.h - dst.h) * 0.5f;
        return SDL_RenderTextureRotated(renderer, lr->core.texture, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
    }

    return SDL_RenderTextureRotated(renderer, lr->core.texture, NULL, &lr->core.renderDstRect, angle, NULL, SDL_FLIP_NONE);
}

/**
 * Gets the width and heigh tof the given libretro context.
 */
void SDL_Libretro_GetSize(const SDL_Libretro* lr, int* w, int* h) {
    if (w) *w = lr ? (int)lr->core.width : 0;
    if (h) *h = lr ? (int)lr->core.height : 0;
}

/**
 * Gets the aspect ratio for the libretro context.
 */
float SDL_Libretro_GetAspectRatio(const SDL_Libretro* lr) {
    return lr ? lr->core.aspectRatio : 0.0f;
}

double SDL_Libretro_GetFPS(const SDL_Libretro* lr) {
    return lr ? lr->core.fps : 0.0;
}

int SDL_Libretro_GetRotation(const SDL_Libretro* lr) {
    return lr ? lr->core.rotation * 90 : 0;
}

/**
 * Sets the desired scale mode for the libretro context when it's displayed.
 */
bool SDL_Libretro_SetFitMode(SDL_Libretro* lr, SDL_LibretroFitMode mode) {
    if (!lr) return false;
    if ((int)mode < (int)SDL_LIBRETRO_FIT_FIRST || (int)mode >= (int)SDL_LIBRETRO_FIT_COUNT) {
        return SDL_InvalidParamError("mode");
    }
    lr->fitMode = mode;
    return true;
}

SDL_LibretroFitMode SDL_Libretro_GetFitMode(const SDL_Libretro* lr) {
    return lr ? lr->fitMode : SDL_LIBRETRO_FIT_ASPECT;
}

/**
 * Sets the texture scale mode used when the libretro frame is scaled.
 *
 * Applied immediately when a texture exists, and remembered for the next one otherwise. The setting persists across core loads. By default, SDL_SCALEMODE_NEAREST is upgraded to SDL_SCALEMODE_PIXELART on SDL >= 3.4; explicitly setting SDL_SCALEMODE_NEAREST opts out of that upgrade.
 *
 * @param lr the libretro context.
 * @param mode SDL_SCALEMODE_NEAREST, SDL_SCALEMODE_LINEAR, or SDL_SCALEMODE_PIXELART (SDL >= 3.4).
 * @returns true on success, false on an invalid context or scale mode.
 */
bool SDL_Libretro_SetScaleMode(SDL_Libretro* lr, SDL_ScaleMode mode) {
    if (!lr) return false;
    bool valid = (mode == SDL_SCALEMODE_NEAREST || mode == SDL_SCALEMODE_LINEAR);
#if SDL_VERSION_ATLEAST(3, 4, 0)
    if (mode == SDL_SCALEMODE_PIXELART) valid = true;
#endif
    if (!valid) {
        return SDL_InvalidParamError("mode");
    }
    lr->scaleMode = mode;
    if (lr->core.texture) {
        SDL_SetTextureScaleMode(lr->core.texture, mode);
    }
    return true;
}

SDL_ScaleMode SDL_Libretro_GetScaleMode(const SDL_Libretro* lr) {
    return lr ? lr->scaleMode : SDL_SCALEMODE_NEAREST;
}

#endif /* SDL_LIBRETRO_VIDEO_IMPL_ONCE */
