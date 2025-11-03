#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "UI/nuklear.h"
#include "UI/nuklear_sdl_renderer.h"
#include "libretro.h"
#include "glad.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#define getcwd _getcwd
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#endif

#define MAX_PATH_LENGTH 1024
#define MAX_FILES 512

// ============================================================================
// File Manager Structures
// ============================================================================
typedef struct {
    char name[256];
    int is_directory;
    long size;
} FileEntry;

typedef struct {
    char current_path[MAX_PATH_LENGTH];
    FileEntry files[MAX_FILES];
    int file_count;
    int selected_index;
    int scroll_offset;
    int visible_items;
} FileManager;

typedef struct {
    SDL_GameController *controller;
    int button_states[SDL_CONTROLLER_BUTTON_MAX];
    int prev_button_states[SDL_CONTROLLER_BUTTON_MAX];
    Uint32 repeat_timer;
    Uint32 repeat_delay;
    int dpad_up_held;
    int dpad_down_held;
} GamepadState;

// ============================================================================
// Emulator Structures
// ============================================================================
static struct retro_frame_time_callback runloop_frame_time;
static retro_usec_t runloop_frame_time_last = 0;
static const uint8_t *g_kbd = NULL;
static struct retro_audio_callback audio_callback;

static float g_scale = 3;
bool running = true;
bool emulator_running = false;

static struct {
    GLuint tex_id;
    GLuint fbo_id;
    GLuint rbo_id;
    int glmajor;
    int glminor;
    GLuint pitch;
    GLint tex_w, tex_h;
    GLuint clip_w, clip_h;
    GLuint pixfmt;
    GLuint pixtype;
    GLuint bpp;
    struct retro_hw_render_callback hw;
} g_video = {0};

static struct {
    GLuint vao;
    GLuint vbo;
    GLuint program;
    GLint i_pos;
    GLint i_coord;
    GLint u_tex;
    GLint u_mvp;
} g_shader = {0};

static struct retro_variable *g_vars = NULL;
void *handle;
bool initialized;
bool supports_no_game;
struct retro_perf_counter* perf_counter_last;

static SDL_Window *g_win = NULL;
static SDL_GLContext g_ctx = NULL;
static SDL_AudioDeviceID g_pcm = 0;

struct keymap {
    unsigned k;
    unsigned rk;
};

static struct keymap g_binds[] = {
    { SDL_SCANCODE_X, RETRO_DEVICE_ID_JOYPAD_A },
    { SDL_SCANCODE_Z, RETRO_DEVICE_ID_JOYPAD_B },
    { SDL_SCANCODE_A, RETRO_DEVICE_ID_JOYPAD_Y },
    { SDL_SCANCODE_S, RETRO_DEVICE_ID_JOYPAD_X },
    { SDL_SCANCODE_UP, RETRO_DEVICE_ID_JOYPAD_UP },
    { SDL_SCANCODE_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN },
    { SDL_SCANCODE_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT },
    { SDL_SCANCODE_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT },
    { SDL_SCANCODE_RETURN, RETRO_DEVICE_ID_JOYPAD_START },
    { SDL_SCANCODE_BACKSPACE, RETRO_DEVICE_ID_JOYPAD_SELECT },
    { SDL_SCANCODE_Q, RETRO_DEVICE_ID_JOYPAD_L },
    { SDL_SCANCODE_W, RETRO_DEVICE_ID_JOYPAD_R },
    { 0, 0 }
};

static unsigned g_joy[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };

// ============================================================================
// Shader Sources
// ============================================================================
static const char *g_vshader_src =
    "#version 150\n"
    "in vec2 i_pos;\n"
    "in vec2 i_coord;\n"
    "out vec2 o_coord;\n"
    "uniform mat4 u_mvp;\n"
    "void main() {\n"
        "o_coord = i_coord;\n"
        "gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;\n"
    "}";

static const char *g_fshader_src =
    "#version 150\n"
    "in vec2 o_coord;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
        "gl_FragColor = texture2D(u_tex, o_coord);\n"
    "}";

// ============================================================================
// Utility Functions
// ============================================================================
static void die(const char *fmt, ...) {
    char buffer[4096];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);
    fputs(buffer, stderr);
    fputc('\n', stderr);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

// ============================================================================
// File Manager Functions
// ============================================================================
long get_file_size(const char *path) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &info)) {
        LARGE_INTEGER size;
        size.LowPart = info.nFileSizeLow;
        size.HighPart = info.nFileSizeHigh;
        return (long)size.QuadPart;
    }
    return 0;
#else
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
#endif
}

