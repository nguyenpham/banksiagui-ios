/*
This file is part of Nemorino.

Nemorino is free software : you can redistribute it and /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Nemorino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Nemorino.If not, see < http://www.gnu.org/licenses/>.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <queue>
#include <condition_variable>
#include "types.h"
#include "book.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include "timemanager.h"
#include "utils.h"
#include "tbprobe.h"


enum struct ThreadType { SINGLE, MASTER, SLAVE };
enum struct SearchResultType { EXACT_RESULT, FAIL_LOW, FAIL_HIGH, TABLEBASE_MOVE, UNIQUE_MOVE, BOOK_MOVE };

class ThreadPool {
public:
	using Task = std::function<void(int)>;
	explicit ThreadPool(size_t numberOfThreads);
	~ThreadPool();
	ThreadPool(const ThreadPool&) = default;
	ThreadPool& operator=(const ThreadPool&) = default;
	ThreadPool(ThreadPool&&) = default;
	ThreadPool& operator=(ThreadPool&&) = default;
	void enqueue(Task t);
	std::queue<Task> tasks;
	inline size_t size() noexcept { return threads.size(); }
	inline int tasks_active() noexcept { return active.load(); }
private:
	std::vector<std::thread> threads;
	std::condition_variable cvStartTask;
	std::mutex mtxStartTask;
	bool shutdown = false;
	std::atomic<int> active{ 0 };

	void start(size_t numberOfThreads);
	void stop() noexcept;
};

//Struct contains thread local data, which isn't shared among threads
struct ThreadData {
	int id;
	//History tables used in move ordering during search
	MoveSequenceHistoryManager cmHistory;
	MoveSequenceHistoryManager followupHistory;
	HistoryManager History;
	killer::Manager killerManager;
};

struct SearchResult {
	ValuatedMove move;
	int depth;
};

class NemorinoSearch {
public:
	bool UciOutput = false;
	bool PrintCurrmove = true;
	//Flag indicating the engine is currently pondering
	std::atomic<bool> PonderMode{ false };
	//The search's result (will be updated while searching)
	ValuatedMove BestMove;
	//Total Node Count (including nodes from Quiescence Search)
	int64_t NodeCount = 0;
	//Quiescence Search Node Count
	int64_t QNodeCount = 0;
	//Maximum Depth reached during Search
	int MaxDepth = 0;
	//Flag, indicating that search shall stop as soon as possible 
	std::atomic<bool> Stop{ true };
	//Polyglot bookfile if provided
	std::string BookFile = "";
	int rootMoveCount = 0;
	ValuatedMove rootMoves[MAX_MOVE_COUNT];
	Position rootPosition;
	//For analysis purposes, list of root moves which shall be considered
	std::vector<Move> searchMoves;
	//Number of root moves for which full analysis shall be done
	int MultiPv = 1;
	//The timemanager, which handles the time assignment and checks if a search shall be continued
	Timemanager timeManager;

	//Re-initialize BestMove, History, CounterMoveHistory, Killer, Node counters, ....
	void Reset() noexcept;
	//Determines the Principal Variation (PV is stored during search in "triangular" array and is completed from hash table, if
	//it's incomplete (due to prunings)
	std::string PrincipalVariation(Position& pos, int depth = PV_MAX_LENGTH);
	//Reset engine to start a new game
	void NewGame() noexcept;
	//Returns the current nominal search depth
	inline int Depth() const noexcept { return _depth; }
	//Returns the thinkTime needed so far (is updated after every iteration)
	inline Time_t ThinkTime() const noexcept { return _thinkTime; }
	//Determines the move the engine assumes that the opponent is playing
	inline Move PonderMove() const noexcept { return ponderMove; }
	//Stops the current search immediatialy
	inline void StopThinking() noexcept {
		PonderMode.store(false);
		Stop.store(true);
		std::lock_guard<std::mutex> lgStart(mtxSearch);
	}
	//Get's the "best" move from the polyglot opening book
	Move GetBestBookMove(Position& pos, ValuatedMove* moves, int moveCount);
	//handling of the thinking output for uci and xboard
	void info(Position& pos, int pvIndx, SearchResultType srt = SearchResultType::EXACT_RESULT);

	void debugInfo(std::string info);

	void log(std::string info);

	NemorinoSearch() noexcept;
	~NemorinoSearch();
	NemorinoSearch(const NemorinoSearch&) = delete;
	NemorinoSearch& operator=(const NemorinoSearch&) = default;
	NemorinoSearch(NemorinoSearch&&) = default;
	NemorinoSearch& operator=(NemorinoSearch&&) = default;
	//Main entry point 
	ValuatedMove Think(Position& pos);
	//In case of SMP, start Slave threads
	void startHelper(int id);

	std::mutex mtxSearch;
	std::atomic<bool> think_started{ false };
	//Utility method (not used when thinking). Checks if a position is quiet (that's static evalution is about the same as QSearch result). Might be
	//useful for tuning
	bool isQuiet(Position& pos);

	Value qscore(Position* pos);

private:
	//Mutex to synchronize access to analysis output
	std::mutex mtxXAnalysisOutput;
	//Polyglot book object to read book moves
	std::unique_ptr<polyglot::Book> book = nullptr;

	ThreadData threadLocalData;

	//in xboard protocol in analysis mode, the GUI determines the point in time when information shall be provided (in UCI the engine determines these 
	//point in times. As this information is only available while the search's recursion level is root the information is prepared, whenever feasible and
	//stored in member XAnylysisOutput, where the protocol driver can retrieve it at any point in time
	std::string* XAnalysisOutput = nullptr;

	//Array where moves from Principal Variation are stored
	Move PVMoves[PV_MAX_LENGTH];
	//Move which engine expects opponent will reply to it's next move
	Move ponderMove = MOVE_NONE;
	std::atomic<int> _depth{ 0 };
	Time_t _thinkTime;
	Move counterMove[12][64];
	uint64_t tbHits = 0;
	//Flag indicating whether TB probes shall be made during search
	bool probeTB = true;
	bool use_tt_in_qsearch = settings::parameter.USE_TT_IN_QSEARCH;
	int currmove_depth = 5;
	std::vector<SearchResult> searchResults;

	std::unordered_map<Move, Value> rootMoveBoni;

	ThreadPool* thread_pool = nullptr;

	inline bool Stopped()  noexcept { return Stop; }

	void SetRootMoveBoni();

	//Main recursive search method
	template<ThreadType T> Value SearchMain(Value alpha, Value beta, Position& pos, int depth, Move* pv, ThreadData& tlData, bool cutNode, bool prune = true, Move excludeMove = MOVE_NONE);
	//At root level there is a different search method (as there is some special logic requested)
	template<ThreadType T> Value SearchRoot(Value alpha, Value beta, Position& pos, int depth, ValuatedMove* moves, Move* pv, ThreadData& tlData, int startWithMove = 0);
	//Quiescence Search (different implementations for positions in check and not in check)
	template<ThreadType T> Value QSearch(Value alpha, Value beta, Position& pos, int depth, ThreadData& tlData);
	//Updates killer, history and counter move history tables, whenever a cutoff has happened
	void updateCutoffStats(ThreadData& tlData, const Move cutoffMove, int depth, Position& pos, int moveIndex) noexcept;
	//Thread id (0: MASTER or SINGLE, SLAVES are sequentially numbered starting with 1

};


template<ThreadType T> Value NemorinoSearch::SearchRoot(Value alpha, Value beta, Position& pos, int depth, ValuatedMove* moves, Move* pv, ThreadData& tlData, int startWithMove) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	pv[1] = MOVE_NONE; //pv[1] will be the ponder move, so we should make sure, that it is initialized as well
	const bool lmr = !pos.Checked() && depth >= 3;
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == ThreadType::SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	tt::NodeType nt = tt::NodeType::UPPER_BOUND;
	//move loop
	for (int i = startWithMove; i < rootMoveCount; ++i) {
		if (T != ThreadType::SLAVE) {
			//Send information about the current move to GUI
			const Time_t currmove_time = currmove_depth == 0 ? 1000 : 3000;
			if (UciOutput && PrintCurrmove && depth > currmove_depth && (now() - timeManager.GetStartTime()) > currmove_time) {
				sync_cout << "info depth " << depth << " currmove " << toString(moves[i].move) << " currmovenumber " << i + 1 << sync_endl;
			}
		}
		//apply move
		Position next(pos);
		next.ApplyMove(moves[i].move);
		CHECK(next.GetPliesFromRoot() == 1);
		const Value bonus = rootMoveBoni[moves[i].move];
		if (i > startWithMove) {
			int reduction = 0;
			//Lmr for root moves
			if (lmr && i >= startWithMove + 5 && pos.IsQuietAndNoCastles(moves[i].move) && !next.Checked()) {
				++reduction;
			}
			score = bonus - SearchMain<T>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1 - reduction, subpv, tlData, true);
			if (reduction > 0 && score > alpha && score < beta) {
				score = bonus - SearchMain<T>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1, subpv, tlData, true);
			}
			if (score > alpha && score < beta) {
				//Research without reduction and with full alpha-beta window
				score = bonus - SearchMain<T>(bonus - beta, bonus - alpha, next, depth - 1, subpv, tlData, false);
			}
		}
		else {
			score = bonus - SearchMain<T>(bonus - beta, bonus - alpha, next, depth - 1, subpv, tlData, false);
		}
		if (Stopped()) break;
		moves[i].score = score;
		if (score > bestScore) {
			bestScore = score;
			pv[0] = moves[i].move;
			memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1) * sizeof(Move));
			if (i > 0) {
				//make sure that best move is always in first place 
				const ValuatedMove bm = moves[i];
				for (int j = i; j > startWithMove; --j)moves[j] = moves[j - 1];
				moves[startWithMove] = bm;
			}
			if (score >= beta) {
				//Update transposition table
				if (T != ThreadType::SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::NodeType::LOWER_BOUND, depth, pv[0], VALUE_NOTYETDETERMINED);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::NodeType::LOWER_BOUND, depth, pv[0], VALUE_NOTYETDETERMINED);
				return score;
			}
			else if (score > alpha)
			{
				alpha = score;
				nt = tt::NodeType::EXACT;
			}
		}
	}
	//Update transposition table
	if (T != ThreadType::SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nt, depth, pv[0], VALUE_NOTYETDETERMINED);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nt, depth, pv[0], VALUE_NOTYETDETERMINED);
	return bestScore;
}

//This is the main alpha-beta search routine
template<ThreadType T> Value NemorinoSearch::SearchMain(Value alpha, Value beta, Position& pos, int depth, Move* pv, ThreadData& tlData, bool cutNode, bool prune, Move excludeMove) {
	if (T != ThreadType::SLAVE) {
		if (depth > 0) {
			++NodeCount;
		}
		if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
	}
	if (Stopped()) return VALUE_ZERO;
	if (pos.GetResult() != Result::OPEN)  return pos.evaluateFinalPosition();
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return SCORE_MDP(alpha);
	//If depth = 0 is reached go to Quiescence Search
	if (depth <= 0) {
		return QSearch<T>(alpha, beta, pos, 0, tlData);
	}
	depth = std::min(depth, MAX_DEPTH - 1);
	uint64_t hashKey = pos.GetHash();
	if (excludeMove) hashKey ^= excludeMove * 14695981039346656037ull;
	//TT lookup
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == ThreadType::SINGLE) ? tt::probe<tt::UNSAFE>(hashKey, ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(hashKey, ttFound, ttEntry);
	const Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	Move ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		if (ttMove && pos.IsQuiet(ttMove) && pos.validateMove(ttMove) && ttValue >= beta) updateCutoffStats(tlData, ttMove, depth, pos, -1);
		return SCORE_TT(ttValue);
	}
	// Tablebase probe
	// Probing is only done if root position was no tablebase position (probeTB is true) and if drawPlyCount = 0 (in that case we get the necessary WDL information) to return
	// immediately
	if (probeTB && pos.GetDrawPlyCount() == 0 && pos.GetMaterialTableEntry()->IsTablebaseEntry() && depth >= settings::parameter.TBProbeDepth)
	{
		tablebases::ProbeState state;
		const tablebases::WDLScore wdl = tablebases::probe_wdl(pos, &state);
		if (state != tablebases::ProbeState::FAIL)
		{
			if (T != ThreadType::SLAVE) tbHits++;
			const Value value = wdl == tablebases::WDLLoss ? -VALUE_MATE + MAX_DEPTH + pos.GetPliesFromRoot()
				: wdl == tablebases::WDLWin ? VALUE_MATE - MAX_DEPTH - pos.GetPliesFromRoot()
				: VALUE_DRAW + 2 * static_cast<int>(wdl);
			if (T != ThreadType::SINGLE) ttPointer->update<tt::THREAD_SAFE>(hashKey, tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			else ttPointer->update<tt::UNSAFE>(hashKey, tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			return SCORE_TB(value);
		}
	}
	const bool PVNode = (beta > alpha + 1);
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	const bool checked = pos.Checked();
	Value staticEvaluation;
	if (checked) staticEvaluation = VALUE_NOTYETDETERMINED;
	else if (ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED) {
		staticEvaluation = ttEntry.evalValue();
		pos.SetStaticEval(staticEvaluation);
	}
	else
		staticEvaluation = pos.evaluate();

	prune = prune && !PVNode && !checked && (pos.GetLastAppliedMove() != MOVE_NONE) && (!pos.GetMaterialTableEntry()->SkipPruning());

	if (prune) {
		// Beta Pruning
		if (depth < 7
			&& staticEvaluation < VALUE_KNOWN_WIN
			&& (staticEvaluation - settings::parameter.BetaPruningMargin(depth)) >= beta
			&& pos.GetMaterialTableEntry()->DoNullmove(pos.GetSideToMove()))
			return SCORE_BP(staticEvaluation - settings::parameter.BetaPruningMargin(depth));

		//Razoring
		if (depth < 4
			&& (staticEvaluation + settings::parameter.RazoringMargin(depth)) <= alpha
			&& ttMove == MOVE_NONE
			&& !pos.PawnOn7thRank())
		{
			//if (depth <= 1 && (effectiveEvaluation + settings::RazoringMargin(depth)) <= alpha) return QSearch(alpha, beta, pos, 0);
			const Value razorAlpha = alpha - settings::parameter.RazoringMargin(depth);
			const Value razorScore = QSearch<T>(razorAlpha, Value(razorAlpha + 1), pos, 0, tlData);
			if (razorScore <= razorAlpha) return SCORE_RAZ(razorScore);
		}

		//Check if Value from TT is better
		Value evaluation = staticEvaluation;
		if (ttFound &&
			((ttValue > evaluation&& ttEntry.type() == tt::LOWER_BOUND)
				|| (ttValue < evaluation && ttEntry.type() == tt::UPPER_BOUND)
				|| (ttEntry.type() == tt::EXACT)
				)) evaluation = ttValue;

		//Null Move Pruning
		if (evaluation >= beta
			&& pos.GetMaterialTableEntry()->DoNullmove(pos.GetSideToMove())
			&& excludeMove == MOVE_NONE
			) {
			int reduction = (depth + 14) / 5;
			if (int(evaluation - beta) > 100) ++reduction;
			const Square epsquare = pos.GetEPSquare();
			const Move lastApplied = pos.GetLastAppliedMove();
			pos.NullMove();
			Value nullscore = -SearchMain<T>(-beta, -beta + 1, pos, depth - reduction, subpv, tlData, !cutNode, false);
			pos.NullMove(epsquare, lastApplied);
			if (nullscore >= beta) {
				if (nullscore >= VALUE_MATE_THRESHOLD) nullscore = beta;
				if (depth < 9 && beta < VALUE_KNOWN_WIN) return SCORE_NMP(nullscore);
				// Do verification search at high depths
				const Value verificationScore = SearchMain<T>(beta - 1, beta, pos, depth - reduction, subpv, tlData, false, false);
				if (verificationScore >= beta) return SCORE_NMP(nullscore);
			}
		}

		// ProbCut (copied from SF)
		if (!PVNode
			&& depth >= 5
			&& std::abs(int(beta)) < VALUE_MATE_THRESHOLD)
		{
			const Value rbeta = std::min(Value(beta + 90), VALUE_INFINITE);
			const int rdepth = depth - 4;

			Position cpos(pos);
			cpos.copy(pos);
			const Value limit = settings::parameter.SEE_VAL[GetPieceType(pos.getCapturedInLastMove())];
			Move ttm = ttMove;
			if (ttm != MOVE_NONE && cpos.SEE(ttMove) < limit) ttm = MOVE_NONE;
			if (pos.mateThread())
				cpos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttm);
			else
				cpos.InitializeMoveIterator<QSEARCH>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttm);
			Move move;
			while ((move = cpos.NextMove()) && move != excludeMove) {
				if (pos.SEE(move) < rbeta - staticEvaluation) continue;
				Position next(cpos);
				if (next.ApplyMove(move)) {
					const Value score = -SearchMain<T>(-rbeta, Value(-rbeta + 1), next, rdepth, subpv, tlData, !cutNode);
					if (score >= rbeta)
						return SCORE_PC(score);
				}
			}
		}
	}
	//Internal Iterative Deepening - it seems as IID helps as well if the found hash entry has very low depth
	const int iidDepth = PVNode ? depth - 2 : depth / 2;
	if ((!ttMove || ttEntry.depth() < iidDepth) && (PVNode ? depth > 3 : depth > 6)) {
		Position next(pos);
		next.copy(pos);
		//If there is no hash move, we are looking for a move => therefore search should be called with prune = false
		SearchMain<T>(alpha, beta, next, iidDepth, subpv, tlData, cutNode, ttMove != MOVE_NONE);
		if (Stopped()) return VALUE_ZERO;
		ttPointer = (T == ThreadType::SINGLE) ? tt::probe<tt::UNSAFE>(hashKey, ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(hashKey, ttFound, ttEntry);
		ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	}
	if (!checked && ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED && pos.GetStaticEval() == VALUE_NOTYETDETERMINED) pos.SetStaticEval(ttEntry.evalValue());
	//bool regular = !checked && pos.GetLastAppliedMove() != MOVE_NONE && pos.Previous()->GetLastAppliedMove() != MOVE_NONE && !pos.Previous()->Previous()->Checked();
	//bool improving = regular && pos.Previous()->Previous()->GetStaticEval() < pos.GetStaticEval();
	const Move counter = pos.GetCounterMove(counterMove);
	Value bestScore = -VALUE_MATE;
	//Futility Pruning I: If quiet moves can't raise alpha, only generate tactical moves and moves which give check
	const bool futilityPruning = pos.GetLastAppliedMove() != MOVE_NONE
		&& !checked
		&& depth <= settings::parameter.FULTILITY_PRUNING_DEPTH
		&& beta < VALUE_MATE_THRESHOLD
		&& pos.GetMaterialTableEntry()->DoNullmove(pos.GetSideToMove())
		&& alpha >(staticEvaluation + settings::parameter.FUTILITY_PRUNING_LIMIT[depth]);
	if (futilityPruning) {
		pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, counter, ttMove);
		bestScore = alpha;
	}
	else
		pos.InitializeMoveIterator<MAIN_SEARCH>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, &tlData.killerManager, counter, ttMove);
	Value score;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	const bool lmr = !checked && depth >= 3;
	Move move;
	int moveIndex = -1;
	int bestMoveIndex = -1;
	bool ZWS = !PVNode;
	const Square recaptureSquare = pos.GetLastAppliedMove() != MOVE_NONE && pos.Previous()->GetPieceOnSquare(to(pos.GetLastAppliedMove())) != BLANK ? to(pos.GetLastAppliedMove()) : OUTSIDE;
	const bool trySE = depth >= 8 && ttMove != MOVE_NONE && abs(ttValue) < VALUE_KNOWN_WIN
		&& excludeMove == MOVE_NONE && (ttEntry.type() == tt::LOWER_BOUND || ttEntry.type() == tt::EXACT) && ttEntry.depth() >= depth - 3;
	tlData.killerManager.enterLevel(pos);
	while ((move = pos.NextMove())) {
		++moveIndex;
		if (move == excludeMove) continue;

		const bool prunable = !PVNode && !checked && move != ttMove && move != counter && std::abs(int(bestScore)) <= VALUE_MATE_THRESHOLD
			&& !tlData.killerManager.isKiller(pos, move) && (pos.IsQuietAndNoCastles(move) || pos.GetMoveGenerationPhase() == MoveGenerationType::LOOSING_CAPTURES)
			&& !pos.IsAdvancedPawnPush(move);
		if (prunable) {
			const int reducedDepth = depth - settings::parameter.LMRReduction(depth, moveIndex);
			if (moveIndex >= depth * 4 || (reducedDepth <= 4 && pos.SEE_Sign(move) < 0)) {
				if (!pos.givesCheck(move)) continue;
			}
		}
		Position next(pos);
		if (next.ApplyMove(move)) {
			//critical = critical || GetPieceType(pos.GetPieceOnSquare(from(move))) == PAWN && ((pos.GetSideToMove() == WHITE && from(move) > H5) || (pos.GetSideToMove() == BLACK && from(move) < A4));
			//Check extension
			int extension = 0;
			//if (depth + next.GetPliesFromRoot() < MAX_DEPTH) {
			if (next.GetMaterialTableEntry()->Phase == 256 && pos.GetMaterialTableEntry()->Phase < 256 && next.GetMaterialScore() >= -settings::parameter.PieceValues[PAWN].egScore && next.GetMaterialScore() <= settings::parameter.PieceValues[PAWN].egScore)
				extension = 3;
			else extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;
			if (!extension && to(move) == recaptureSquare) {
				++extension;
			}
			if (trySE && move == ttMove && !extension)
			{
				const Value rBeta = ttValue - 2 * depth;
				Position spos(pos);
				spos.copy(pos);
				const Value sval = SearchMain<T>(rBeta - 1, rBeta, spos, std::max(5, depth / 3), subpv, tlData, cutNode, true, move);
				if (sval < rBeta) {
					++extension;
				}
				else if (rBeta > beta) {
					return rBeta;
				}
			}
			if (!extension && moveIndex == 0 && pos.GeneratedMoveCount() == 1 && (pos.GetMoveGenerationPhase() == MoveGenerationType::CHECK_EVASION || pos.QuietMoveGenerationPhaseStarted())) {
				++extension;
			}
			//}
			int reduction = 0;
			//LMR: Late move reduction
			if (lmr && moveIndex != 0 && move != counter && pos.QuietMoveGenerationPhaseStarted() 
				&& (next.GetPawnEntry()->Score - pos.GetPawnEntry()->Score).getScore(next.GetMaterialTableEntry()->Phase) * (1 - 2 * static_cast<int>(next.GetSideToMove())) < settings::parameter.PAWN_REDUCTION_LIMIT) {
				reduction = settings::parameter.LMRReduction(depth, moveIndex);
				if (cutNode) ++reduction;
				if (PVNode || extension) --reduction;
				if (reduction && (ToBitboard(from(move)) & pos.AttackedByThem()) != 0
					&& type(move) == MoveType::NORMAL && next.SEE(createMove(to(move), from(move))) < 0)
					reduction--;
				reduction = std::max(0, reduction);
			}
			if (ZWS) {
				score = -SearchMain<T>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, subpv, tlData, !cutNode);
				if (score > alpha && reduction)
					score = -SearchMain<T>(Value(-alpha - 1), -alpha, next, depth - 1 + extension, subpv, tlData, !cutNode);
				if (score > alpha && score < beta) {
					score = -SearchMain<T>(-beta, -alpha, next, depth - 1 + extension, subpv, tlData, false);
				}
			}
			else {
				score = -SearchMain<T>(-beta, -alpha, next, depth - 1 - reduction + extension, subpv, tlData, (PVNode ? false : !cutNode));
				if (score > alpha && reduction > 0) {
					score = -SearchMain<T>(-beta, -alpha, next, depth - 1 + extension, subpv, tlData, (PVNode ? false : !cutNode));
				}
			}
			if (score >= beta) {
				updateCutoffStats(tlData, move, depth, pos, moveIndex);
				//Update transposition table
				if (T != ThreadType::SINGLE)  ttPointer->update<tt::THREAD_SAFE>(hashKey, tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				else ttPointer->update<tt::UNSAFE>(hashKey, tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				return SCORE_BC(score);
			}
			ZWS = true;
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					nodeType = tt::EXACT;
					alpha = score;
					pv[0] = move;
					bestMoveIndex = moveIndex;
					memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1) * sizeof(Move));
				}
			}
		}
	}
	if (bestMoveIndex >= 0 && pos.IsQuiet(pv[0])) updateCutoffStats(tlData, pv[0], depth, pos, bestMoveIndex);
	//Update transposition table
	if (T != ThreadType::SINGLE) ttPointer->update<tt::THREAD_SAFE>(hashKey, tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	else ttPointer->update<tt::UNSAFE>(hashKey, tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	return SCORE_EXACT(bestScore);
}

template<ThreadType T> Value NemorinoSearch::QSearch(Value alpha, Value beta, Position& pos, int depth, ThreadData& tlData) {
	if (T != ThreadType::SLAVE) {
		++QNodeCount;
		++NodeCount;
		MaxDepth = std::max(MaxDepth, pos.GetPliesFromRoot());
		if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
	}
	if (Stopped()) return VALUE_ZERO;
	if (pos.GetResult() != Result::OPEN)  return SCORE_FINAL(pos.evaluateFinalPosition());
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return SCORE_MDP(alpha);
	Move ttMove = MOVE_NONE;
	//TT lookup
	tt::Entry ttEntry;
	bool ttFound = false;
	Value ttValue = VALUE_NOTYETDETERMINED;
	tt::Entry* ttPointer = nullptr;
	if (use_tt_in_qsearch) {
		ttPointer = (T == ThreadType::SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
		if (ttFound) ttValue = tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot());
		if (ttFound
			&& ttEntry.depth() >= depth
			&& ttValue != VALUE_NOTYETDETERMINED
			&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
			return SCORE_TT(ttValue);
		}
		if (ttFound && ttEntry.move()) ttMove = ttEntry.move();
	}
	const bool WithChecks = depth == 0 || pos.mateThread();
	const bool checked = pos.Checked();
	Value standPat = VALUE_NOTYETDETERMINED;
	Value staticEval = VALUE_NOTYETDETERMINED;
	if (!checked) {
		standPat = ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate();
		staticEval = standPat;
		//check if ttValue is better
		if (ttFound && ttValue != VALUE_NOTYETDETERMINED && ((ttValue > standPat && ttEntry.type() == tt::LOWER_BOUND) || (ttValue < standPat && ttEntry.type() == tt::UPPER_BOUND))) standPat = ttValue;
		if (standPat >= beta) {
			if (use_tt_in_qsearch) {
				if (T == ThreadType::SINGLE) ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, MOVE_NONE, staticEval);
				else ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, MOVE_NONE, staticEval);
			}
			return SCORE_SP(beta);
		}
		//Delta Pruning
		if (!pos.GetMaterialTableEntry()->IsLateEndgame()) {
			const Value delta = settings::parameter.PieceValues[pos.GetMostValuableAttackedPieceType()].egScore + int(pos.PawnOn7thRank()) * (settings::parameter.PieceValues[QUEEN].egScore - settings::parameter.PieceValues[PAWN].egScore) + settings::parameter.DELTA_PRUNING_SAFETY_MARGIN;
			if (standPat + delta < alpha) return SCORE_DP(alpha);
		}
		alpha = std::max(alpha, standPat);
	}
	if (WithChecks) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttMove);
	else pos.InitializeMoveIterator<QSEARCH>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttMove);
	Move move;
	Value score = standPat;
	tt::NodeType nt = tt::UPPER_BOUND;
	Move bestMove = MOVE_NONE;
	//Value FBase = checked ? -VALUE_INFINITE : static_cast<Value>(score + 64);
	Value bestScore = score;
	while ((move = pos.NextMove())) {
		if (!checked) {
			if (depth > settings::parameter.LIMIT_QSEARCH) {
				if (pos.SEE_Sign(move) < VALUE_ZERO) continue;
			}
			else return standPat + pos.SEE(move);
		}
		Position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<T>(-beta, -alpha, next, depth - 1, tlData);
			if (score >= beta) {
				if (use_tt_in_qsearch) {
					if (T != ThreadType::SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, move, staticEval);
					else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, move, staticEval);
				}
				return SCORE_BC(beta);
			}
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha) {
					nt = tt::EXACT;
					alpha = score;
					bestMove = move;
				}
			}
		}
	}
	if (use_tt_in_qsearch) {
		if (T != ThreadType::SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), bestScore, nt, depth, bestMove, staticEval);
		else ttPointer->update<tt::UNSAFE>(pos.GetHash(), bestScore, nt, depth, bestMove, staticEval);
	}
	return SCORE_EXACT(bestScore);
}
