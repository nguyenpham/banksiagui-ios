/*
  Ethereal is a UCI chess playing engine authored by Andrew Grant.
  <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>

  Ethereal is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Ethereal is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attacks.h"
#include "board.h"
#include "cmdline.h"
#include "evaluate.h"
#include "pyrrhic/tbprobe.h"
#include "history.h"
#include "masks.h"
#include "move.h"
#include "movegen.h"
#include "network.h"
#include "search.h"
#include "thread.h"
#include "time.h"
#include "transposition.h"
#include "types.h"
#include "uci.h"
#include "zobrist.h"

#include "engineids.h"
void engine_message_c(int eid, const char* s);

//void ethereal_initialize();
//void ethereal_uci_cmd(const char* str);

extern int ContemptDrawPenalty;   // Defined by thread.c
extern int ContemptComplexity;    // Defined by thread.c
extern int MoveOverhead;          // Defined by time.c
extern unsigned TB_PROBE_DEPTH;   // Defined by syzygy.c
extern volatile int ABORT_SIGNAL; // Defined by search.c
extern volatile int IS_PONDERING; // Defined by search.c
extern volatile int ANALYSISMODE; // Defined by search.c
extern PKNetwork PKNN;            // Defined by network.c

pthread_mutex_t READYLOCK = PTHREAD_MUTEX_INITIALIZER;
const char *StartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


static Board board;
//char str[8192];
static Thread *threads;
static pthread_t pthreadsgo;
static UCIGoStruct uciGoStruct;

static int chess960 = 0;
static int multiPV  = 1;


//static char etherealMsgBuf[1024 * 32];
//static char* etherealMsgBufEnd = etherealMsgBuf;
////static std::mutex etherealMsgVecMutex;
//
//void ethereal_message(const char* str) {
////  std::lock_guard<std::mutex> lock(etherealMsgVecMutex);
////  etherealMsgVec.push_back(str);
////  sync_cout << str << sync_endl;
//  strcpy(etherealMsgBufEnd, str);
//  etherealMsgBufEnd += strlen(str) + 1;
//}
//
//const char* ethereal_getSearchMessage() {
//  if (etherealMsgBuf < etherealMsgBufEnd) {
////    std::lock_guard<std::mutex> lock(etherealMsgVecMutex);
//    if (etherealMsgBuf < etherealMsgBufEnd) {
//      static char tmpString[8192];
//      strcpy(tmpString, etherealMsgBuf);
//      unsigned long len = strlen(etherealMsgBuf) + 1;
//      if (len == etherealMsgBufEnd - etherealMsgBuf) {
//        etherealMsgBufEnd = etherealMsgBuf;
//      } else {
//        memcpy(etherealMsgBuf, etherealMsgBuf + len, (etherealMsgBufEnd - etherealMsgBuf) - len);
//        etherealMsgBufEnd -= len;
//      }
//      return tmpString;
//    }
//  }
//  return 0;
//}

void ethereal_initialize()
{
    // Initialize core components of Ethereal
    initAttacks(); initMasks(); initEval();
    initSearch(); initZobrist(); initTT(16);
    initPKNetwork(&PKNN);

    // Create the UCI-board and our threads
    threads = createThreadPool(1);
    boardFromFEN(&board, StartPosition, chess960);

//    // Handle any command line requests
//    handleCommandLine(argc, argv);
//
//    /*
//    |------------|-----------------------------------------------------------------------|
//    |  Commands  | Response. * denotes that the command blocks until no longer searching |
//    |------------|-----------------------------------------------------------------------|
//    |        uci |           Outputs the engine name, authors, and all available options |
//    |    isready | *           Responds with readyok when no longer searching a position |
//    | ucinewgame | *  Resets the TT and any Hueristics to ensure determinism in searches |
//    |  setoption | *     Sets a given option and reports that the option was set if done |
//    |   position | *  Sets the board position via an optional FEN and optional move list |
//    |         go | *       Searches the current position with the provided time controls |
//    |  ponderhit |          Flags the search to indicate that the ponder move was played |
//    |       stop |            Signals the search threads to finish and report a bestmove |
//    |       quit |             Exits the engine and any searches by killing the UCI loop |
//    |      perft |            Custom command to compute PERFT(N) of the current position |
//    |      print |         Custom command to print an ASCII view of the current position |
//    |------------|-----------------------------------------------------------------------|
//    */

  printf("ethereal_initialize called\n");
}