void list_directory(FileManager *fm) {
    fm->file_count = 0;
    
#ifdef _WIN32
    WIN32_FIND_DATAA ffd;
    HANDLE hFind;
    char search_path[MAX_PATH_LENGTH];
    
    snprintf(search_path, sizeof(search_path), "%s\\*", fm->current_path);
    hFind = FindFirstFileA(search_path, &ffd);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    
    do {
        if (fm->file_count >= MAX_FILES) break;
        if (strcmp(ffd.cFileName, ".") == 0) continue;
        
        FileEntry *entry = &fm->files[fm->file_count++];
        strncpy(entry->name, ffd.cFileName, sizeof(entry->name) - 1);
        entry->is_directory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        if (!entry->is_directory) {
            char full_path[MAX_PATH_LENGTH];
            snprintf(full_path, sizeof(full_path), "%s\\%s", fm->current_path, entry->name);
            entry->size = get_file_size(full_path);
        } else {
            entry->size = 0;
        }
    } while (FindNextFileA(hFind, &ffd) != 0);
    
    FindClose(hFind);
#else
    DIR *dir = opendir(fm->current_path);
    if (!dir) return;
    
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && fm->file_count < MAX_FILES) {
        if (strcmp(ent->d_name, ".") == 0) continue;
        
        FileEntry *entry = &fm->files[fm->file_count++];
        strncpy(entry->name, ent->d_name, sizeof(entry->name) - 1);
        
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", fm->current_path, ent->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            entry->is_directory = S_ISDIR(st.st_mode);
            entry->size = entry->is_directory ? 0 : st.st_size;
        } else {
            entry->is_directory = 0;
            entry->size = 0;
        }
    }
    closedir(dir);
#endif
    
    // Sort: directories first, then files
    for (int i = 0; i < fm->file_count - 1; i++) {
        for (int j = i + 1; j < fm->file_count; j++) {
            if (fm->files[i].is_directory < fm->files[j].is_directory ||
                (fm->files[i].is_directory == fm->files[j].is_directory &&
                 strcmp(fm->files[i].name, fm->files[j].name) > 0)) {
                FileEntry temp = fm->files[i];
                fm->files[i] = fm->files[j];
                fm->files[j] = temp;
            }
        }
    }
}

void change_directory(FileManager *fm, const char *dir) {
    char new_path[MAX_PATH_LENGTH];
    
    if (strcmp(dir, "..") == 0) {
        char *last_sep = strrchr(fm->current_path, PATH_SEPARATOR);
        if (last_sep && last_sep != fm->current_path) {
            *last_sep = '\0';
        }
    } else {
        snprintf(new_path, sizeof(new_path), "%s%c%s", 
                 fm->current_path, PATH_SEPARATOR, dir);
        strncpy(fm->current_path, new_path, sizeof(fm->current_path) - 1);
    }
    
    list_directory(fm);
    fm->selected_index = 0;
    fm->scroll_offset = 0;
}

void format_size(long size, char *buffer, size_t buf_size) {
    if (size < 1024) {
        snprintf(buffer, buf_size, "%ld B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buf_size, "%.2f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, buf_size, "%.2f MB", size / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, buf_size, "%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
}

bool is_rom_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    const char *ext = filename + len - 4;
    return (strcasecmp(ext, ".z64") == 0 || strcasecmp(ext, ".n64") == 0);
}

void init_gamepad(GamepadState *gp) {
    gp->controller = NULL;
    gp->repeat_timer = 0;
    gp->repeat_delay = 150;
    gp->dpad_up_held = 0;
    gp->dpad_down_held = 0;
    
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        gp->button_states[i] = 0;
        gp->prev_button_states[i] = 0;
    }
    
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            gp->controller = SDL_GameControllerOpen(i);
            if (gp->controller) {
                printf("Gamepad connected: %s\n", SDL_GameControllerName(gp->controller));
                break;
            }
        }
    }
}

void update_gamepad(GamepadState *gp) {
    if (!gp->controller) return;
    
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        gp->prev_button_states[i] = gp->button_states[i];
    }
    
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        gp->button_states[i] = SDL_GameControllerGetButton(gp->controller, i);
    }
}

int button_pressed(GamepadState *gp, SDL_GameControllerButton button) {
    return gp->button_states[button] && !gp->prev_button_states[button];
}

// ============================================================================
// Emulator Core Functions
// ============================================================================
static void core_log(enum retro_log_level level, const char *fmt, ...) {
    char buffer[4096] = {0};
    static const char * levelstr[] = { "dbg", "inf", "wrn", "err" };
    va_list va;

    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);

    if (level == 0)
        return;

    fprintf(stderr, "[%s] %s", levelstr[level], buffer);
    fflush(stderr);

    if (level == RETRO_LOG_ERROR)
        exit(EXIT_FAILURE);
}

