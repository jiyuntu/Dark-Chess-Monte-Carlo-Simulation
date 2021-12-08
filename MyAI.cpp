#include "float.h"

#ifdef WINDOWS
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "MyAI.h"

#define TIME_LIMIT 9.5

#define WIN 1.0
#define DRAW 0.2
#define LOSE 0.0
#define BONUS 0.3

#define MAX_SCORE (WIN + BONUS)

#define NOEATFLIP_LIMIT 60
#define POSITION_REPETITION_LIMIT 3

#define SIMULATE_COUNT_PER_CHILD 10
#define MAX_SIMULATION_COUNT 20000

MyAI::MyAI(void) {
  pcg32_srandom_r(&this->rng, time(NULL) ^ (intptr_t)&printf,
                  (intptr_t) & this->Color);
}

MyAI::~MyAI(void) {}

bool MyAI::protocol_version(const char* data[], char* response) {
  strcpy(response, "1.0.0");
  return 0;
}

bool MyAI::name(const char* data[], char* response) {
  strcpy(response, "MyAI");
  return 0;
}

bool MyAI::version(const char* data[], char* response) {
  strcpy(response, "1.0.0");
  return 0;
}

bool MyAI::known_command(const char* data[], char* response) {
  for (int i = 0; i < COMMAND_NUM; i++) {
    if (!strcmp(data[0], commands_name[i])) {
      strcpy(response, "true");
      return 0;
    }
  }
  strcpy(response, "false");
  return 0;
}

bool MyAI::list_commands(const char* data[], char* response) {
  for (int i = 0; i < COMMAND_NUM; i++) {
    strcat(response, commands_name[i]);
    if (i < COMMAND_NUM - 1) {
      strcat(response, "\n");
    }
  }
  return 0;
}

bool MyAI::quit(const char* data[], char* response) {
  fprintf(stderr, "Bye\n");
  return 0;
}

bool MyAI::boardsize(const char* data[], char* response) {
  fprintf(stderr, "BoardSize: %s x %s\n", data[0], data[1]);
  return 0;
}

bool MyAI::reset_board(const char* data[], char* response) {
  this->Red_Time = -1;    // unknown
  this->Black_Time = -1;  // unknown
  this->initBoardState();
  return 0;
}

bool MyAI::num_repetition(const char* data[], char* response) { return 0; }

bool MyAI::num_moves_to_draw(const char* data[], char* response) { return 0; }

bool MyAI::move(const char* data[], char* response) {
  char move[6];
  sprintf(move, "%s-%s", data[0], data[1]);
  this->MakeMove(&(this->main_chessboard), move);
  return 0;
}

bool MyAI::flip(const char* data[], char* response) {
  char move[6];
  sprintf(move, "%s(%s)", data[0], data[1]);
  this->MakeMove(&(this->main_chessboard), move);
  return 0;
}

bool MyAI::genmove(const char* data[], char* response) {
  // set color
  if (!strcmp(data[0], "red")) {
    this->Color = RED;
  } else if (!strcmp(data[0], "black")) {
    this->Color = BLACK;
  } else {
    this->Color = 2;
  }
  // genmove
  char move[6];
  this->generateMove(move);
  sprintf(response, "%c%c %c%c", move[0], move[1], move[3], move[4]);
  return 0;
}

bool MyAI::game_over(const char* data[], char* response) {
  fprintf(stderr, "Game Result: %s\n", data[0]);
  return 0;
}

bool MyAI::ready(const char* data[], char* response) { return 0; }

bool MyAI::time_settings(const char* data[], char* response) { return 0; }

bool MyAI::time_left(const char* data[], char* response) {
  if (!strcmp(data[0], "red")) {
    sscanf(data[1], "%d", &(this->Red_Time));
  } else {
    sscanf(data[1], "%d", &(this->Black_Time));
  }
  fprintf(stderr, "Time Left(%s): %s\n", data[0], data[1]);
  return 0;
}

bool MyAI::showboard(const char* data[], char* response) {
  Pirnf_Chessboard();
  return 0;
}

// *********************** AI FUNCTION *********************** //

