#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VIDEO_IMPL_ONCE)
#define SDL_LIBRETRO_VIDEO_IMPL_ONCE

/*
 * SDL_libretro - video subsystem
 */


#include <string.h>

static SDL_PixelFormat SDL_Libretro_GetTextureFormat(enum retro_pixel_format fmt) {
    switch (fmt) {
        case RETRO_PIXEL_FORMAT_XRGB8888: return SDL_PIXELFORMAT_XRGB8888;
        case RETRO_PIXEL_FORMAT_RGB565:   return SDL_PIXELFORMAT_RGB565;
        case RETRO_PIXEL_FORMAT_0RGB1555: return SDL_PIXELFORMAT_XRGB1555;
        default:                          return SDL_PIXELFORMAT_RGB565;
    }
}

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

    lr->core.textureRebuild = false;
    return true;
}

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

    // See if the video needs to be reinitialized.
    if (width != lr->core.width || height != lr->core.height) {
        lr->core.width = width;
        lr->core.height = height;
        SDL_Libretro_InitVideo(lr);
    }

    SDL_UpdateTexture(lr->core.texture, NULL, data, (int)pitch);
}

SDL_Texture* SDL_Libretro_GetTexture(const SDL_Libretro* lr) {
    return (lr && lr->core.texture) ? lr->core.texture : NULL;
}

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

    /* Fit within destination maintaining aspect ratio */
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

#endif /* SDL_LIBRETRO_VIDEO_IMPL_ONCE */