static uintptr_t core_get_current_framebuffer() {
    return g_video.fbo_id;
}

retro_time_t cpu_features_get_time_usec(void) {
    return (retro_time_t)SDL_GetTicks() * 1000;
}

static uint64_t core_get_cpu_features() {
    uint64_t cpu = 0;
    if (SDL_HasAVX()) cpu |= RETRO_SIMD_AVX;
    if (SDL_HasAVX2()) cpu |= RETRO_SIMD_AVX2;
    if (SDL_HasMMX()) cpu |= RETRO_SIMD_MMX;
    if (SDL_HasSSE()) cpu |= RETRO_SIMD_SSE;
    if (SDL_HasSSE2()) cpu |= RETRO_SIMD_SSE2;
    if (SDL_HasSSE3()) cpu |= RETRO_SIMD_SSE3;
    if (SDL_HasSSE41()) cpu |= RETRO_SIMD_SSE4;
    if (SDL_HasSSE42()) cpu |= RETRO_SIMD_SSE42;
    return cpu;
}

static retro_perf_tick_t core_get_perf_counter() {
    return (retro_perf_tick_t)SDL_GetPerformanceCounter();
}

static void core_perf_register(struct retro_perf_counter* counter) {
    perf_counter_last = counter;
    counter->registered = true;
}

static void core_perf_start(struct retro_perf_counter* counter) {
    if (counter->registered) {
        counter->start = core_get_perf_counter();
    }
}

static void core_perf_stop(struct retro_perf_counter* counter) {
    counter->total = core_get_perf_counter() - counter->start;
}

static void core_perf_log() {
    core_log(RETRO_LOG_INFO, "[timer] %s: %i - %i", perf_counter_last->ident, perf_counter_last->start, perf_counter_last->total);
}

static bool video_set_pixel_format(unsigned format) {
    switch (format) {
    case RETRO_PIXEL_FORMAT_0RGB1555:
        g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
        g_video.pixtype = GL_BGRA;
        g_video.bpp = sizeof(uint16_t);
        break;
    case RETRO_PIXEL_FORMAT_XRGB8888:
        g_video.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
        g_video.pixtype = GL_BGRA;
        g_video.bpp = sizeof(uint32_t);
        break;
    case RETRO_PIXEL_FORMAT_RGB565:
        g_video.pixfmt  = GL_UNSIGNED_SHORT_5_6_5;
        g_video.pixtype = GL_RGB;
        g_video.bpp = sizeof(uint16_t);
        break;
    default:
        die("Unknown pixel type %u", format);
    }
    return true;
}

static bool core_environment(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_VARIABLES: {
        const struct retro_variable *vars = (const struct retro_variable *)data;
        size_t num_vars = 0;

        for (const struct retro_variable *v = vars; v->key; ++v) {
            num_vars++;
        }

        g_vars = (struct retro_variable*)calloc(num_vars + 1, sizeof(*g_vars));
        for (unsigned i = 0; i < num_vars; ++i) {
            const struct retro_variable *invar = &vars[i];
            struct retro_variable *outvar = &g_vars[i];

            const char *semicolon = strchr(invar->value, ';');
            const char *first_pipe = strchr(invar->value, '|');

            SDL_assert(semicolon && *semicolon);
            semicolon++;
            while (isspace(*semicolon))
                semicolon++;

            if (first_pipe) {
                outvar->value = malloc((first_pipe - semicolon) + 1);
                memcpy((char*)outvar->value, semicolon, first_pipe - semicolon);
                ((char*)outvar->value)[first_pipe - semicolon] = '\0';
            } else {
                outvar->value = strdup(semicolon);
            }

            outvar->key = strdup(invar->key);
            SDL_assert(outvar->key && outvar->value);
        }
        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *var = (struct retro_variable *)data;
        if (!g_vars) return false;
        for (const struct retro_variable *v = g_vars; v->key; ++v) {
            if (strcmp(var->key, v->key) == 0) {
                var->value = v->value;
                break;
            }
        }
        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
        bool *bval = (bool*)data;
        *bval = false;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
        struct retro_log_callback *cb = (struct retro_log_callback *)data;
        cb->log = core_log;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
        struct retro_perf_callback *perf = (struct retro_perf_callback *)data;
        perf->get_time_usec = cpu_features_get_time_usec;
        perf->get_cpu_features = core_get_cpu_features;
        perf->get_perf_counter = core_get_perf_counter;
        perf->perf_register = core_perf_register;
        perf->perf_start = core_perf_start;
        perf->perf_stop = core_perf_stop;
        perf->perf_log = core_perf_log;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
        bool *bval = (bool*)data;
        *bval = true;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
        const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
        if (*fmt > RETRO_PIXEL_FORMAT_RGB565)
            return false;
        return video_set_pixel_format(*fmt);
    }
    case RETRO_ENVIRONMENT_SET_HW_RENDER: {
        struct retro_hw_render_callback *hw = (struct retro_hw_render_callback*)data;
        hw->get_current_framebuffer = core_get_current_framebuffer;
        hw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
        g_video.hw = *hw;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
        const struct retro_frame_time_callback *frame_time =
            (const struct retro_frame_time_callback*)data;
        runloop_frame_time = *frame_time;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
        struct retro_audio_callback *audio_cb = (struct retro_audio_callback*)data;
        audio_callback = *audio_cb;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char**)data;
        *dir = ".";
        return true;
    }
    case RETRO_ENVIRONMENT_SET_GEOMETRY: {
        const struct retro_game_geometry *geom = (const struct retro_game_geometry *)data;
        g_video.clip_w = geom->base_width;
        g_video.clip_h = geom->base_height;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
        supports_no_game = *(bool*)data;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
        int *value = (int*)data;
        *value = 1 << 0 | 1 << 1;
        return true;
    }
    default:
        core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
        return false;
    }
    return false;
}

