#pragma once

#include "game.hpp"

class MultiGame : public Game {
  public:
    explicit MultiGame(const StartConfig& config);

    MoveResult applyMove(const Json& move) override;
    Json toJson() const override;
    bool currentPlayerIsAi() const override;
    void performAiTurn(const std::string& difficulty) override;
    bool isOver() const override;

  private:
    void finalizeTurn();

    Board board_;
    std::vector<Player> players_;
    int boardSize_ {5};
    int winLength_ {3};
    int currentPlayer_ {0};
    int winner_ {-1};
    bool draw_ {false};
    std::vector<std::vector<int>> winningCells_;
};