int MyAI::GetFin(char c) {
  static const char skind[] = {'-', 'K', 'G', 'M', 'R', 'N', 'C', 'P',
                               'X', 'k', 'g', 'm', 'r', 'n', 'c', 'p'};
  for (int f = 0; f < 16; f++)
    if (c == skind[f]) return f;
  return -1;
}

int MyAI::ConvertChessNo(int input) {
  switch (input) {
    case 0:
      return CHESS_EMPTY;
      break;
    case 8:
      return CHESS_COVER;
      break;
    case 1:
      return 6;
      break;
    case 2:
      return 5;
      break;
    case 3:
      return 4;
      break;
    case 4:
      return 3;
      break;
    case 5:
      return 2;
      break;
    case 6:
      return 1;
      break;
    case 7:
      return 0;
      break;
    case 9:
      return 13;
      break;
    case 10:
      return 12;
      break;
    case 11:
      return 11;
      break;
    case 12:
      return 10;
      break;
    case 13:
      return 9;
      break;
    case 14:
      return 8;
      break;
    case 15:
      return 7;
      break;
  }
  return -1;
}

bool play[32][32][32][32];
std::vector<int> graph[32];

void MyAI::initBoardState() {
  memset(play, 0, sizeof(play));
  for (int i = 0; i < 32; i++) {
    int Move[4] = {i - 4, i + 1, i + 4, i - 1};
    for (int k = 0; k < 4; k++) {
      if (Move[k] >= 0 && Move[k] < 32 &&
          abs(Move[k] / 4 - i / 4) + abs(Move[k] % 4 - i % 4) == 1) {
        graph[i].push_back(Move[k]);
      }
    }
  }

  int iPieceCount[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
  memcpy(main_chessboard.CoverChess, iPieceCount, sizeof(int) * 14);
  main_chessboard.Red_Chess_Num = 16;
  main_chessboard.Black_Chess_Num = 16;
  main_chessboard.NoEatFlip = 0;
  main_chessboard.HistoryCount = 0;

  // convert to my format
  int Index = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 4; j++) {
      main_chessboard.Board[Index] = CHESS_COVER;
      Index++;
    }
  }
  Pirnf_Chessboard();
}

void MyAI::assignUCTNode(int id, int last_move) {
  while (!UCT_nodes[id].pq.empty()) UCT_nodes[id].pq.pop();
  UCT_nodes[id].last_move = last_move;
  UCT_nodes[id].real_score = 0.;
  UCT_nodes[id].RAVE_score = 0.;
  UCT_nodes[id].real_score_square_sum = 0.;
  UCT_nodes[id].RAVE_score_square_sum = 0.;
  UCT_nodes[id].UCT_score = 0.;
  UCT_nodes[id].real_simulation_times = 0;
  UCT_nodes[id].RAVE_simulation_times = 0;
}

UCTNode UCT_nodes[MAX_NODE];
int UCT_nodes_size;

std::priority_queue<std::pair<double, int>,
                    std::vector<std::pair<double, int> >,
                    std::greater<std::pair<double, int> > >
    root_pq;