// ============================================================================
// OpenGL Functions
// ============================================================================
static GLuint compile_shader(unsigned type, unsigned count, const char **strings) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, count, strings, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        char buffer[4096];
        glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
        die("Failed to compile %s shader: %s", type == GL_VERTEX_SHADER ? "vertex" : "fragment", buffer);
    }

    return shader;
}

void ortho2d(float m[4][4], float left, float right, float bottom, float top) {
    m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
    m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;
    m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
    m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;

    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -1.0f;
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
}

static void init_shaders() {
    GLuint vshader = compile_shader(GL_VERTEX_SHADER, 1, &g_vshader_src);
    GLuint fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &g_fshader_src);
    GLuint program = glCreateProgram();

    SDL_assert(program);

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    glValidateProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if(status == GL_FALSE) {
        char buffer[4096];
        glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
        die("Failed to link shader program: %s", buffer);
    }

    g_shader.program = program;
    g_shader.i_pos   = glGetAttribLocation(program,  "i_pos");
    g_shader.i_coord = glGetAttribLocation(program,  "i_coord");
    g_shader.u_tex   = glGetUniformLocation(program, "u_tex");
    g_shader.u_mvp   = glGetUniformLocation(program, "u_mvp");

    glGenVertexArrays(1, &g_shader.vao);
    glGenBuffers(1, &g_shader.vbo);

    glUseProgram(g_shader.program);
    glUniform1i(g_shader.u_tex, 0);

    float m[4][4];
    if (g_video.hw.bottom_left_origin)
        ortho2d(m, -1, 1, 1, -1);
    else
        ortho2d(m, -1, 1, -1, 1);

    glUniformMatrix4fv(g_shader.u_mvp, 1, GL_FALSE, (float*)m);
    glUseProgram(0);
}

