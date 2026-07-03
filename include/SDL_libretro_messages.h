/**
 * SDL_libretro - on-screen display message queue
 *
 * @file SDL_libretro_messages.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_MESSAGES_IMPL_ONCE)
#define SDL_LIBRETRO_MESSAGES_IMPL_ONCE

static void SDL_Libretro_OsdPush(SDL_Libretro* lr, const char* msg, double durationSec, unsigned priority, enum retro_message_type type, int8_t progress) {
    if (!lr || !msg || msg[0] == '\0') return;

    Uint64 endMs = SDL_GetTicks() + (Uint64)(durationSec * 1000.0);

    for (int i = 0; i < lr->osdQueueCount; i++) {
        if (lr->osdQueue[i].msg && SDL_strcmp(lr->osdQueue[i].msg, msg) == 0) {
            lr->osdQueue[i].endTimeMs = endMs;
            lr->osdQueue[i].priority = priority;
            lr->osdQueue[i].type = type;
            lr->osdQueue[i].progress = progress;
            return;
        }
    }

    Uint64 now = SDL_GetTicks();
    int slot = -1;
    for (int i = 0; i < lr->osdQueueCount; i++) {
        if (now > lr->osdQueue[i].endTimeMs) {
            SDL_free(lr->osdQueue[i].msg);
            lr->osdQueue[i].msg = NULL;
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        if (lr->osdQueueCount >= lr->osdQueueCapacity) {
            int newCap = lr->osdQueueCapacity ? lr->osdQueueCapacity * 2 : SDL_LIBRETRO_OSD_INITIAL_CAPACITY;
            SDL_LibretroOsdEntry* newQueue = (SDL_LibretroOsdEntry*)SDL_realloc(lr->osdQueue, (size_t)newCap * sizeof(SDL_LibretroOsdEntry));
            if (!newQueue) return;
            lr->osdQueue = newQueue;
            lr->osdQueueCapacity = newCap;
        }
        slot = lr->osdQueueCount++;
    }
    lr->osdQueue[slot].msg = SDL_strdup(msg);
    lr->osdQueue[slot].endTimeMs = endMs;
    lr->osdQueue[slot].priority = priority;
    lr->osdQueue[slot].type = type;
    lr->osdQueue[slot].progress = progress;
}

static int SDL_Libretro_OsdFindTop(SDL_Libretro* lr) {
    Uint64 now = SDL_GetTicks();
    int best = -1;
    unsigned bestPri = 0;
    int dst = 0;

    for (int i = 0; i < lr->osdQueueCount; i++) {
        if (now > lr->osdQueue[i].endTimeMs) {
            SDL_free(lr->osdQueue[i].msg);
            lr->osdQueue[i].msg = NULL;
            continue;
        }
        if (dst != i) {
            lr->osdQueue[dst] = lr->osdQueue[i];
        }
        if (best == -1 || lr->osdQueue[dst].priority > bestPri) {
            bestPri = lr->osdQueue[dst].priority;
            best = dst;
        }
        dst++;
    }
    lr->osdQueueCount = dst;
    return best;
}

static void SDL_Libretro_FreeMessages(SDL_Libretro* lr) {
    for (int i = 0; i < lr->osdQueueCount; i++) {
        SDL_free(lr->osdQueue[i].msg);
    }
    SDL_free(lr->osdQueue);
    lr->osdQueue = NULL;
    lr->osdQueueCount = 0;
    lr->osdQueueCapacity = 0;
}

/**
 * Add a message to be displayed within the libretro context.
 *
 * @param the libretro context.
 * @param msg The message to display.
 * @param duration The amount of time in seconds to display the message.
 */
void SDL_Libretro_SetMessage(SDL_Libretro* lr, const char* msg, double duration) {
    if (!lr) return;
    if (!msg || msg[0] == '\0') {
        for (int i = 0; i < lr->osdQueueCount; i++) {
            SDL_free(lr->osdQueue[i].msg);
        }
        lr->osdQueueCount = 0;
        return;
    }
    SDL_Libretro_OsdPush(lr, msg, duration, 0, RETRO_MESSAGE_TYPE_NOTIFICATION, -1);
}

/**
 * Gets the most relevent libretro message to display from the queue.
 *
 * @return A string for the message, or NULL if there isn't a message to display.
 */
const char* SDL_Libretro_GetMessage(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return NULL;
    int top = SDL_Libretro_OsdFindTop(lr);
    if (top < 0) return NULL;
    return lr->osdQueue[top].msg;
}

int SDL_Libretro_GetMessageProgress(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return -1;
    int top = SDL_Libretro_OsdFindTop(lr);
    if (top < 0) return -1;
    return lr->osdQueue[top].progress;
}

int SDL_Libretro_GetMessageType(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return 0;
    int top = SDL_Libretro_OsdFindTop(lr);
    if (top < 0) return 0;
    return (int)lr->osdQueue[top].type;
}

unsigned SDL_Libretro_GetMessageCount(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return 0;
    SDL_Libretro_OsdFindTop(lr);
    return (unsigned)lr->osdQueueCount;
}

/**
 * Retrieves all message details for the given message index.
 *
 * @param index The index of the message to retrieve. If -1, will get the most relevent message.
 */
bool SDL_Libretro_GetMessageByIndex(SDL_Libretro* lr, int index,
    const char** msg, int* progress, int* type) {
    if (!lr) return false;
    if (index < 0) {
        index = SDL_Libretro_OsdFindTop(lr);
    }
    else {
        SDL_Libretro_OsdFindTop(lr);
    }
    if (index < 0 || index >= lr->osdQueueCount) return false;
    if (msg) *msg = lr->osdQueue[index].msg;
    if (progress) *progress = lr->osdQueue[index].progress;
    if (type) *type = (int)lr->osdQueue[index].type;
    return true;
}

#endif /* SDL_LIBRETRO_MESSAGES_IMPL_ONCE */
