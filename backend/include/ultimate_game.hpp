#pragma once

#include "game.hpp"

struct MiniBoardState {
    Board board {Board(3, std::vector<std::string>(3))};
    std::string winner;
    bool draw {false};
    std::vector<std::vector<int>> winningCells;
};

class UltimateGame : public Game {
  public:
    explicit UltimateGame(const StartConfig& config);

    MoveResult applyMove(const Json& move) override;
    Json toJson() const override;
    bool currentPlayerIsAi() const override;
    void performAiTurn(const std::string& difficulty) override;
    bool isOver() const override;

  private:
    void refreshMacroResult();
    bool allMiniBoardsFinished() const;
    int countRemainingMoves() const;
    std::vector<std::vector<int>> collectLegalMoves() const;
    std::vector<int> chooseHeuristicMove(const std::string& difficulty);
    int evaluateHeuristicMove(int macroIndex, int x, int y, bool withOpponentReply);
    int scoreUltimatePosition(int perspectivePlayer) const;

    Board macroBoard_;
    std::vector<MiniBoardState> miniBoards_;
    std::vector<Player> players_;
    int currentPlayer_ {0};
    int activeMacroIndex_ {-1};
    int winner_ {-1};
    bool draw_ {false};
};