void ethereal_uci_cmd(const char* str) {
  printf("ethereal_uci_cmd %s\n", str);
    if (strEquals(str, "uci")) {
      engine_message_c(ethereal, "id name Ethereal " ETHEREAL_VERSION "");
      engine_message_c(ethereal, "id author Andrew Grant, Alayan & Laldon");
      engine_message_c(ethereal, "option name Hash type spin default 16 min 2 max 131072");
      engine_message_c(ethereal, "option name Threads type spin default 1 min 1 max 2048");
      engine_message_c(ethereal, "option name MultiPV type spin default 1 min 1 max 256");
      engine_message_c(ethereal, "option name ContemptDrawPenalty type spin default 0 min -300 max 300");
      engine_message_c(ethereal, "option name ContemptComplexity type spin default 0 min -100 max 100");
      engine_message_c(ethereal, "option name MoveOverhead type spin default 100 min 0 max 10000");
      engine_message_c(ethereal, "option name SyzygyPath type string default <empty>");
      engine_message_c(ethereal, "option name SyzygyProbeDepth type spin default 0 min 0 max 127");
      engine_message_c(ethereal, "option name Ponder type check default false");
      engine_message_c(ethereal, "option name AnalysisMode type check default false");
      engine_message_c(ethereal, "option name UCI_Chess960 type check default false");
      engine_message_c(ethereal, "uciok");
      //, fflush(stdout);
    }

    else if (strEquals(str, "isready")) {
        pthread_mutex_lock(&READYLOCK);
      engine_message_c(ethereal, "readyok"); //, fflush(stdout);
        pthread_mutex_unlock(&READYLOCK);
    }

    else if (strEquals(str, "ucinewgame")) {
        pthread_mutex_lock(&READYLOCK);
        resetThreadPool(threads), clearTT();
        pthread_mutex_unlock(&READYLOCK);
    }

    else if (strStartsWith(str, "setoption")) {
        pthread_mutex_lock(&READYLOCK);
        uciSetOption(str, &threads, &multiPV, &chess960);
        pthread_mutex_unlock(&READYLOCK);
    }

    else if (strStartsWith(str, "position")) {
        pthread_mutex_lock(&READYLOCK);
        uciPosition(str, &board, chess960);
        pthread_mutex_unlock(&READYLOCK);
    }

    else if (strStartsWith(str, "go")) {
        pthread_mutex_lock(&READYLOCK);
        uciGoStruct.multiPV = multiPV;
        uciGoStruct.board   = &board;
        uciGoStruct.threads = threads;
        strncpy(uciGoStruct.str, str, 512);
        pthread_create(&pthreadsgo, NULL, &uciGo, &uciGoStruct);
        pthread_detach(pthreadsgo);
      printf("Ethereal go\n");
    }

    else if (strEquals(str, "ponderhit"))
        IS_PONDERING = 0;

    else if (strEquals(str, "stop"))
        ABORT_SIGNAL = 1, IS_PONDERING = 0;

    else if (strEquals(str, "quit"))
        return;

    else if (strStartsWith(str, "perft"))
        printf("%"PRIu64"\n", ethereal_perft(&board, atoi(str + strlen("perft ")))), fflush(stdout);

    else if (strStartsWith(str, "print"))
        printBoard(&board), fflush(stdout);
}

