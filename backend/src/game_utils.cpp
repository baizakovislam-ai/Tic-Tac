#include "game.hpp"

#include "classic_game.hpp"
#include "multi_game.hpp"
#include "ultimate_game.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>

namespace {
std::vector<std::string> symbolSet(const std::string& key) {
    if (key == "blade") {
        return {"⚔", "🛡", "🪓", "🏹"};
    }
    if (key == "elemental") {
        return {"🔥", "❄", "⚡", "🌪"};
    }
    if (key == "cosmic") {
        return {"☀", "☾", "✦", "☄"};
    }
    if (key == "digits") {
        return {"1", "0", "2", "3"};
    }
    return {"X", "O", "△", "□"};
}
}

Game::Game(std::string mode) : mode_(std::move(mode)) {}

const std::string& Game::mode() const {
    return mode_;
}

std::vector<Player> buildPlayers(int count, int humanCount, const std::string& setKey) {
    const auto symbols = symbolSet(setKey);
    std::vector<Player> players;
    players.reserve(count);
    for (int index = 0; index < count; ++index) {
        players.push_back(Player{
            index,
            index >= humanCount ? "ИИ " + std::to_string(index - humanCount + 1) : "Игрок " + std::to_string(index + 1),
            symbols.at(index),
            index >= humanCount
        });
    }
    return players;
}

std::vector<Player> buildDuelPlayers(const std::string& setKey, int humanSymbolIndex) {
    const auto symbols = symbolSet(setKey);
    const int clamped = std::clamp(humanSymbolIndex, 0, 1);
    if (clamped == 0) {
        return {
            Player{0, "Игрок", symbols[0], false},
            Player{1, "ИИ", symbols[1], true}
        };
    }
    return {
        Player{0, "ИИ", symbols[0], true},
        Player{1, "Игрок", symbols[1], false}
    };
}

LineResult evaluateBoard(const Board& board, int winLength) {
    const int height = static_cast<int>(board.size());
    const int width = static_cast<int>(board.front().size());
    const std::vector<std::pair<int, int>> directions = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto& symbol = board[y][x];
            if (symbol.empty()) {
                continue;
            }
            for (const auto& [dx, dy] : directions) {
                std::vector<std::vector<int>> cells = {{x, y}};
                bool valid = true;
                for (int step = 1; step < winLength; ++step) {
                    const int nx = x + dx * step;
                    const int ny = y + dy * step;
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height || board[ny][nx] != symbol) {
                        valid = false;
                        break;
                    }
                    cells.push_back({nx, ny});
                }
                if (valid) {
                    return LineResult{symbol, false, cells};
                }
            }
        }
    }

    return LineResult{std::nullopt, countEmptyCells(board) == 0, {}};
}

int countEmptyCells(const Board& board) {
    int empty = 0;
    for (const auto& row : board) {
        for (const auto& cell : row) {
            if (cell.empty()) {
                ++empty;
            }
        }
    }
    return empty;
}

std::vector<int> randomEmptyCell(const Board& board) {
    std::vector<std::vector<int>> emptyCells;
    for (int y = 0; y < static_cast<int>(board.size()); ++y) {
        for (int x = 0; x < static_cast<int>(board[y].size()); ++x) {
            if (board[y][x].empty()) {
                emptyCells.push_back({x, y});
            }
        }
    }

    static std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(0, emptyCells.size() - 1);
    return emptyCells.at(distribution(engine));
}

std::optional<std::vector<int>> chooseBlockingMove(const Board& board, const std::vector<Player>& players, int currentPlayer, int winLength) {
    const auto aiSymbol = players.at(currentPlayer).symbol;
    Board sandbox = board;

    for (int y = 0; y < static_cast<int>(sandbox.size()); ++y) {
        for (int x = 0; x < static_cast<int>(sandbox[y].size()); ++x) {
            if (!sandbox[y][x].empty()) {
                continue;
            }
            sandbox[y][x] = aiSymbol;
            if (evaluateBoard(sandbox, winLength).winnerSymbol == aiSymbol) {
                return std::vector<int>{x, y};
            }
            sandbox[y][x].clear();
        }
    }

    for (const auto& player : players) {
        if (player.symbol == aiSymbol) {
            continue;
        }
        for (int y = 0; y < static_cast<int>(sandbox.size()); ++y) {
            for (int x = 0; x < static_cast<int>(sandbox[y].size()); ++x) {
                if (!sandbox[y][x].empty()) {
                    continue;
                }
                sandbox[y][x] = player.symbol;
                if (evaluateBoard(sandbox, winLength).winnerSymbol == player.symbol) {
                    return std::vector<int>{x, y};
                }
                sandbox[y][x].clear();
            }
        }
    }

    return std::nullopt;
}

std::unique_ptr<Game> makeGame(const StartConfig& config) {
    if (config.mode == "classic") {
        return std::make_unique<ClassicGame>(config);
    }
    if (config.mode == "ultimate") {
        return std::make_unique<UltimateGame>(config);
    }
    if (config.mode == "multi") {
        return std::make_unique<MultiGame>(config);
    }
    throw std::runtime_error("Unsupported mode");
}