void MyAI::generateMove(char move[6]) {
#ifdef WINDOWS
  begin = clock();
#else
  gettimeofday(&begin, 0);
#endif

  while (!root_pq.empty()) root_pq.pop();
  UCT_nodes_size = 0;
  assignUCTNode(UCT_nodes_size++, 0);
  while (!isTimeUp()) {
    std::pair<std::pair<std::pair<double, double>, int>,
              std::pair<std::pair<double, double>, int> >
        ret = nega_Max(this->main_chessboard, 0, this->Color, 0);
    UCT_nodes[0].real_score += ret.first.first.first;
    UCT_nodes[0].real_score_square_sum += ret.first.first.second;
    UCT_nodes[0].real_simulation_times += ret.first.second;
    UCT_nodes[0].RAVE_score += ret.second.first.first;
    UCT_nodes[0].RAVE_score_square_sum += ret.second.first.second;
    UCT_nodes[0].RAVE_simulation_times += ret.second.second;
  }

  double s;
  int id = 0;
  while (!root_pq.empty()) {
    s = root_pq.top().first;
    id = root_pq.top().second;
    if (abs(s - UCT_nodes[id].real_score /
                    UCT_nodes[id].real_simulation_times) > eps)
      root_pq.pop();
    else
      break;
  }
  int mv = UCT_nodes[id].last_move;

  // log
  int tmp_start = mv / 100;
  int tmp_end = mv % 100;
  // change int to char
  sprintf(move, "%c%c-%c%c", 'a' + (tmp_start % 4), '1' + (7 - tmp_start / 4),
          'a' + (tmp_end % 4), '1' + (7 - tmp_end / 4));
  fprintf(stderr, "Move: %s, Score: %+5lf, Sim_Count: %7d\n", move,
          UCT_nodes[id].UCT_score, UCT_nodes[id].real_simulation_times);
  fflush(stderr);

  // set return value
  char chess_Start[4] = "";
  char chess_End[4] = "";
  Pirnf_Chess(main_chessboard.Board[tmp_start], chess_Start);
  Pirnf_Chess(main_chessboard.Board[tmp_end], chess_End);
  printf("My result: \n--------------------------\n");
  printf("UCT: %lf (simulation count: %d)\n", UCT_nodes[id].UCT_score,
         UCT_nodes[id].real_simulation_times);
  printf("(%d) -> (%d)\n", tmp_start, tmp_end);
  printf("<%s> -> <%s>\n", chess_Start, chess_End);
  printf("move:%s\n", move);
  printf("--------------------------\n");
  this->Pirnf_Chessboard();
}

void MyAI::MakeMove(ChessBoard* chessboard, const int move, const int chess) {
  int src = move / 100, dst = move % 100;
  if (src == dst) {  // flip
    chessboard->Board[src] = chess;
    chessboard->CoverChess[chess]--;
    chessboard->NoEatFlip = 0;
  } else {  // move
    if (chessboard->Board[dst] != CHESS_EMPTY) {
      if (chessboard->Board[dst] / 7 == 0) {  // red
        (chessboard->Red_Chess_Num)--;
      } else {  // black
        (chessboard->Black_Chess_Num)--;
      }
      chessboard->NoEatFlip = 0;
    } else {
      chessboard->NoEatFlip += 1;
    }
    chessboard->Board[dst] = chessboard->Board[src];
    chessboard->Board[src] = CHESS_EMPTY;
  }
  chessboard->History[chessboard->HistoryCount++] = move;
}

void MyAI::MakeMove(ChessBoard* chessboard, const char move[6]) {
  int src, dst, m;
  src = ('8' - move[1]) * 4 + (move[0] - 'a');
  if (move[2] == '(') {  // flip
    m = src * 100 + src;
    printf("# call flip(): flip(%d,%d) = %d\n", src, src, GetFin(move[3]));
  } else {  // move
    dst = ('8' - move[4]) * 4 + (move[3] - 'a');
    m = src * 100 + dst;
    printf("# call move(): move : %d-%d \n", src, dst);
  }
  MakeMove(chessboard, m, ConvertChessNo(GetFin(move[3])));
  Pirnf_Chessboard();
}

int MyAI::Expand(const int* board, const int color, int* Result) {
  int ResultCount = 0;
  for (int i = 0; i < 32; i++) {
    if (board[i] >= 0 && board[i] / 7 == color) {
      // Gun
      if (board[i] % 7 == 1) {
        int row = i / 4;
        int col = i % 4;
        for (int rowCount = row * 4; rowCount < (row + 1) * 4; rowCount++) {
          if (Referee(board, i, rowCount, color)) {
            Result[ResultCount] = i * 100 + rowCount;
            ResultCount++;
          }
        }
        for (int colCount = col; colCount < 32; colCount += 4) {
          if (Referee(board, i, colCount, color)) {
            Result[ResultCount] = i * 100 + colCount;
            ResultCount++;
          }
        }
      } else {
        for (int mv : graph[i]) {
          if (Referee(board, i, mv, color)) {
            Result[ResultCount] = i * 100 + mv;
            ResultCount++;
          }
        }
      }
    };
  }
  return ResultCount;
}

