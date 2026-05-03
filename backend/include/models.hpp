#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

struct Settings {
    std::string theme {"light"};
    bool soundEnabled {false};
    bool animationsEnabled {true};
    std::string aiDifficulty {"medium"};
    std::string symbolSet {"classic"};
};

struct Player {
    int id {};
    std::string label;
    std::string symbol;
    bool isAI {false};
};

struct MoveResult {
    bool ok {false};
    std::string message {"ok"};
};

struct LineResult {
    std::optional<std::string> winnerSymbol;
    bool draw {false};
    std::vector<std::vector<int>> winningCells;
};

struct ClassicConfig {
    std::string matchType {"pvp"};
    int firstPlayer {0};
};

struct UltimateConfig {
    std::string matchType {"pvp"};
    int firstPlayer {0};
};

struct MultiConfig {
    int playersCount {3};
    int humansCount {1};
    int boardSize {5};
    int winLength {3};
};

struct StartConfig {
    std::string mode {"classic"};
    ClassicConfig classic;
    UltimateConfig ultimate;
    MultiConfig multi;
    Settings settings;
};

using Json = nlohmann::json;

inline void to_json(Json& json, const Player& player) {
    json = Json{
        {"id", player.id},
        {"label", player.label},
        {"symbol", player.symbol},
        {"isAI", player.isAI}
    };
}