static void refresh_vertex_data() {
    SDL_assert(g_video.tex_w);
    SDL_assert(g_video.tex_h);
    SDL_assert(g_video.clip_w);
    SDL_assert(g_video.clip_h);

    float bottom = (float)g_video.clip_h / g_video.tex_h;
    float right  = (float)g_video.clip_w / g_video.tex_w;

    float vertex_data[] = {
        -1.0f, -1.0f, 0.0f,  bottom,
        -1.0f,  1.0f, 0.0f,  0.0f,
         1.0f, -1.0f, right,  bottom,
         1.0f,  1.0f, right,  0.0f,
    };

    glBindVertexArray(g_shader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_shader.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STREAM_DRAW);

    glEnableVertexAttribArray(g_shader.i_pos);
    glEnableVertexAttribArray(g_shader.i_coord);
    glVertexAttribPointer(g_shader.i_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, 0);
    glVertexAttribPointer(g_shader.i_coord, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void init_framebuffer(int width, int height) {
    glGenFramebuffers(1, &g_video.fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, g_video.fbo_id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_video.tex_id, 0);

    if (g_video.hw.depth && g_video.hw.stencil) {
        glGenRenderbuffers(1, &g_video.rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, g_video.rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_video.rbo_id);
    } else if (g_video.hw.depth) {
        glGenRenderbuffers(1, &g_video.rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, g_video.rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_video.rbo_id);
    }

    if (g_video.hw.depth || g_video.hw.stencil)
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void resize_cb(int w, int h) {
    glViewport(0, 0, w, h);
}

static void create_window(int width, int height) {
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    if (g_video.hw.context_type == RETRO_HW_CONTEXT_OPENGL_CORE || g_video.hw.version_major >= 3) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, g_video.hw.version_major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, g_video.hw.version_minor);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    switch (g_video.hw.context_type) {
    case RETRO_HW_CONTEXT_OPENGL_CORE:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;
    case RETRO_HW_CONTEXT_OPENGLES2:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        break;
    case RETRO_HW_CONTEXT_OPENGL:
        if (g_video.hw.version_major >= 3)
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        break;
    default:
        die("Unsupported hw context %i. (only OPENGL, OPENGL_CORE and OPENGLES2 supported)", g_video.hw.context_type);
    }

    g_win = SDL_CreateWindow("N64 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);

    if (!g_win)
        die("Failed to create window: %s", SDL_GetError());

    g_ctx = SDL_GL_CreateContext(g_win);
    SDL_GL_MakeCurrent(g_win, g_ctx);

    if (!g_ctx)
        die("Failed to create OpenGL context: %s", SDL_GetError());

    if (g_video.hw.context_type == RETRO_HW_CONTEXT_OPENGLES2) {
        if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress))
            die("Failed to initialize glad.");
    } else {
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
            die("Failed to initialize glad.");
    }

    fprintf(stderr, "GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    fprintf(stderr, "GL_VERSION: %s\n", glGetString(GL_VERSION));

    init_shaders();
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SwapWindow(g_win);
    resize_cb(width, height);
}

static void resize_to_aspect(double ratio, int sw, int sh, int *dw, int *dh) {
    *dw = sw;
    *dh = sh;

    if (ratio <= 0)
        ratio = (double)sw / sh;

    if ((float)sw / sh < 1)
        *dw = *dh * ratio;
    else
        *dh = *dw / ratio;
}

static void video_configure(const struct retro_game_geometry *geom) {
    int nwidth, nheight;

    resize_to_aspect(geom->aspect_ratio, geom->base_width * 1, geom->base_height * 1, &nwidth, &nheight);

    nwidth *= g_scale;
    nheight *= g_scale;

    if (!g_win)
        create_window(nwidth, nheight);

    if (g_video.tex_id) {
        glDeleteTextures(1, &g_video.tex_id);
        g_video.tex_id = 0;
    }

    glGenTextures(1, &g_video.tex_id);
    glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    const char* version = (const char*)glGetString(GL_VERSION);
    bool is_gles = version && strstr(version, "OpenGL ES");
    
    GLint internalFormat = is_gles ? GL_RGBA : GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 
                 geom->max_width, geom->max_height, 0,
                 format, type, NULL);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("glTexImage2D failed with error: 0x%X, trying fallback\n", error);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     geom->max_width, geom->max_height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    init_framebuffer(geom->max_width, geom->max_height);

    g_video.tex_w = geom->max_width;
    g_video.tex_h = geom->max_height;
    g_video.clip_w = geom->base_width;
    g_video.clip_h = geom->base_height;

    refresh_vertex_data();

    g_video.hw.context_reset();
}

static void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch) {
    if (g_video.clip_w != width || g_video.clip_h != height) {
        g_video.clip_h = height;
        g_video.clip_w = width;
        refresh_vertex_data();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

    if (pitch != g_video.pitch)
        g_video.pitch = pitch;

    if (data && data != RETRO_HW_FRAME_BUFFER_VALID) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, g_video.pitch / g_video.bpp);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        g_video.pixtype, g_video.pixfmt, data);
    }

    int w = 0, h = 0;
    SDL_GetWindowSize(g_win, &w, &h);
    glViewport(0, 0, w, h);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(g_shader.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

    glBindVertexArray(g_shader.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);

    SDL_GL_SwapWindow(g_win);
}

static void video_deinit() {
    if (g_video.fbo_id)
        glDeleteFramebuffers(1, &g_video.fbo_id);

    if (g_video.tex_id)
        glDeleteTextures(1, &g_video.tex_id);

    if (g_shader.vao)
        glDeleteVertexArrays(1, &g_shader.vao);

    if (g_shader.vbo)
        glDeleteBuffers(1, &g_shader.vbo);

    if (g_shader.program)
        glDeleteProgram(g_shader.program);

    g_video.fbo_id = 0;
    g_video.tex_id = 0;
    g_shader.vao = 0;
    g_shader.vbo = 0;
    g_shader.program = 0;

    if (g_ctx) {
        SDL_GL_MakeCurrent(g_win, g_ctx);
        SDL_GL_DeleteContext(g_ctx);
        g_ctx = NULL;
    }

    if (g_win) {
        SDL_DestroyWindow(g_win);
        g_win = NULL;
    }
}

