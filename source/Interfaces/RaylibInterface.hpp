/**
 * This file is part of the Fall 2023, CSE 491 course project.
 * @brief A low-end realtime interface providing a bare-minimum level of interaction.
 * @note Status: PROPOSAL
 **/

#pragma once

#include <iostream>
#include <vector>

#include "../core/Data.hpp"
#include "../core/InterfaceBase.hpp"

namespace cse491 {

  class RaylibInterface : public InterfaceBase {
  protected:
    bool wait_for_input = true;  ///< Should we pause and wait for the user to hit a key?
    size_t tile_size = 32;

    // -- Helper Functions --
    void DrawGrid(const WorldGrid & grid, const type_options_t & type_options,
                  const item_set_t & item_set, const agent_set_t & agent_set)
    {
      if(grid.GetHeight() * tile_size != static_cast<size_t>(GetScreenHeight()) || 
          grid.GetWidth() * tile_size != static_cast<size_t>(GetScreenWidth())){
        SetWindowSize(grid.GetWidth() * tile_size, grid.GetHeight() * tile_size);
      }
      BeginDrawing();
      ClearBackground(RAYWHITE);
      // Load the world into the symbol_grid;
      for (size_t y=0; y < grid.GetHeight(); ++y) {
        for (size_t x=0; x < grid.GetWidth(); ++x) {
          DrawRectangle(
              x * tile_size, 
              y * tile_size, 
              tile_size, 
              tile_size, 
              (type_options[ grid.At(x,y) ].symbol == ' ' ? RAYWHITE : BLACK)
          );
        }
      }
      // Add in the agents / entities
      for (const auto & entity_ptr : item_set) {
        GridPosition pos = entity_ptr->GetPosition();
        DrawCircle(pos.CellX() * tile_size, pos.CellY() * tile_size, tile_size / 2, RED);
      }

      for (const auto & agent_ptr : agent_set) {
        GridPosition pos = agent_ptr->GetPosition();
        char c = '*';
        if(agent_ptr->HasProperty("char")){
          c = static_cast<char>(agent_ptr->GetProperty("char"));
        }
        DrawCircle(
            (pos.CellX() + 0.5f) * tile_size, 
            (pos.CellY() + 0.5f) * tile_size, 
            tile_size / 2, 
            (c == '@' ? LIME : RED));
      }
      EndDrawing();
    }

  public:
    RaylibInterface(size_t id, const std::string & name) : InterfaceBase(id, name) { }
    ~RaylibInterface() = default; // Already virtual from Entity

    // -- AgentBase overrides --

    bool Initialize() override {
      InitWindow(100, 100, "C4CP");
      SetTargetFPS(60);
      return 
        HasAction("up") && 
        HasAction("down") && 
        HasAction("left") && 
        HasAction("right") &&
        HasAction("quit") &&
        HasAction("do_nothing");
    }

    size_t SelectAction(const WorldGrid & grid,
                        const type_options_t & type_options,
                        const item_set_t & item_set,
                        const agent_set_t & agent_set) override
    {
      // Update the current state of the world.
      DrawGrid(grid, type_options, item_set, agent_set);
      if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) return GetActionID("quit");
      else if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) return GetActionID("up"); 
      else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) return GetActionID("down"); 
      else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) return GetActionID("left"); 
      else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) return GetActionID("right"); 
      return 0;
    }
  };

} // End of namespace cse491
