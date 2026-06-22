#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_INFO_IMPL_ONCE)
#define SDL_LIBRETRO_INFO_IMPL_ONCE

/*
 * SDL_libretro - .info file parser
 */

static char* SDL_Libretro_InfoStrdup(const char* start, size_t len) {
    char* s = (char*)SDL_malloc(len + 1);
    if (s) {
        SDL_memcpy(s, start, len);
        s[len] = '\0';
    }
    return s;
}

static void SDL_Libretro_InfoSetStr(char** field, const char* val, size_t len) {
    SDL_free(*field);
    *field = SDL_Libretro_InfoStrdup(val, len);
}

static bool SDL_Libretro_InfoBool(const char* val, size_t len) {
    return (len == 4 && SDL_strncasecmp(val, "true", 4) == 0);
}

bool SDL_Libretro_LoadCoreInfo_IO(SDL_IOStream* io, SDL_Libretro_CoreInfo* out, bool closeio) {
    if (!io || !out) {
        if (closeio && io) SDL_CloseIO(io);
        SDL_SetError("SDL_libretro: Invalid arguments");
        return false;
    }

    SDL_memset(out, 0, sizeof(*out));

    Sint64 fileSize = SDL_GetIOSize(io);
    if (fileSize <= 0) {
        if (closeio) SDL_CloseIO(io);
        SDL_SetError("SDL_libretro: Empty or unreadable .info file");
        return false;
    }

    char* buf = (char*)SDL_malloc((size_t)fileSize + 1);
    if (!buf) {
        if (closeio) SDL_CloseIO(io);
        return false;
    }

    size_t read = SDL_ReadIO(io, buf, (size_t)fileSize);
    if (closeio) SDL_CloseIO(io);
    buf[read] = '\0';

    char* line = buf;
    while (*line) {
        while (*line == ' ' || *line == '\t') line++;

        char* eol = line;
        while (*eol && *eol != '\n' && *eol != '\r') eol++;

        if (*line == '#' || *line == '\0' || line == eol) {
            line = (*eol) ? eol + 1 : eol;
            if (line > buf && *(line - 1) == '\r' && *line == '\n') line++;
            continue;
        }

        char* eq = line;
        while (eq < eol && *eq != '=') eq++;
        if (eq >= eol) {
            line = (*eol) ? eol + 1 : eol;
            continue;
        }

        char* keyEnd = eq;
        while (keyEnd > line && (*(keyEnd - 1) == ' ' || *(keyEnd - 1) == '\t')) keyEnd--;
        size_t keyLen = (size_t)(keyEnd - line);

        char* valStart = eq + 1;
        while (valStart < eol && (*valStart == ' ' || *valStart == '\t')) valStart++;

        char* valEnd = eol;
        while (valEnd > valStart && (*(valEnd - 1) == ' ' || *(valEnd - 1) == '\t')) valEnd--;

        if (*valStart == '"' && valEnd > valStart + 1 && *(valEnd - 1) == '"') {
            valStart++;
            valEnd--;
        }

        size_t valLen = (size_t)(valEnd - valStart);

#define INFO_STR(name, field) \
    if (keyLen == sizeof(name) - 1 && SDL_strncmp(line, name, keyLen) == 0) { \
        SDL_Libretro_InfoSetStr(&out->field, valStart, valLen); \
    }
#define INFO_BOOL(name, field) \
    if (keyLen == sizeof(name) - 1 && SDL_strncmp(line, name, keyLen) == 0) { \
        out->field = SDL_Libretro_InfoBool(valStart, valLen); \
    }
#define INFO_UINT(name, field) \
    if (keyLen == sizeof(name) - 1 && SDL_strncmp(line, name, keyLen) == 0) { \
        out->field = (unsigned)SDL_atoi(valStart); \
    }

        INFO_STR("display_name", display_name)
        else INFO_STR("corename", corename)
        else INFO_STR("authors", authors)
        else INFO_STR("supported_extensions", supported_extensions)
        else INFO_STR("license", license)
        else INFO_STR("permissions", permissions)
        else INFO_STR("display_version", display_version)
        else INFO_STR("categories", categories)
        else INFO_STR("manufacturer", manufacturer)
        else INFO_STR("systemname", systemname)
        else INFO_STR("systemid", systemid)
        else INFO_STR("database", database)
        else INFO_STR("description", description)
        else INFO_STR("notes", notes)
        else INFO_STR("required_hw_api", required_hw_api)
        else INFO_UINT("firmware_count", firmware_count)
        else INFO_BOOL("supports_no_game", supports_no_game)
        else INFO_BOOL("savestate", savestate)
        else INFO_BOOL("cheats", cheats)
        else INFO_BOOL("hw_render", hw_render)
        else INFO_BOOL("needs_fullpath", needs_fullpath)
        else INFO_BOOL("disk_control", disk_control)
        else INFO_BOOL("is_experimental", is_experimental)

#undef INFO_STR
#undef INFO_BOOL
#undef INFO_UINT

        line = (*eol) ? eol + 1 : eol;
        if (line > buf && *(line - 1) == '\r' && *line == '\n') line++;
    }

    SDL_free(buf);
    return true;
}

bool SDL_Libretro_LoadCoreInfo(const char* path, SDL_Libretro_CoreInfo* out) {
    if (!path || !out) {
        SDL_SetError("SDL_libretro: Invalid arguments");
        return false;
    }
    SDL_IOStream* io = SDL_IOFromFile(path, "rb");
    if (!io) {
        SDL_SetError("SDL_libretro: Failed to open '%s'", path);
        return false;
    }
    return SDL_Libretro_LoadCoreInfo_IO(io, out, true);
}

void SDL_Libretro_FreeCoreInfo(SDL_Libretro_CoreInfo* info) {
    if (!info) return;
    SDL_free(info->display_name);
    SDL_free(info->corename);
    SDL_free(info->authors);
    SDL_free(info->supported_extensions);
    SDL_free(info->license);
    SDL_free(info->permissions);
    SDL_free(info->display_version);
    SDL_free(info->categories);
    SDL_free(info->manufacturer);
    SDL_free(info->systemname);
    SDL_free(info->systemid);
    SDL_free(info->database);
    SDL_free(info->description);
    SDL_free(info->notes);
    SDL_free(info->required_hw_api);
    SDL_memset(info, 0, sizeof(*info));
}

#endif /* SDL_LIBRETRO_INFO_IMPL_ONCE */
