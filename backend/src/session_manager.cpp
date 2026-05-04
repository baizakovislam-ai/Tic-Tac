#include "session_manager.hpp"

#include <algorithm>

Json SessionManager::updateSettings(const Json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    settings_.theme = payload.value("theme", settings_.theme);
    settings_.soundEnabled = payload.value("soundEnabled", settings_.soundEnabled);
    settings_.animationsEnabled = payload.value("animationsEnabled", settings_.animationsEnabled);
    settings_.aiDifficulty = payload.value("aiDifficulty", settings_.aiDifficulty);
    settings_.symbolSet = payload.value("symbolSet", settings_.symbolSet);

    return {
        {"status", "success"},
        {"settings", {
            {"theme", settings_.theme},
            {"soundEnabled", settings_.soundEnabled},
            {"animationsEnabled", settings_.animationsEnabled},
            {"aiDifficulty", settings_.aiDifficulty},
            {"symbolSet", settings_.symbolSet}
        }}
    };
}

Json SessionManager::startGame(const Json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    lastConfig_ = parseStartConfig(payload, settings_);
    game_ = makeGame(*lastConfig_);
    scores_.assign(game_->toJson()["players"].size(), 0);
    roundScored_ = false;
    runAiUntilHumanTurn();
    updateScoreIfNeeded();
    return {
        {"status", "success"},
        {"game", buildGamePayload()}
    };
}

Json SessionManager::restartGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!lastConfig_.has_value()) {
        return {{"status", "error"}, {"message", "Нет предыдущей игры для перезапуска."}};
    }
    game_ = makeGame(*lastConfig_);
    roundScored_ = false;
    runAiUntilHumanTurn();
    updateScoreIfNeeded();
    return {
        {"status", "success"},
        {"game", buildGamePayload()}
    };
}

Json SessionManager::endGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    game_.reset();
    scores_.clear();
    roundScored_ = false;
    return {{"status", "success"}, {"game", nullptr}};
}

Json SessionManager::getState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {
        {"status", "success"},
        {"settings", {
            {"theme", settings_.theme},
            {"soundEnabled", settings_.soundEnabled},
            {"animationsEnabled", settings_.animationsEnabled},
            {"aiDifficulty", settings_.aiDifficulty},
            {"symbolSet", settings_.symbolSet}
        }},
        {"game", game_ ? buildGamePayload() : Json(nullptr)}
    };
}

Json SessionManager::makeMove(const Json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!game_) {
        return {{"status", "error"}, {"message", "Сначала начните новую игру."}};
    }

    const auto result = game_->applyMove(payload);
    if (!result.ok) {
        return {{"status", "error"}, {"message", result.message}, {"game", buildGamePayload()}};
    }
    runAiUntilHumanTurn();
    updateScoreIfNeeded();
    return {{"status", "success"}, {"game", buildGamePayload()}};
}

void SessionManager::runAiUntilHumanTurn() {
    if (!game_) {
        return;
    }
    while (!game_->isOver() && game_->currentPlayerIsAi()) {
        game_->performAiTurn(settings_.aiDifficulty);
    }
}

void SessionManager::updateScoreIfNeeded() {
    if (!game_ || roundScored_) {
        return;
    }
    const Json gameJson = game_->toJson();
    if (gameJson["winner"].is_number_integer()) {
        const int winner = gameJson["winner"].get<int>();
        if (winner >= 0 && winner < static_cast<int>(scores_.size())) {
            ++scores_[winner];
        }
        roundScored_ = true;
        return;
    }
    if (gameJson.value("isDraw", false)) {
        roundScored_ = true;
    }
}

Json SessionManager::buildGamePayload() const {
    Json gameJson = game_ ? game_->toJson() : Json(nullptr);
    if (!game_.get()) {
        return gameJson;
    }
    gameJson["scores"] = scores_;
    return gameJson;
}

StartConfig SessionManager::parseStartConfig(const Json& payload, const Settings& currentSettings) {
    StartConfig config;
    config.mode = payload.value("mode", "classic");
    config.settings = currentSettings;

    if (payload.contains("settings") && payload["settings"].is_object()) {
        const auto& settings = payload["settings"];
        config.settings.theme = settings.value("theme", currentSettings.theme);
        config.settings.soundEnabled = settings.value("soundEnabled", currentSettings.soundEnabled);
        config.settings.animationsEnabled = settings.value("animationsEnabled", currentSettings.animationsEnabled);
        config.settings.aiDifficulty = settings.value("aiDifficulty", currentSettings.aiDifficulty);
        config.settings.symbolSet = settings.value("symbolSet", currentSettings.symbolSet);
    }

    if (payload.contains("config") && payload["config"].is_object()) {
        const auto& gameConfig = payload["config"];
        if (config.mode == "classic") {
            config.classic.matchType = gameConfig.value("classicMatchType", "pvp");
            config.classic.userSymbolIndex = gameConfig.value("classicUserSymbolIndex", 0);
        } else if (config.mode == "ultimate") {
            config.ultimate.matchType = gameConfig.value("ultimateMatchType", "pvp");
            config.ultimate.userSymbolIndex = gameConfig.value("ultimateUserSymbolIndex", 0);
        } else if (config.mode == "multi") {
            config.multi.playersCount = gameConfig.value("multiPlayersCount", 3);
            config.multi.humansCount = gameConfig.value("multiHumansCount", 1);
            config.multi.boardSize = gameConfig.value("multiBoardSize", 5);
            config.multi.winLength = gameConfig.value("multiWinLength", 3);
            config.multi.humansCount = std::min(config.multi.humansCount, config.multi.playersCount);
            config.multi.winLength = std::min(config.multi.winLength, config.multi.boardSize);
        }
    }

    settings_.theme = config.settings.theme;
    settings_.soundEnabled = config.settings.soundEnabled;
    settings_.animationsEnabled = config.settings.animationsEnabled;
    settings_.aiDifficulty = config.settings.aiDifficulty;
    settings_.symbolSet = config.settings.symbolSet;
    return config;
}
