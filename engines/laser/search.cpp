/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

    Laser is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Laser is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Laser.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include "eval.h"
#include "hash.h"
#include "search.h"
#include "moveorder.h"
#include "searchparams.h"
#include "timeman.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

#include "engineids.h"
void engine_message(int eid, const std::string& s);

using std::cout;
using std::cerr;
using std::endl;


// Records search statistics required by the UCI protocol
struct SearchStatistics {
    uint64_t nodes;
    uint64_t tbhits;

    SearchStatistics() {
        reset();
    }

    void reset() {
        nodes = 0;
        tbhits = 0;
    }
};

// Records the PV found by the search.
struct SearchPV {
    int pvLength;
    Move pv[MAX_DEPTH+1];

    SearchPV() {
        pvLength = 0;
    }
};

// Stores all of the per-thread search structs.
struct ThreadMemory {
    SearchParameters searchParams;
    SearchStatistics searchStats;
    SearchStackInfo ssInfo[129];
    TwoFoldStack twoFoldPositions;

    ThreadMemory() {
        for (int i = 0; i < 129; i++)
            ssInfo[i].ply = i;
    }

    ~ThreadMemory() = default;
};

//-------------------------------Search Constants-------------------------------
constexpr int SMP_SKIP_DEPTHS[16] = {
    1, 2, 2, 4, 4, 3, 2, 5, 4, 3, 2, 6, 5, 4, 3, 2
};
constexpr int SMP_SKIP_AMOUNT[16] = {
    1, 1, 1, 2, 2, 2, 1, 3, 2, 2, 1, 3, 3, 2, 2, 1
};

// Razor margins indexed by depth. If static eval is far below alpha, use a
// qsearch to confirm fail low and then return.
constexpr int RAZOR_MARGIN = 300;

// Move count pruning
constexpr unsigned int LMP_MOVE_COUNTS[2][13] = {
    {0, 2, 4,  7, 11, 16, 22, 29, 37, 46,  56,  67,  79},
    {0, 4, 7, 12, 20, 30, 42, 56, 73, 92, 113, 136, 161}
};

// LMR reduction values, initialized at program start
int lmrReductions[64][64];

void initReductionTable() {
    for (int depth = 1; depth < 64; depth++) {
        for (int movesSearched = 1; movesSearched < 64; movesSearched++)
            lmrReductions[depth][movesSearched] = (int) (0.5 + log(depth) * log(movesSearched) / 2.1);
    }
    // Set reductions for depth 1 and movesSearched >= 7 to 1. This only affects
    // lmrDepth-based pruning.
    for (int movesSearched = 7; movesSearched < 64; movesSearched++)
        lmrReductions[1][movesSearched] = 1;
}


//-----------------------------Global variables---------------------------------
static Hash transpositionTable(DEFAULT_HASH_SIZE);
static std::vector<ThreadMemory *> threadMemoryArray;

// Variables for time management
ChessTime startTime;
uint64_t timeLimit;

// Used to break out of the search thread if the stop command is given
std::atomic<bool> isStop(true);
// Additional stop signal to stop helper threads during SMP
std::atomic<bool> stopSignal(true);

// Dummy variables for lazy SMP since we don't care about these results
int dummyBestIndex[MAX_THREADS-1];
int dummyBestScore[MAX_THREADS-1];

// Values for UCI options
unsigned int multiPV;
int numThreads;
bool isPonderSearch = false;

// Accessible from tbcore.c
int Laser_TBlargest = 0;
static int probeLimit = 0;


// Search functions
void getBestMove(const Board *b, TimeManagement *timeParams, MoveList legalMoves,
    int tbScore, bool tbProbeSuccess, int threadID);
void getBestMoveAtDepth(const Board *b, const MoveList *legalMoves, int depth, int alpha, int beta,
    int *bestMoveIndex, int *bestScore, unsigned int startMove, int threadID, SearchPV *pvLine);
int PVS(Board &b, int depth, int alpha, int beta, int threadID, bool isCutNode, SearchStackInfo *ssi, SearchPV *pvLine);
int quiescence(Board &b, int plies, int alpha, int beta, int threadID);
int checkQuiescence(Board &b, int plies, int alpha, int beta, int threadID);

// Search helpers
int scoreMate(bool isInCheck, int plies);
int adjustHashScore(int score, int plies);

// Other utility functions
Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index);
uint64_t getTBHits();
void changePV(Move best, SearchPV *parent, SearchPV *child);
std::string retrievePV(SearchPV *pvLine);
int getSelectiveDepth();
double getPercentage(uint64_t numerator, uint64_t denominator);


// Spawns the appropriate number of getBestMove threads and cleans up the helpers
// when the main thread is done.
void getBestMoveThreader(const Board *b, TimeManagement *timeParams, MoveList *movesToSearch) {
    const int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);

    // Special case if we are given a mate/stalemate position
    if (legalMoves.size() <= 0) {
        stopSignal = true;
        isStop = true;
//        cout << "bestmove none" << endl;
      engine_message(laser, "bestmove none");
        return;
    }

    // Reset all search parameters (killers, plies, etc)
    for (int i = 0; i < numThreads; i++) {
        threadMemoryArray[i]->searchParams.reset();
        threadMemoryArray[i]->searchStats.reset();
        threadMemoryArray[i]->searchParams.selectiveDepth = 0;
    }

    // Copy over the two-fold stack to use
    for (int i = 1; i < numThreads; i++)
        threadMemoryArray[i]->twoFoldPositions = threadMemoryArray[0]->twoFoldPositions;


    // Root probe Syzygy
    probeLimit = Laser_TBlargest;
    int tbScore = 0;
    bool tbProbeSuccess = false;
    unsigned int prevLMSize = legalMoves.size();
    if (Laser_TBlargest && count(b->getAllPieces(WHITE) | b->getAllPieces(BLACK)) <= Laser_TBlargest) {
        ScoreList scores;
        // Try probing with DTZ tables first
        int tbProbeResult = root_probe(b, legalMoves, scores, tbScore);
        if (tbProbeResult) {
            // With DTZ table filtering, we have guaranteed that we will not
            // make a mistake so do not probe TBs in search
            probeLimit = 0;
            tbProbeSuccess = true;
            threadMemoryArray[0]->searchStats.tbhits += prevLMSize;
        }
        // If unsuccessful, try WDL tables
        else {
            scores.clear();
            tbScore = 0;
            tbProbeResult = root_probe_wdl(b, legalMoves, scores, tbScore);
            if (tbProbeResult) {
                tbProbeSuccess = true;
                threadMemoryArray[0]->searchStats.tbhits += prevLMSize;
                // Only probe to maintain a win
                if (tbScore <= 0)
                    probeLimit = 0;
            }
        }
    }


    // If we were told to search specific moves, filter them here
    if (movesToSearch->size() > 0) {
        MoveList temp;
        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            for (unsigned int j = 0; j < movesToSearch->size(); j++) {
                if (legalMoves.get(i) == movesToSearch->get(j))
                    temp.add(legalMoves.get(i));
            }
        }
        legalMoves = temp;
    }


    // Set up timing
    timeLimit = (timeParams->searchMode == TIME) ? timeParams->maxAllotment
                                                 : (timeParams->searchMode == MOVETIME) ? timeParams->allotment
                                                                                        : MAX_TIME;
    startTime = ChessClock::now();

    // Special case if there is only one legal move: use less search time,
    // only to get a rough PV/score
    if (legalMoves.size() == 1 && timeParams->searchMode == TIME) {
        timeLimit = std::min(timeLimit / 32, ONE_SECOND);
    }

    // Increment hash table age
    transpositionTable.incrementAge();


    // Create threads for SMP if necessary
    if (numThreads > 1) {
        std::thread *threadPool = new std::thread[numThreads];

        // Start and join all threads
        for (int i = 0; i < numThreads; i++) {
            threadPool[i] = std::thread(getBestMove, b, timeParams, legalMoves, tbScore, tbProbeSuccess, i);
        }
        for (int i = 0; i < numThreads; i++) {
            threadPool[i].join();
        }

        stopSignal = false;
        delete[] threadPool;
    }
    // Otherwise, just search with one thread
    else {
        getBestMove(b, timeParams, legalMoves, tbScore, tbProbeSuccess, 0);
    }
}

