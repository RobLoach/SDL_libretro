/**
 * SDL_libretro - on-screen display message queue
 *
 * @file SDL_libretro_messages.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_MESSAGES_IMPL_ONCE)
#define SDL_LIBRETRO_MESSAGES_IMPL_ONCE

static void SDL_Libretro_OsdPush(SDL_Libretro* lr, const char* msg, double durationSec, unsigned priority, enum retro_message_type type, int8_t progress) {
    if (!lr || !msg || msg[0] == '\0') return;

    // Negative (or NaN) durations expire right away, and huge ones are capped so
    // the conversion to milliseconds stays in range.
    if (!(durationSec > 0.0)) durationSec = 0.0;
    if (durationSec > (double)SDL_MAX_UINT32) durationSec = (double)SDL_MAX_UINT32;
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

    // Copy first, so an allocation failure never leaves a slot with a NULL msg.
    char* dup = SDL_strdup(msg);
    if (!dup) return;

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
            if (!newQueue) {
                SDL_free(dup);
                return;
            }
            lr->osdQueue = newQueue;
            lr->osdQueueCapacity = newCap;
        }
        slot = lr->osdQueueCount++;
    }
    lr->osdQueue[slot].msg = dup;
    lr->osdQueue[slot].endTimeMs = endMs;
    lr->osdQueue[slot].priority = priority;
    lr->osdQueue[slot].type = type;
    lr->osdQueue[slot].progress = progress;
}

/**
 * Expires old on-screen-display messages from the OSD queue.
 *
 * @internal
 */
static void SDL_Libretro_OsdExpire(SDL_Libretro* lr) {
    Uint64 now = SDL_GetTicks();
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
        dst++;
    }
    lr->osdQueueCount = dst;
}

/**
 * Gets the message index of the highest priority message.
 *
 * @returns the message index, or -1 if none.
 *
 * @internal
 */
static int SDL_Libretro_OsdFindTop(const SDL_Libretro* lr) {
    Uint64 now = SDL_GetTicks();
    int best = -1;
    unsigned bestPri = 0;

    for (int i = 0; i < lr->osdQueueCount; i++) {
        if (now > lr->osdQueue[i].endTimeMs) continue;
        if (best == -1 || lr->osdQueue[i].priority > bestPri) {
            bestPri = lr->osdQueue[i].priority;
            best = i;
        }
    }
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
 * The message is copied. Setting a message that is already queued refreshes
 * its duration instead of adding a duplicate.
 *
 * @param lr the libretro context.
 * @param msg the message to display, or NULL or an empty string to clear all queued messages.
 * @param duration the amount of time in seconds to display the message.
 *
 * @see SDL_Libretro_GetMessage()
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
 * Gets the most relevant libretro message to display from the queue.
 *
 * This is the highest priority message that has not yet expired. The string is
 * owned by the context and stays valid until the message expires or is replaced.
 *
 * @param lr the libretro context.
 * @returns a string for the message, or NULL if there isn't a message to display.
 *
 * @see SDL_Libretro_GetMessageProgress()
 * @see SDL_Libretro_GetMessageType()
 */
const char* SDL_Libretro_GetMessage(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return NULL;
    int top = SDL_Libretro_OsdFindTop(lr);
    if (top < 0) return NULL;
    return lr->osdQueue[top].msg;
}

/**
 * Gets the progress of the message returned by SDL_Libretro_GetMessage().
 *
 * @param lr the libretro context.
 * @returns the progress as a percentage from 0 to 100, or -1 when the message
 *          has no progress indicator or there is no message to display.
 *
 * @see SDL_Libretro_GetMessage()
 */
int SDL_Libretro_GetMessageProgress(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return -1;
    int top = SDL_Libretro_OsdFindTop(lr);
    if (top < 0) return -1;
    return lr->osdQueue[top].progress;
}

/**
 * Gets the type of the message returned by SDL_Libretro_GetMessage().
 *
 * @param lr the libretro context.
 * @returns the message's `enum retro_message_type` value, such as
 *          RETRO_MESSAGE_TYPE_NOTIFICATION or RETRO_MESSAGE_TYPE_PROGRESS; 0
 *          (RETRO_MESSAGE_TYPE_NOTIFICATION) when there is no message.
 *
 * @see SDL_Libretro_GetMessage()
 */
int SDL_Libretro_GetMessageType(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return 0;
    int top = SDL_Libretro_OsdFindTop(lr);
    if (top < 0) return 0;
    return (int)lr->osdQueue[top].type;
}

/**
 * Gets the number of messages currently in the queue.
 *
 * Will expire any old messages to make sure the count is correct.
 *
 * @param lr the libretro context.
 * @returns the number of queued messages.
 *
 * @see SDL_Libretro_GetMessageByIndex()
 */
unsigned SDL_Libretro_GetMessageCount(SDL_Libretro* lr) {
    if (!lr || lr->osdQueueCount == 0) return 0;
    SDL_Libretro_OsdExpire(lr);
    return (unsigned)lr->osdQueueCount;
}

/**
 * Retrieves all message details for the given message index.
 *
 * @param lr the libretro context.
 * @param index the index of the message to retrieve, from 0 to SDL_Libretro_GetMessageCount() - 1. If negative, gets the most relevant message.
 * @param msg a pointer filled in with the message string, owned by the context, or NULL.
 * @param progress a pointer filled in with the progress percentage (0-100, or -1 for none), or NULL.
 * @param type a pointer filled in with the message's `enum retro_message_type`, or NULL.
 * @returns true if the message exists, false otherwise.
 *
 * @see SDL_Libretro_GetMessageCount()
 */
bool SDL_Libretro_GetMessageByIndex(SDL_Libretro* lr, int index, const char** msg, int* progress, int* type) {
    if (!lr) return false;
    if (index < 0) {
        index = SDL_Libretro_OsdFindTop(lr);
    }
    if (index < 0 || index >= lr->osdQueueCount) return false;
    if (msg) *msg = lr->osdQueue[index].msg;
    if (progress) *progress = lr->osdQueue[index].progress;
    if (type) *type = (int)lr->osdQueue[index].type;
    return true;
}

#endif /* SDL_LIBRETRO_MESSAGES_IMPL_ONCE */