static void audio_init(int frequency) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;

    SDL_zero(desired);
    SDL_zero(obtained);

    desired.format = AUDIO_S16;
    desired.freq   = frequency;
    desired.channels = 2;
    desired.samples = 4096;

    g_pcm = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!g_pcm)
        die("Failed to open playback device: %s", SDL_GetError());

    SDL_PauseAudioDevice(g_pcm, 0);

    if (audio_callback.set_state) {
        audio_callback.set_state(true);
    }
}

static void audio_deinit() {
    if (g_pcm) {
        SDL_CloseAudioDevice(g_pcm);
        g_pcm = 0;
    }
}

static size_t audio_write(const int16_t *buf, unsigned frames) {
    SDL_QueueAudio(g_pcm, buf, sizeof(*buf) * frames * 2);
    return frames;
}

static void core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    video_refresh(data, width, height, pitch);
}

static void core_input_poll(void) {
    g_kbd = SDL_GetKeyboardState(NULL);

    for (int i = 0; g_binds[i].k || g_binds[i].rk; ++i)
        g_joy[g_binds[i].rk] = g_kbd[g_binds[i].k];

    if (g_kbd[SDL_SCANCODE_ESCAPE])
        running = false;
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (port || index || device != RETRO_DEVICE_JOYPAD)
        return 0;

    return g_joy[id];
}

static void core_audio_sample(int16_t left, int16_t right) {
    int16_t buf[2] = {left, right};
    audio_write(buf, 1);
}

static size_t core_audio_sample_batch(const int16_t *data, size_t frames) {
    return audio_write(data, frames);
}

static void core_load_game(const char *filename) {
    struct retro_system_av_info av = {0};
    struct retro_system_info system = {0};
    struct retro_game_info info = { filename, 0 };

    info.path = filename;
    info.meta = "";
    info.data = NULL;
    info.size = 0;

    if (filename) {
        retro_get_system_info(&system);

        if (!system.need_fullpath) {
            SDL_RWops *file = SDL_RWFromFile(filename, "rb");
            Sint64 size;

            if (!file)
                die("Failed to load %s: %s", filename, SDL_GetError());

            size = SDL_RWsize(file);

            if (size < 0)
                die("Failed to query game file size: %s", SDL_GetError());

            info.size = size;
            info.data = SDL_malloc(info.size);

            if (!info.data)
                die("Failed to allocate memory for the content");

            if (!SDL_RWread(file, (void*)info.data, info.size, 1))
                die("Failed to read file data: %s", SDL_GetError());

            SDL_RWclose(file);
        }
    }

    if (!retro_load_game(&info))
        die("The core failed to load the content.");

    retro_get_system_av_info(&av);

    video_configure(&av.geometry);
    audio_init(av.timing.sample_rate);

    if (info.data)
        SDL_free((void*)info.data);

    char window_title[255];
    snprintf(window_title, sizeof(window_title), "N64 Emulator - %s", filename ? filename : "No Game");
    SDL_SetWindowTitle(g_win, window_title);
}

static void core_unload() {
    if (initialized)
        retro_deinit();
}

static void noop() {}