// Finds a best move for a position according to the given search parameters.
void getBestMove(const Board *b, TimeManagement *timeParams, MoveList legalMoves,
        int tbScore, bool tbProbeSuccess, int threadID) {
    Move ponder = NULL_MOVE;
    Move bestMove = legalMoves.get(0);
    uint64_t timeSoFar;

    int bestScore = -INFTY, bestMoveIndex = -1;
    int rootDepth = 1;
    Move prevBest = NULL_MOVE;
    int prevScore = -INFTY;
    int pvStreak = 0;
    double timeChangeFactor = 1.0;

    // Iterative deepening loop
    do {
        // For recording the PV
        SearchPV pvLine;
        // Decay time change factor
        timeChangeFactor = (2 * timeChangeFactor + 1) / 3;

        // Handle multi PV (if multiPV = 1 then the loop is only run once)
        for (unsigned int multiPVNum = 1;
                          multiPVNum <= multiPV
                            && multiPVNum <= legalMoves.size();
                          multiPVNum++) {
            // Initial aspiration window based on depth and score
            int deltaAlpha = 14 - std::min(rootDepth/4, 6) + abs(bestScore) / 25;
            int deltaBeta = deltaAlpha;

            // Set up aspiration windows
            int aspAlpha = -MATE_SCORE;
            int aspBeta = MATE_SCORE;
            if (rootDepth >= 6 && multiPV == 1 && abs(bestScore) < NEAR_MATE_SCORE) {
                aspAlpha = bestScore - deltaAlpha;
                aspBeta = bestScore + deltaBeta;
            }
            prevScore = bestScore;

            // Aspiration loop
            while (!isStop) {
                // Reset search parameters such as killers
                threadMemoryArray[threadID]->searchParams.reset();
                pvLine.pvLength = 0;

                // Get the index of the best move
                getBestMoveAtDepth(b, &legalMoves, rootDepth, aspAlpha, aspBeta,
                    &bestMoveIndex, &bestScore, multiPVNum-1, threadID, &pvLine);

                timeSoFar = getTimeElapsed(startTime);
                // Calculate values for printing
                uint64_t nps = 1000 * getNodes() / timeSoFar;
                if (pvLine.pvLength > 1)
                    ponder = pvLine.pv[1];
                else if (bestMoveIndex != 0)
                    ponder = NULL_MOVE;

                // Handle fail highs and fail lows
                // Fail low: no best move found
                if (bestMoveIndex == -1 && !isStop) {
                    if (threadID == 0) {
//                        cout << "info depth " << rootDepth;
//                        cout << " seldepth " << getSelectiveDepth();
//                        cout << " score";
//                        cout << " cp " << (tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS]
//                             << " upperbound";
//                        cout << " time " << timeSoFar
//                             << " nodes " << getNodes() << " nps " << nps
//                             << " tbhits " << getTBHits()
//                             << " hashfull " << transpositionTable.estimateHashfull()
//                             << " pv " << retrievePV(&pvLine) << endl;
                      
                      std::string s =
                      "info depth " + std::to_string(rootDepth) +
                      " seldepth " + std::to_string(getSelectiveDepth()) +
                      " score cp "  + std::to_string((tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS]) +
                      " upperbound" +
                      " time " + std::to_string(timeSoFar) +
                      " nodes " + std::to_string(getNodes()) + " nps " + std::to_string(nps) +
                      " tbhits " + std::to_string(getTBHits()) +
                      " hashfull " + std::to_string(transpositionTable.estimateHashfull()) +
                      " pv " + retrievePV(&pvLine);
                      engine_message(laser, s);
                    }

                    aspBeta = (aspAlpha + aspBeta) / 2;
                    aspAlpha = bestScore - deltaAlpha;
                    deltaAlpha = 3 * deltaAlpha / 2;
                    if (aspAlpha < -NEAR_MATE_SCORE)
                        aspAlpha = -MATE_SCORE;
                    timeChangeFactor *= 1.15;
                }
                // Fail high: best score is at least beta
                else if (bestScore >= aspBeta) {
                    if (threadID == 0) {
//                        cout << "info depth " << rootDepth;
//                        cout << " seldepth " << getSelectiveDepth();
//                        cout << " score";
//                        cout << " cp " << (tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS]
//                             << " lowerbound";
//                        cout << " time " << timeSoFar
//                             << " nodes " << getNodes() << " nps " << nps
//                             << " tbhits " << getTBHits()
//                             << " hashfull " << transpositionTable.estimateHashfull()
//                             << " pv " << retrievePV(&pvLine) << endl;
                      
                      std::string s =
                      "info depth " + std::to_string(rootDepth) +
                      " seldepth " + std::to_string(getSelectiveDepth()) +
                      " score" +
                      " cp " + std::to_string((tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS]) +
                      " lowerbound" +
                      " time " + std::to_string(timeSoFar) +
                       " nodes " + std::to_string(getNodes()) + " nps " + std::to_string(nps) +
                       " tbhits " + std::to_string(getTBHits()) +
                       " hashfull " + std::to_string(transpositionTable.estimateHashfull()) +
                       " pv " + retrievePV(&pvLine);
                      engine_message(laser, s);
                    }

                    aspAlpha = (aspAlpha + aspBeta) / 2;
                    aspBeta = bestScore + deltaBeta;
                    deltaBeta = 3 * deltaBeta / 2;
                    if (aspBeta > NEAR_MATE_SCORE)
                        aspBeta = MATE_SCORE;

                    // If the best move is still the same, do not necessarily
                    // resolve the fail high
                    if (threadID == 0
                     && (int) multiPVNum - 1 == bestMoveIndex
                     && bestMove == prevBest
                     && timeParams->searchMode == TIME
                     && (timeSoFar >= timeParams->allotment * TIME_FACTOR))
                        break;

                    legalMoves.swap(multiPVNum-1, bestMoveIndex);
                    bestMove = legalMoves.get(0);
                }
                // If no fails, we are done
                else break;
            }
            // End aspiration loop

            // Calculate values for printing
            timeSoFar = getTimeElapsed(startTime);
            uint64_t nps = 1000 * getNodes() / timeSoFar;

            // If we broke out before getting any new results, end the search
            if (bestMoveIndex == -1) {
                if (threadID == 0) {
//                    cout << "info depth " << rootDepth-1;
//                    cout << " seldepth " << getSelectiveDepth();
//                    cout << " time " << timeSoFar
//                         << " nodes " << getNodes() << " nps " << nps
//                         << " tbhits " << getTBHits()
//                         << " hashfull " << transpositionTable.estimateHashfull()
//                         << endl;

                  std::string s =
                  "info depth " + std::to_string(rootDepth-1) +
                  " seldepth " + std::to_string(getSelectiveDepth()) +
                  " time " + std::to_string(timeSoFar) +
                  " nodes " + std::to_string(getNodes()) + " nps " + std::to_string(nps) +
                  " tbhits " + std::to_string(getTBHits()) +
                  " hashfull " + std::to_string(transpositionTable.estimateHashfull());
                  engine_message(laser, s);
                }
                break;
            }

            // Swap the PV to be searched first next iteration
            legalMoves.swap(multiPVNum-1, bestMoveIndex);
            bestMove = legalMoves.get(0);

            // Sort the remaining moves, using a simplified version of
            // move ordering from moveorder.cpp
            const int color = b->getPlayerToMove();
            ScoreList scores;
            for (unsigned int i = 0; i < legalMoves.size(); i++) {
                if (i < multiPVNum) {
                    scores.add(0);
                    continue;
                }

                Move m = legalMoves.get(i);
                if (isCapture(m)) {
                    int startSq = getStartSq(m);
                    int endSq = getEndSq(m);
                    int pieceID = b->getPieceOnSquare(color, startSq);
                    int capturedID = b->getPieceOnSquare(color, endSq);
                    if (capturedID == -1) capturedID = 0;

                    int adjustedMVVLVA = 8 * b->getMVVLVAScore(color, m) / (4 + rootDepth);
                    scores.add(16384 + adjustedMVVLVA
                        + threadMemoryArray[threadID]->searchParams.captureHistory[color][pieceID][capturedID][endSq]);
                }
                else if (getPromotion(m) == QUEENS)
                    scores.add(2048);
                else {
                    int startSq = getStartSq(m);
                    int endSq = getEndSq(m);
                    int pieceID = b->getPieceOnSquare(color, startSq);

                    scores.add(threadMemoryArray[threadID]->searchParams.historyTable[color][pieceID][endSq]);
                }
            }
            for (unsigned int i = multiPVNum; i < legalMoves.size(); i++) {
                nextMove(legalMoves, scores, i);
            }

            // Output info using UCI protocol
            if (threadID == 0) {
//                cout << "info depth " << rootDepth;
//                cout << " seldepth " << getSelectiveDepth();
//                if (multiPV > 1)
//                    cout << " multipv " << multiPVNum;
//                cout << " score";
//
//                // Print score in mate or centipawns
//                if (bestScore >= MAX_PLY_MATE_SCORE)
//                    // If it is our mate, it takes plies / 2 + 1 moves to mate since
//                    // our move ends the game
//                    cout << " mate " << (MATE_SCORE - bestScore) / 2 + 1;
//                else if (bestScore <= -MAX_PLY_MATE_SCORE)
//                    // If we are being mated, it takes plies / 2 moves since our
//                    // opponent's move ends the game
//                    cout << " mate " << (-MATE_SCORE - bestScore) / 2;
//                else
//                    // Scale score into centipawns using our internal pawn value
//                    cout << " cp " << (tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS];
//
//                cout << " time " << timeSoFar
//                     << " nodes " << getNodes() << " nps " << nps
//                     << " tbhits " << getTBHits()
//                     << " hashfull " << transpositionTable.estimateHashfull()
//                     << " pv " << retrievePV(&pvLine) << endl;
              
              std::string s =
              "info depth " + std::to_string(rootDepth) +
              " seldepth " + std::to_string(getSelectiveDepth());
              if (multiPV > 1)
                  s += " multipv " + std::to_string(multiPVNum);
              s += " score";

              // Print score in mate or centipawns
              if (bestScore >= MAX_PLY_MATE_SCORE)
                  // If it is our mate, it takes plies / 2 + 1 moves to mate since
                  // our move ends the game
                                                    s += " mate " + std::to_string((MATE_SCORE - bestScore) / 2 + 1);
              else if (bestScore <= -MAX_PLY_MATE_SCORE)
                  // If we are being mated, it takes plies / 2 moves since our
                  // opponent's move ends the game
                                                    s += " mate " + std::to_string((-MATE_SCORE - bestScore) / 2);
              else
                  // Scale score into centipawns using our internal pawn value
                                                    s += " cp " + std::to_string((tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS]);

              s += " time " + std::to_string(timeSoFar) +
                   " nodes " + std::to_string(getNodes()) + " nps " + std::to_string(nps) +
                   " tbhits " + std::to_string(getTBHits()) +
                   " hashfull " + std::to_string(transpositionTable.estimateHashfull()) +
                   " pv " + retrievePV(&pvLine);
              engine_message(laser, s);
            }
        }
        // End multiPV loop

        if (bestMove == prevBest) {
            pvStreak++;
            timeChangeFactor *= 0.92;
        }
        else {
            prevBest = bestMove;
            pvStreak = 1;
            if (timeChangeFactor < 1.0) timeChangeFactor = 1.0;
            timeChangeFactor *= 1.3;
        }

        // Adjust search time based on PV and score behavior
        if (threadID == 0 && timeParams->searchMode == TIME)
            timeChangeFactor *= 0.92 + std::min(7.0, sqrt(abs(prevScore - bestScore))) / 25.0;
        else
            timeChangeFactor = 1.0;

        // Easymove confirmation
        if (threadID == 0 && !isPonderSearch && timeParams->searchMode == TIME && multiPV == 1
         && pvStreak >= 8 + rootDepth / 5
         && timeSoFar > (uint64_t) timeParams->allotment / 16
         && timeSoFar < (uint64_t) timeParams->allotment / 2
         && abs(bestScore) < NEAR_MATE_SCORE) {
            int secondBestMove;
            int secondBestScore;
            int easymoveWindow = bestScore - EASYMOVE_MARGIN - rootDepth - abs(bestScore) / 3;
            int easymoveDepth = rootDepth - 4 - rootDepth / 8;

            getBestMoveAtDepth(b, &legalMoves, easymoveDepth, easymoveWindow - 1, easymoveWindow,
                &secondBestMove, &secondBestScore, 1, 0, &pvLine);

            if (secondBestScore < easymoveWindow)
                break;
            else
                pvStreak = -128;
        }

        rootDepth++;
        if (threadID != 0) {
            int threadCycle = threadID % 16;
            if ((rootDepth + threadCycle) % SMP_SKIP_DEPTHS[threadCycle] == 0)
                rootDepth += SMP_SKIP_AMOUNT[threadCycle];
        }
    }
    // Conditions for iterative deepening loop
    while (!isStop
         && ((threadID != 0 && rootDepth <= MAX_DEPTH)
          || ((((timeParams->searchMode == TIME && timeSoFar < (uint64_t) timeParams->allotment * TIME_FACTOR * timeChangeFactor)
              || isPonderSearch) && rootDepth <= MAX_DEPTH)
           || (timeParams->searchMode == MOVETIME && timeSoFar < (uint64_t) timeParams->allotment && rootDepth <= MAX_DEPTH)
           || (timeParams->searchMode == DEPTH && rootDepth <= timeParams->allotment))));

    // When pondering, we must continue "searching" until given a stop or ponderhit command.
    while (isPonderSearch && !isStop)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send the stop signal, output best move and statistics to UCI interface
    if (threadID == 0) {
        stopSignal = true;
        isStop = true;

        if (ponder != NULL_MOVE)
//            cout << "bestmove " << moveToString(bestMove) << " ponder " << moveToString(ponder) << endl;
            engine_message(laser, "bestmove " + moveToString(bestMove) + " ponder " + moveToString(ponder) );
        else
//            cout << "bestmove " << moveToString(bestMove) << endl;
            engine_message(laser, "bestmove " + moveToString(bestMove));
    }
}