void *uciGo(void *cargo) {
  printf("Ethereal uciGo\n");

    // Get our starting time as soon as possible
    double start = getRealTime();

    Limits limits = {0};
    uint16_t bestMove, ponderMove;
    char moveStr[6];

    int depth = 0, infinite = 0;
    double wtime = 0, btime = 0, movetime = 0;
    double winc = 0, binc = 0, mtg = -1;

    int multiPV     = ((UCIGoStruct*)cargo)->multiPV;
    char *str       = ((UCIGoStruct*)cargo)->str;
    Board *board    = ((UCIGoStruct*)cargo)->board;
    Thread *threads = ((UCIGoStruct*)cargo)->threads;

    uint16_t moves[MAX_MOVES];
    int size = genAllLegalMoves(board, moves);
    int idx = 0, searchmoves = 0;

    // Reset global signals
    IS_PONDERING = 0;

    // Init the tokenizer with spaces
    char* ptr = strtok(str, " ");

    // Parse any time control and search method information that was sent
    for (ptr = strtok(NULL, " "); ptr != NULL; ptr = strtok(NULL, " ")) {

        if (strEquals(ptr, "wtime"      )) wtime    = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "btime"      )) btime    = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "winc"       )) winc     = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "binc"       )) binc     = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "movestogo"  )) mtg      = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "depth"      )) depth    = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "movetime"   )) movetime = atoi(strtok(NULL, " "));

        if (strEquals(ptr, "infinite"   )) infinite = 1;
        if (strEquals(ptr, "searchmoves")) searchmoves = 1;
        if (strEquals(ptr, "ponder"     )) IS_PONDERING = 1;

        for (int i = 0; i < size; i++) {
            moveToString(moves[i], moveStr, board->chess960);
            if (strEquals(ptr, moveStr)) limits.searchMoves[idx++] = moves[i];
        }
    }

    // Initialize limits for the search
    limits.limitedByNone  = infinite != 0;
    limits.limitedByTime  = movetime != 0;
    limits.limitedByDepth = depth    != 0;
    limits.limitedBySelf  = !depth && !movetime && !infinite;
    limits.limitedByMoves = searchmoves;
    limits.timeLimit      = movetime;
    limits.depthLimit     = depth;

    // Pick the time values for the colour we are playing as
    limits.start = (board->turn == WHITE) ? start : start;
    limits.time  = (board->turn == WHITE) ? wtime : btime;
    limits.inc   = (board->turn == WHITE) ?  winc :  binc;
    limits.mtg   = (board->turn == WHITE) ?   mtg :   mtg;

    // Cap our MultiPV search based on the suggested or legal moves
    limits.multiPV = MIN(multiPV, searchmoves ? idx : size);

    // Execute search, return best and ponder moves
    getBestMove(threads, board, &limits, &bestMove, &ponderMove);

    // UCI spec does not want reports until out of pondering
    while (IS_PONDERING);

    // Report best move ( we should always have one )
    moveToString(bestMove, moveStr, board->chess960);
  
    char buf[128];
    sprintf(buf, "bestmove %s ", moveStr);

    // Report ponder move ( if we have one )
    if (ponderMove != NONE_MOVE) {
        moveToString(ponderMove, moveStr, board->chess960);
      sprintf(buf + strlen(buf), "ponder %s", moveStr);
    }

    // Make sure this all gets reported
    //printf("\n"); fflush(stdout);
    engine_message_c(ethereal, buf);

    // Drop the ready lock, as we are prepared to handle a new search
    pthread_mutex_unlock(&READYLOCK);

    return NULL;
}

