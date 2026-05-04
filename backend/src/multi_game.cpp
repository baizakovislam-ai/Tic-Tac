#include "multi_game.hpp"

#include <limits>

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
    if (countEmptyCells(board_) == 1) {
        draw_ = true;
        winningCells_.clear();
        return {true, "draw"};
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
    if (countEmptyCells(board_) == 1) {
        draw_ = true;
        winningCells_.clear();
        return;
    }

    std::vector<int> move;
    if (difficulty == "easy") {
        move = randomEmptyCell(board_);
    } else if (difficulty == "medium") {
        move = chooseBlockingMove(board_, players_, currentPlayer_, winLength_).value_or(chooseHeuristicMove());
    } else {
        move = chooseHeuristicMove();
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

std::vector<int> MultiGame::chooseHeuristicMove() const {
    int bestScore = std::numeric_limits<int>::min();
    std::vector<int> bestMove = randomEmptyCell(board_);
    for (int y = 0; y < boardSize_; ++y) {
        for (int x = 0; x < boardSize_; ++x) {
            if (!board_[y][x].empty()) {
                continue;
            }
            const int score = evaluateCellScore(x, y, currentPlayer_);
            if (score > bestScore) {
                bestScore = score;
                bestMove = {x, y};
            }
        }
    }
    return bestMove;
}

int MultiGame::evaluateCellScore(int x, int y, int playerIndex) const {
    const auto& aiSymbol = players_[playerIndex].symbol;
    int score = 0;

    Board sandbox = board_;
    sandbox[y][x] = aiSymbol;
    const auto selfResult = evaluateBoard(sandbox, winLength_);
    if (selfResult.winnerSymbol == aiSymbol) {
        return 100000;
    }

    for (const auto& player : players_) {
        if (player.id == playerIndex) {
            continue;
        }
        sandbox = board_;
        sandbox[y][x] = player.symbol;
        if (evaluateBoard(sandbox, winLength_).winnerSymbol == player.symbol) {
            score += 40000;
        }
    }

    const int center = boardSize_ / 2;
    score += 30 - (std::abs(center - x) + std::abs(center - y)) * 4;

    const std::vector<std::pair<int, int>> directions = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (const auto& [dx, dy] : directions) {
        int owned = 0;
        int open = 0;
        for (int step = -(winLength_ - 1); step <= winLength_ - 1; ++step) {
            const int nx = x + dx * step;
            const int ny = y + dy * step;
            if (nx < 0 || ny < 0 || nx >= boardSize_ || ny >= boardSize_) {
                continue;
            }
            if (board_[ny][nx] == aiSymbol) {
                ++owned;
            } else if (board_[ny][nx].empty()) {
                ++open;
            }
        }
        score += owned * owned * 8 + open;
    }

    return score;
}