// Returns the index of the best move in legalMoves
void getBestMoveAtDepth(const Board *b, const MoveList *legalMoves, int depth, int alpha,
        int beta, int *bestMoveIndex, int *bestScore, unsigned int startMove,
        int threadID, SearchPV *pvLine) {
    SearchParameters *searchParams = &(threadMemoryArray[threadID]->searchParams);
    SearchStatistics *searchStats = &(threadMemoryArray[threadID]->searchStats);
    SearchPV line;
    int color = b->getPlayerToMove();
    int tempMove = -1;
    int score = -MATE_SCORE;
    *bestScore = -INFTY;
    SearchStackInfo *ssi = &(threadMemoryArray[threadID]->ssInfo[0]);

    // Push current position to two fold stack
    threadMemoryArray[threadID]->twoFoldPositions.push(b->getZobristKey());

    // Helps prevent stalls when using more threads than physical cores,
    // presumably by preventing this function from completing before the thread
    // is able to detach.
    std::this_thread::yield();

    for (unsigned int i = startMove; i < legalMoves->size(); i++) {
        Move m = legalMoves->get(i);
        // Output current move info to the GUI. Only do so if 5 seconds of
        // search have elapsed to avoid clutter
        uint64_t timeSoFar = getTimeElapsed(startTime);
        uint64_t nps = 1000 * getNodes() / timeSoFar;
      if (threadID == 0 && timeSoFar > 5 * ONE_SECOND) {
//            cout << "info depth " << depth << " currmove " << moveToString(m)
//                 << " currmovenumber " << i+1 << " nodes " << getNodes() << " nps " << nps << endl;
        std::string s =
        "info depth " + std::to_string(depth) + " currmove " + moveToString(m) +
        " currmovenumber " + std::to_string(i+1) + " nodes " + std::to_string(getNodes()) + " nps " + std::to_string(nps);
        engine_message(laser, s);
      }
        Board copy = b->staticCopy();
        copy.doMove(m, color);
        searchStats->nodes++;

        int startSq = getStartSq(m);
        int endSq = getEndSq(m);
        int pieceID = b->getPieceOnSquare(color, startSq);
        (ssi+1)->counterMoveHistory = searchParams->counterMoveHistory[pieceID][endSq];
        (ssi+1)->followupMoveHistory = nullptr;
        (ssi+2)->followupMoveHistory = searchParams->followupMoveHistory[pieceID][endSq];

        // Root LMR
        unsigned int movesSearched = i - startMove + 1;
        int reduction = 0;
        if (depth >= 3 && movesSearched > 1
         && !isCapture(m) && !isPromotion(m)) {
            reduction = lmrReductions[std::min(63, depth)][std::max(1U, std::min(63U, movesSearched))] - 1;
            reduction = std::max(0, reduction);
        }

        // Check extensions
        int extension = 0;
        if (copy.isInCheck(color^1)
         && b->isSEEAbove(color, m, 0)) {
            extension++;
        }

        if (i != 0) {
            score = -PVS(copy, depth-1+extension-reduction, -alpha-1, -alpha, threadID, true, ssi+1, &line);
            if (reduction > 0 && score > alpha) {
                score = -PVS(copy, depth-1+extension, -alpha-1, -alpha, threadID, true, ssi+1, &line);
            }
            if (alpha < score && score < beta) {
                score = -PVS(copy, depth-1+extension, -beta, -alpha, threadID, false, ssi+1, &line);
            }
        }
        else {
            score = -PVS(copy, depth-1+extension, -beta, -alpha, threadID, false, ssi+1, &line);
        }

        // Stop condition. If stopping, return search results from incomplete
        // search, if any.
        if (stopSignal.load(std::memory_order_seq_cst))
            break;

        if (score > *bestScore) {
            *bestScore = score;
            if (score > alpha) {
                alpha = score;
                tempMove = (int) i;
                if (pvLine != nullptr)
                    changePV(m, pvLine, &line);
            }
            // To get a PV if failing low
            else if (pvLine != nullptr && i == 0)
                changePV(m, pvLine, &line);
        }

        if (score >= beta)
            break;
    }

    threadMemoryArray[threadID]->twoFoldPositions.pop();
    *bestMoveIndex = tempMove;
}