// Referee
bool MyAI::Referee(const int* chess, const int from_location_no,
                   const int to_location_no, const int UserId) {
  // int MessageNo = 0;
  bool IsCurrent = true;
  int from_chess_no = chess[from_location_no];
  int to_chess_no = chess[to_location_no];
  int from_row = from_location_no / 4;
  int to_row = to_location_no / 4;
  int from_col = from_location_no % 4;
  int to_col = to_location_no % 4;

  if (from_chess_no < 0 || (to_chess_no < 0 && to_chess_no != CHESS_EMPTY)) {
    // MessageNo = 1;
    // strcat(Message,"**no chess can move**");
    // strcat(Message,"**can't move darkchess**");
    IsCurrent = false;
  } else if (from_chess_no >= 0 && from_chess_no / 7 != UserId) {
    // MessageNo = 2;
    // strcat(Message,"**not my chess**");
    IsCurrent = false;
  } else if ((from_chess_no / 7 == to_chess_no / 7) && to_chess_no >= 0) {
    // MessageNo = 3;
    // strcat(Message,"**can't eat my self**");
    IsCurrent = false;
  }
  // check attack
  else if (to_chess_no == CHESS_EMPTY &&
           abs(from_row - to_row) + abs(from_col - to_col) == 1)  // legal move
  {
    IsCurrent = true;
  } else if (from_chess_no % 7 == 1)  // judge gun
  {
    int row_gap = from_row - to_row;
    int col_gap = from_col - to_col;
    int between_Count = 0;
    // slant
    if (from_row - to_row == 0 || from_col - to_col == 0) {
      // row
      if (row_gap == 0) {
        for (int i = 1; i < abs(col_gap); i++) {
          int between_chess;
          if (col_gap > 0)
            between_chess = chess[from_location_no - i];
          else
            between_chess = chess[from_location_no + i];
          if (between_chess != CHESS_EMPTY) between_Count++;
        }
      }
      // column
      else {
        for (int i = 1; i < abs(row_gap); i++) {
          int between_chess;
          if (row_gap > 0)
            between_chess = chess[from_location_no - 4 * i];
          else
            between_chess = chess[from_location_no + 4 * i];
          if (between_chess != CHESS_EMPTY) between_Count++;
        }
      }

      if (between_Count != 1) {
        // MessageNo = 4;
        // strcat(Message,"**gun can't eat opp without between one piece**");
        IsCurrent = false;
      } else if (to_chess_no == CHESS_EMPTY) {
        // MessageNo = 5;
        // strcat(Message,"**gun can't eat opp without between one piece**");
        IsCurrent = false;
      }
    }
    // slide
    else {
      // MessageNo = 6;
      // strcat(Message,"**cant slide**");
      IsCurrent = false;
    }
  } else  // non gun
  {
    // judge pawn or king

    // distance
    if (abs(from_row - to_row) + abs(from_col - to_col) > 1) {
      // MessageNo = 7;
      // strcat(Message,"**cant eat**");
      IsCurrent = false;
    }
    // judge pawn
    else if (from_chess_no % 7 == 0) {
      if (to_chess_no % 7 != 0 && to_chess_no % 7 != 6) {
        // MessageNo = 8;
        // strcat(Message,"**pawn only eat pawn and king**");
        IsCurrent = false;
      }
    }
    // judge king
    else if (from_chess_no % 7 == 6 && to_chess_no % 7 == 0) {
      // MessageNo = 9;
      // strcat(Message,"**king can't eat pawn**");
      IsCurrent = false;
    } else if (from_chess_no % 7 < to_chess_no % 7) {
      // MessageNo = 10;
      // strcat(Message,"**cant eat**");
      IsCurrent = false;
    }
  }
  return IsCurrent;
}