void uciSetOption(const char *str, Thread **threads, int *multiPV, int *chess960) {

    // Handle setting UCI options in Ethereal. Options include:
    //  Hash                : Size of the Transposition Table in Megabyes
    //  Threads             : Number of search threads to use
    //  MultiPV             : Number of search lines to report per iteration
    //  ContemptDrawPenalty : Evaluation bonus in internal units to avoid forced draws
    //  ContemptComplexity  : Evaluation bonus for keeping a position with more non-pawn material
    //  MoveOverhead        : Overhead on time allocation to avoid time losses
    //  SyzygyPath          : Path to Syzygy Tablebases
    //  SyzygyProbeDepth    : Minimal Depth to probe the highest cardinality Tablebase
    //  UCI_Chess960        : Set when playing FRC, but not required in order to work

  char buf[300];
    if (strStartsWith(str, "setoption name Hash value ")) {
        int megabytes = atoi(str + strlen("setoption name Hash value "));
        initTT(megabytes); printf("info string set Hash to %dMB\n", hashSizeMBTT());
    }

    if (strStartsWith(str, "setoption name Threads value ")) {
        int nthreads = atoi(str + strlen("setoption name Threads value "));
        free(*threads); *threads = createThreadPool(nthreads);
        sprintf(buf, "info string set Threads to %d\n", nthreads);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name MultiPV value ")) {
        *multiPV = atoi(str + strlen("setoption name MultiPV value "));
        printf("info string set MultiPV to %d\n", *multiPV);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name ContemptDrawPenalty value ")){
        ContemptDrawPenalty = atoi(str + strlen("setoption name ContemptDrawPenalty value "));
      sprintf(buf, "info string set ContemptDrawPenalty to %d\n", ContemptDrawPenalty);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name ContemptComplexity value ")){
        ContemptComplexity = atoi(str + strlen("setoption name ContemptComplexity value "));
      sprintf(buf, "info string set ContemptComplexity to %d\n", ContemptComplexity);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name MoveOverhead value ")) {
        MoveOverhead = atoi(str + strlen("setoption name MoveOverhead value "));
      sprintf(buf, "info string set MoveOverhead to %d\n", MoveOverhead);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name SyzygyPath value ")) {
        char *ptr = str + strlen("setoption name SyzygyPath value ");
        tb_init(ptr);
      sprintf(buf, "info string set SyzygyPath to %s\n", ptr);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name SyzygyProbeDepth value ")) {
        TB_PROBE_DEPTH = atoi(str + strlen("setoption name SyzygyProbeDepth value "));
      sprintf(buf, "info string set SyzygyProbeDepth to %u\n", TB_PROBE_DEPTH);
      engine_message_c(ethereal, buf);
    }

    if (strStartsWith(str, "setoption name AnalysisMode value ")) {
      if (strStartsWith(str, "setoption name AnalysisMode value true")) {
        sprintf(buf, "info string set AnalysisMode to true\n"), ANALYSISMODE = 1;
      engine_message_c(ethereal, buf);
      }
      if (strStartsWith(str, "setoption name AnalysisMode value false")) {
        sprintf(buf, "info string set AnalysisMode to false\n"), ANALYSISMODE = 0;
        engine_message_c(ethereal, buf);
      }
    }

    if (strStartsWith(str, "setoption name UCI_Chess960 value ")) {
      if (strStartsWith(str, "setoption name UCI_Chess960 value true")) {
        sprintf(buf, "info string set UCI_Chess960 to true\n"), *chess960 = 1;
        engine_message_c(ethereal, buf);
      }
      if (strStartsWith(str, "setoption name UCI_Chess960 value false")) {
        sprintf(buf, "info string set UCI_Chess960 to false\n"), *chess960 = 0;
        engine_message_c(ethereal, buf);
      }
    }

    fflush(stdout);
}

