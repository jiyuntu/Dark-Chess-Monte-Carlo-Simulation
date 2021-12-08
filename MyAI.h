#ifndef MYAI_INCLUDED
#define MYAI_INCLUDED

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <functional>
#include <queue>
#include <utility>
#include <vector>
#include <algorithm>

#include "pcg_basic.h"

#define RED 0
#define BLACK 1
#define CHESS_COVER -1
#define CHESS_EMPTY -2
#define COMMAND_NUM 18

#define MAX_CHILD 64
#define MAX_NODE 10000000

struct ChessBoard {
  int Board[32];
  int CoverChess[14];
  int Red_Chess_Num, Black_Chess_Num;
  int NoEatFlip;
  int History[4096];
  int HistoryCount;
};

struct UCTNode {
  std::priority_queue<std::pair<double, int>,
                      std::vector<std::pair<double, int> >,
                      std::greater<std::pair<double, int> > >
      pq;
  int last_move, real_simulation_times, RAVE_simulation_times;
  double real_score, RAVE_score;
  double real_score_square_sum, RAVE_score_square_sum;
  double UCT_score;
};

extern UCTNode UCT_nodes[MAX_NODE];
extern int UCT_nodes_size;
extern bool play[32][32][32][32];
extern int RAVE_moves[1024][128];
extern int RAVE_moves_from[1024][128], RAVE_moves_to[1024][128];
extern std::priority_queue<std::pair<double, int>,
                           std::vector<std::pair<double, int> >,
                           std::greater<std::pair<double, int> > >
    root_pq;

class MyAI {
  const char* commands_name[COMMAND_NUM] = {
      "protocol_version",  "name",          "version",
      "known_command",     "list_commands", "quit",
      "boardsize",         "reset_board",   "num_repetition",
      "num_moves_to_draw", "move",          "flip",
      "genmove",           "game_over",     "ready",
      "time_settings",     "time_left",     "showboard"};

 public:
  MyAI(void);
  ~MyAI(void);

  // commands
  bool protocol_version(const char* data[], char* response);   // 0
  bool name(const char* data[], char* response);               // 1
  bool version(const char* data[], char* response);            // 2
  bool known_command(const char* data[], char* response);      // 3
  bool list_commands(const char* data[], char* response);      // 4
  bool quit(const char* data[], char* response);               // 5
  bool boardsize(const char* data[], char* response);          // 6
  bool reset_board(const char* data[], char* response);        // 7
  bool num_repetition(const char* data[], char* response);     // 8
  bool num_moves_to_draw(const char* data[], char* response);  // 9
  bool move(const char* data[], char* response);               // 10
  bool flip(const char* data[], char* response);               // 11
  bool genmove(const char* data[], char* response);            // 12
  bool game_over(const char* data[], char* response);          // 13
  bool ready(const char* data[], char* response);              // 14
  bool time_settings(const char* data[], char* response);      // 15
  bool time_left(const char* data[], char* response);          // 16
  bool showboard(const char* data[], char* response);          // 17

 private:
  int Color;
  int Red_Time, Black_Time;
  ChessBoard main_chessboard;
  const double eps = 1e-6;
  const double exploration = -0.25;
  const double parameter_c1 = 0.1, parameter_c2 = 100;
  const double RAVE_parameter = 2.;

#ifdef WINDOWS
  clock_t begin;
#else
  struct timeval begin;
#endif

  pcg32_random_t rng;

  // Utils
  int GetFin(char c);
  int ConvertChessNo(int input);
  bool isTimeUp();
  uint32_t randIndex(uint32_t max);

  // Board
  void initBoardState();
  void generateMove(char move[6]);
  void MakeMove(ChessBoard* chessboard, const int move, const int chess);
  void MakeMove(ChessBoard* chessboard, const char move[6]);
  bool Referee(const int* board, const int Startoint, const int EndPoint,
               const int color);
  int Expand(const int* board, const int color, int* Result);
  double Evaluate(const ChessBoard* chessboard, const int legal_move_count,
                  const int color);
  std::pair<double, int> Simulate(ChessBoard chessboard, int color);
  bool isDraw(const ChessBoard* chessboard);
  bool isFinish(const ChessBoard* chessboard, int move_count);
  void assignUCTNode(int id, int last_move);
  std::pair<std::pair<std::pair<double, double>, int>,
            std::pair<std::pair<double, double>, int> >
  nega_Max(ChessBoard chessboard, int node_id, int color, int depth);
  double calculate_uct(double real_score, int real_simulation_times,
                       double RAVE_score, int RAVE_simulation_times,
                       double real_score_square_sum,
                       double RAVE_score_square_sum,
                       int parent_real_simulation_times,
                       int parent_RAVE_simulation_times);

  // Display
  void Pirnf_Chess(int chess_no, char* Result);
  void Pirnf_Chessboard();
};

#endif
