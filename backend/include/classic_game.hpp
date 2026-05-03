#pragma once

#include "game.hpp"

class ClassicGame : public Game {
  public:
    explicit ClassicGame(const StartConfig& config);

    MoveResult applyMove(const Json& move) override;
    Json toJson() const override;
    bool currentPlayerIsAi() const override;
    void performAiTurn(const std::string& difficulty) override;
    bool isOver() const override;

  private:
    int minimax(bool maximizing, const std::string& aiSymbol, const std::string& humanSymbol);
    std::vector<int> chooseHardMove();
    void finalizeTurn();

    Board board_;
    std::vector<Player> players_;
    int currentPlayer_ {0};
    int winner_ {-1};
    bool draw_ {false};
    std::vector<std::vector<int>> winningCells_;
};
