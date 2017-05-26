#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/worldimp.hpp"

#include "DedicatedActor.hpp"
#include "Main.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"

using namespace mwmp;
using namespace std;

DedicatedActor::DedicatedActor()
{
    drawState = 0;
    movementFlags = 0;
    animation.groupname = "";
    sound = "";

    hasPositionData = false;
    hasStatsDynamicData = false;
    hasChangedCell = true;

    attack.pressed = false;
}

DedicatedActor::~DedicatedActor()
{

}

void DedicatedActor::update(float dt)
{
    // Only move and set anim flags if the framerate isn't too low
    if (dt < 0.1)
    {
        move(dt);
        setAnimFlags();
    }

    playAnimation();
    playSound();
    setStatsDynamic();
}

void DedicatedActor::setCell(MWWorld::CellStore *cellStore)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    ptr = world->moveObject(ptr, cellStore, position.pos[0], position.pos[1], position.pos[2]);
    setMovementSettings();

    hasChangedCell = true;
}

void DedicatedActor::move(float dt)
{
    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();

    // Apply interpolation only if the position hasn't changed too much from last time
    bool shouldInterpolate = abs(position.pos[0] - refPos.pos[0]) < 4 && abs(position.pos[1] - refPos.pos[1]) < 4 && abs(position.pos[2] - refPos.pos[2]) < 4;

    // Don't apply linear interpolation if the DedicatedActor has just gone through a cell change, because
    // the interpolated position will be invalid, causing a slight hopping glitch
    if (shouldInterpolate && !hasChangedCell)
    {
        static const int timeMultiplier = 15;
        osg::Vec3f lerp = Main::get().getMechanicsHelper()->getLinearInterpolation(refPos.asVec3(), position.asVec3(), dt * timeMultiplier);
        refPos.pos[0] = lerp.x();
        refPos.pos[1] = lerp.y();
        refPos.pos[2] = lerp.z();

        world->moveObject(ptr, refPos.pos[0], refPos.pos[1], refPos.pos[2]);
    }
    else
    {
        setPosition();
        hasChangedCell = false;
    }

    setMovementSettings();
    world->rotateObject(ptr, position.rot[0], position.rot[1], position.rot[2]);
}

void DedicatedActor::setMovementSettings()
{
    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = direction.pos[0];
    move->mPosition[1] = direction.pos[1];
    move->mPosition[2] = direction.pos[2];
}

void DedicatedActor::setPosition()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    world->moveObject(ptr, position.pos[0], position.pos[1], position.pos[2]);
}

void DedicatedActor::setAnimFlags()
{
    using namespace MWMechanics;

    if (drawState == 0)
        ptr.getClass().getCreatureStats(ptr).setDrawState(DrawState_Nothing);
    else if (drawState == 1)
        ptr.getClass().getCreatureStats(ptr).setDrawState(DrawState_Weapon);
    else if (drawState == 2)
        ptr.getClass().getCreatureStats(ptr).setDrawState(DrawState_Spell);

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Run, (movementFlags & CreatureStats::Flag_Run) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Sneak, (movementFlags & CreatureStats::Flag_Sneak) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceJump, (movementFlags & CreatureStats::Flag_ForceJump) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceMoveJump, (movementFlags & CreatureStats::Flag_ForceMoveJump) != 0);
}

void DedicatedActor::setStatsDynamic()
{
    // Only set dynamic stats if we have received at least one packet about them
    if (!hasStatsDynamicData) return;

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    MWMechanics::DynamicStat<float> value;

    // Resurrect this Actor if it's not supposed to be dead according to its authority
    if (creatureStats.mDynamic[0].mCurrent > 0)
        ptrCreatureStats->resurrect();

    for (int i = 0; i < 3; ++i)
    {
        value.readState(creatureStats.mDynamic[i]);
        ptrCreatureStats->setDynamic(i, value);
    }
}

void DedicatedActor::setEquipment()
{
    if (!ptr.getClass().hasInventoryStore(ptr))
        return;

    MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);

        const string &dedicItem = equipedItems[slot].refId;
        std::string item = "";
        bool equal = false;
        if (it != invStore.end())
        {
            item = it->getCellRef().getRefId();
            if (!Misc::StringUtils::ciEqual(item, dedicItem)) // if other item equiped
            {
                MWWorld::ContainerStore &store = ptr.getClass().getContainerStore(ptr);
                store.remove(item, store.count(item), ptr);
            }
            else
                equal = true;
        }

        if (dedicItem.empty() || equal)
            continue;

        int count = equipedItems[slot].count;
        ptr.getClass().getContainerStore(ptr).add(dedicItem, count, ptr);

        for (MWWorld::ContainerStoreIterator it2 = invStore.begin(); it2 != invStore.end(); ++it2)
        {
            if (::Misc::StringUtils::ciEqual(it2->getCellRef().getRefId(), dedicItem)) // equip item
            {
                boost::shared_ptr<MWWorld::Action> action = it2->getClass().use(*it2);
                action->execute(ptr);
                break;
            }
        }
    }
}

void DedicatedActor::playAnimation()
{
    if (!animation.groupname.empty())
    {
        MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptr,
            animation.groupname, animation.mode, animation.count, animation.persist);

        animation.groupname.clear();
    }
}

void DedicatedActor::playSound()
{
    if (!sound.empty())
    {
        MWBase::Environment::get().getSoundManager()->say(ptr, sound);

        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
        if (winMgr->getSubtitlesEnabled())
            winMgr->messageBox(response, MWGui::ShowInDialogueMode_Never);

        sound.clear();
    }
}

MWWorld::Ptr DedicatedActor::getPtr()
{
    return ptr;
}

void DedicatedActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;

    refId = ptr.getCellRef().getRefId();
    refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    mpNum = ptr.getCellRef().getMpNum();

    position = ptr.getRefData().getPosition();
    drawState = ptr.getClass().getCreatureStats(ptr).getDrawState();
}
