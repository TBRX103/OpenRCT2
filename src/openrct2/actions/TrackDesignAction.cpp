/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TrackDesignAction.h"

#include "../management/Finance.h"
#include "../management/Research.h"
#include "../object/ObjectRepository.h"
#include "../ride/RideGroupManager.h"
#include "../ride/TrackDesign.h"
#include "RideSetSetting.hpp"
#include "RideSetVehiclesAction.hpp"

static int32_t place_virtual_track(
    const TrackDesign& td6, uint8_t ptdOperation, bool placeScenery, Ride* ride, const CoordsXYZ& loc)
{
    return place_virtual_track(const_cast<TrackDesign*>(&td6), ptdOperation, placeScenery, ride, loc.x, loc.y, loc.z);
}

GameActionResult::Ptr TrackDesignAction::Query() const
{
    auto res = MakeResult();
    res->Position.x = _loc.x + 16;
    res->Position.y = _loc.y + 16;
    res->Position.z = _loc.z;
    res->ExpenditureType = RCT_EXPENDITURE_TYPE_RIDE_CONSTRUCTION;

    if (!(GetFlags() & GAME_COMMAND_FLAG_ALLOW_DURING_PAUSED))
    {
        if (game_is_paused() && !gCheatsBuildInPauseMode)
        {
            return MakeResult(GA_ERROR::GAME_PAUSED, STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED);
        }
    }

    const rct_object_entry* rideEntryObject = &_td.vehicle_object;

    uint8_t entryType, entryIndex;
    if (!find_object_in_entry_group(rideEntryObject, &entryType, &entryIndex))
    {
        entryIndex = 0xFF;
    }
    // Force a fallback if the entry is not invented yet a td6 of it is selected, which can happen in select-by-track-type mode.
    else if (!ride_entry_is_invented(entryIndex) && !gCheatsIgnoreResearchStatus)
    {
        entryIndex = 0xFF;
    }

    // The rest of the cases are handled by the code in ride_create()
    if (RideGroupManager::RideTypeHasRideGroups(_td.type) && entryIndex == 0xFF)
    {
        const ObjectRepositoryItem* ori = object_repository_find_object_by_name(rideEntryObject->name);
        if (ori != nullptr)
        {
            uint8_t rideGroupIndex = ori->RideInfo.RideGroupIndex;
            const RideGroup* td6RideGroup = RideGroupManager::RideGroupFind(_td.type, rideGroupIndex);

            uint8_t* availableRideEntries = get_ride_entry_indices_for_ride_type(_td.type);
            for (uint8_t* rei = availableRideEntries; *rei != RIDE_ENTRY_INDEX_NULL; rei++)
            {
                rct_ride_entry* ire = get_ride_entry(*rei);

                if (!ride_entry_is_invented(*rei) && !gCheatsIgnoreResearchStatus)
                {
                    continue;
                }

                const RideGroup* irg = RideGroupManager::GetRideGroup(_td.type, ire);
                if (td6RideGroup->Equals(irg))
                {
                    entryIndex = *rei;
                    break;
                }
            }
        }
    }

    ride_id_t rideIndex;
    uint8_t rideColour;
    money32 createRideResult = ride_create_command(_td.type, entryIndex, GetFlags(), &rideIndex, &rideColour);
    if (createRideResult == MONEY32_UNDEFINED)
    {
        return MakeResult(GA_ERROR::NO_FREE_ELEMENTS, STR_CANT_CREATE_NEW_RIDE_ATTRACTION, STR_NONE);
    }

    auto ride = get_ride(rideIndex);
    if (ride == nullptr)
    {
        log_warning("Invalid game command for track placement, ride id = %d", rideIndex);
        return MakeResult(GA_ERROR::UNKNOWN);
    }

    money32 cost = 0;

    bool placeScenery = true;
    cost = place_virtual_track(_td, PTD_OPERATION_PLACE_QUERY, placeScenery, ride, _loc);
    if (_trackDesignPlaceStateSceneryUnavailable)
    {
        placeScenery = false;
        cost = place_virtual_track(_td, PTD_OPERATION_PLACE_QUERY, placeScenery, ride, _loc);
    }

    rct_string_id error_reason = gGameCommandErrorText;
    ride_action_modify(ride, RIDE_MODIFY_DEMOLISH, GetFlags());
    if (cost == MONEY32_UNDEFINED)
    {
        return MakeResult(GA_ERROR::DISALLOWED, error_reason);
    }
    res->Cost = cost;
    return res;
}

