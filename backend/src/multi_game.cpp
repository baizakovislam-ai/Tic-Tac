#include "multi_game.hpp"

MultiGame::MultiGame(const StartConfig& config)
    : Game("multi"),
      board_(config.multi.boardSize, std::vector<std::string>(config.multi.boardSize)),
      players_(buildPlayers(config.multi.playersCount, config.multi.humansCount, config.settings.symbolSet)),
      boardSize_(config.multi.boardSize),
      winLength_(config.multi.winLength) {}

MoveResult MultiGame::applyMove(const Json& move) {
    if (isOver()) {
        return {false, "Игра уже завершена."};
    }
    const int x = move.value("x", -1);
    const int y = move.value("y", -1);
    if (x < 0 || y < 0 || x >= boardSize_ || y >= boardSize_) {
        return {false, "Координаты вне поля."};
    }
    if (!board_[y][x].empty()) {
        return {false, "Клетка уже занята."};
    }

    board_[y][x] = players_[currentPlayer_].symbol;
    finalizeTurn();
    return {true, "ok"};
}

Json MultiGame::toJson() const {
    return {
        {"mode", mode_},
        {"board", board_},
        {"boardSize", boardSize_},
        {"winLength", winLength_},
        {"players", players_},
        {"currentPlayer", currentPlayer_},
        {"winner", winner_ >= 0 ? Json(winner_) : Json(nullptr)},
        {"isDraw", draw_},
        {"winningCells", winningCells_}
    };
}

bool MultiGame::currentPlayerIsAi() const {
    return players_.at(currentPlayer_).isAI;
}

void MultiGame::performAiTurn(const std::string& difficulty) {
    if (isOver() || !currentPlayerIsAi()) {
        return;
    }
    std::vector<int> move;
    if (difficulty == "easy") {
        move = randomEmptyCell(board_);
    } else {
        move = chooseBlockingMove(board_, players_, currentPlayer_, winLength_).value_or(randomEmptyCell(board_));
    }
    board_[move[1]][move[0]] = players_[currentPlayer_].symbol;
    finalizeTurn();
}

bool MultiGame::isOver() const {
    return winner_ >= 0 || draw_;
}

void MultiGame::finalizeTurn() {
    const auto result = evaluateBoard(board_, winLength_);
    if (result.winnerSymbol.has_value()) {
        winningCells_ = result.winningCells;
        for (const auto& player : players_) {
            if (player.symbol == result.winnerSymbol.value()) {
                winner_ = player.id;
                return;
            }
        }
    }
    if (result.draw) {
        draw_ = true;
        return;
    }
    currentPlayer_ = (currentPlayer_ + 1) % static_cast<int>(players_.size());
}
