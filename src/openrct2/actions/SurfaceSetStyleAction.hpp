/*****************************************************************************
 * Copyright (c) 2014-2018 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Context.h"
#include "../object/ObjectManager.h"
#include "../object/TerrainEdgeObject.h"
#include "../object/TerrainSurfaceObject.h"
#include "GameAction.h"

DEFINE_GAME_ACTION(SurfaceSetStyleAction, GAME_COMMAND_CHANGE_SURFACE_STYLE, GameActionResult)
{
private:
    MapRange _range;
    uint8_t _surfaceStyle;
    uint8_t _edgeStyle;

public:
    SurfaceSetStyleAction()
    {
    }

    SurfaceSetStyleAction(MapRange range, uint8_t surfaceStyle, uint8_t edgeStyle)
        : _range(range)
        , _surfaceStyle(surfaceStyle)
        , _edgeStyle(edgeStyle)
    {
    }

    GameActionResult::Ptr Query() const override
    {
        auto res = MakeResult();
        res->ExpenditureType = RCT_EXPENDITURE_TYPE_LANDSCAPING;

        auto x0 = std::max(_range.GetLeft(), 32);
        auto y0 = std::max(_range.GetTop(), 32);
        auto x1 = std::min(_range.GetRight(), (int32_t)gMapSizeMaxXY);
        auto y1 = std::min(_range.GetBottom(), (int32_t)gMapSizeMaxXY);

        MapRange validRange{ x0, y0, x1, y1 };
        auto& objManager = GetContext()->GetObjectManager();
        if (_surfaceStyle != 0xFF)
        {
            if (_surfaceStyle > 0x1F)
            {
                log_error("Invalid surface style.");
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_CHANGE_LAND_TYPE);
            }

            const auto surfaceObj = static_cast<TerrainSurfaceObject*>(
                objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_SURFACE, _surfaceStyle));

            if (surfaceObj == nullptr)
            {
                log_error("Invalid surface style.");
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_CHANGE_LAND_TYPE);
            }
        }

        if (_edgeStyle != 0xFF)
        {
            if (_edgeStyle > 0xF)
            {
                log_error("Invalid edge style.");
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_CHANGE_LAND_TYPE);
            }

            const auto edgeObj = static_cast<TerrainEdgeObject*>(
                objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_SURFACE, _edgeStyle));

            if (edgeObj == nullptr)
            {
                log_error("Invalid edge style.");
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_CHANGE_LAND_TYPE);
            }
        }

        auto xMid = (validRange.GetLeft() + validRange.GetRight()) / 2 + 16;
        auto yMid = (validRange.GetTop() + validRange.GetBottom()) / 2 + 16;
        auto heightMid = tile_element_height(xMid, yMid) & 0xFFFF;

        res->Position.x = xMid;
        res->Position.y = yMid;
        res->Position.z = heightMid;

        // Do nothing if not in editor, sandbox mode or landscaping is forbidden
        if (!(gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) && !gCheatsSandboxMode
            && (gParkFlags & PARK_FLAGS_FORBID_LANDSCAPE_CHANGES))
        {
            return MakeResult(GA_ERROR::DISALLOWED, STR_CANT_CHANGE_LAND_TYPE, STR_FORBIDDEN_BY_THE_LOCAL_AUTHORITY);
        }

        money32 surfaceCost = 0;
        money32 edgeCost = 0;
        for (int32_t x = validRange.GetLeft(); x <= validRange.GetRight(); x += 32)
        {
            for (int32_t y = validRange.GetTop(); y <= validRange.GetBottom(); y += 32)
            {
                if (!(gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) && !gCheatsSandboxMode)
                {
                    if (!map_is_location_in_park({ x, y }))
                        continue;
                }

                auto tileElement = map_get_surface_element_at({ x, y });
                if (tileElement == nullptr)
                {
                    continue;
                }

                auto surfaceElement = tileElement->AsSurface();
                if (_surfaceStyle != 0xFF)
                {
                    uint8_t curSurfaceStyle = surfaceElement->GetSurfaceStyle();

                    if (_surfaceStyle != curSurfaceStyle)
                    {
                        auto& objManager = GetContext()->GetObjectManager();
                        const auto surfaceObj = static_cast<TerrainSurfaceObject*>(
                            objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_SURFACE, _surfaceStyle));
                        if (surfaceObj != nullptr)
                        {
                            surfaceCost += surfaceObj->Price;
                        }
                    }
                }

                if (_edgeStyle != 0xFF)
                {
                    uint8_t curEdgeStyle = surfaceElement->GetEdgeStyle();

                    if (_edgeStyle != curEdgeStyle)
                    {
                        edgeCost += 100;
                    }
                }
            }
        }
        res->Cost = surfaceCost + edgeCost;

        return res;
    }

    GameActionResult::Ptr Execute() const override
    {
        auto res = MakeResult();
        res->ExpenditureType = RCT_EXPENDITURE_TYPE_LANDSCAPING;

        auto x0 = std::max(_range.GetLeft(), 32);
        auto y0 = std::max(_range.GetTop(), 32);
        auto x1 = std::min(_range.GetRight(), (int32_t)gMapSizeMaxXY);
        auto y1 = std::min(_range.GetBottom(), (int32_t)gMapSizeMaxXY);

        MapRange validRange{ x0, y0, x1, y1 };

        auto xMid = (validRange.GetLeft() + validRange.GetRight()) / 2 + 16;
        auto yMid = (validRange.GetTop() + validRange.GetBottom()) / 2 + 16;
        auto heightMid = tile_element_height(xMid, yMid) & 0xFFFF;

        res->Position.x = xMid;
        res->Position.y = yMid;
        res->Position.z = heightMid;

        money32 surfaceCost = 0;
        money32 edgeCost = 0;
        for (int32_t x = validRange.GetLeft(); x <= validRange.GetRight(); x += 32)
        {
            for (int32_t y = validRange.GetTop(); y <= validRange.GetBottom(); y += 32)
            {
                auto tileElement = map_get_surface_element_at({ x, y });
                if (tileElement == nullptr)
                {
                    continue;
                }

                auto surfaceElement = tileElement->AsSurface();
                if (_surfaceStyle != 0xFF)
                {
                    uint8_t curSurfaceStyle = surfaceElement->GetSurfaceStyle();

                    if (_surfaceStyle != curSurfaceStyle)
                    {
                        auto& objManager = GetContext()->GetObjectManager();
                        const auto surfaceObj = static_cast<TerrainSurfaceObject*>(
                            objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_SURFACE, _surfaceStyle));
                        if (surfaceObj != nullptr)
                        {
                            surfaceCost += surfaceObj->Price;

                            surfaceElement->SetSurfaceStyle(_surfaceStyle);

                            map_invalidate_tile_full(x, y);
                            footpath_remove_litter(x, y, tile_element_height(x, y));
                        }
                    }
                }

                if (_edgeStyle != 0xFF)
                {
                    uint8_t curEdgeStyle = surfaceElement->GetEdgeStyle();

                    if (_edgeStyle != curEdgeStyle)
                    {
                        edgeCost += 100;

                        surfaceElement->SetEdgeStyle(_edgeStyle);
                        map_invalidate_tile_full(x, y);
                    }
                }

                if (surfaceElement->CanGrassGrow() && (surfaceElement->GetGrassLength() & 7) != GRASS_LENGTH_CLEAR_0)
                {
                    surfaceElement->SetGrassLength(GRASS_LENGTH_CLEAR_0);
                    map_invalidate_tile_full(x, y);
                }
            }
        }
        res->Cost = surfaceCost + edgeCost;

        return res;
    }
};