// ============================================================================
// File Manager UI
// ============================================================================
void run_file_manager(char *selected_rom, size_t rom_buffer_size) {
    SDL_Window *fm_win;
    SDL_Renderer *renderer;
    int fm_running = 1;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER);
    fm_win = SDL_CreateWindow("N64 ROM File Manager",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(fm_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    struct nk_context *ctx = nk_sdl_init(fm_win, renderer);
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();
    
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(20, 30, 50, 200));
    ctx->style.window.background = nk_rgba(20, 30, 50, 200);
    
    FileManager fm = {0};
    getcwd(fm.current_path, sizeof(fm.current_path));
    list_directory(&fm);
    fm.selected_index = 0;
    fm.scroll_offset = 0;
    fm.visible_items = 15;
    
    GamepadState gp = {0};
    init_gamepad(&gp);
    
    selected_rom[0] = '\0';
    
    while (fm_running) {
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) {
                fm_running = 0;
            } else if (evt.type == SDL_CONTROLLERDEVICEADDED && !gp.controller) {
                init_gamepad(&gp);
            } else if (evt.type == SDL_CONTROLLERDEVICEREMOVED && gp.controller) {
                SDL_GameControllerClose(gp.controller);
                gp.controller = NULL;
            }
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);
        
        update_gamepad(&gp);
        
        // Gamepad navigation
        if (gp.controller) {
            Uint32 current_time = SDL_GetTicks();
            
            if (gp.button_states[SDL_CONTROLLER_BUTTON_DPAD_UP]) {
                if (!gp.dpad_up_held || (current_time - gp.repeat_timer > gp.repeat_delay)) {
                    if (fm.selected_index > 0) {
                        fm.selected_index--;
                        if (fm.selected_index < fm.scroll_offset) {
                            fm.scroll_offset = fm.selected_index;
                        }
                    }
                    gp.repeat_timer = current_time;
                    gp.dpad_up_held = 1;
                }
            } else {
                gp.dpad_up_held = 0;
            }
            
            if (gp.button_states[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) {
                if (!gp.dpad_down_held || (current_time - gp.repeat_timer > gp.repeat_delay)) {
                    if (fm.selected_index < fm.file_count - 1) {
                        fm.selected_index++;
                        if (fm.selected_index >= fm.scroll_offset + fm.visible_items) {
                            fm.scroll_offset = fm.selected_index - fm.visible_items + 1;
                        }
                    }
                    gp.repeat_timer = current_time;
                    gp.dpad_down_held = 1;
                }
            } else {
                gp.dpad_down_held = 0;
            }
            
            if (button_pressed(&gp, SDL_CONTROLLER_BUTTON_A)) {
                if (fm.file_count > 0 && fm.selected_index >= 0 && fm.selected_index < fm.file_count) {
                    FileEntry *entry = &fm.files[fm.selected_index];
                    if (entry->is_directory) {
                        change_directory(&fm, entry->name);
                    } else if (is_rom_file(entry->name)) {
                        snprintf(selected_rom, rom_buffer_size, "%s%c%s", 
                                fm.current_path, PATH_SEPARATOR, entry->name);
                        fm_running = 0;
                    }
                }
            }
            
            if (button_pressed(&gp, SDL_CONTROLLER_BUTTON_B)) {
                change_directory(&fm, "..");
            }
            
            if (button_pressed(&gp, SDL_CONTROLLER_BUTTON_START)) {
                fm_running = 0;
            }
            
            if (button_pressed(&gp, SDL_CONTROLLER_BUTTON_Y)) {
                list_directory(&fm);
            }
        }
        
        // GUI rendering
        if (nk_begin(ctx, "ROM File Manager", nk_rect(0, 0, 800, 600),
            NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
            
            nk_layout_row_dynamic(ctx, 30, 1);
            char path_label[MAX_PATH_LENGTH + 20];
            snprintf(path_label, sizeof(path_label), "Path: %s", fm.current_path);
            nk_label(ctx, path_label, NK_TEXT_LEFT);
            
            nk_layout_row_dynamic(ctx, 20, 1);
            if (gp.controller) {
                nk_label(ctx, "Gamepad: Connected", NK_TEXT_LEFT);
            } else {
                nk_label(ctx, "Gamepad: Not Connected", NK_TEXT_LEFT);
            }
            
            nk_layout_row_dynamic(ctx, 5, 1);
            nk_spacing(ctx, 1);
            
            nk_layout_row_dynamic(ctx, 380, 1);
            ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(30, 40, 60, 180));
            if (nk_group_begin(ctx, "File List", NK_WINDOW_BORDER)) {
                int visible_end = fm.scroll_offset + fm.visible_items;
                if (visible_end > fm.file_count) visible_end = fm.file_count;
                
                for (int i = fm.scroll_offset; i < visible_end; i++) {
                    FileEntry *entry = &fm.files[i];
                    
                    nk_layout_row_begin(ctx, NK_STATIC, 25, 3);
                    
                    nk_layout_row_push(ctx, 30);
                    if (i == fm.selected_index) {
                        nk_label(ctx, ">>", NK_TEXT_LEFT);
                    } else {
                        nk_label(ctx, "", NK_TEXT_LEFT);
                    }
                    
                    nk_layout_row_push(ctx, 450);
                    char label[300];
                    if (entry->is_directory) {
                        snprintf(label, sizeof(label), "[DIR] %s", entry->name);
                    } else if (is_rom_file(entry->name)) {
                        snprintf(label, sizeof(label), "[ROM] %s", entry->name);
                    } else {
                        snprintf(label, sizeof(label), "[FILE] %s", entry->name);
                    }
                    
                    if (i == fm.selected_index) {
                        struct nk_style_button style = ctx->style.button;
                        style.normal.data.color = nk_rgb(100, 100, 200);
                        style.hover.data.color = nk_rgb(120, 120, 220);
                        nk_button_label_styled(ctx, &style, label);
                    } else {
                        nk_label(ctx, label, NK_TEXT_LEFT);
                    }
                    
                    nk_layout_row_push(ctx, 100);
                    if (!entry->is_directory) {
                        char size_str[32];
                        format_size(entry->size, size_str, sizeof(size_str));
                        nk_label(ctx, size_str, NK_TEXT_RIGHT);
                    }
                    
                    nk_layout_row_end(ctx);
                }
                nk_group_end(ctx);
            }
            
            nk_layout_row_dynamic(ctx, 5, 1);
            nk_spacing(ctx, 1);
            
            nk_layout_row_dynamic(ctx, 60, 1);
            ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(30, 40, 60, 180));
            if (nk_group_begin(ctx, "Controls", NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, "D-Pad: Navigate | A: Select | B: Back | Y: Refresh | Start: Exit", NK_TEXT_LEFT);
                nk_label(ctx, "Select .z64 or .n64 ROM files to launch emulator", NK_TEXT_LEFT);
                nk_group_end(ctx);
            }
        }
        nk_end(ctx);
        
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderClear(renderer);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
        
        SDL_Delay(16);
    }
    
    if (gp.controller) {
        SDL_GameControllerClose(gp.controller);
    }
    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(fm_win);
}