// always use my point of view, so use this->Color
double MyAI::Evaluate(const ChessBoard* chessboard, const int legal_move_count,
                      const int color) {
  // score = My Score - Opponent's Score
  double score = 0;
  int win = 0;

  if (color == this->Color || isDraw(chessboard)) {  // Lose
    score += LOSE - WIN;
  } else {  // Win
    score += WIN - LOSE;
    win = 1;
  }

  const double values[14] = {1, 180, 6, 18, 90, 270, 810,
                             1, 180, 6, 18, 90, 270, 810};

  double piece_value = 0;
  // flipped
  for (int i = 0; i < 32; i++) {
    if (chessboard->Board[i] != CHESS_EMPTY &&
        chessboard->Board[i] != CHESS_COVER) {
      if (chessboard->Board[i] / 7 == this->Color) {
        piece_value += values[chessboard->Board[i]];
      } else {
        piece_value -= values[chessboard->Board[i]];
      }
    }
  }
  // covered
  for (int i = 0; i < 14; ++i) {
    if (chessboard->CoverChess[i] > 0) {
      if (i / 7 == this->Color) {
        piece_value += chessboard->CoverChess[i] * values[i];
      } else {
        piece_value -= chessboard->CoverChess[i] * values[i];
      }
    }
  }

  if (!win) {               // I lose
    if (piece_value > 0) {  // but net value > 0
      piece_value = 0;
    }
  } else {                  // Opponent lose
    if (piece_value < 0) {  // but net value < 0
      piece_value = 0;
    }
  }

  // linear map to [-<BONUS>, <BONUS>]
  // score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
  // <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * <BONUS>
  piece_value = piece_value / 1943 * BONUS;
  score += piece_value;

  return score;
}

double MyAI::calculate_uct(double real_score, int real_simulation_times,
                           double RAVE_score, int RAVE_simulation_times,
                           double real_score_square_sum,
                           double RAVE_score_square_sum,
                           int parent_real_simulation_times,
                           int parent_RAVE_simulation_times) {
  if (real_simulation_times == 0) return 0.;
  double real_variance =
      real_score_square_sum / real_simulation_times -
      real_score / real_simulation_times * real_score / real_simulation_times;
  double real_V =
      real_variance + parameter_c1 * sqrt(log(parent_real_simulation_times) /
                                          real_simulation_times);
  double real_UCT =
      real_score / real_simulation_times +
      exploration * sqrt(log(parent_real_simulation_times) /
                         real_simulation_times * fmin(real_V, parameter_c2));
  double RAVE_variance =
      RAVE_score_square_sum / RAVE_simulation_times -
      RAVE_score / RAVE_simulation_times * RAVE_score / RAVE_simulation_times;
  double RAVE_V = RAVE_variance + parameter_c1 * sqrt(log(parent_RAVE_simulation_times) / RAVE_simulation_times);
  double RAVE_UCT = RAVE_score / RAVE_simulation_times +
                exploration * sqrt(log(parent_RAVE_simulation_times) /
                                   RAVE_simulation_times * fmin(RAVE_V, parameter_c2));

<<<<<<< HEAD
  double beta = RAVE_simulation_times /
                (real_simulation_times + RAVE_simulation_times +
                 4 * RAVE_parameter * RAVE_parameter * real_simulation_times *
                     RAVE_simulation_times);
=======
  // double beta = RAVE_simulation_times /
  //               (real_simulation_times + RAVE_simulation_times +
  //                4 * RAVE_parameter * RAVE_parameter * real_simulation_times
  //                *
  //                    RAVE_simulation_times);

  double beta = 0.;
>>>>>>> 7fb4843 (revised UCB)

  return (1 - beta) * real_UCT + beta * RAVE_UCT;
}

std::pair<std::pair<std::pair<double, double>, int>,
          std::pair<std::pair<double, double>, int> >