//------------------------------------------------------------------------------
//------------------------------Search functions--------------------------------
//------------------------------------------------------------------------------
// The standard implementation of a fail-soft PVS search.
int PVS(Board &b, int depth, int alpha, int beta, int threadID, bool isCutNode, SearchStackInfo *ssi, SearchPV *pvLine) {
    SearchParameters *searchParams = &(threadMemoryArray[threadID]->searchParams);
    SearchStatistics *searchStats = &(threadMemoryArray[threadID]->searchStats);
    // Reset the PV line
    pvLine->pvLength = 0;
    // When the standard search is done, enter quiescence search.
    if (depth <= 0 || ssi->ply >= MAX_DEPTH) {
        // Update selective depth if necessary
        if (ssi->ply > searchParams->selectiveDepth)
            searchParams->selectiveDepth = ssi->ply;
        searchParams->ply = ssi->ply;

        return quiescence(b, 0, alpha, beta, threadID);
    }

    // Draw check
    if (b.isDraw())
        return 0;
    if (b.getFiftyMoveCounter() >= 2 && threadMemoryArray[threadID]->twoFoldPositions.find(b.getZobristKey()))
        return 0;


    // Mate distance pruning
    int matingScore = MATE_SCORE - ssi->ply;
    if (matingScore < beta) {
        beta = matingScore;
        if (alpha >= matingScore)
            return alpha;
    }

    int matedScore = -MATE_SCORE + ssi->ply;
    if (matedScore > alpha) {
        alpha = matedScore;
        if (beta <= matedScore)
            return beta;
    }


    // Check for a timeout
    if (threadID == 0 && (searchStats->nodes & 1023) == 1023 && !isPonderSearch) {
        uint64_t timeSoFar = getTimeElapsed(startTime);
        if (timeSoFar > timeLimit) {
            isStop = true;
            stopSignal = true;
        }
    }
    if (stopSignal.load(std::memory_order_relaxed))
        return 0;


    int prevAlpha = alpha;
    int color = b.getPlayerToMove();
    // For PVS, the node is a PV node if beta - alpha != 1 (not a null window)
    // We do not want to do most pruning techniques on PV nodes
    bool isPVNode = (beta - alpha != 1);
    // Reset killers of children to keep them local to similar positions
    searchParams->killers[ssi->ply+1][0] = NULL_MOVE;
    searchParams->killers[ssi->ply+1][1] = NULL_MOVE;


    // Transposition table probe
    // If a cutoff or exact score hit occurred, probeTT will return a value
    // other than -INFTY
    Move hashed = NULL_MOVE;
    int hashScore = -INFTY;
    int hashDepth = 0;
    uint8_t nodeType = NO_NODE_INFO;

    HashEntry *hashEntry = transpositionTable.get(b);
    if (hashEntry != nullptr) {
        hashScore = hashEntry->score;
        nodeType = hashEntry->ageNodeType & 0x3;
        hashDepth = hashEntry->depth;
        hashed = hashEntry->move;

        // Count hashed tb hits
        if (nodeType == PV_NODE && hashed == NULL_MOVE)
            searchStats->tbhits++;

        if (hashScore != -INFTY) {
            // Adjust the hash score to mate distance from root if necessary
            if (hashScore >= MAX_PLY_MATE_SCORE)
                hashScore -= ssi->ply;
            else if (hashScore <= -MAX_PLY_MATE_SCORE)
                hashScore += ssi->ply;

            // Return hash score failing soft if hash depth >= current depth and:
            //   The node is a hashed all node and score <= alpha
            //   The node is a hashed cut node and score >= beta
            //   The node is a hashed PV node and we are searching on a null window
            //    (we do not return immediately on full PVS windows since this cuts
            //     short the PV line)
            if (!isPVNode && hashDepth >= depth) {
                if ((nodeType == ALL_NODE && hashScore <= alpha)
                 || (nodeType == CUT_NODE && hashScore >= beta)
                 || (nodeType == PV_NODE)) {
                    return hashScore;
                }
            }
        }
    }


    // Tablebase probe
    // We use Stockfish's strategy of only probing WDL tables in the main search
    if (probeLimit
     && count(b.getAllPieces(WHITE) | b.getAllPieces(BLACK)) <= probeLimit
     && b.getFiftyMoveCounter() == 0
     && !b.getAnyCanCastle()) {
        int tbProbeResult;
        int tbValue = probe_wdl(b, &tbProbeResult);

        // Probe was successful
        if (tbProbeResult != 0) {
            searchStats->tbhits++;

            int tbScore = tbValue < -1 ? -TB_WIN - MAX_DEPTH + ssi->ply
                        : tbValue >  1 ?  TB_WIN + MAX_DEPTH - ssi->ply
                                       : 2 * tbValue;

            // Hash the TB result
            int tbDepth = std::min(depth+4, MAX_DEPTH);
            transpositionTable.add(b, adjustHashScore(tbScore, ssi->ply), NULL_MOVE, INFTY, tbDepth, PV_NODE);

            return tbScore;
        }
    }


    // Keeps track of the PV to propagate up to root
    SearchPV line;
    // We do not want to do pruning if we are in check
    bool isInCheck = b.isInCheck(color);
    // A static evaluation, used to make numerous pruning decisions
    int staticEval = INFTY;
    ssi->staticEval = INFTY;
    if (!isInCheck) {
        // Check the hash entry for a saved evaluation
        if (hashEntry != nullptr && hashEntry->eval != INFTY) {
            ssi->staticEval = staticEval = hashEntry->eval;
        }
        else {
            Eval e;
            ssi->staticEval = staticEval = (color == WHITE) ? e.evaluate(b) : -e.evaluate(b);
            transpositionTable.add(b, -INFTY, NULL_MOVE, staticEval, -8, NO_NODE_INFO);
        }
    }

    // Use the TT score as a better "static" eval, if available.
    if (hashScore != -INFTY && staticEval != INFTY) {
        if ((nodeType == ALL_NODE && hashScore < staticEval)
         || (nodeType == CUT_NODE && hashScore > staticEval)
         || (nodeType == PV_NODE))
            staticEval = hashScore;
    }


    // Is static eval improving across plies? Used to make pruning decisions. Idea from Stockfish
    bool evalImproving = ssi->ply >= 3 && !isInCheck
                      && (ssi->staticEval >= (ssi-2)->staticEval || (ssi-2)->staticEval == INFTY);


    // Reverse futility pruning
    // If we are already doing really well and it's our turn, our opponent
    // probably wouldn't have let us get here (a form of the null-move observation
    // adapted to low depths, also called static null move pruning)
    if (!isPVNode && !isInCheck
     && depth <= 6
     && staticEval - 70 * depth >= beta
     && b.getNonPawnMaterial(color))
        return staticEval;


    // Razoring
    // If static eval is a good amount below alpha, we are probably at an all-node.
    // Do a qsearch just to confirm. If the qsearch fails high, a capture gained back
    // the material and trust its result since a quiet move probably can't gain
    // as much.
    if (!isPVNode && !isInCheck
     && depth <= 2 && staticEval <= alpha - RAZOR_MARGIN) {
        searchParams->ply = ssi->ply;
        if (depth == 1)
            return quiescence(b, 0, alpha, beta, threadID);

        int rWindow = alpha - RAZOR_MARGIN;
        int value = quiescence(b, 0, rWindow, rWindow+1, threadID);
        if (value <= rWindow)
            return value;
    }


    // Null move pruning
    // If we are in a position good enough that even after passing and giving
    // our opponent a free turn, we still exceed beta, then simply return beta
    // Only if doing a null move does not leave player in check
    // Do not do if the side to move has only pawns
    // Do not do more than 2 null moves in a row
    if (!isPVNode && !isInCheck
     && depth >= 2 && staticEval >= beta
     && b.getNonPawnMaterial(color)) {
        // Reduce more if we are further ahead
        int reduction = 2 + (32 * depth + std::min(staticEval - beta, 384)) / 128;

        uint16_t epCaptureFile = b.getEPCaptureFile();
        b.doNullMove();
        (ssi+1)->counterMoveHistory = nullptr;
        (ssi+2)->followupMoveHistory = nullptr;
        int nullScore = -PVS(b, depth-1-reduction, -beta, -alpha, threadID, !isCutNode, ssi+1, &line);

        // Undo the null move
        b.undoNullMove(epCaptureFile);

        if (nullScore >= beta) {
            if (depth >= 10) {
                int verifyScore = PVS(b, depth-1-reduction, alpha, beta, threadID, false, ssi, &line);
                if (verifyScore >= beta)
                    return verifyScore;
            }
            else return nullScore;
        }

        searchParams->killers[ssi->ply+1][0] = NULL_MOVE;
        searchParams->killers[ssi->ply+1][1] = NULL_MOVE;
    }


    // Create list of legal moves
    MoveList legalMoves;
    if (isInCheck)
        b.getPseudoLegalCheckEscapes(legalMoves, color);
    else
        b.getAllPseudoLegalMoves(legalMoves, color);


    // ProbCut
    // If a winning capture scores much higher than beta on a shallow search,
    // then we can assume a beta cutoff would happen on the full search as
    // well, and return early.
    // Idea from Stockfish
    if (!isPVNode && !isInCheck
     && depth >= 6 && staticEval >= beta - 100 - 20 * depth
     && abs(beta) < MAX_PLY_MATE_SCORE) {
        int probCutMargin = beta + 90;
        int probCutCount = 0;
        MoveOrder moveSorter(&b, color, depth, searchParams, ssi, NULL_MOVE, legalMoves, probCutMargin - staticEval);
        moveSorter.generateMoves();

        for (Move m = moveSorter.nextMove(); m != NULL_MOVE && probCutCount < 3 && isCapture(m);
                  m = moveSorter.nextMove()) {
            probCutCount++;
            Board copy = b.staticCopy();
            // Search every move except the hash move
            if (m == hashed)
                continue;
            if (!copy.doPseudoLegalMove(m, color))
                continue;

            (ssi+1)->counterMoveHistory = searchParams->counterMoveHistory[b.getPieceOnSquare(color, getStartSq(m))][getEndSq(m)];
            (ssi+2)->followupMoveHistory = searchParams->followupMoveHistory[b.getPieceOnSquare(color, getStartSq(m))][getEndSq(m)];

            int score = -PVS(copy, depth - depth/4 - 4, -probCutMargin, -probCutMargin+1, threadID, !isCutNode, ssi+1, &line);

            if (score >= probCutMargin)
                return score;
        }
    }


    // Internal iterative deepening
    // When there is no hash move available, it is sometimes worth doing a
    // shallow search to try and look for one
    // This is especially true at PV nodes and potential cut nodes
    if (hashed == NULL_MOVE && !isInCheck
     && ((isPVNode && depth >= 6)
      || (!isPVNode && depth >= 8))) {
        int iidDepth = isPVNode ? depth - depth/4 - 1 : (depth - 5) / 2;
        PVS(b, iidDepth, alpha, beta, threadID, isCutNode, ssi, &line);

        HashEntry *iidEntry = transpositionTable.get(b);
        if (iidEntry != nullptr) {
            hashScore = iidEntry->score;
            nodeType = iidEntry->ageNodeType & 0x3;
            hashDepth = iidEntry->depth;
            hashed = iidEntry->move;
        }
    }


    // Initialize the module for move ordering
    MoveOrder moveSorter(&b, color, depth, searchParams, ssi, hashed, legalMoves);
    moveSorter.generateMoves();

    // Keeps track of the best move for storing into the TT
    Move toHash = NULL_MOVE;
    // separate counter only incremented when valid move is searched
    unsigned int movesSearched = 0;
    int bestScore = -INFTY;
    int score = -INFTY;


    //----------------------------Main search loop------------------------------
    for (Move m = moveSorter.nextMove(); m != NULL_MOVE;
              m = moveSorter.nextMove()) {
        bool isCheckMove = b.isCheckMove(color, m);
        // Conditions for whether to do futility and move count pruning
        bool moveIsPrunable = !isCapture(m)
                           && !isPromotion(m)
                           && m != hashed
                           && bestScore > -MAX_PLY_MATE_SCORE
                           && !isCheckMove;

        // For accessing history tables
        int startSq = getStartSq(m);
        int endSq = getEndSq(m);
        int pieceID = b.getPieceOnSquare(color, startSq);

        // Calculate LMR: increase reduction with higher depth and later moves
        int lmrReduction = lmrReductions[std::min(63, depth)][std::max(1U, std::min(63U, movesSearched))];
        int lmrDepth = std::max(0, depth - lmrReduction);

        // Used to adjust pruning amount so that PV nodes are pruned slightly less
        int pruneDepth = isPVNode      ? lmrDepth+1 :
                         evalImproving ? lmrDepth   : lmrDepth - 1;
        pruneDepth = std::max(0, pruneDepth);


        // Futility pruning
        // If we are already a decent amount of material below alpha, a quiet
        // move probably won't raise our prospects much, so don't bother
        // q-searching it.
        if (moveIsPrunable
         && !isInCheck
         && pruneDepth <= 6 && staticEval <= alpha - 115 - 90 * pruneDepth)
            continue;


        // Move count based pruning / Late move pruning
        // At low depths, moves late in the list with poor history are pruned
        // As used in Fruit/Stockfish:
        // https://chessprogramming.wikispaces.com/Futility+Pruning#MoveCountBasedPruning
        bool doMoveCountPruning = depth <= 12
                               && movesSearched > LMP_MOVE_COUNTS[evalImproving][depth] + (isPVNode ? depth : 0);
        if (moveIsPrunable
         && doMoveCountPruning)
            continue;


        // Prune moves with low history
        if (moveIsPrunable
         && pruneDepth <= 2
         && ((ssi->counterMoveHistory != nullptr) ? ssi->counterMoveHistory[pieceID][endSq] : -1) < 0
         && ((ssi->followupMoveHistory != nullptr) ? ssi->followupMoveHistory[pieceID][endSq] : -1) < 0)
            continue;


        // Futility pruning using SEE
        if (moveIsPrunable
         && pruneDepth <= 6
         && !b.isSEEAbove(color, m, -24 * pruneDepth * pruneDepth))
            continue;

        if (!isPVNode
         && m != hashed
         && bestScore > -MAX_PLY_MATE_SCORE
         && depth <= 5
         && !b.isSEEAbove(color, m, -100 * depth))
            continue;


        // Copy the board and do the move
        Board copy = b.staticCopy();
        // If we are searching the hash move, we must use to a special
        // move generator for extra verification.
        if (m == hashed) {
            if (!copy.doHashMove(m, color)) {
                hashed = NULL_MOVE;
                moveSorter.hashed = NULL_MOVE;
                moveSorter.generateMoves();
                continue;
            }
            moveSorter.generateMoves();
        }
        else if (!copy.doPseudoLegalMove(m, color))
            continue;
        searchStats->nodes++;

        movesSearched++;


        int reduction = 0;
        // Late move reduction
        // With good move ordering, later moves are less likely to increase
        // alpha, so we search them to a shallower depth hoping for a quick
        // fail-low.
        if (depth >= 3 && movesSearched > 1
         && !isCapture(m) && !isPromotion(m)) {
            reduction = lmrReduction;
            // Reduce less for killers
            if (m == searchParams->killers[ssi->ply][0]
             || m == searchParams->killers[ssi->ply][1])
                reduction--;
            // Reduce more for moves with poor history
            int historyValue = searchParams->historyTable[color][pieceID][endSq]
                + ((ssi->counterMoveHistory != nullptr) ? ssi->counterMoveHistory[pieceID][endSq] : 0)
                + ((ssi->followupMoveHistory != nullptr) ? ssi->followupMoveHistory[pieceID][endSq] : 0);
            reduction -= historyValue / 512;
            // Reduce more for expected cut nodes
            if (isCutNode)
                reduction++;
            // Reduce less at PV nodes
            if (isPVNode)
                reduction--;
            else if (!evalImproving)
                reduction++;

            // Do not let search descend directly into q-search
            reduction = std::max(0, std::min(reduction, depth - 2));
        }


        int extension = 0;
        // Check extensions
        if (!doMoveCountPruning
         && isCheckMove
         && b.isSEEAbove(color, m, 0)) {
            extension++;
        }

        // Record two-fold stack since we may do a search for singular extensions
        threadMemoryArray[threadID]->twoFoldPositions.push(b.getZobristKey());

        // Singular extensions
        // If the TT move appears to be much better than all others, extend the move
        if (depth >= 8 && reduction == 0 && extension == 0
         && m == hashed
         && abs(hashScore) < NEAR_MATE_SCORE
         && (nodeType == CUT_NODE || nodeType == PV_NODE)
         && hashDepth >= depth - 3) {
            bool isSingular = true;

            // Do a reduced depth search with a lowered window for a fail low check
            for (unsigned int i = 0; i < legalMoves.size(); i++) {
                Move seMove = legalMoves.get(i);
                Board seCopy = b.staticCopy();
                // Search every move except the hash move
                if (seMove == hashed)
                    continue;
                if (!seCopy.doPseudoLegalMove(seMove, color))
                    continue;

                (ssi+1)->counterMoveHistory = searchParams->counterMoveHistory
                    [b.getPieceOnSquare(color, getStartSq(seMove))][getEndSq(seMove)];
                (ssi+2)->followupMoveHistory = searchParams->followupMoveHistory
                    [b.getPieceOnSquare(color, getStartSq(seMove))][getEndSq(seMove)];

                // The window is lowered more for higher depths
                int SEWindow = hashScore - depth;
                // Do a reduced search for fail-low confirmation
                int SEDepth = depth / 2 - 1;

                score = -PVS(seCopy, SEDepth, -SEWindow - 1, -SEWindow, threadID, !isCutNode, ssi+1, &line);

                // If a move did not fail low, no singular extension
                if (score > SEWindow) {
                    isSingular = false;
                    break;
                }
            }

            searchParams->killers[ssi->ply+1][0] = NULL_MOVE;
            searchParams->killers[ssi->ply+1][1] = NULL_MOVE;

            // If all moves other than the hash move failed low, we extend for
            // the singular move
            if (isSingular)
                extension++;
        }


        (ssi+1)->counterMoveHistory = searchParams->counterMoveHistory[pieceID][endSq];
        (ssi+2)->followupMoveHistory = searchParams->followupMoveHistory[pieceID][endSq];

        // Null-window search, with re-search if applicable
        if (movesSearched > 1) {
            score = -PVS(copy, depth-1-reduction+extension, -alpha-1, -alpha, threadID, true, ssi+1, &line);

            // LMR re-search if the reduced search did not fail low
            if (reduction > 0 && score > alpha) {
                score = -PVS(copy, depth-1+extension, -alpha-1, -alpha, threadID, !isCutNode, ssi+1, &line);
            }

            // Re-search for a scout window at PV nodes
            if (alpha < score && score < beta) {
                score = -PVS(copy, depth-1+extension, -beta, -alpha, threadID, false, ssi+1, &line);
            }
        }

        // The first move is always searched at a normal depth
        else {
            score = -PVS(copy, depth-1+extension, -beta, -alpha, threadID, (isPVNode ? false : !isCutNode), ssi+1, &line);
        }

        // Pop the position in case we return early from this search
        threadMemoryArray[threadID]->twoFoldPositions.pop();

        // Stop condition to help break out as quickly as possible
        if (stopSignal.load(std::memory_order_relaxed))
            return 0;

        // Beta cutoff
        if (score >= beta) {
            // Hash the cut move and score
            transpositionTable.add(b, adjustHashScore(score, ssi->ply), m, ssi->staticEval, depth, CUT_NODE);

            // Update killers and histories for quiet moves
            if (!isCapture(m)) {
                // Ensure the same killer does not fill both slots
                if (m != searchParams->killers[ssi->ply][0]) {
                    searchParams->killers[ssi->ply][1] =
                        searchParams->killers[ssi->ply][0];
                    searchParams->killers[ssi->ply][0] = m;
                }
                moveSorter.updateHistories(m);
            }
            else
                moveSorter.updateCaptureHistories(m);

            changePV(m, pvLine, &line);

            return score;
        }

        // If alpha was raised, we have a new PV
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
                toHash = m;
                changePV(m, pvLine, &line);
            }
        }
    }
    // End main search loop


    // If there were no legal moves
    if (bestScore == -INFTY && movesSearched == 0)
        return scoreMate(isInCheck, ssi->ply);

    // Exact scores indicate a principal variation
    if (prevAlpha < alpha && alpha < beta) {
        transpositionTable.add(b, adjustHashScore(alpha, ssi->ply), toHash, ssi->staticEval, depth, PV_NODE);

        // Update histories for quiet moves
        if (!isCapture(toHash))
            moveSorter.updateHistories(toHash);
        else
            moveSorter.updateCaptureHistories(toHash);
    }

    // Record all-nodes
    else if (alpha <= prevAlpha) {
        // If we had a hash move, save it in case the node becomes a PV or cut node next time
        if (!isPVNode && hashed != NULL_MOVE) {
            transpositionTable.add(b, adjustHashScore(bestScore, ssi->ply), hashed, ssi->staticEval, depth, ALL_NODE);
        }
        // Otherwise, just store no best move as expected
        else {
            transpositionTable.add(b, adjustHashScore(bestScore, ssi->ply), NULL_MOVE, ssi->staticEval, depth, ALL_NODE);
        }
    }

    return bestScore;
}