// ============================================================================
// Main Entry Point
// ============================================================================
int main(int argc, char *argv[]) {
    char rom_path[MAX_PATH_LENGTH] = {0};
    
    // If no arguments, show file manager
    if (argc < 2) {
        printf("No ROM specified, launching file manager...\n");
        run_file_manager(rom_path, sizeof(rom_path));
        
        if (rom_path[0] == '\0') {
            printf("No ROM selected, exiting.\n");
            return EXIT_SUCCESS;
        }
        
        printf("Selected ROM: %s\n", rom_path);
    } else {
        // ROM specified as argument
        strncpy(rom_path, argv[1], sizeof(rom_path) - 1);
        
        if (!is_rom_file(rom_path)) {
            die("File must be a .z64 or .n64 ROM: %s", rom_path);
        }
    }
    
    // Initialize SDL for emulator
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS) < 0)
        die("Failed to initialize SDL");

    g_video.hw.version_major = 4;
    g_video.hw.version_minor = 5;
    g_video.hw.context_type  = RETRO_HW_CONTEXT_OPENGL_CORE;
    g_video.hw.context_reset   = noop;
    g_video.hw.context_destroy = noop;

    // Initialize libretro core
    retro_set_environment(core_environment);
    retro_set_video_refresh(core_video_refresh);
    retro_set_input_poll(core_input_poll);
    retro_set_input_state(core_input_state);
    retro_set_audio_sample(core_audio_sample);
    retro_set_audio_sample_batch(core_audio_sample_batch);
    retro_init();
    initialized = true;

    // Load the game
    core_load_game(rom_path);

    // Configure the player input devices
    retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);

    SDL_Event ev;
    emulator_running = true;

    printf("Starting emulator...\n");
    printf("Press ESC to exit\n");

    while (emulator_running && running) {
        // Update the game loop timer
        if (runloop_frame_time.callback) {
            retro_time_t current = cpu_features_get_time_usec();
            retro_time_t delta = current - runloop_frame_time_last;

            if (!runloop_frame_time_last)
                delta = runloop_frame_time.reference;
            runloop_frame_time_last = current;
            runloop_frame_time.callback(delta);
        }

        // Ask the core to emit the audio
        if (audio_callback.callback) {
            audio_callback.callback();
        }

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT: 
                emulator_running = false; 
                running = false;
                break;
            case SDL_WINDOWEVENT:
                switch (ev.window.event) {
                case SDL_WINDOWEVENT_CLOSE: 
                    emulator_running = false;
                    running = false;
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    resize_cb(ev.window.data1, ev.window.data2);
                    break;
                }
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        retro_run();
    }

    // Cleanup
    core_unload();
    audio_deinit();
    video_deinit();

    if (g_vars) {
        for (const struct retro_variable *v = g_vars; v->key; ++v) {
            free((char*)v->key);
            free((char*)v->value);
        }
        free(g_vars);
    }

    SDL_Quit();

    return EXIT_SUCCESS;
}
