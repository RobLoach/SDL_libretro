#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VIDEO_IMPL_ONCE)
#define SDL_LIBRETRO_VIDEO_IMPL_ONCE

/*
 * SDL_libretro - video subsystem
 */

/**
 * Maps a libretro pixel format over to an SDL_PixelFormat.
 *
 * @internal
 */
static SDL_PixelFormat SDL_Libretro_GetTextureFormat(enum retro_pixel_format fmt) {
    switch (fmt) {
        case RETRO_PIXEL_FORMAT_XRGB8888: return SDL_PIXELFORMAT_XRGB8888;
        case RETRO_PIXEL_FORMAT_RGB565:   return SDL_PIXELFORMAT_RGB565;
        case RETRO_PIXEL_FORMAT_0RGB1555: return SDL_PIXELFORMAT_XRGB1555;
        default:                          return SDL_PIXELFORMAT_RGB565;
    }
}

/**
 * Initialize the libretro video system.
 *
 * @internal
 */
static bool SDL_Libretro_InitVideo(SDL_Libretro* lr) {
    if (!lr || !lr->core.renderer) return false;

    if (lr->core.texture) {
        SDL_DestroyTexture(lr->core.texture);
        lr->core.texture = NULL;
    }

    if (lr->core.width == 0 || lr->core.height == 0) {
        lr->core.width = 320;
        lr->core.height = 240;
    }

    SDL_PixelFormat fmt = SDL_Libretro_GetTextureFormat(lr->core.pixelFormat);
    lr->core.texture = SDL_CreateTexture(lr->core.renderer, fmt,
        SDL_TEXTUREACCESS_STREAMING, lr->core.width, lr->core.height);

    if (!lr->core.texture) {
        SDL_SetError("SDL_libretro: Failed to create texture: %s", SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(lr->core.texture, SDL_SCALEMODE_NEAREST);
    lr->core.videoReinitPending = false;
    return true;
}

/**
 * Cleans up any video memory reserved for the core.
 *
 * @internal
 */
static void SDL_Libretro_CloseVideo(SDL_Libretro* lr) {
    if (!lr) return;

    if (lr->core.texture) {
        SDL_DestroyTexture(lr->core.texture);
        lr->core.texture = NULL;
    }
}

static void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || !data) return;

    // Rebuild the texture when the core's frame dimensions change.
    if (width != lr->core.width || height != lr->core.height) {
        lr->core.width = width;
        lr->core.height = height;
        SDL_Libretro_InitVideo(lr);
    }

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
    for (size_t y = 0; y < height; y++) {
        SDL_memcpy(dst + y * lockPitch, src + y * pitch, rowBytes);
    }

    SDL_UnlockTexture(lr->core.texture);
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
    if (!lr || !lr->core.texture) return NULL;

    void* pixels = NULL;
    int pitch = 0;
    if (!SDL_LockTexture(lr->core.texture, NULL, &pixels, &pitch)) return NULL;

    SDL_PixelFormat fmt = SDL_Libretro_GetTextureFormat(lr->core.pixelFormat);
    SDL_Surface* ref = SDL_CreateSurfaceFrom((int)lr->core.width, (int)lr->core.height, fmt, pixels, pitch);
    SDL_Surface* out = NULL;
    if (ref) {
        out = SDL_CreateSurface((int)lr->core.width, (int)lr->core.height, fmt);
        if (out) SDL_BlitSurface(ref, NULL, out, NULL);
        SDL_DestroySurface(ref);
    }

    SDL_UnlockTexture(lr->core.texture);
    return out;
}

/**
 * Render the libretro context, using the provided scale method in the loaded renderer.
 *
 * @param lr The libretro context.
 * @param dstRect The desintation rectangle, or NULL to fit within the full width and height of the renderer.
 */
bool SDL_Libretro_Render(SDL_Libretro* lr, const SDL_FRect* dstRect) {
    if (!lr || !lr->core.texture || !lr->core.renderer) return false;

    SDL_FRect dst;
    if (dstRect) {
        dst = *dstRect;
    } else {
        int w, h;
        SDL_GetRenderOutputSize(lr->core.renderer, &w, &h);
        dst.x = 0;
        dst.y = 0;
        dst.w = (float)w;
        dst.h = (float)h;
    }

    float srcAspect = lr->core.aspectRatio;
    if (srcAspect <= 0.0f && lr->core.width > 0 && lr->core.height > 0) {
        srcAspect = (float)lr->core.width / (float)lr->core.height;
    }

    // Fit within destination maintaining aspect ratio
    if (srcAspect > 0.0f) {
        float dstAspect = dst.w / dst.h;
        if (srcAspect > dstAspect) {
            float newH = dst.w / srcAspect;
            dst.y += (dst.h - newH) * 0.5f;
            dst.h = newH;
        } else {
            float newW = dst.h * srcAspect;
            dst.x += (dst.w - newW) * 0.5f;
            dst.w = newW;
        }
    }

    // Snap to integer multiples of core resolution
    if (lr->scaleMode == SDL_LIBRETRO_SCALE_INTEGER && lr->core.width > 0 && lr->core.height > 0) {
        float coreW = (float)lr->core.width;
        float coreH = (float)lr->core.height;
        int scaleX = (int)(dst.w / coreW);
        int scaleY = (int)(dst.h / coreH);
        int scale = scaleX < scaleY ? scaleX : scaleY;
        if (scale < 1) scale = 1;
        float intW = coreW * (float)scale;
        float intH = coreH * (float)scale;
        dst.x += (dst.w - intW) * 0.5f;
        dst.y += (dst.h - intH) * 0.5f;
        dst.w = intW;
        dst.h = intH;
    }

    lr->core.renderDstRect = dst;

    double angle = lr->core.rotation * 90.0;
    SDL_FPoint center = { dst.w * 0.5f, dst.h * 0.5f };

    return SDL_RenderTextureRotated(lr->core.renderer, lr->core.texture,
        NULL, &dst, angle, &center, SDL_FLIP_NONE);
}

void SDL_Libretro_GetSize(const SDL_Libretro* lr, int* w, int* h) {
    if (w) *w = lr ? (int)lr->core.width : 0;
    if (h) *h = lr ? (int)lr->core.height : 0;
}

float SDL_Libretro_GetAspectRatio(const SDL_Libretro* lr) {
    return lr ? lr->core.aspectRatio : 0.0f;
}

double SDL_Libretro_GetFPS(const SDL_Libretro* lr) {
    return lr ? lr->core.fps : 0.0;
}

int SDL_Libretro_GetRotation(const SDL_Libretro* lr) {
    return lr ? lr->core.rotation * 90 : 0;
}

void SDL_Libretro_SetScaleMode(SDL_Libretro* lr, SDL_LibretroScaleMode mode) {
    if (lr) lr->scaleMode = mode;
}

SDL_LibretroScaleMode SDL_Libretro_GetScaleMode(const SDL_Libretro* lr) {
    return lr ? lr->scaleMode : SDL_LIBRETRO_SCALE_ASPECT;
}

#endif /* SDL_LIBRETRO_VIDEO_IMPL_ONCE */