/* Quiescence search, which completes all capture and check lines (thus reaching
 * a "quiet" position.)
 * This diminishes the horizon effect and greatly improves playing strength.
 * Delta pruning and static-exchange evaluation are used to reduce the time
 * spent here.
 * The search is a fail-soft PVS.
 */
int quiescence(Board &b, int plies, int alpha, int beta, int threadID) {
    SearchParameters *searchParams = &(threadMemoryArray[threadID]->searchParams);
    SearchStatistics *searchStats = &(threadMemoryArray[threadID]->searchStats);
    int color = b.getPlayerToMove();

    // If in check, we must consider all legal check evasions
    if (b.isInCheck(color))
        return checkQuiescence(b, plies, alpha, beta, threadID);

    if (b.isInsufficientMaterial())
        return 0;
    // Check for repetition draws while we are still considering checks
    if (b.getFiftyMoveCounter() >= 2 && threadMemoryArray[threadID]->twoFoldPositions.find(b.getZobristKey()))
        return 0;

    // Stop condition to help break out as quickly as possible
    if (stopSignal.load(std::memory_order_relaxed))
        return 0;

    // Qsearch hash table probe
    int hashScore = -INFTY;
    HashEntry *hashEntry = transpositionTable.get(b);
    uint8_t nodeType = NO_NODE_INFO;
    if (hashEntry != nullptr) {
        hashScore = hashEntry->score;

        if (hashScore != -INFTY) {
            // Adjust the hash score to mate distance from root if necessary
            if (hashScore >= MAX_PLY_MATE_SCORE)
                hashScore -= searchParams->ply + plies;
            else if (hashScore <= -MAX_PLY_MATE_SCORE)
                hashScore += searchParams->ply + plies;

            nodeType = hashEntry->ageNodeType & 0x3;
            // Only used a hashed score if the search depth was at least
            // the current depth
            if (hashEntry->depth >= -plies) {
                // Check for the correct node type and bounds
                if ((nodeType == ALL_NODE && hashScore <= alpha)
                 || (nodeType == CUT_NODE && hashScore >= beta)
                 || (nodeType == PV_NODE))
                    return hashScore;
            }
        }
    }


    // Stand pat: if our current position is already way too good or way too bad
    // we can simply stop the search here.
    int hashEval, staticEval;
    // Check the hash entry for a saved evaluation
    if (hashEntry != nullptr) {
        if (hashEntry->eval != INFTY) {
            hashEval = staticEval = hashEntry->eval;
        }
        else {
            Eval e;
            hashEval = staticEval = (color == WHITE) ? e.evaluate(b) : -e.evaluate(b);
        }
    }
    else {
        Eval e;
        hashEval = staticEval = (color == WHITE) ? e.evaluate(b) : -e.evaluate(b);
        transpositionTable.add(b, -INFTY, NULL_MOVE, hashEval, -8, NO_NODE_INFO);
    }

    // Use the TT score as a better "static" eval, if available.
    if (hashScore != -INFTY) {
        if ((nodeType == ALL_NODE && hashScore < staticEval)
         || (nodeType == CUT_NODE && hashScore > staticEval)
         || (nodeType == PV_NODE))
            staticEval = hashScore;
    }

    // The stand pat cutoff
    if (staticEval >= beta)
        return staticEval;

    if (alpha < staticEval)
        alpha = staticEval;

    int bestScore = staticEval;


    // Initialize the module for move ordering
    MoveOrder moveSorter(&b, color, -plies, searchParams);
    moveSorter.generateMoves();

    for (Move m = moveSorter.nextMove(); m != NULL_MOVE;
              m = moveSorter.nextMove()) {
        // If we are on the final stage of movegen, we are searching QS checks
        bool isCheckMove = (moveSorter.mgStage == STAGE_QS_DONE);

        // Futility pruning
        if (!isCheckMove && !isPromotion(m)
         && staticEval < alpha - 80 && !b.isSEEAbove(color, m, 1)) {
            bestScore = std::max(bestScore, staticEval + 80);
            continue;
        }

        // Static exchange evaluation pruning
        if (!b.isSEEAbove(color, m, 0))
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;

        searchStats->nodes++;
        int score = isCheckMove ? -checkQuiescence(copy, plies+1, -beta, -alpha, threadID)
                                : -quiescence(copy, plies+1, -beta, -alpha, threadID);

        if (score >= beta) {
            transpositionTable.add(b, adjustHashScore(score, searchParams->ply + plies), m, hashEval, -plies, CUT_NODE);
            return score;
        }

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }
    }

    return bestScore;
}

