#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "MinHook.h"
#include "recipes_generated.h"

/* All addresses and structure offsets below are for WHGame.dll 1.5.6, Steam
 * assembly 15693. The PE guard prevents hooking a different game executable. */
#define RVA_START               0x00897E1C
#define RVA_END                 0x009F1A8C
#define RVA_TRIGGER             0x0085A360
#define RVA_GET_ITEM            0x00707FA8
#define RVA_CLEAR_INGREDIENTS   0x00A95760
#define RVA_RELEASE_STRING      0x004FD898
#define RVA_FRAMEWORK_POINTER   0x0549D328
#define RVA_GLOBAL_ENV          0x0492D800

#define EXPECTED_PE_TIMESTAMP   0x6A350E20
#define EXPECTED_IMAGE_SIZE     0x05B2D000

typedef char (__fastcall *StartFn)(void*, uint32_t);
typedef uint64_t (__fastcall *EndFn)(void*);
typedef void (__fastcall *TriggerFn)(void*, void*);
typedef void* (__fastcall *GetItemFn)(void*, uint32_t);
typedef void (__fastcall *ClearIngredientsFn)(void*);
typedef void (__fastcall *ReleaseStringFn)(void*);
typedef uint8_t (__fastcall *ExecuteBufferFn)(void*, const char*, size_t, const char*, void*);
typedef uint8_t (__fastcall *GetGlobalAnyFn)(void*, const char*, void*);

typedef struct { uint32_t type; uint32_t pad; float number; uint8_t rest[12]; } ScriptNumber;
_Static_assert(sizeof(void*) == 8, "x64 build required");
_Static_assert(sizeof(ScriptNumber) == 0x18, "script value layout changed");

static HMODULE g_self;
static uint8_t* g_game;
static char g_log_path[MAX_PATH];
static StartFn g_start;
static EndFn g_end;
static TriggerFn g_trigger;
static void* g_session;
static int g_inside_trigger;

static void log_line(const char* format, ...)
{
    char line[512];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(line, sizeof(line) - 3, format, args);
    va_end(args);
    if (written < 0) return;
    size_t length = (size_t)written;
    if (length > sizeof(line) - 3) length = sizeof(line) - 3;
    line[length++] = '\r';
    line[length++] = '\n';
    line[length] = '\0';
    OutputDebugStringA(line);
    if (!g_log_path[0]) return;
    HANDLE file = CreateFileA(g_log_path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        DWORD ignored;
        WriteFile(file, line, (DWORD)length, &ignored, NULL);
        CloseHandle(file);
    }
}

static int append(char* buffer, size_t capacity, size_t* used, const char* format, ...)
{
    if (*used >= capacity) return 0;
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + *used, capacity - *used, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= capacity - *used) return 0;
    *used += (size_t)written;
    return 1;
}

static const Recipe* recipe_by_id(uint32_t id)
{
    size_t low = 0, high = RECIPE_COUNT;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (kRecipes[middle].id < id) low = middle + 1;
        else high = middle;
    }
    return low < RECIPE_COUNT && kRecipes[low].id == id ? &kRecipes[low] : NULL;
}

static const Recipe* open_recipe(void* session)
{
    const uint8_t* state = (const uint8_t*)session;
    const char* context = *(const char* const*)(state + 0x30);
    if (!context || strcmp(context, "alchemy_reading") != 0 || state[0x300] != 4)
        return NULL;

    uint32_t book_id = *(const uint32_t*)(state + 0x150 + 4 * 19);
    void* framework = *(void**)(g_game + RVA_FRAMEWORK_POINTER);
    if (!book_id || !framework) return NULL;
    void* item_system = *(void**)((uint8_t*)framework + 0x520);
    if (!item_system) return NULL;
    void* book = ((GetItemFn)(g_game + RVA_GET_ITEM))((uint8_t*)item_system + 8, book_id);
    if (!book) return NULL;
    const uint8_t* bytes = (const uint8_t*)book;
    if (bytes[0x170] != 2 || bytes[0x12D] != 0) return NULL;
    return recipe_by_id(*(const uint32_t*)(bytes + 0x130));
}

/* Status: 0 = insufficient inventory (allow vanilla), 1 = created,
 * 2 = attempted but failed (suppress vanilla to prevent double spending). */
