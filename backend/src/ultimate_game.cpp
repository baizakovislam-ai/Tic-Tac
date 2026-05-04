#include "ultimate_game.hpp"

#include <limits>
#include <random>

namespace {
std::vector<int> macroCoords(int index) {
    return {index % 3, index / 3};
}

int macroIndexFromCoords(int x, int y) {
    return y * 3 + x;
}
}

UltimateGame::UltimateGame(const StartConfig& config)
    : Game("ultimate"),
      macroBoard_(3, std::vector<std::string>(3)),
      miniBoards_(9),
      players_(config.ultimate.matchType == "ai"
          ? buildDuelPlayers(config.settings.symbolSet, config.ultimate.userSymbolIndex)
          : buildPlayers(2, 2, config.settings.symbolSet)),
      currentPlayer_(0) {}

MoveResult UltimateGame::applyMove(const Json& move) {
    if (isOver()) {
        return {false, "Игра уже завершена."};
    }
    if (countRemainingMoves() == 1) {
        draw_ = true;
        return {true, "draw"};
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
    return players_.at(currentPlayer_).isAI;
}

void UltimateGame::performAiTurn(const std::string& difficulty) {
    if (isOver() || !currentPlayerIsAi()) {
        return;
    }
    if (countRemainingMoves() == 1) {
        draw_ = true;
        return;
    }

    std::vector<int> move;
    if (difficulty == "easy") {
        const auto legalMoves = collectLegalMoves();
        static std::mt19937 engine(std::random_device{}());
        std::uniform_int_distribution<std::size_t> distribution(0, legalMoves.size() - 1);
        move = legalMoves.at(distribution(engine));
    } else {
        move = chooseHeuristicMove(difficulty);
    }
    applyMove({
        {"macroIndex", move[0]},
        {"x", move[1]},
        {"y", move[2]}
    });
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

int UltimateGame::countRemainingMoves() const {
    int total = 0;
    for (const auto& mini : miniBoards_) {
        if (!mini.winner.empty() || mini.draw) {
            continue;
        }
        total += countEmptyCells(mini.board);
    }
    return total;
}

std::vector<std::vector<int>> UltimateGame::collectLegalMoves() const {
    std::vector<std::vector<int>> moves;
    for (int macroIndex = 0; macroIndex < static_cast<int>(miniBoards_.size()); ++macroIndex) {
        const auto& mini = miniBoards_[macroIndex];
        if (!mini.winner.empty() || mini.draw) {
            continue;
        }
        if (activeMacroIndex_ != -1 && activeMacroIndex_ != macroIndex) {
            continue;
        }
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                if (mini.board[y][x].empty()) {
                    moves.push_back({macroIndex, x, y});
                }
            }
        }
    }

    if (!moves.empty()) {
        return moves;
    }

    for (int macroIndex = 0; macroIndex < static_cast<int>(miniBoards_.size()); ++macroIndex) {
        const auto& mini = miniBoards_[macroIndex];
        if (!mini.winner.empty() || mini.draw) {
            continue;
        }
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                if (mini.board[y][x].empty()) {
                    moves.push_back({macroIndex, x, y});
                }
            }
        }
    }

    return moves;
}

std::vector<int> UltimateGame::chooseHeuristicMove(const std::string& difficulty) {
    const auto legalMoves = collectLegalMoves();
    int bestScore = std::numeric_limits<int>::min();
    std::vector<int> bestMove = legalMoves.front();
    const bool withOpponentReply = difficulty == "hard";

    for (const auto& move : legalMoves) {
        const int score = evaluateHeuristicMove(move[0], move[1], move[2], withOpponentReply);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    return bestMove;
}

int UltimateGame::evaluateHeuristicMove(int macroIndex, int x, int y, bool withOpponentReply) {
    UltimateGame sandbox = *this;
    auto& mini = sandbox.miniBoards_[macroIndex];
    mini.board[y][x] = sandbox.players_[sandbox.currentPlayer_].symbol;

    const auto miniResult = evaluateBoard(mini.board, 3);
    if (miniResult.winnerSymbol.has_value()) {
        mini.winner = miniResult.winnerSymbol.value();
        mini.winningCells = miniResult.winningCells;
        const auto coords = macroCoords(macroIndex);
        sandbox.macroBoard_[coords[1]][coords[0]] = mini.winner;
    } else if (miniResult.draw) {
        mini.draw = true;
    }

    const int nextMacro = y * 3 + x;
    if (!sandbox.miniBoards_[nextMacro].winner.empty() || sandbox.miniBoards_[nextMacro].draw) {
        sandbox.activeMacroIndex_ = -1;
    } else {
        sandbox.activeMacroIndex_ = nextMacro;
    }

    sandbox.refreshMacroResult();
    int score = sandbox.scoreUltimatePosition(currentPlayer_);
    score += (x == 1 && y == 1 ? 24 : 0);
    score += (macroCoords(macroIndex)[0] == 1 && macroCoords(macroIndex)[1] == 1 ? 10 : 0);

    if (!withOpponentReply || sandbox.isOver()) {
        return score;
    }

    sandbox.currentPlayer_ = (sandbox.currentPlayer_ + 1) % static_cast<int>(sandbox.players_.size());
    int worstReply = std::numeric_limits<int>::min();
    for (const auto& reply : sandbox.collectLegalMoves()) {
        const int replyScore = sandbox.evaluateHeuristicMove(reply[0], reply[1], reply[2], false);
        if (replyScore > worstReply) {
            worstReply = replyScore;
        }
    }
    if (worstReply == std::numeric_limits<int>::min()) {
        worstReply = 0;
    }
    return score - static_cast<int>(worstReply * 0.7);
}

int UltimateGame::scoreUltimatePosition(int perspectivePlayer) const {
    int score = 0;
    const auto& self = players_[perspectivePlayer].symbol;
    const auto& other = players_[(perspectivePlayer + 1) % 2].symbol;

    const auto macroResult = evaluateBoard(macroBoard_, 3);
    if (macroResult.winnerSymbol == self) {
        return 100000;
    }
    if (macroResult.winnerSymbol == other) {
        return -100000;
    }

    for (int macroIndex = 0; macroIndex < 9; ++macroIndex) {
        const auto& mini = miniBoards_[macroIndex];
        if (mini.winner == self) {
            score += 1200;
        } else if (mini.winner == other) {
            score -= 1200;
        } else if (!mini.draw) {
            const auto miniEval = evaluateBoard(mini.board, 3);
            if (miniEval.winnerSymbol == self) {
                score += 900;
            } else if (miniEval.winnerSymbol == other) {
                score -= 900;
            }
            if (mini.board[1][1] == self) {
                score += 25;
            } else if (mini.board[1][1] == other) {
                score -= 25;
            }
        }
    }

    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (macroBoard_[y][x] == self) {
                score += (x == 1 && y == 1) ? 100 : 45;
            } else if (macroBoard_[y][x] == other) {
                score -= (x == 1 && y == 1) ? 100 : 45;
            }
        }
    }

    return score;
}
