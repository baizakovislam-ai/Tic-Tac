#pragma once

#include "models.hpp"

#include <memory>
#include <string>
#include <vector>

using Board = std::vector<std::vector<std::string>>;

std::vector<Player> buildPlayers(int count, int humanCount, const std::string& symbolSet);
LineResult evaluateBoard(const Board& board, int winLength);
std::vector<int> randomEmptyCell(const Board& board);
std::optional<std::vector<int>> chooseBlockingMove(const Board& board, const std::vector<Player>& players, int currentPlayer, int winLength);

class Game {
  public:
    explicit Game(std::string mode);
    virtual ~Game() = default;

    virtual MoveResult applyMove(const Json& move) = 0;
    virtual Json toJson() const = 0;
    virtual bool currentPlayerIsAi() const = 0;
    virtual void performAiTurn(const std::string& difficulty) = 0;
    virtual bool isOver() const = 0;

    const std::string& mode() const;

  protected:
    std::string mode_;
};

std::unique_ptr<Game> makeGame(const StartConfig& config);