MyAI::nega_Max(ChessBoard chessboard, int node_id, int color, int depth) {
  // return < < <real score, real_score_square_sum>, real simulation times>, <
  // <RAVE score, RAVE score square sum>, RAVE simulation times> >
  if (isFinish(&chessboard, 100) || isTimeUp()) {
    return std::make_pair(
        std::make_pair(std::make_pair(0, 0), 0),
        std::make_pair(std::make_pair(0, 0), 0));  // 100: dummy
  }
  double real_score = 0., RAVE_score = 0.;
  double real_score_square_sum = 0., RAVE_score_square_sum = 0.;
  int real_simulation_times = 0, RAVE_simulation_times = 0;
  if (UCT_nodes[node_id].pq.size() != 0) {  // not leaf node
    int x;
    double s;
    while (!UCT_nodes[node_id].pq.empty()) {
      s = UCT_nodes[node_id].pq.top().first;
      x = UCT_nodes[node_id].pq.top().second;
      if (abs(UCT_nodes[x].UCT_score - s) > eps ||
          UCT_nodes[x].real_simulation_times > MAX_SIMULATION_COUNT) {
        UCT_nodes[node_id].pq.pop();
      } else {
        break;
      }
    }
    UCTNode next_node = UCT_nodes[x];
    MakeMove(&chessboard, next_node.last_move, 0);
    std::pair<std::pair<std::pair<double, double>, int>,
              std::pair<std::pair<double, double>, int> >
        ret = nega_Max(chessboard, x, color ^ 1, depth + 1);

    real_score -= ret.first.first.first;
    real_simulation_times += ret.first.second;
    real_score_square_sum += ret.first.first.second;
    RAVE_score -= ret.second.first.first;
    RAVE_simulation_times += ret.second.second;
    RAVE_score_square_sum += ret.second.first.second;

    UCT_nodes[x].real_score += ret.first.first.first;
    UCT_nodes[x].real_simulation_times += ret.first.second;
    UCT_nodes[x].real_score_square_sum += ret.first.first.second;
    UCT_nodes[x].RAVE_score += ret.second.first.first;
    UCT_nodes[x].RAVE_simulation_times += ret.second.second;
    UCT_nodes[x].RAVE_score_square_sum += ret.second.first.second;
    UCT_nodes[x].UCT_score = calculate_uct(
        UCT_nodes[x].real_score, UCT_nodes[x].real_simulation_times,
        UCT_nodes[x].RAVE_score, UCT_nodes[x].RAVE_simulation_times,
        UCT_nodes[x].real_score_square_sum, UCT_nodes[x].RAVE_score_square_sum,
        UCT_nodes[node_id].real_simulation_times + real_simulation_times,
        UCT_nodes[node_id].RAVE_simulation_times + RAVE_simulation_times);
    UCT_nodes[node_id].pq.push(std::make_pair(UCT_nodes[x].UCT_score, x));
    if (node_id == 0)
      root_pq.push(std::make_pair(
          UCT_nodes[x].real_score / UCT_nodes[x].real_simulation_times, x));
  } else {  // leaf node
    int Moves[2048];
    int move_count = Expand(chessboard.Board, color, Moves);

    for (int i = 0; i < move_count; ++i) {
      int child_id = UCT_nodes_size;
      assignUCTNode(UCT_nodes_size++, Moves[i]);
      ChessBoard child_board = chessboard;
      MakeMove(&child_board, Moves[i], 0);

      for (int k = 0; k < SIMULATE_COUNT_PER_CHILD; ++k) {
        std::pair<double, int> s = Simulate(child_board, color ^ 1);
        UCT_nodes[child_id].real_score += s.first / s.second;
        UCT_nodes[child_id].real_score_square_sum +=
            (s.first / s.second) * (s.first / s.second);
        UCT_nodes[child_id].RAVE_score += s.first;
        UCT_nodes[child_id].RAVE_score_square_sum +=
            s.second * (s.first / s.second) * (s.first / s.second);
        real_score -= s.first / s.second;
        real_score_square_sum += (s.first / s.second) * (s.first / s.second);
        RAVE_score -= s.first;
        RAVE_score_square_sum +=
            s.second * (s.first / s.second) * (s.first / s.second);
        UCT_nodes[child_id].real_simulation_times += 1;
        UCT_nodes[child_id].RAVE_simulation_times += s.second;
        real_simulation_times++;
        RAVE_simulation_times += s.second;
      }
    }

    for (int i = 0; i < move_count; i++) {
      int child_id = UCT_nodes_size - move_count + i;
      UCT_nodes[child_id].UCT_score = calculate_uct(
          UCT_nodes[child_id].real_score,
          UCT_nodes[child_id].real_simulation_times,
          UCT_nodes[child_id].RAVE_score,
          UCT_nodes[child_id].RAVE_simulation_times,
          UCT_nodes[child_id].real_score_square_sum,
          UCT_nodes[child_id].RAVE_score_square_sum,
          UCT_nodes[node_id].real_simulation_times + real_simulation_times,
          UCT_nodes[node_id].RAVE_simulation_times + RAVE_simulation_times);
      UCT_nodes[node_id].pq.push(
          std::make_pair(UCT_nodes[child_id].UCT_score, child_id));
      if (node_id == 0)
        root_pq.push(
            std::make_pair(UCT_nodes[child_id].real_score /
                               UCT_nodes[child_id].real_simulation_times,
                           child_id));
    }
  }

  return std::make_pair(
      std::make_pair(std::make_pair(real_score, real_score_square_sum),
                     real_simulation_times),
      std::make_pair(std::make_pair(RAVE_score, RAVE_score_square_sum),
                     RAVE_simulation_times));
}