GameActionResult::Ptr TrackDesignAction::Execute() const
{
    auto res = MakeResult();
    res->Position.x = _loc.x + 16;
    res->Position.y = _loc.y + 16;
    res->Position.z = _loc.z;
    res->ExpenditureType = RCT_EXPENDITURE_TYPE_RIDE_CONSTRUCTION;

    if (!(GetFlags() & GAME_COMMAND_FLAG_ALLOW_DURING_PAUSED))
    {
        if (game_is_paused() && !gCheatsBuildInPauseMode)
        {
            return MakeResult(GA_ERROR::GAME_PAUSED, STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED);
        }
    }

    const rct_object_entry* rideEntryObject = &_td.vehicle_object;

    uint8_t entryType, entryIndex;
    if (!find_object_in_entry_group(rideEntryObject, &entryType, &entryIndex))
    {
        entryIndex = 0xFF;
    }
    // Force a fallback if the entry is not invented yet a td6 of it is selected, which can happen in select-by-track-type mode.
    else if (!ride_entry_is_invented(entryIndex) && !gCheatsIgnoreResearchStatus)
    {
        entryIndex = 0xFF;
    }

    // The rest of the cases are handled by the code in ride_create()
    if (RideGroupManager::RideTypeHasRideGroups(_td.type) && entryIndex == 0xFF)
    {
        const ObjectRepositoryItem* ori = object_repository_find_object_by_name(rideEntryObject->name);
        if (ori != nullptr)
        {
            uint8_t rideGroupIndex = ori->RideInfo.RideGroupIndex;
            const RideGroup* td6RideGroup = RideGroupManager::RideGroupFind(_td.type, rideGroupIndex);

            uint8_t* availableRideEntries = get_ride_entry_indices_for_ride_type(_td.type);
            for (uint8_t* rei = availableRideEntries; *rei != RIDE_ENTRY_INDEX_NULL; rei++)
            {
                rct_ride_entry* ire = get_ride_entry(*rei);

                if (!ride_entry_is_invented(*rei) && !gCheatsIgnoreResearchStatus)
                {
                    continue;
                }

                const RideGroup* irg = RideGroupManager::GetRideGroup(_td.type, ire);
                if (td6RideGroup->Equals(irg))
                {
                    entryIndex = *rei;
                    break;
                }
            }
        }
    }

    ride_id_t rideIndex;
    uint8_t rideColour;
    money32 createRideResult = ride_create_command(_td.type, entryIndex, GetFlags(), &rideIndex, &rideColour);
    if (createRideResult == MONEY32_UNDEFINED)
    {
        return MakeResult(GA_ERROR::NO_FREE_ELEMENTS, STR_CANT_CREATE_NEW_RIDE_ATTRACTION, STR_NONE);
    }

    auto ride = get_ride(rideIndex);
    if (ride == nullptr)
    {
        log_warning("Invalid game command for track placement, ride id = %d", rideIndex);
        return MakeResult(GA_ERROR::UNKNOWN);
    }

    money32 cost = 0;

    bool placeScenery = true;
    cost = place_virtual_track(_td, PTD_OPERATION_PLACE_QUERY, placeScenery, ride, _loc);
    if (_trackDesignPlaceStateSceneryUnavailable)
    {
        placeScenery = false;
        cost = place_virtual_track(_td, PTD_OPERATION_PLACE_QUERY, placeScenery, ride, _loc);
    }

    if (cost != MONEY32_UNDEFINED)
    {
        uint8_t operation;
        if (GetFlags() & GAME_COMMAND_FLAG_GHOST)
        {
            operation = PTD_OPERATION_PLACE_GHOST;
        }
        else
        {
            operation = PTD_OPERATION_PLACE;
        }

        cost = place_virtual_track(_td, operation, placeScenery, ride, _loc);
    }

    if (cost == MONEY32_UNDEFINED)
    {
        rct_string_id error_reason = gGameCommandErrorText;
        ride_action_modify(ride, RIDE_MODIFY_DEMOLISH, GetFlags());
        return MakeResult(GA_ERROR::DISALLOWED, error_reason);
    }

    if (entryIndex != 0xFF)
    {
        auto colour = ride_get_unused_preset_vehicle_colour(entryIndex);
        auto rideSetVehicleAction = RideSetVehicleAction(ride->id, RideSetVehicleType::RideEntry, entryIndex, colour);
        GameActions::ExecuteNested(&rideSetVehicleAction);
    }

    set_operating_setting_nested(ride->id, RideSetSetting::Mode, _td.ride_mode, GAME_COMMAND_FLAG_APPLY);
    auto rideSetVehicleAction2 = RideSetVehicleAction(ride->id, RideSetVehicleType::NumTrains, _td.number_of_trains);
    GameActions::ExecuteNested(&rideSetVehicleAction2);

    auto rideSetVehicleAction3 = RideSetVehicleAction(
        ride->id, RideSetVehicleType::NumCarsPerTrain, _td.number_of_cars_per_train);
    GameActions::ExecuteNested(&rideSetVehicleAction3);

    set_operating_setting_nested(ride->id, RideSetSetting::Departure, _td.depart_flags, GAME_COMMAND_FLAG_APPLY);
    set_operating_setting_nested(ride->id, RideSetSetting::MinWaitingTime, _td.min_waiting_time, GAME_COMMAND_FLAG_APPLY);
    set_operating_setting_nested(ride->id, RideSetSetting::MaxWaitingTime, _td.max_waiting_time, GAME_COMMAND_FLAG_APPLY);
    set_operating_setting_nested(ride->id, RideSetSetting::Operation, _td.operation_setting, GAME_COMMAND_FLAG_APPLY);
    set_operating_setting_nested(ride->id, RideSetSetting::LiftHillSpeed, _td.lift_hill_speed & 0x1F, GAME_COMMAND_FLAG_APPLY);

    uint8_t num_circuits = _td.num_circuits;
    if (num_circuits == 0)
    {
        num_circuits = 1;
    }
    set_operating_setting_nested(ride->id, RideSetSetting::NumCircuits, num_circuits, GAME_COMMAND_FLAG_APPLY);
    ride->SetToDefaultInspectionInterval();
    ride->lifecycle_flags |= RIDE_LIFECYCLE_NOT_CUSTOM_DESIGN;
    ride->colour_scheme_type = _td.colour_scheme;

    ride->entrance_style = _td.entrance_style;

    for (int32_t i = 0; i < RCT12_NUM_COLOUR_SCHEMES; i++)
    {
        ride->track_colour[i].main = _td.track_spine_colour[i];
        ride->track_colour[i].additional = _td.track_rail_colour[i];
        ride->track_colour[i].supports = _td.track_support_colour[i];
    }

    for (int32_t i = 0; i < MAX_VEHICLES_PER_RIDE; i++)
    {
        ride->vehicle_colours[i].Body = _td.vehicle_colours[i].body_colour;
        ride->vehicle_colours[i].Trim = _td.vehicle_colours[i].trim_colour;
        ride->vehicle_colours[i].Ternary = _td.vehicle_additional_colour[i];
    }

    ride_set_name(ride, _td.name.c_str(), GAME_COMMAND_FLAG_APPLY);
    res->Cost = cost;
    res->rideIndex = ride->id;
    return res;
}