/*
 * When checks are considered in quiescence, the responses must include all moves,
 * not just captures, necessitating this function.
 */
int checkQuiescence(Board &b, int plies, int alpha, int beta, int threadID) {
    if (b.getFiftyMoveCounter() >= 2 && threadMemoryArray[threadID]->twoFoldPositions.find(b.getZobristKey()))
        return 0;

    SearchParameters *searchParams = &(threadMemoryArray[threadID]->searchParams);
    SearchStatistics *searchStats = &(threadMemoryArray[threadID]->searchStats);
    int color = b.getPlayerToMove();
    MoveList legalMoves;
    b.getPseudoLegalCheckEscapes(legalMoves, color);

    int bestScore = -INFTY;
    int score = -INFTY;
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        if (bestScore > -MAX_PLY_MATE_SCORE
         && !isCapture(m)
         && !b.isSEEAbove(color, m, 0))
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;

        searchStats->nodes++;
        threadMemoryArray[threadID]->twoFoldPositions.push(b.getZobristKey());

        score = -quiescence(copy, plies+1, -beta, -alpha, threadID);

        threadMemoryArray[threadID]->twoFoldPositions.pop();

        if (score >= beta)
            return score;

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }
    }

    // If there were no legal moves
    if (bestScore == -INFTY) {
        // We already know we are in check, so it must be a checkmate
        // Adjust score so that quicker mates are better
        return (-MATE_SCORE + searchParams->ply + plies);
    }

    return bestScore;
}