int RAVE_moves[1024][128];
int moveNum[1024];
int RAVE_moves_from[1024][128], RAVE_moves_to[1024][128];

std::pair<double, int> MyAI::Simulate(ChessBoard chessboard, int color) {
  int moves[1024], chess[1024];
  int turn_color = color;
  double score;

  int simulate_depth = 0;
  for (;; simulate_depth++) {
    // Expand
    moveNum[simulate_depth] =
        Expand(chessboard.Board, turn_color, RAVE_moves[simulate_depth]);
    for (int i = 0; i < moveNum[simulate_depth]; i++) {
      RAVE_moves_to[simulate_depth][i] =
          chessboard.Board[RAVE_moves[simulate_depth][i] % 100];
      RAVE_moves_from[simulate_depth][i] =
          chessboard.Board[RAVE_moves[simulate_depth][i] / 100];
    }

    // Check if is finish
    if (isFinish(&chessboard, moveNum[simulate_depth])) {
      score = Evaluate(&chessboard, moveNum[simulate_depth], turn_color) *
              (color == this->Color ? 1 : -1);
      break;
    }

    // distinguish eat-move and pure-move
    int eatMove[128], eatMoveNum = 0;
    for (int i = 0; i < moveNum[simulate_depth]; ++i) {
      int dstPiece = chessboard.Board[RAVE_moves[simulate_depth][i] % 100];
      if (dstPiece != CHESS_EMPTY) {
        // eat-move
        eatMove[eatMoveNum] = RAVE_moves[simulate_depth][i];
        eatMoveNum++;
      }
    }

    // Random Move
    bool selectEat = (eatMoveNum == 0 ? false : randIndex(2));
    int move =
        (selectEat
             ? eatMove[randIndex(eatMoveNum)]
             : RAVE_moves[simulate_depth][randIndex(moveNum[simulate_depth])]);
    moves[simulate_depth] = move;
    chess[simulate_depth] =
        chessboard.Board[move / 100] * 100 + chessboard.Board[move % 100];
    MakeMove(&chessboard, move, 0);  // 0: dummy

    // Change color
    turn_color ^= 1;
  }

  int cnt = 1;
  for (int rev_depth = simulate_depth - 1; rev_depth >= 0; rev_depth--) {
    play[moves[rev_depth] / 100][moves[rev_depth] % 100]
        [chess[rev_depth] / 100][chess[rev_depth] % 100] = true;
    for (int i = 0; i < moveNum[rev_depth]; i++) {
      int mov = RAVE_moves[rev_depth][i];
      if (mov == moves[rev_depth]) continue;
      if (play[mov / 100][mov % 100][RAVE_moves_from[rev_depth][i]]
              [RAVE_moves_to[rev_depth][i]]) {
        cnt++;
      }
    }
  }

  for (int i = 0; i < simulate_depth; i++)
    play[moves[i] / 100][moves[i] % 100][chess[i] / 100][chess[i] % 100] =
        false;

  return std::make_pair(score * cnt, cnt);
}