static int brew(const Recipe* recipe)
{
    char lua[4096];
    size_t used = 0;
    if (!append(lua, sizeof(lua), &used,
        "InstantPrepareStatus=2\n"
        "local inventory=player and player.inventory\n"
        "if not inventory then return end\n"
        "local requirements={")) return 0;
    for (uint32_t i = 0; i < recipe->count; ++i) {
        const Ingredient* ingredient = &recipe->items[i];
        if (!append(lua, sizeof(lua), &used, "{'%s',%u,'%s'},",
                    ingredient->fresh, ingredient->quantity, ingredient->dried)) return 0;
    }
    if (!append(lua, sizeof(lua), &used,
        "}\n"
        "local product='%s'\n"
        "local planned={}\n"
        "for _,row in ipairs(requirements) do\n"
        "  local fresh=inventory:GetCountOfClass(row[1])\n"
        "  local dried=row[3]~='' and inventory:GetCountOfClass(row[3]) or 0\n"
        "  local takeFresh=math.min(fresh,row[2])\n"
        "  local takeDried=row[2]-takeFresh\n"
        "  if takeDried>dried then InstantPrepareStatus=0; return end\n"
        "  if takeFresh>0 then planned[#planned+1]={row[1],takeFresh} end\n"
        "  if takeDried>0 then planned[#planned+1]={row[3],takeDried} end\n"
        "end\n"
        "local oldProduct=inventory:GetCountOfClass(product)\n"
        "local removed={}\n"
        "local ok,problem=pcall(function()\n"
        "  inventory:CreateItem(product,1,1)\n"
        "  assert(inventory:GetCountOfClass(product)>=oldProduct+1,'product creation failed')\n"
        "  for _,row in ipairs(planned) do\n"
        "    local count=inventory:DeleteItemOfClass(row[1],row[2])\n"
        "    if count and count>0 then removed[#removed+1]={row[1],count} end\n"
        "    assert(count==row[2],'ingredient removal failed')\n"
        "  end\n"
        "end)\n"
        "if not ok then\n"
        "  for _,row in ipairs(removed) do pcall(inventory.CreateItem,inventory,row[1],1,row[2]) end\n"
        "  local excess=inventory:GetCountOfClass(product)-oldProduct\n"
        "  if excess>0 then pcall(inventory.DeleteItemOfClass,inventory,product,excess) end\n"
        "  InstantPrepareStatus=2\n"
        "  return\n"
        "end\n"
        "InstantPrepareStatus=1\n"
        "if Game and Game.ShowItemsTransfer then pcall(Game.ShowItemsTransfer,product,1) end\n",
        recipe->product)) return 0;

    void* script = *(void**)(g_game + RVA_GLOBAL_ENV + 0x28);
    if (!script) return 0;
    void** table = *(void***)script;
    if (!((ExecuteBufferFn)table[6])(script, lua, used, "InstantPrepare", NULL))
        return 2;
    ScriptNumber status = {0};
    status.type = 4; /* ANY_TNUMBER */
    if (!((GetGlobalAnyFn)table[32])(script, "InstantPrepareStatus", &status) ||
        status.type != 4) return 2;
    return (int)status.number;
}

static char __fastcall hook_start(void* session, uint32_t table_id)
{
    char result = g_start(session, table_id);
    if (result) g_session = session;
    return result;
}

static uint64_t __fastcall hook_end(void* session)
{
    if (g_session == session) g_session = NULL;
    return g_end(session);
}

static void __fastcall hook_trigger(void* actions, void* action)
{
    if (g_session && action && !g_inside_trigger) {
        const char* name = *(const char**)action;
        if (name && strcmp(name, "read_select") == 0) {
            const Recipe* recipe = open_recipe(g_session);
            if (recipe) {
                g_inside_trigger = 1;
                int status = brew(recipe);
                g_inside_trigger = 0;
                if (status == 1 || status == 2) {
                    if (status == 1) {
                        ((ClearIngredientsFn)(g_game + RVA_CLEAR_INGREDIENTS))(g_session);
                        log_line("Recipe %u: created one quality-4 potion", recipe->id);
                    } else {
                        log_line("Recipe %u: transaction failed; vanilla action suppressed", recipe->id);
                    }
                    /* TriggerAction normally destroys its by-value CryString argument. */
                    ((ReleaseStringFn)(g_game + RVA_RELEASE_STRING))((uint8_t*)name - 12);
                    return;
                }
            }
        }
    }
    g_trigger(actions, action);
}

static int matches_game_build(uint8_t* game)
{
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)game;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(game + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.TimeDateStamp != EXPECTED_PE_TIMESTAMP ||
        nt->OptionalHeader.SizeOfImage != EXPECTED_IMAGE_SIZE) return 0;
    static const uint8_t start[] = {0x48,0x89,0x5c,0x24,0x10,0x57};
    static const uint8_t end[] = {0x48,0x89,0x5c,0x24,0x18,0x48};
    static const uint8_t trigger[] = {0x48,0x89,0x5c,0x24,0x18,0x57};
    return memcmp(game + RVA_START, start, sizeof(start)) == 0 &&
           memcmp(game + RVA_END, end, sizeof(end)) == 0 &&
           memcmp(game + RVA_TRIGGER, trigger, sizeof(trigger)) == 0;
}

static DWORD WINAPI initialize(LPVOID unused)
{
    (void)unused;
    DWORD length = GetModuleFileNameA(g_self, g_log_path, MAX_PATH);
    if (length > 4 && length < MAX_PATH) {
        char* extension = strrchr(g_log_path, '.');
        if (extension) strcpy(extension, ".log");
    } else g_log_path[0] = '\0';

    for (int i = 0; i < 600; ++i) {
        g_game = (uint8_t*)GetModuleHandleA("WHGame.dll");
        if (g_game) break;
        Sleep(100);
    }
    if (!g_game) { log_line("WHGame.dll not loaded"); return 0; }
    if (!matches_game_build(g_game)) {
        log_line("Unsupported WHGame.dll build; hooks not installed");
        return 0;
    }
    if (MH_Initialize() != MH_OK) {
        log_line("MinHook initialization failed");
        return 0;
    }
    void* targets[3] = {g_game + RVA_START, g_game + RVA_END, g_game + RVA_TRIGGER};
    void* replacements[3] = {(void*)hook_start, (void*)hook_end, (void*)hook_trigger};
    void** originals[3] = {(void**)&g_start, (void**)&g_end, (void**)&g_trigger};
    for (int i = 0; i < 3; ++i) {
        if (MH_CreateHook(targets[i], replacements[i], originals[i]) != MH_OK ||
            MH_EnableHook(targets[i]) != MH_OK) {
            for (int j = 0; j <= i; ++j) {
                MH_DisableHook(targets[j]);
                MH_RemoveHook(targets[j]);
            }
            log_line("Hook installation failed");
            return 0;
        }
    }
    log_line("Loaded for Steam assembly 15693 (%u recipes)", (unsigned)RECIPE_COUNT);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(NULL, 0, initialize, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