//------------------------------------------------------------------------------
//-----------------------------Search Helpers-----------------------------------
//------------------------------------------------------------------------------

// Used to get a score when we have realized that we have no legal moves.
int scoreMate(bool isInCheck, int plies) {
    // If we are in check, then it is a checkmate
    if (isInCheck)
        // Adjust score so that quicker mates are better
        return (-MATE_SCORE + plies);

    // Else, it is a stalemate
    else return 0;
}

// Adjust a mate score to accurately reflect distance to mate from the
// current position, if necessary.
int adjustHashScore(int score, int plies) {
    if (score >= MAX_PLY_MATE_SCORE)
        return score + plies;
    if (score <= -MAX_PLY_MATE_SCORE)
        return score - plies;
    return score;
}


// Pondering
void startPonder() {
    isPonderSearch = true;
}

void stopPonder() {
    isPonderSearch = false;
}


//------------------------------------------------------------------------------
//------------------------------Other functions---------------------------------
//------------------------------------------------------------------------------

// These functions help to communicate with uci.cpp
void clearTables() {
    transpositionTable.clear();
    for (int i = 0; i < numThreads; i++)
        threadMemoryArray[i]->searchParams.resetHistoryTable();
}

void setHashSize(uint64_t MB) {
    transpositionTable.setSize(MB);
}