bool MyAI::isDraw(const ChessBoard* chessboard) {
  // No Eat Flip
  if (chessboard->NoEatFlip >= NOEATFLIP_LIMIT) {
    return true;
  }

  // Position Repetition
  int last_idx = chessboard->HistoryCount - 1;
  // -2: my previous ply
  int idx = last_idx - 2;
  // All ply must be move type
  int smallest_repetition_idx =
      last_idx - (chessboard->NoEatFlip / POSITION_REPETITION_LIMIT);
  // check loop
  while (idx >= 0 && idx >= smallest_repetition_idx) {
    if (chessboard->History[idx] == chessboard->History[last_idx]) {
      // how much ply compose one repetition
      int repetition_size = last_idx - idx;
      bool isEqual = true;
      for (int i = 1; i < POSITION_REPETITION_LIMIT && isEqual; ++i) {
        for (int j = 0; j < repetition_size; ++j) {
          int src_idx = last_idx - j;
          int checked_idx = last_idx - i * repetition_size - j;
          if (chessboard->History[src_idx] !=
              chessboard->History[checked_idx]) {
            isEqual = false;
            break;
          }
        }
      }
      if (isEqual) {
        return true;
      }
    }
    idx -= 2;
  }

  return false;
}

bool MyAI::isFinish(const ChessBoard* chessboard, int move_count) {
  return (chessboard->Red_Chess_Num == 0 ||    // terminal node (no chess type)
          chessboard->Black_Chess_Num == 0 ||  // terminal node (no chess type)
          move_count == 0 ||                   // terminal node (no move type)
          isDraw(chessboard)                   // draw
  );
}

bool MyAI::isTimeUp() {
  double elapsed;  // ms

  // design for different os
#ifdef WINDOWS
  clock_t end = clock();
  elapsed = (end - begin);
#else
  struct timeval end;
  gettimeofday(&end, 0);
  long seconds = end.tv_sec - begin.tv_sec;
  long microseconds = end.tv_usec - begin.tv_usec;
  elapsed = (seconds * 1000 + microseconds * 1e-3);
#endif

  return elapsed >= TIME_LIMIT * 1000;
}

// return range: [0, max)
uint32_t MyAI::randIndex(uint32_t max) {
  return pcg32_boundedrand_r(&this->rng, max);
}

// Display chess board
void MyAI::Pirnf_Chessboard() {
  char Mes[1024] = "";
  char temp[1024];
  char myColor[10] = "";
  if (Color == -99)
    strcpy(myColor, "Unknown");
  else if (this->Color == RED)
    strcpy(myColor, "Red");
  else
    strcpy(myColor, "Black");
  sprintf(temp, "------------%s-------------\n", myColor);
  strcat(Mes, temp);
  strcat(Mes, "<8> ");
  for (int i = 0; i < 32; i++) {
    if (i != 0 && i % 4 == 0) {
      sprintf(temp, "\n<%d> ", 8 - (i / 4));
      strcat(Mes, temp);
    }
    char chess_name[4] = "";
    Pirnf_Chess(this->main_chessboard.Board[i], chess_name);
    sprintf(temp, "%5s", chess_name);
    strcat(Mes, temp);
  }
  strcat(Mes, "\n\n     ");
  for (int i = 0; i < 4; i++) {
    sprintf(temp, " <%c> ", 'a' + i);
    strcat(Mes, temp);
  }
  strcat(Mes, "\n\n");
  printf("%s", Mes);
}

// Print chess
void MyAI::Pirnf_Chess(int chess_no, char* Result) {
  // XX -> Empty
  if (chess_no == CHESS_EMPTY) {
    strcat(Result, " - ");
    return;
  }
  // OO -> DarkChess
  else if (chess_no == CHESS_COVER) {
    strcat(Result, " X ");
    return;
  }

  switch (chess_no) {
    case 0:
      strcat(Result, " P ");
      break;
    case 1:
      strcat(Result, " C ");
      break;
    case 2:
      strcat(Result, " N ");
      break;
    case 3:
      strcat(Result, " R ");
      break;
    case 4:
      strcat(Result, " M ");
      break;
    case 5:
      strcat(Result, " G ");
      break;
    case 6:
      strcat(Result, " K ");
      break;
    case 7:
      strcat(Result, " p ");
      break;
    case 8:
      strcat(Result, " c ");
      break;
    case 9:
      strcat(Result, " n ");
      break;
    case 10:
      strcat(Result, " r ");
      break;
    case 11:
      strcat(Result, " m ");
      break;
    case 12:
      strcat(Result, " g ");
      break;
    case 13:
      strcat(Result, " k ");
      break;
  }
}
