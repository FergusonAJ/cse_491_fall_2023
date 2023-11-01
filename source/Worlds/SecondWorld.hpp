/**
 * @file SecondWorld.hpp
 * @author Jayson Van Dam
 * @author Kurt LaBlanc
 * @author Satvik Ravipati
 */

#pragma once
#include <algorithm>

#include "../core/WorldBase.hpp"
#include "../Agents/PacingAgent.hpp"
#include <nlohmann/json.hpp>

namespace group4 {
/**
 * Creates a world with agents and a win flag
 */
class SecondWorld : public cse491::WorldBase {
 protected:
  enum ActionType {
    REMAIN_STILL = 0,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
  };

  /// Easy access to floor CellType ID.
  size_t floor_id;

  /// Easy access to flag CellType ID.
  size_t flag_id;

  /// Easy access to wall CellType ID.
  size_t wall_id;

  /// Easy access to Entity char
  const size_t entity_id = '+';

  std::vector<std::unique_ptr<cse491::ItemBase>> inventory;

  /// Provide the agent with movement actions.
  void ConfigAgent(cse491::AgentBase& agent) override {
    agent.AddAction("up", MOVE_UP);
    agent.AddAction("down", MOVE_DOWN);
    agent.AddAction("left", MOVE_LEFT);
    agent.AddAction("right", MOVE_RIGHT);
  }

 public:
  /**
   * Constructor with no arguments
   */
  SecondWorld() {
    floor_id =
        AddCellType("floor", "Floor that you can easily walk over.", ' ');
    flag_id = AddCellType("flag", "Goal flag for a game end state", 'g');
    wall_id = AddCellType(
        "wall", "Impenetrable wall that you must find a way around.", '#');
    main_grid.Read("../assets/grids/group4_maze.grid", type_options);
  }

  /**
   * Constructor with grid and agent file names
   * @param grid_filename Relative path to grid file
   * @param agent_filename Relative path to agent input.json file
   */
  SecondWorld(std::string grid_filename, std::string agent_filename) {
    floor_id =
        AddCellType("floor", "Floor that you can easily walk over.", ' ');
    flag_id = AddCellType("flag", "Goal flag for a game end state", 'g');
    wall_id = AddCellType(
        "wall", "Impenetrable wall that you must find a way around.", '#');
    main_grid.Read(grid_filename, type_options);

    LoadFromFile(agent_filename);
  }

  /**
   * Destructor
   */
  ~SecondWorld() = default;

  /**
   * Loads data from a JSON file
   * and adds agents with specified properties into the world.
   * @param input_filename Relative path to input.json file
   */
  void LoadFromFile(const std::string& input_filename) {
    std::ifstream input_file(input_filename);

    if (!input_file.is_open()) {
      std::cerr << "Error: could not open file " << input_filename << std::endl;
      return;
    }

    nlohmann::json data;
    try {
      input_file >> data;
    } catch (const nlohmann::json::parse_error& err) {
      std::cerr << "JSON parsing error: " << err.what() << std::endl;
      return;
    }

    for (const auto& agent : data) {
      // May get a json.exception.type_error here if you assign to the wrong C++ type,
      // so make sure to nail down what types things are in JSON first!
      // My intuition is that each agent object will have:
      // name: string (C++ std::string)
      // x: number (C++ int)
      // y: number (C++ int)
      // entities: array<string> (C++ std::vector<std::string>)
      std::string agent_name = agent.at("name");
      int x_pos = agent.at("x");
      int y_pos = agent.at("y");

      const int BASE_MAX_HEALTH = 100;
      int additional_max_health = 0;
      std::vector<std::string> entities = agent.at("entities");

      for (const auto& entity : entities) {
        // TODO: How should we set the entity properties here?
        // Just adding to MaxHealth now, but this doesn't seem very scalable.
        if (entity == "chocolate_bar") {
          additional_max_health = 10;
        }
      }
      
      AddAgent<cse491::PacingAgent>(agent_name).SetPosition(x_pos, y_pos).SetProperty("MaxHealth", BASE_MAX_HEALTH + additional_max_health);
    }
  }

