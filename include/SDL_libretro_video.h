#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VIDEO_IMPL_ONCE)
#define SDL_LIBRETRO_VIDEO_IMPL_ONCE

/*
 * SDL_libretro - video subsystem
 */


#include <string.h>

static void SDL_Libretro_PixelFormatARGB1555ToRGB565(void* output, const void* input,
    int width, int height, int out_stride, int in_stride) {
    const uint16_t* in = (const uint16_t*)input;
    uint16_t* out = (uint16_t*)output;
    int in_pixels = in_stride / 2;
    int out_pixels = out_stride / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t col = in[y * in_pixels + x];
            uint16_t rg = (col << 1) & ((0x1f << 11) | (0x1f << 6));
            uint16_t b = col & 0x1f;
            out[y * out_pixels + x] = rg | b;
        }
    }
}

static SDL_PixelFormat SDL_Libretro_GetTextureFormat(enum retro_pixel_format fmt) {
    switch (fmt) {
        case RETRO_PIXEL_FORMAT_XRGB8888: return SDL_PIXELFORMAT_XRGB8888;
        case RETRO_PIXEL_FORMAT_RGB565:   return SDL_PIXELFORMAT_RGB565;
        case RETRO_PIXEL_FORMAT_0RGB1555: return SDL_PIXELFORMAT_RGB565;
        default:                          return SDL_PIXELFORMAT_RGB565;
    }
}

static int SDL_Libretro_GetBytesPerPixel(enum retro_pixel_format fmt) {
    switch (fmt) {
        case RETRO_PIXEL_FORMAT_XRGB8888: return 4;
        case RETRO_PIXEL_FORMAT_RGB565:   return 2;
        case RETRO_PIXEL_FORMAT_0RGB1555: return 2;
        default:                          return 2;
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

    int bpp = SDL_Libretro_GetBytesPerPixel(lr->core.pixelFormat);
    size_t bufSize = (size_t)lr->core.width * lr->core.height * bpp;
    if (bufSize > lr->core.frameBufferSize) {
        SDL_free(lr->core.frameBuffer);
        lr->core.frameBuffer = SDL_malloc(bufSize);
        lr->core.frameBufferSize = bufSize;
    }

    lr->core.textureRebuild = false;
    return true;
}

static void SDL_Libretro_CloseVideo(SDL_Libretro* lr) {
    if (!lr) return;

    if (lr->core.texture) {
        SDL_DestroyTexture(lr->core.texture);
        lr->core.texture = NULL;
    }
    if (lr->core.frameBuffer) {
        SDL_free(lr->core.frameBuffer);
        lr->core.frameBuffer = NULL;
        lr->core.frameBufferSize = 0;
    }
}

static void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || !data) return;

    if (width != lr->core.width || height != lr->core.height) {
        lr->core.width = width;
        lr->core.height = height;
        SDL_Libretro_InitVideo(lr);
    }

    int bpp = SDL_Libretro_GetBytesPerPixel(lr->core.pixelFormat);

    if (lr->core.pixelFormat == RETRO_PIXEL_FORMAT_0RGB1555) {
        int out_stride = (int)(width * 2);
        SDL_Libretro_PixelFormatARGB1555ToRGB565(
            lr->core.frameBuffer, data,
            (int)width, (int)height,
            out_stride, (int)pitch);
        SDL_UpdateTexture(lr->core.texture, NULL, lr->core.frameBuffer, out_stride);
    } else {
        SDL_UpdateTexture(lr->core.texture, NULL, data, (int)pitch);
    }
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
