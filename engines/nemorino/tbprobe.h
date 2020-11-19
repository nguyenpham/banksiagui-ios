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

/*
  This file is an adapted copy from Stockfish (see
  https://github.com/official-stockfish/Stockfish), which is based on the
  code of Ronald de Man
*/

#ifndef TBPROBE_H
#define TBPROBE_H

#include <ostream>
#include <vector>
#include "types.h"
#include <unordered_map>

namespace tablebases {

	/// RootMove struct is used for moves at the root of the tree. For each root move
	/// we store a score and a PV (really a refutation in the case of moves which
	/// fail low). Score is normally set at -VALUE_INFINITE for all non-pv moves.

	struct RootMove {

		explicit RootMove(Move m) : pv(1, m) {}
		bool extract_ponder_from_tt(Position& pos);
		bool operator==(const Move& m) const noexcept { return pv[0] == m; }
		bool operator<(const RootMove& m) const noexcept { // Sort in descending order
			return m.score != score ? m.score < score
				: m.previousScore < previousScore;
		}

		Value score = -VALUE_INFINITE;
		Value previousScore = -VALUE_INFINITE;
		int selDepth = 0;
		int tbRank;
		Value tbScore;
		std::vector<Move> pv;
	};

	typedef std::vector<RootMove> RootMoves;

	enum WDLScore {
		WDLLoss = -2, // Loss
		WDLBlessedLoss = -1, // Loss, but draw under 50-move rule
		WDLDraw = 0, // Draw
		WDLCursedWin = 1, // Win, but draw under 50-move rule
		WDLWin = 2, // Win

		WDLScoreNone = -1000
	};

	// Possible states after a probing operation
	enum ProbeState {
		FAIL = 0, // Probe failed (missing file table)
		OK = 1, // Probe succesful
		CHANGE_STM = -1, // DTZ should check the other side
		ZEROING_BEST_MOVE = 2  // Best move zeroes DTZ (capture or pawn move)
	};

	extern int MaxCardinality;

	void init(const std::string& paths);
	WDLScore probe_wdl(Position& pos, ProbeState* result);
	int probe_dtz(Position& pos, ProbeState* result);
	bool root_probe(Position& pos, tablebases::RootMoves& rootMoves);
	bool root_probe_wdl(Position& pos, tablebases::RootMoves& rootMoves);
	bool rank_root_moves(Position& pos, tablebases::RootMoves& rootMoves);
	bool IsInTB(MaterialKey_t key) noexcept;

	inline std::ostream& operator<<(std::ostream& os, const WDLScore v) {

		os << (v == WDLLoss ? "Loss" :
			v == WDLBlessedLoss ? "Blessed loss" :
			v == WDLDraw ? "Draw" :
			v == WDLCursedWin ? "Cursed win" :
			v == WDLWin ? "Win" : "None");

		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const ProbeState v) {

		os << (v == FAIL ? "Failed" :
			v == OK ? "Success" :
			v == CHANGE_STM ? "Probed opponent side" :
			v == ZEROING_BEST_MOVE ? "Best move zeroes DTZ" : "None");

		return os;
	}

}

#endif
