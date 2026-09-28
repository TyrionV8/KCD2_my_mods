#include <MinHook.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

#include "KCSE/KCSEAPI.h"
#include "REL.h"
#include "crysystem/CCryAction.h"
#include "crysystem/SSystemGlobalEnvironment.h"
#include "entitymodule/C_Actor.h"
#include "entitymodule/C_DocumentItem.h"
#include "entitymodule/C_Inventory.h"
#include "entitymodule/C_ItemDatabase.h"
#include "entitymodule/E_ItemType.h"
#include "entitymodule/S_HerbItemClass.h"
#include "playermodule/C_ActionSets.h"
#include "playermodule/C_Alchemy.h"
#include "playermodule/C_AlchemyRecipeDatabase.h"
#include "playermodule/E_AlchemyMode.h"
#include "playermodule/E_AlchemyVerb.h"
#include "Offsets/vtables/ILog.h"
#include "Offsets/vtables/IScriptSystem.h"

KCSE_PLUGIN_INFO("InstantPrepare", "Codex", 1);

namespace {

using wh::playermodule::C_Alchemy;
using wh::playermodule::C_ActionSets;

using StartFn = char(__fastcall*)(C_Alchemy*, std::uint32_t);
using EndFn = std::uint64_t(__fastcall*)(C_Alchemy*);
using TriggerFn = void(__fastcall*)(C_ActionSets*, CryStringT<char>);

StartFn originalStart = nullptr;
EndFn originalEnd = nullptr;
TriggerFn originalTrigger = nullptr;
C_Alchemy* currentSession = nullptr;
bool crafting = false;

void Log(const char* message)
{
    auto* env = SSystemGlobalEnvironment::GetInstance();
    if (env && env->pLog)
        env->pLog->LogAlways("[InstantPrepare] %s", message);
}

// The game stores GUID bytes in Windows GUID layout. Its Lua inventory methods
// take the conventional xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx text form.
std::string GuidText(const CryGUID& guid)
{
    const auto* b = reinterpret_cast<const unsigned char*>(&guid);
    char out[37];
    std::snprintf(out, sizeof(out),
                  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  b[3], b[2], b[1], b[0], b[5], b[4], b[7], b[6],
                  b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    return out;
}

std::uint32_t OpenRecipeId(C_Alchemy* session)
{
    const auto bookId = static_cast<EntityId>(
        session->m_brewState.m_kindEntityId[wh::playermodule::E_AlchemyVerb::OpenBook]);
    auto* framework = CCryAction::GetInstance();
    auto* items = framework ? framework->GetIItemSystem() : nullptr;
    auto* book = reinterpret_cast<wh::entitymodule::C_DocumentItem*>(
        items && bookId ? items->GetItem(bookId) : nullptr);
    if (!book || book->m_bookKind != wh::entitymodule::E_DocumentBookKind::AlchemyBook ||
        book->m_useGuidPageNavigation)
        return 0;
    return book->m_currentNumericPageId;
}

std::string DriedVariant(const CryGUID& ingredient)
{
    auto* db = wh::entitymodule::C_ItemDatabase::GetInstance();
    auto* itemClass = db ? db->FindClassByGuid(ingredient) : nullptr;
    if (!itemClass || !itemClass->IsType(wh::entitymodule::E_ItemType::Herb))
        return "";
    const auto dried = itemClass->GetAsHerbItemClass()->m_driedItemId;
    return dried == CryGUID{} ? "" : GuidText(dried);
}

bool BrewOne(C_Alchemy* session)
{
    if (std::strcmp(session->m_currentContext.c_str(), "alchemy_reading") != 0 ||
        session->m_brewState.m_mode != wh::playermodule::E_AlchemyMode::Reading)
        return false;

    const std::uint32_t recipeId = OpenRecipeId(session);
    auto* recipe = wh::playermodule::C_AlchemyRecipeDatabase::GetRecipeById(recipeId);
    auto* actor = session->m_pPlayerActor;
    auto* inventory = actor ? actor->GetInventory() : nullptr;
    if (!recipe || !inventory || !recipe->CanBrewFrom(inventory))
        return false;

    const wh::playermodule::C_AlchemyRecipeProduct* best = nullptr;
    for (const auto& product : recipe->m_products)
        if (!best || product.m_quality > best->m_quality)
            best = &product;
    if (!best || best->m_quality < 1 || best->m_productItemId == CryGUID{})
        return false;

    // The Lua transaction uses the game's inventory bindings. It checks every
    // stack before changing anything, then adds one product and removes exactly
    // the recipe's ingredient quantities. Failed removals roll the change back.
    std::ostringstream lua;
    lua << "local requirements={";
    for (const auto& ingredient : recipe->m_ingredients) {
        if (ingredient.m_quantity == 0)
            return false;
        lua << "{'" << GuidText(ingredient.m_ingredientItemId) << "',"
            << ingredient.m_quantity << ",'" << DriedVariant(ingredient.m_ingredientItemId)
            << "'},";
    }
    lua << "}\n"
           "local inventory=player and player.inventory\n"
           "assert(inventory, 'player inventory unavailable')\n"
           "local product='" << GuidText(best->m_productItemId) << "'\n"
           "local amount=1\n"
           "local planned={}\n"
           "for _,row in ipairs(requirements) do\n"
           "  local fresh=inventory:GetCountOfClass(row[1])\n"
           "  local dried=row[3]~='' and inventory:GetCountOfClass(row[3]) or 0\n"
           "  local takeFresh=math.min(fresh,row[2])\n"
           "  local takeDried=row[2]-takeFresh\n"
           "  assert(takeDried<=dried, 'not enough ingredients')\n"
           "  if takeFresh>0 then planned[#planned+1]={row[1],takeFresh} end\n"
           "  if takeDried>0 then planned[#planned+1]={row[3],takeDried} end\n"
           "end\n"
           "local oldProduct=inventory:GetCountOfClass(product)\n"
           "local removed={}\n"
           "local ok,problem=pcall(function()\n"
           "  inventory:CreateItem(product,1,amount)\n"
           "  assert(inventory:GetCountOfClass(product)>=oldProduct+amount, 'product creation failed')\n"
           "  for _,row in ipairs(planned) do\n"
           "    local count=inventory:DeleteItemOfClass(row[1],row[2])\n"
           "    if count and count>0 then removed[#removed+1]={row[1],count} end\n"
           "    assert(count==row[2], 'ingredient removal failed')\n"
           "  end\n"
           "end)\n"
           "if not ok then\n"
           "  for _,row in ipairs(removed) do inventory:CreateItem(row[1],1,row[2]) end\n"
           "  local excess=inventory:GetCountOfClass(product)-oldProduct\n"
           "  if excess>0 then inventory:DeleteItemOfClass(product,excess) end\n"
           "  error(problem)\n"
           "end\n"
           "if Game and Game.ShowItemsTransfer then pcall(Game.ShowItemsTransfer,product,amount) end\n";

    auto* env = SSystemGlobalEnvironment::GetInstance();
    if (!env || !env->pScriptSystem)
        return false;
    const std::string source = lua.str();
    if (!env->pScriptSystem->ExecuteBuffer(source.c_str(), source.size(),
                                            "InstantPrepare recipe transaction", nullptr)) {
        Log("recipe transaction failed; see the Lua error above");
        return true; // the rollback has run; never trigger vanilla preparation afterward
    }

    session->ClearIngredientSlots();
    char message[100];
    std::snprintf(message, sizeof(message), "recipe %u: created quality %d potion",
                  recipeId, best->m_quality);
    Log(message);
    return true;
}

char __fastcall Start(C_Alchemy* session, std::uint32_t tableId)
{
    const char started = originalStart(session, tableId);
    if (started)
        currentSession = session;
    return started;
}

std::uint64_t __fastcall End(C_Alchemy* session)
{
    if (currentSession == session)
        currentSession = nullptr;
    return originalEnd(session);
}

void __fastcall Trigger(C_ActionSets* actions, CryStringT<char> action)
{
    if (currentSession && !crafting && std::strcmp(action.c_str(), "read_select") == 0) {
        crafting = true;
        const bool handled = BrewOne(currentSession);
        crafting = false;
        if (handled)
            return;
    }
    originalTrigger(actions, action);
}

} // namespace

KCSE_PLUGIN_LOAD(kcse)
{
    (void)kcse;
    const auto initialized = MH_Initialize();
    if (initialized != MH_OK && initialized != MH_ERROR_ALREADY_INITIALIZED)
        return false;

    struct Hook {
        void* target;
        void* replacement;
        void** original;
    };
    const std::array<Hook, 3> hooks{{
        {reinterpret_cast<void*>(REL::ID(47131).address()),
         reinterpret_cast<void*>(&Start), reinterpret_cast<void**>(&originalStart)},
        {reinterpret_cast<void*>(REL::ID(54603).address()),
         reinterpret_cast<void*>(&End), reinterpret_cast<void**>(&originalEnd)},
        {reinterpret_cast<void*>(REL::ID(45781).address()),
         reinterpret_cast<void*>(&Trigger), reinterpret_cast<void**>(&originalTrigger)},
    }};

    std::size_t installed = 0;
    for (const auto& hook : hooks) {
        if (!hook.target || MH_CreateHook(hook.target, hook.replacement, hook.original) != MH_OK ||
            MH_EnableHook(hook.target) != MH_OK)
            break;
        ++installed;
    }
    if (installed != hooks.size()) {
        // A failed enable can still leave its hook registered.
        for (const auto& hook : hooks) {
            if (hook.target) {
                MH_DisableHook(hook.target);
                MH_RemoveHook(hook.target);
            }
        }
        Log("could not install game hooks");
        return false;
    }
    Log("loaded");
    return true;
}
