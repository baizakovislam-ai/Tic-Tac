#include "ultimate_game.hpp"

namespace {
std::vector<int> macroCoords(int index) {
    return {index % 3, index / 3};
}
}

UltimateGame::UltimateGame(const StartConfig& config)
    : Game("ultimate"),
      macroBoard_(3, std::vector<std::string>(3)),
      miniBoards_(9),
      players_(buildPlayers(2, 2, config.settings.symbolSet)) {}

MoveResult UltimateGame::applyMove(const Json& move) {
    if (isOver()) {
        return {false, "Игра уже завершена."};
    }
    const int macroIndex = move.value("macroIndex", -1);
    const int x = move.value("x", -1);
    const int y = move.value("y", -1);
    if (macroIndex < 0 || macroIndex >= 9 || x < 0 || x >= 3 || y < 0 || y >= 3) {
        return {false, "Некорректный ход."};
    }
    if (activeMacroIndex_ != -1 && activeMacroIndex_ != macroIndex) {
        return {false, "Этот сектор сейчас недоступен."};
    }

    auto& mini = miniBoards_[macroIndex];
    if (!mini.winner.empty() || mini.draw) {
        return {false, "Малое поле уже завершено."};
    }
    if (!mini.board[y][x].empty()) {
        return {false, "Клетка уже занята."};
    }

    mini.board[y][x] = players_[currentPlayer_].symbol;
    const auto miniResult = evaluateBoard(mini.board, 3);
    if (miniResult.winnerSymbol.has_value()) {
        mini.winner = miniResult.winnerSymbol.value();
        mini.winningCells = miniResult.winningCells;
        const auto coords = macroCoords(macroIndex);
        macroBoard_[coords[1]][coords[0]] = mini.winner;
    } else if (miniResult.draw) {
        mini.draw = true;
    }

    const int nextMacro = y * 3 + x;
    if (!miniBoards_[nextMacro].winner.empty() || miniBoards_[nextMacro].draw) {
        activeMacroIndex_ = -1;
    } else {
        activeMacroIndex_ = nextMacro;
    }

    refreshMacroResult();
    if (!isOver()) {
        currentPlayer_ = (currentPlayer_ + 1) % static_cast<int>(players_.size());
    }

    return {true, "ok"};
}

Json UltimateGame::toJson() const {
    Json miniBoardsJson = Json::array();
    for (const auto& mini : miniBoards_) {
        miniBoardsJson.push_back({
            {"board", mini.board},
            {"winner", mini.winner.empty() ? Json(nullptr) : Json(mini.winner)},
            {"draw", mini.draw},
            {"winningCells", mini.winningCells}
        });
    }
    return {
        {"mode", mode_},
        {"macroBoard", macroBoard_},
        {"miniBoards", miniBoardsJson},
        {"players", players_},
        {"currentPlayer", currentPlayer_},
        {"activeMacroIndex", activeMacroIndex_ >= 0 ? Json(activeMacroIndex_) : Json(nullptr)},
        {"winner", winner_ >= 0 ? Json(winner_) : Json(nullptr)},
        {"isDraw", draw_}
    };
}

bool UltimateGame::currentPlayerIsAi() const {
    return false;
}

void UltimateGame::performAiTurn(const std::string& difficulty) {
    (void)difficulty;
}

bool UltimateGame::isOver() const {
    return winner_ >= 0 || draw_;
}

void UltimateGame::refreshMacroResult() {
    const auto macroResult = evaluateBoard(macroBoard_, 3);
    if (macroResult.winnerSymbol.has_value()) {
        for (const auto& player : players_) {
            if (player.symbol == macroResult.winnerSymbol.value()) {
                winner_ = player.id;
                return;
            }
        }
    }
    if (allMiniBoardsFinished()) {
        draw_ = true;
    }
}

bool UltimateGame::allMiniBoardsFinished() const {
    for (const auto& mini : miniBoards_) {
        if (mini.winner.empty() && !mini.draw) {
            return false;
        }
    }
    return true;
}
