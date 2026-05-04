#pragma once

#include "game.hpp"
#include "models.hpp"

#include <memory>
#include <mutex>
#include <optional>

class SessionManager {
  public:
    Json updateSettings(const Json& payload);
    Json startGame(const Json& payload);
    Json restartGame();
    Json endGame();
    Json getState() const;
    Json makeMove(const Json& payload);

  private:
    void runAiUntilHumanTurn();
    void updateScoreIfNeeded();
    Json buildGamePayload() const;
    StartConfig parseStartConfig(const Json& payload, const Settings& currentSettings);

    mutable std::mutex mutex_;
    Settings settings_;
    std::unique_ptr<Game> game_;
    std::optional<StartConfig> lastConfig_;
    std::vector<int> scores_;
    bool roundScored_ {false};
};