  void UpdateWorld() override {}

  /**
   * This function gives us an output.json file
   */
  virtual void Run() override {
    while (!run_over) {
      RunAgents();
      UpdateWorld();
    }

    std::string filename = "output.json";

    std::ofstream ofs(filename);

    ofs << "[" << std::endl;
    size_t index = 0;

    for (const auto& [agent_id, agent_ptr] : agent_map) {
      auto new_position = agent_ptr->GetPosition();

      std::string agent_name = agent_ptr->GetName();

      double x_pos = new_position.GetX();
      double y_pos = new_position.GetY();

      ofs << "  {" << std::endl;
      ofs << "    \"name\": "
          << "\"" << agent_name << "\""
          << ",\n";

      ofs << "    \"x\": " << x_pos << "," << std::endl;
      ofs << "    \"y\": " << y_pos << std::endl;

      if (index == agent_map.size() - 1) {
        ofs << "  }" << std::endl;
      } else {
        ofs << "  }," << std::endl;
      }
      index++;
    }
    ofs << "]" << std::endl;
  }

  /**
   * Allows agents to move around the maze.
   * Also checks if the next agent position will be the win flag.
   */
  int DoAction(cse491::AgentBase& agent, size_t action_id) override {
    cse491::GridPosition new_position;
    switch (action_id) {
      case REMAIN_STILL:
        new_position = agent.GetPosition();
        break;
      case MOVE_UP:
        new_position = agent.GetPosition().Above();
        break;
      case MOVE_DOWN:
        new_position = agent.GetPosition().Below();
        break;
      case MOVE_LEFT:
        new_position = agent.GetPosition().ToLeft();
        break;
      case MOVE_RIGHT:
        new_position = agent.GetPosition().ToRight();
        break;
    }
    if (!main_grid.IsValid(new_position)) {
      return false;
    }
    if (main_grid.At(new_position) == wall_id) {
      return false;
    }

    if (main_grid.At(new_position) == flag_id) {
      // Set win flag to true
      std::cout << "flag found" << std::endl;
      run_over = true;
    }

    if (main_grid.At(new_position) == entity_id) {
      std::cout << "found entity id " << entity_id << std::endl;
      auto items_found = FindItemsAt(new_position, 0);
      if (items_found.empty()) {
        agent.Notify("Warning: there is no item here", "item_alert");
      }
      auto item_found = items_found.at(0);

      agent.Notify("You found " + std::to_string(item_found) + "!", "item_alert");

      // Move the ownership of the item to the agents inventory
//      inventory.push_back(std::move(*(item_found)));

//      CleanEntities();

      // Change the grid position to floor_id so it's not seen on the grid
      main_grid.At(new_position) = floor_id;
    }

    agent.SetPosition(new_position);
    return true;
  }

  /**
   * Cleans itemset of all null entities
   */
  void CleanEntities() {
//    std::erase_if(item_map,
//                  [](auto id, const auto& item_ptr) {
//                    return item_ptr == nullptr;
//                  });
  }

  /**
   * Removes an Entity from itemset (testing)
   * @param removeID The ID of the entity we're removing
   */
  void RemoveEntity(size_t removeID) {
//    std::erase_if(item_map,
//                  [removeID](auto id, const auto& item_ptr) {
//                    return item_ptr->GetID() == removeID;
//                  });
//
//    CleanEntities();
  }

  /**
   * Prints the entities in item_set (testing)
   */
  void PrintEntities() {
    for (const auto& [id, item_ptr] : item_map) {
      if (!item_ptr) {
        continue;
      }
      std::cout << item_ptr->GetName() << "\n";
    }
    std::cout << std::endl;
  }
};
}  // namespace group4