void uciPosition(const char *str, Board *board, int chess960) {

    int size;
    uint16_t moves[MAX_MOVES];
    char *ptr, moveStr[6],testStr[6];
    Undo undo[1];

    // Position is defined by a FEN, X-FEN or Shredder-FEN
    if (strContains(str, "fen"))
        boardFromFEN(board, strstr(str, "fen") + strlen("fen "), chess960);

    // Position is simply the usual starting position
    else if (strContains(str, "startpos"))
        boardFromFEN(board, StartPosition, chess960);

    // Position command may include a list of moves
    ptr = strstr(str, "moves");
    if (ptr != NULL)
        ptr += strlen("moves ");

    // Apply each move in the move list
    while (ptr != NULL && *ptr != '\0') {

        // UCI sends moves in long algebraic notation
        for (int i = 0; i < 4; i++) moveStr[i] = *ptr++;
        moveStr[4] = *ptr == '\0' || *ptr == ' ' ? '\0' : *ptr++;
        moveStr[5] = '\0';

        // Generate moves for this position
        size = genAllLegalMoves(board, moves);

        // Find and apply the given move
        for (int i = 0; i < size; i++) {
            moveToString(moves[i], testStr, board->chess960);
            if (strEquals(moveStr, testStr)) {
                applyMove(board, moves[i], undo);
                break;
            }
        }

        // Reset move history whenever we reset the fifty move rule. This way
        // we can track all positions that are candidates for repetitions, and
        // are still able to use a fixed size for the history array (512)
        if (board->halfMoveCounter == 0)
            board->numMoves = 0;

        // Skip over all white space
        while (*ptr == ' ') ptr++;
    }
}

void uciReport(Thread *threads, int alpha, int beta, int value) {

    // Gather all of the statistics that the UCI protocol would be
    // interested in. Also, bound the value passed by alpha and
    // beta, since Ethereal uses a mix of fail-hard and fail-soft

    int hashfull    = hashfullTT();
    int depth       = threads->depth;
    int seldepth    = threads->seldepth;
    int multiPV     = threads->multiPV + 1;
    int elapsed     = elapsedTime(threads->info);
    int bounded     = MAX(alpha, MIN(value, beta));
    uint64_t nodes  = nodesSearchedThreadPool(threads);
    uint64_t tbhits = tbhitsThreadPool(threads);
    int nps         = (int)(1000 * (nodes / (1 + elapsed)));

    // If the score is MATE or MATED in X, convert to X
    int score   = bounded >=  MATE_IN_MAX ?  (MATE - bounded + 1) / 2
                : bounded <= -MATE_IN_MAX ? -(bounded + MATE)     / 2 : bounded;

    // Two possible score types, mate and cp = centipawns
    const char *type  = abs(bounded) >= MATE_IN_MAX ? "mate" : "cp";

    // Partial results from a windowed search have bounds
  const char *bound = bounded >=  beta ? " lowerbound "
                : bounded <= alpha ? " upperbound " : " ";

  char buf[256];
  sprintf(buf, "info depth %d seldepth %d multipv %d score %s %d%stime %d "
           "nodes %"PRIu64" nps %d tbhits %"PRIu64" hashfull %d pv ",
           depth, seldepth, multiPV, type, score, bound, elapsed, nodes, nps, tbhits, hashfull);

    // Iterate over the PV and print each move
    for (int i = 0; i < threads->pv.length; i++) {
        char moveStr[6];
        moveToString(threads->pv.line[i], moveStr, threads->board.chess960);
      sprintf(buf + strlen(buf), "%s ", moveStr);
    }

    // Send out a newline and flush
//    puts(""); fflush(stdout);
  engine_message_c(ethereal, buf);
}

void uciReportCurrentMove(Board *board, uint16_t move, int currmove, int depth) {

    char moveStr[6];
  char buf[256];
    moveToString(move, moveStr, board->chess960);
    sprintf(buf, "info depth %d currmove %s currmovenumber %d\n", depth, moveStr, currmove);
//    fflush(stdout);
  engine_message_c(ethereal, buf);

}

int strEquals(const char *str1, const char *str2) {
    return strcmp(str1, str2) == 0;
}

int strStartsWith(const  char *str, const char *key) {
    return strstr(str, key) == str;
}

int strContains(const char *str, const char *key) {
    return strstr(str, key) != NULL;
}

int getInput(char *str) {

    char *ptr;

    if (fgets(str, 8192, stdin) == NULL)
        return 0;

    ptr = strchr(str, '\n');
    if (ptr != NULL) *ptr = '\0';

    ptr = strchr(str, '\r');
    if (ptr != NULL) *ptr = '\0';

    return 1;
}
