/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>

#include "GroupHandler.h"
#include "Group.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/CameraHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Input/KeyInput.h"
#include "System/Log/ILog.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/StringHash.h"

#include "System/Misc/TracyDefs.h"

std::vector<CGroupHandler> uiGroupHandlers;

CR_BIND(CGroupHandler, (0))
CR_REG_METADATA(CGroupHandler, (
	CR_MEMBER(groups),
	CR_MEMBER(freeGroups),
	CR_MEMBER(changedGroups),
	CR_MEMBER(unitGroups),

	CR_MEMBER(team),
	CR_MEMBER(firstUnusedGroup)

))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupHandler::CGroupHandler(int teamId): team(teamId)
{
	RECOIL_DETAILED_TRACY_ZONE;
	groups.reserve(FIRST_SPECIAL_GROUP);

	for (int groupId = 0; groupId < FIRST_SPECIAL_GROUP; ++groupId) {
		groups.emplace_back(groupId, teamId);
	}
}


void CGroupHandler::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		for (CGroup& g: groups) {
			// may invoke RemoveGroup, but can not cause iterator invalidation
			g.Update();
		}
	}

	{
		if (changedGroups.empty())
			return;

		std::vector<int> grpChg;

		// swap containers to prevent recursion through lua
		changedGroups.swap(grpChg);

		for (int i: grpChg) {
			eventHandler.GroupChanged(i);
		}
	}
}

bool CGroupHandler::GroupCommand(int num)
{
	/** The following behavior is considered stable and should be modified
	 *  with care - its purpose is to maintain compatibility with games which
	 *  still rely upon groupN unsynced command (GroupIDActionExecutor).
	 */

	RECOIL_DETAILED_TRACY_ZONE;
	CGroup* group = GetGroup(num);

	// stable equivalents of "set" and "add" commands
	if (KeyInput::GetKeyModState(KMOD_CTRL))
	{
		// holding shift emulates "add" command
		if(!KeyInput::GetKeyModState(KMOD_SHIFT))
			group->ClearUnits();

		for (const int unitID: selectedUnitsHandler.selectedUnits) {
			CUnit* u = unitHandler.GetUnit(unitID);

			if (u == nullptr) {
				assert(false);
				continue;
			}

			// change group, but do not call SUH::AddUnit while iterating
			u->SetGroup(group, false, false);
		}
	}
	// stable equivalent of "selectadd" command
	else if (KeyInput::GetKeyModState(KMOD_SHIFT))
	{
		// do not select the group, just add its members to the current selection
		for (const int unitID: group->units) {
			selectedUnitsHandler.AddUnit(unitHandler.GetUnit(unitID));
		}

		return true;
	}
	// stable equivalent of "selecttoggle" command
	else if (KeyInput::GetKeyModState(KMOD_ALT))
	{
		// do not select the group, just toggle its members with the current selection
		const auto& selUnits = selectedUnitsHandler.selectedUnits;

		for (const int unitID: group->units) {
			CUnit* unit = unitHandler.GetUnit(unitID);

			if (selUnits.find(unitID) == selUnits.end()) {
				selectedUnitsHandler.AddUnit(unit);
			} else {
				selectedUnitsHandler.RemoveUnit(unit);
			}
		}

		return true;
	}

	// select or focus group, depending on whether it's already selected

	if (group->units.empty())
		return false;

	if (selectedUnitsHandler.IsGroupSelected(num)) {
		camHandler->CameraTransition(0.5f);
		camHandler->GetCurrentController().SetPos(group->CalculateCenter());
	}

	selectedUnitsHandler.SelectGroup(num);
	return true;
}

bool CGroupHandler::GroupCommand(int num, const std::string& cmd, bool& error)
{
	RECOIL_DETAILED_TRACY_ZONE;
	error = false;
	CGroup* group = GetGroup(num);

	switch (hashString(cmd.c_str())) {
		case hashString("set"):
		case hashString("add"): {
			if (cmd[0] == 's')
				group->ClearUnits();

			for (const int unitID: selectedUnitsHandler.selectedUnits) {
				CUnit* u = unitHandler.GetUnit(unitID);

				if (u == nullptr) {
					assert(false);
					continue;
				}

				// change group, but do not call SUH::AddUnit while iterating
				u->SetGroup(group, false, false);
			}

			// if adding to group, do not proceed to select/focus logic
			if(cmd[0] == 'a')
				return !group->units.empty();
		} break;

		case hashString("select"): {
			if (group->units.empty())
				return false;

			selectedUnitsHandler.SelectGroup(num);

			return true;
		} break;

		case hashString("focus"): {
			if (group->units.empty())
				return false;

			camHandler->CameraTransition(0.5f);
			camHandler->GetCurrentController().SetPos(group->CalculateCenter());

			return true;
		} break;

		case hashString("selectadd"): {
			// do not select the group, just add its members to the current selection
			for (const int unitID: group->units) {
				selectedUnitsHandler.AddUnit(unitHandler.GetUnit(unitID));
			}

			return true;
		} break;

		case hashString("selectclear"): {
			// do not select the group, just remove its members from the current selection
			for (const int unitID: group->units) {
				selectedUnitsHandler.RemoveUnit(unitHandler.GetUnit(unitID));
			}

			return true;
		} break;

		case hashString("selecttoggle"): {
			// do not select the group, just toggle its members with the current selection
			const auto& selUnits = selectedUnitsHandler.selectedUnits;

			for (const int unitID: group->units) {
				CUnit* unit = unitHandler.GetUnit(unitID);

				if (selUnits.find(unitID) == selUnits.end()) {
					selectedUnitsHandler.AddUnit(unit);
				} else {
					selectedUnitsHandler.RemoveUnit(unit);
				}
			}

			return true;
		} break;
		// Selects or focuses camera on group.
		case hashString(""): break;
		// Unrecognized command
		default: {
			error = true;
			return false;
		}
	}

	if (group->units.empty())
		return false;

	if (selectedUnitsHandler.IsGroupSelected(num)) {
		camHandler->CameraTransition(0.5f);
		camHandler->GetCurrentController().SetPos(group->CalculateCenter());
	}

	selectedUnitsHandler.SelectGroup(num);
	return true;
}


CGroup* CGroupHandler::CreateNewGroup()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (freeGroups.empty()) {
		groups.emplace_back(firstUnusedGroup++, team);
		return &groups[groups.size() - 1];
	}

	return &groups[ spring::VectorBackPop(freeGroups) ];
}

void CGroupHandler::RemoveGroup(CGroup* group)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (group->id < FIRST_SPECIAL_GROUP) {
		LOG_L(L_WARNING, "[GroupHandler::%s] trying to remove hot-key group %i", __func__, group->id);
		return;
	}

	if (selectedUnitsHandler.IsGroupSelected(group->id))
		selectedUnitsHandler.ClearSelected();

	group->ClearUnits();
	freeGroups.push_back(group->id);
}

void CGroupHandler::PushGroupChange(int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::VectorInsertUnique(changedGroups, id, true);
}