uint64_t getNodes() {
    uint64_t total = 0;
    for (int i = 0; i < numThreads; i++) {
        total += threadMemoryArray[i]->searchStats.nodes;
    }
    return total;
}

uint64_t getTBHits() {
    uint64_t total = 0;
    for (int i = 0; i < numThreads; i++) {
        total += threadMemoryArray[i]->searchStats.tbhits;
    }
    return total;
}

void setMultiPV(unsigned int n) {
    multiPV = n;
}

void setNumThreads(int n) {
    numThreads = n;

    while ((int) threadMemoryArray.size() < n)
        threadMemoryArray.push_back(new ThreadMemory());
    while ((int) threadMemoryArray.size() > n) {
        delete threadMemoryArray.back();
        threadMemoryArray.pop_back();
    }
}

void initPerThreadMemory() {
    threadMemoryArray.push_back(new ThreadMemory());
}

TwoFoldStack *getTwoFoldStackPointer() {
    return &(threadMemoryArray[0]->twoFoldPositions);
}


// Retrieves the next move with the highest score, starting from index using a
// partial selection sort. This way, the entire list does not have to be sorted
// if an early cutoff occurs.
Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index) {
    if (index >= moves.size())
        return NULL_MOVE;
    // Find the index of the next best move
    int bestIndex = index;
    int bestScore = scores.get(index);
    for (unsigned int i = index + 1; i < moves.size(); i++) {
        if (scores.get(i) > bestScore) {
            bestIndex = i;
            bestScore = scores.get(bestIndex);
        }
    }
    // Swap the best move to the correct position
    moves.swap(bestIndex, index);
    scores.swap(bestIndex, index);
    return moves.get(index);
}

// Copies the new PV line when alpha is raised
void changePV(Move best, SearchPV *parent, SearchPV *child) {
    parent->pv[0] = best;
    for (int i = 0; i < child->pvLength; i++) {
        parent->pv[i+1] = child->pv[i];
    }
    parent->pvLength = child->pvLength + 1;
}

// Recover PV for outputting to terminal / GUI
std::string retrievePV(SearchPV *pvLine) {
    std::string pvStr = moveToString(pvLine->pv[0]);
    for (int i = 1; i < pvLine->pvLength; i++) {
        pvStr += " " + moveToString(pvLine->pv[i]);
    }

    return pvStr;
}

// The selective depth in a parallel search is the max selective depth reached
// by any of the threads
int getSelectiveDepth() {
    int max = 0;
    for (int i = 0; i < numThreads; i++)
        if (threadMemoryArray[i]->searchParams.selectiveDepth > max)
            max = threadMemoryArray[i]->searchParams.selectiveDepth;
    return max;
}

// Formats a fraction into a percentage value (0 to 100) for printing
double getPercentage(uint64_t numerator, uint64_t denominator) {
    if (denominator == 0)
        return 0;
    uint64_t tenThousandths = (numerator * 10000) / denominator;
    double percent = ((double) tenThousandths) / 100.0;
    return percent;
}
