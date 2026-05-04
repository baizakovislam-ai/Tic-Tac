#include "classic_game.hpp"

#include <limits>

ClassicGame::ClassicGame(const StartConfig& config)
    : Game("classic"),
      board_(3, std::vector<std::string>(3)),
      players_(config.classic.matchType == "ai"
          ? buildDuelPlayers(config.settings.symbolSet, config.classic.userSymbolIndex)
          : buildPlayers(2, 2, config.settings.symbolSet)),
      currentPlayer_(0) {}

MoveResult ClassicGame::applyMove(const Json& move) {
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
    if (x < 0 || y < 0 || x >= 3 || y >= 3) {
        return {false, "Координаты вне поля."};
    }
    if (!board_[y][x].empty()) {
        return {false, "Клетка уже занята."};
    }

    board_[y][x] = players_[currentPlayer_].symbol;
    finalizeTurn();
    return {true, "ok"};
}

Json ClassicGame::toJson() const {
    return {
        {"mode", mode_},
        {"board", board_},
        {"players", players_},
        {"currentPlayer", currentPlayer_},
        {"winner", winner_ >= 0 ? Json(winner_) : Json(nullptr)},
        {"isDraw", draw_},
        {"winningCells", winningCells_}
    };
}

bool ClassicGame::currentPlayerIsAi() const {
    return players_.at(currentPlayer_).isAI;
}

void ClassicGame::performAiTurn(const std::string& difficulty) {
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
        move = chooseBlockingMove(board_, players_, currentPlayer_, 3).value_or(randomEmptyCell(board_));
    } else {
        move = chooseHardMove();
    }

    board_[move[1]][move[0]] = players_[currentPlayer_].symbol;
    finalizeTurn();
}

bool ClassicGame::isOver() const {
    return winner_ >= 0 || draw_;
}

int ClassicGame::minimax(bool maximizing, const std::string& aiSymbol, const std::string& humanSymbol) {
    const auto result = evaluateBoard(board_, 3);
    if (result.winnerSymbol == aiSymbol) {
        return 1;
    }
    if (result.winnerSymbol == humanSymbol) {
        return -1;
    }
    if (result.draw) {
        return 0;
    }

    if (maximizing) {
        int best = std::numeric_limits<int>::min();
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                if (!board_[y][x].empty()) {
                    continue;
                }
                board_[y][x] = aiSymbol;
                best = std::max(best, minimax(false, aiSymbol, humanSymbol));
                board_[y][x].clear();
            }
        }
        return best;
    }

    int best = std::numeric_limits<int>::max();
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (!board_[y][x].empty()) {
                continue;
            }
            board_[y][x] = humanSymbol;
            best = std::min(best, minimax(true, aiSymbol, humanSymbol));
            board_[y][x].clear();
        }
    }
    return best;
}

std::vector<int> ClassicGame::chooseHardMove() {
    const auto aiSymbol = players_[currentPlayer_].symbol;
    const auto humanSymbol = players_[(currentPlayer_ + 1) % 2].symbol;
    int bestScore = std::numeric_limits<int>::min();
    std::vector<int> bestMove = randomEmptyCell(board_);

    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (!board_[y][x].empty()) {
                continue;
            }
            board_[y][x] = aiSymbol;
            const int score = minimax(false, aiSymbol, humanSymbol);
            board_[y][x].clear();
            if (score > bestScore) {
                bestScore = score;
                bestMove = {x, y};
            }
        }
    }
    return bestMove;
}

void ClassicGame::finalizeTurn() {
    const auto result = evaluateBoard(board_, 3);
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
