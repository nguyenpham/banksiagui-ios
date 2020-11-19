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

#include <map>
#include <memory>
#include "types.h"
#include "board.h"
#include "utils.h"

constexpr int PV_MAX_LENGTH = 32; //Maximum Length of displayed Principal Variation
constexpr int MASK_TIME_CHECK = (1 << 14) - 1; //Time is only checked each MASK_TIME_CHECK nodes

constexpr int PAWN_TABLE_SIZE = 1 << 14; //has to be power of 2
constexpr int KILLER_TABLE_SIZE = 1 << 11; //has to be power of 2

////Bonus for Passed Pawns (dynamic evaluations)
//const Eval BONUS_PASSED_PAWN_BACKED = Eval(0, 15); //Passed pawn with own rook behind
//const Eval MALUS_PASSED_PAWN_BLOCKED[6] = { Eval(0, 3), Eval(0,10), Eval(0,10), Eval(0,30), EVAL_ZERO, Eval(0,30) }; //Passed pawn with own rook behind

namespace settings {

	enum CAPTURES {
		QxP, BxP, NxP, RxP, QxN,
		QxR, RxN, QxB, BxN, RxB,
		KxP, EP, PxP, KxN, PxB,
		BxR, PxN, NxR, NxN, NxB,
		RxR, BxB, PxR, KxB, KxR,
		QxQ,
		RxQ, BxQ, NxQ, PxQ, KxQ //Winning Captures of Queen
	};

	enum VerbosityLevel {   
		MINIMAL,  //No non-mandatory info messages
		LOW,      //No currmove info messages
		DEFAULT,  //default
		MAXIMUM   //info messages at all depths
	};

	struct Parameters {
	public:
		Parameters() noexcept;
		int LMRReduction(int depth, int moveNumber) noexcept;

		bool UseNNUE = false;
		std::string NNUEFile = DEFAULT_NET;
		int NNScaleFactor = 95;
		Value NNEvalLimit = Value(200);

		int HelperThreads = 0;
		Value Contempt = Value(10);
		Color EngineSide = WHITE;
		int EmergencyTime = 0;
		VerbosityLevel Verbosity = VerbosityLevel::DEFAULT;
		bool extendedOptions = false;
		bool USE_TT_IN_QSEARCH = true;
		Eval SCALE_BISHOP_PAIR_WITH_PAWNS = EVAL_ZERO; //Reduce Bonus Bishop Pair by this value for each pawn on the board
		Eval BONUS_BISHOP_PAIR_NO_OPP_MINOR = Eval(10); //Bonus for Bishop pair, if opponent has no minor piece for exchange
		Eval SCALE_EXCHANGE_WITH_PAWNS = EVAL_ZERO; //Decrease Value of Exchange with number of pawns
		Eval SCALE_EXCHANGE_WITH_MAJORS = EVAL_ZERO; //Decrease Value of Exchange with number of majors
		Eval PAWN_STORM[4] = { Eval(10, 0), Eval(25, 0), Eval(15, 0), Eval(5, 0) };
		Value BETA_PRUNING_FACTOR = Value(95);
		inline Value BetaPruningMargin(int depth) noexcept { return Value(depth * BETA_PRUNING_FACTOR); }
		Value RAZORING_FACTOR = Value(50);
		Value RAZORING_OFFSET = Value(50);
		inline Value RazoringMargin(int depth) noexcept { return Value(depth * RAZORING_FACTOR + RAZORING_OFFSET); }
		int LIMIT_QSEARCH = -3;
		Eval HANGING = Eval(16, 13);
		Eval KING_ON_ONE = Eval(1, 29);
		Eval KING_ON_MANY = Eval(3, 63);
		Eval ROOK_ON_OPENFILE = Eval(15, 0);
		Eval ROOK_ON_SEMIOPENFILE = Eval(15, 0);
		Eval ROOK_ON_7TH = Eval(20, 0);
		Eval MALUS_LOST_CASTLING = Eval(50, -50);

		Eval BONUS_BISHOP_PAIR = Eval(50);
		Value DELTA_PRUNING_SAFETY_MARGIN = Value(VALUE_100CP);
		Eval PAWN_SHELTER_2ND_RANK = Eval(30, -10);
		Eval PAWN_SHELTER_3RD_RANK = Eval(15, -8);
		Eval PAWN_SHELTER_4TH_RANK = Eval(8, -4);
		Eval PAWN_SHELTER[9] = { PAWN_SHELTER_2ND_RANK / 2,  3*PAWN_SHELTER_2ND_RANK/2, PAWN_SHELTER_2ND_RANK,
								 PAWN_SHELTER_3RD_RANK / 2, 3 * PAWN_SHELTER_3RD_RANK / 2, PAWN_SHELTER_3RD_RANK,
								 PAWN_SHELTER_4TH_RANK, Eval(0), Eval(0) };
		Value PROBCUT_MARGIN = Value(90);
		Eval PieceValues[7]{ Eval(1025), Eval(490, 550), Eval(325), Eval(325), Eval(80, 100), Eval(VALUE_KNOWN_WIN), Eval(0) };
		const Value SEE_VAL[7] = { Value(1025), Value(490), Value(325), Value(325), Value(80), VALUE_KNOWN_WIN, VALUE_DRAW };
		int FULTILITY_PRUNING_DEPTH = 3;
		Value FUTILITY_PRUNING_LIMIT[4] = { VALUE_ZERO, static_cast<Value>(325), static_cast<Value>(490), static_cast<Value>(1025) };
		Value PAWN_REDUCTION_LIMIT = static_cast<Value>(33);
		Eval PSQT[12][64]{
			{  // White Queens
				Eval(-10,-8), Eval(-7,-5), Eval(-4,-2), Eval(-2,0), Eval(-2,0), Eval(-4,-2), Eval(-7,-5), Eval(-10,-8),  // Rank 1
				Eval(-4,-5), Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(3,2), Eval(0,0), Eval(-2,-2), Eval(-4,-5),  // Rank 2
				Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(5,5), Eval(5,5), Eval(3,2), Eval(0,0), Eval(-2,-2),  // Rank 3
				Eval(0,0), Eval(3,2), Eval(5,5), Eval(8,8), Eval(8,8), Eval(5,5), Eval(3,2), Eval(0,0),  // Rank 4
				Eval(0,0), Eval(3,2), Eval(5,5), Eval(8,8), Eval(8,8), Eval(5,5), Eval(3,2), Eval(0,0),  // Rank 5
				Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(5,5), Eval(5,5), Eval(3,2), Eval(0,0), Eval(-2,-2),  // Rank 6
				Eval(-4,-5), Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(3,2), Eval(0,0), Eval(-2,-2), Eval(-4,-5),  // Rank 7
				Eval(-7,-8), Eval(-4,-5), Eval(-2,-2), Eval(0,0), Eval(0,0), Eval(-2,-2), Eval(-4,-5), Eval(-7,-8)  // Rank 8
			},
			{  // Black Queen
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0)
			},
			{  // White Rook
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0),  // Rank 1
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0),  // Rank 2
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0),  // Rank 3
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0),  // Rank 4
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0),  // Rank 5
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0),  // Rank 6
				Eval(3,0), Eval(3,0), Eval(4,0), Eval(6,0), Eval(6,0), Eval(4,0), Eval(3,0), Eval(3,0),  // Rank 7
				Eval(-1,0), Eval(-1,0), Eval(0,0), Eval(1,0), Eval(1,0), Eval(0,0), Eval(-1,0), Eval(-1,0)  // Rank 8
			},
			{  // Black Rook
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0)
			},
			{  // White Bishop
				Eval(-12,-8), Eval(-10,-5), Eval(-7,-2), Eval(-4,0), Eval(-4,0), Eval(-7,-2), Eval(-10,-5), Eval(-12,-8),  // Rank 1
				Eval(-4,-5), Eval(3,-2), Eval(0,0), Eval(3,2), Eval(3,2), Eval(0,0), Eval(3,-2), Eval(-4,-5),  // Rank 2
				Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(5,5), Eval(5,5), Eval(3,2), Eval(0,0), Eval(-2,-2),  // Rank 3
				Eval(0,0), Eval(3,2), Eval(5,5), Eval(8,8), Eval(8,8), Eval(5,5), Eval(3,2), Eval(0,0),  // Rank 4
				Eval(0,0), Eval(5,2), Eval(5,5), Eval(8,8), Eval(8,8), Eval(5,5), Eval(5,2), Eval(0,0),  // Rank 5
				Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(5,5), Eval(5,5), Eval(3,2), Eval(0,0), Eval(-2,-2),  // Rank 6
				Eval(-4,-5), Eval(-2,-2), Eval(0,0), Eval(3,2), Eval(3,2), Eval(0,0), Eval(-2,-2), Eval(-4,-5),  // Rank 7
				Eval(-7,-8), Eval(-4,-5), Eval(-2,-2), Eval(0,0), Eval(0,0), Eval(-2,-2), Eval(-4,-5), Eval(-7,-8)  // Rank 8
			},
			{  // Black Bishop
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0)
			},
			{  // White Knight
				Eval(-19,-18), Eval(-14,-12), Eval(-8,-7), Eval(-3,-2), Eval(-3,-2), Eval(-8,-7), Eval(-14,-12), Eval(-19,-18),  // Rank 1
				Eval(-14,-12), Eval(-8,-7), Eval(-3,-2), Eval(4,5), Eval(4,5), Eval(-3,-2), Eval(-8,-7), Eval(-14,-12),  // Rank 2
				Eval(-8,-7), Eval(-3,-2), Eval(4,5), Eval(15,16), Eval(15,16), Eval(4,5), Eval(-3,-2), Eval(-8,-7),  // Rank 3
				Eval(-3,-2), Eval(4,5), Eval(15,16), Eval(21,22), Eval(21,22), Eval(15,16), Eval(4,5), Eval(-3,-2),  // Rank 4
				Eval(-3,-2), Eval(4,5), Eval(26,16), Eval(31,22), Eval(31,22), Eval(26,16), Eval(4,5), Eval(-3,-2),  // Rank 5
				Eval(-8,-7), Eval(-3,-2), Eval(15,5), Eval(36,16), Eval(36,16), Eval(15,5), Eval(-3,-2), Eval(-8,-7),  // Rank 6
				Eval(-14,-12), Eval(-8,-7), Eval(-3,-2), Eval(4,5), Eval(4,5), Eval(-3,-2), Eval(-8,-7), Eval(-14,-12),  // Rank 7
				Eval(-19,-18), Eval(-14,-12), Eval(-8,-7), Eval(-3,-2), Eval(-3,-2), Eval(-8,-7), Eval(-14,-12), Eval(-19,-18)  // Rank 8
			},
			{  // Black Knight
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0)
			},
			{  // White Pawn
				Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0),  // Rank 1
				Eval(-16 ,-9), Eval(-12 ,-7), Eval(-8 ,-10), Eval(-8 ,-10), Eval(-7 ,-10), Eval(-5 ,-10), Eval(-9 ,-10), Eval(-18 ,-10),  // Rank 2
				Eval(-14 ,-9), Eval(-11 ,-8), Eval(-6 ,-8), Eval(0 ,-8), Eval(0 ,-8), Eval(-6 ,-9), Eval(-11 ,-8), Eval(-14 ,-8),  // Rank 3
				Eval(-15 ,-5), Eval(-8 ,-5), Eval(-6 ,-5), Eval(5 ,-5), Eval(7 ,-5), Eval(-6 ,-5), Eval(-8 ,-5), Eval(-16 ,-6),  // Rank 4
				Eval(-8 ,-2), Eval(-6 ,-2), Eval(-3 ,-2), Eval(10 ,-1), Eval(9 ,-2), Eval(-3 ,-2), Eval(-6 ,-2), Eval(-8 ,-2), // Rank 5
				Eval(4 ,11), Eval(7 ,10), Eval(10 ,10), Eval(12 ,10), Eval(12 ,10), Eval(10 ,10), Eval(7 ,10), Eval(4 ,10), // Rank 6
				Eval(23 ,21), Eval(23 ,21), Eval(23 ,21), Eval(23 ,21), Eval(23 ,21), Eval(23 ,21), Eval(23 ,21), Eval(23 ,21), // Rank 7
				Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0), Eval(0,0)  // Rank 8
			},
			{  //  Black Pawn
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0)
			},
			{  // White King
				Eval(16,-20), Eval(17,-15), Eval(16,-9), Eval(14,-4), Eval(14,-4), Eval(16,-9), Eval(17,-15), Eval(16,-20),  // Rank 1
				Eval(16,-15), Eval(16,-9), Eval(14,-4), Eval(11,7), Eval(11,7), Eval(14,-4), Eval(16,-9), Eval(16,-15),  // Rank 2
				Eval(11,-9), Eval(11,-4), Eval(11,7), Eval(8,23), Eval(8,23), Eval(11,7), Eval(11,-4), Eval(11,-9),  // Rank 3
				Eval(8,-4), Eval(8,7), Eval(3,23), Eval(-2,28), Eval(-2,28), Eval(3,23), Eval(8,7), Eval(8,-4),  // Rank 4
				Eval(3,-4), Eval(0,7), Eval(-2,23), Eval(-7,28), Eval(-7,28), Eval(-2,23), Eval(0,7), Eval(3,-4),  // Rank 5
				Eval(-7,-9), Eval(-7,-4), Eval(-12,7), Eval(-18,23), Eval(-18,23), Eval(-12,7), Eval(-7,-4), Eval(-7,-9),  // Rank 6
				Eval(-12,-15), Eval(-12,-9), Eval(-18,-4), Eval(-18,7), Eval(-18,7), Eval(-18,-4), Eval(-12,-9), Eval(-12,-15),  // Rank 7
				Eval(-18,-20), Eval(-18,-15), Eval(-18,-9), Eval(-18,-4), Eval(-18,-4), Eval(-18,-9), Eval(-18,-15), Eval(-18,-20)  // Rank 8
			},
			{  // Black King
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0),
				Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0), Eval(0)
			}
		};
		Value CAPTURE_SCORES[6][7] = {
			// Captured:  QUEEN, ROOK,   BISHOP,   KNIGHT,    PAWN,   King,   EP-Capture/Promotion
			{ Value(QxQ), Value(QxR),  Value(QxB),  Value(QxN),  Value(QxP),  Value(0),  Value(0) },   // QUEEN
			{ Value(RxQ), Value(RxR), Value(RxB), Value(RxN),  Value(RxP),  Value(0),  Value(0) },   // ROOK
			{ Value(BxQ), Value(BxR), Value(BxB), Value(BxN),  Value(BxP),  Value(0),  Value(0) },   // BISHOP
			{ Value(NxQ), Value(NxR), Value(NxB), Value(NxN), Value(NxP),  Value(0),  Value(0) },   // KNIGHT
			{ Value(PxQ), Value(PxR), Value(PxB), Value(PxN), Value(PxP),  Value(0), Value(EP) },   // PAWN
			{ Value(KxQ), Value(KxR), Value(KxB), Value(KxN), Value(KxP), Value(0),  Value(0) }    // KING
		};
		Eval MOBILITY_BONUS_KNIGHT[9] = { Eval(-29, -23), Eval(-19, -14), Eval(-4, -4), Eval(1, 0), Eval(7, 4), Eval(12, 9), Eval(16, 12), Eval(19, 14), Eval(21, 15) };
		Eval MOBILITY_BONUS_BISHOP[14] = { Eval(-23, -22), Eval(-12, -11), Eval(2, 0), Eval(9, 7), Eval(15, 14), Eval(22, 19), Eval(28, 25), Eval(32, 29), Eval(33, 32),
			Eval(36, 33), Eval(37, 35), Eval(37, 36), Eval(39, 36), Eval(40, 37) };
		Eval MOBILITY_BONUS_ROOK[15] = { Eval(-22, -25), Eval(-14, -12), Eval(-2, 0), Eval(0, 7), Eval(2, 15), Eval(5, 22), Eval(8, 29), Eval(9, 37), Eval(12, 44),
			Eval(14, 50), Eval(14, 53), Eval(15, 56), Eval(16, 57), Eval(16, 57), Eval(16, 58) };
		Eval MOBILITY_BONUS_QUEEN[28] = { Eval(-19, -18), Eval(-12, -11), Eval(-2, -2), Eval(0, 0), Eval(2, 4), Eval(5, 8), Eval(5, 14), Eval(8, 18), Eval(9, 18), Eval(9, 19),
			Eval(9, 19), Eval(9, 19), Eval(9, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19),
			Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19), Eval(11, 19) };
		// Threat[defended/weak][minor/major attacking][attacked PieceType] contains
		// bonuses according to which piece type attacks which one.
		// "inspired" by SF
		Eval Threat[2][2][6] = {
			{ { Eval(0, 0), Eval(0, 0), Eval(10, 18), Eval(12, 19), Eval(22, 49), Eval(18, 53) }, // Defended Minor
			{ Eval(0, 0), Eval(0, 0), Eval(5, 7), Eval(5, 7), Eval(4, 7), Eval(12, 24) } }, // Defended Major
			{ { Eval(0, 0), Eval(0, 16), Eval(17, 21), Eval(16, 25), Eval(21, 50), Eval(18, 52) }, // Weak Minor
			{ Eval(0, 0), Eval(0, 14), Eval(13, 29), Eval(13, 29), Eval(0, 22), Eval(12, 26) } } // Weak Major
		};
		Eval PASSED_PAWN_BONUS[6] = { Eval(0), Eval(0), Eval(8), Eval(35), Eval(81), Eval(140) };
		Eval MALUS_BLOCKED[6] = { Eval(30), Eval(20), Eval(10), Eval(5), Eval(0), Eval(0) }; //indexed by distance to conversion
		Eval BONUS_PROTECTED_PASSED_PAWN[6] = { Eval(0), Eval(0), Eval(14), Eval(8), Eval(3), Eval(55) };
		Eval MALUS_ISOLATED_PAWN = Eval(5);
		Eval MALUS_BACKWARD_PAWN = Eval(14);
		Eval MALUS_WEAK_PAWN = Eval(6);
		Eval MALUS_ISLAND_COUNT = Eval(5);
		Eval BONUS_CANDIDATE = Eval(5);
		Eval BONUS_LEVER = Eval(5);
		Eval MALUS_DOUBLED_PAWN = Eval(0, 20);
		int TBProbeDepth = 1;
		int TBProbeLimit = 7;
		int ATTACK_WEIGHT[4] = { 53, 82, 23, 17 }; //Indexed by Piece Type
		int SAFE_CHECK[4] = { 800, 900, 450, 800 };
		int KING_DANGER_SCALE = 14;
		int KING_RING_ATTACK_FACTOR = 142;
		int WEAK_SQUARES_FACTOR = 200;
		int PINNED_FACTOR = 125;
		int ATTACK_WITH_QUEEN = 900;
		int BONUS_LEVER_ON_KINGSIDE = 10;
		Value MALUS_KNIGHT_DISLOCATED = Value(4);

		const Eval MOBILITY_CENTER_KNIGHT = Eval(3, 0);
		const Eval MOBILITY_CENTER_EXTENDED_KNIGHT = Eval(3, 0);

		Eval BONUS_TEMPO = Eval(10, 5);

		Eval IMBALANCE_Q_vs_RN = Eval(75);
		Eval IMBALANCE_Q_vs_RB = Eval(75);

		void UCIExpose();
		void SetFromUCI(std::string name, std::string value);

		void setParam(std::string key, std::string value);
	private:
		void Initialize();
		int LMR_REDUCTION[64][64];
	};

	extern Parameters parameter;

	void Initialize();

	enum OptionType { SPIN, CHECK, BUTTON, STRING };

	const std::string OPTION_HASH = "Hash";
	const std::string OPTION_CHESS960 = "UCI_Chess960";
	const std::string OPTION_CLEAR_HASH = "Clear Hash";
	const std::string OPTION_PRINT_OPTIONS = "Print Options";
	const std::string OPTION_CONTEMPT = "Contempt";
	const std::string OPTION_MULTIPV = "MultiPV";
	const std::string OPTION_THREADS = "Threads";
	const std::string OPTION_PONDER = "Ponder";
	const std::string OPTION_BOOK_FILE = "BookFile";
	const std::string OPTION_OWN_BOOK = "OwnBook";
	const std::string OPTION_OPPONENT = "UCI_Opponent";
	const std::string OPTION_EMERGENCY_TIME = "MoveOverhead";
	const std::string OPTION_NODES_TIME = "Nodestime"; //Nodes per millisecond
	const std::string OPTION_SYZYGY_PATH = "SyzygyPath";
	const std::string OPTION_SYZYGY_PROBE_DEPTH = "SyzygyProbeDepth";
	const std::string OPTION_SYZYGY_PROBE_LIMIT = "SyzygyProbeLimit";
	const std::string OPTION_VERBOSITY = "Verbosity";
	const std::string OPTION_EVAL_FILE = "EvalFile";
	const std::string OPTION_USE_NNUE = "UseNNUE";
	const std::string OPTION_NNEVAL_SCALE_FACTOR = "NNEvalScaleFactor";
	const std::string OPTION_NNEVAL_LIMIT = "NNEvalLimit";

	const std::string DEFAULT_SYZYGY_PATH = "<4men internal>";

	class Option {
	public:
		Option() noexcept { otype = OptionType::STRING; };
		Option(std::string Name, OptionType Type = OptionType::BUTTON, std::string DefaultValue = "", std::string MinValue = "", std::string MaxValue = "", bool Technical = false);
		virtual ~Option() { };
		Option(const Option&) = default;
		Option& operator=(const Option&) = default;
		Option(Option&&) = default;
		Option& operator=(Option&&) = default;
		void virtual set(std::string value) = 0;
		void virtual read(std::vector<std::string>& tokens) = 0;
		std::string printUCI();
		std::string printInfo() const;
		inline std::string getName() { return name; }
		inline bool isTechnical() noexcept { return technical; }
		inline OptionType getType()  noexcept { return otype; }
	protected:
		std::string name;
		OptionType otype;
		std::string defaultValue;
		std::string minValue;
		std::string maxValue;
		bool technical = false;
	};



	class OptionSpin : public Option {
	public:
		OptionSpin(std::string Name, int Value, int Min, int Max, bool Technical = false) : Option(Name, OptionType::SPIN, std::to_string(Value), std::to_string(Min), std::to_string(Max), Technical) { };
		virtual ~OptionSpin() { };
		OptionSpin(const OptionSpin&) = default;
		OptionSpin& operator=(const OptionSpin&) = default;
		OptionSpin(OptionSpin&&) = default;
		OptionSpin& operator=(OptionSpin&&) = default;
		inline void set(std::string value) override { _value = stoi(value); }
		inline virtual void set(int value) noexcept { _value = value; }
		inline void setDefault(std::string value) { defaultValue = value; }
		inline void setDefault(int value) { defaultValue = std::to_string(value); }
		inline int getValue() {
			if (_value == INT_MIN) set(defaultValue);
			return _value;
		}
		inline void read(std::vector<std::string>& tokens) override { set(tokens[4]); }
	protected:
		int _value = INT_MIN;
	};

	class OptionCheck : public Option {
	public:
		OptionCheck(std::string Name, bool value, bool Technical = false);
		virtual ~OptionCheck() { };
		OptionCheck(const OptionCheck&) = default;
		OptionCheck& operator=(const OptionCheck&) = default;
		OptionCheck(OptionCheck&&) = default;
		OptionCheck& operator=(OptionCheck&&) = default;
		void set(std::string value)  noexcept override;
		inline bool getValue()  noexcept { return _value; }
		inline void read(std::vector<std::string>& tokens) override { set(tokens[4]); }
		inline virtual void set(bool value) noexcept { _value = value; };
	protected:
		bool _value = true;
	};

	class OptionString : public Option {
	public:
		OptionString(std::string Name, std::string defaultValue = "", bool Technical = false) : Option(Name, OptionType::STRING, defaultValue, "", "", Technical) { _value = defaultValue; };
		virtual ~OptionString() { };
		OptionString(const OptionString&) = default;
		OptionString& operator=(const OptionString&) = default;
		OptionString(OptionString&&) = default;
		OptionString& operator=(OptionString&&) = default;
		void set(std::string value) override { _value = value; }
		inline std::string getValue() { return _value; }
		void read(std::vector<std::string>& tokens) override;
	protected:
		std::string _value;
	};

	class OptionButton : public Option {
	public:
		OptionButton(std::string Name, bool Technical = false) : Option(Name, OptionType::BUTTON, "", "", "", Technical) { };
		virtual ~OptionButton() { };
		OptionButton(const OptionButton&) = default;
		OptionButton& operator=(const OptionButton&) = default;
		OptionButton(OptionButton&&) = default;
		OptionButton& operator=(OptionButton&&) = default;
		void set(std::string value) noexcept override { }
#pragma warning(disable:4100) 
		inline void read(std::vector<std::string>& tokens) noexcept override { }
#pragma warning(default:4100) 
	};

	class OptionThread : public OptionSpin {
	public:
		OptionThread() noexcept : OptionSpin(OPTION_THREADS, parameter.HelperThreads + 1, 1, 256) { };
		virtual ~OptionThread() { };
		OptionThread(const OptionThread&) = default;
		OptionThread& operator=(const OptionThread&) = default;
		OptionThread(OptionThread&&) = default;
		OptionThread& operator=(OptionThread&&) = default;
		void set(std::string value) override;
	};

	class Option960 : public OptionCheck {
	public:
		Option960()  noexcept : OptionCheck(OPTION_CHESS960, Chess960) { };
		virtual ~Option960() { };
		Option960(const Option960&) = default;
		Option960& operator=(const Option960&) = default;
		Option960(Option960&&) = default;
		Option960& operator=(Option960&&) = default;
		void set(std::string value)  noexcept override;
		inline void set(bool value) noexcept override { Chess960 = value; };
	};

	class OptionContempt : public OptionSpin {
	public:
		OptionContempt() noexcept : OptionSpin(OPTION_CONTEMPT, parameter.Contempt, -1000, 1000) { };
		virtual ~OptionContempt() = default;
		OptionContempt(const OptionContempt&) = default;
		OptionContempt& operator=(const OptionContempt&) = default;
		OptionContempt(OptionContempt&&) = default;
		OptionContempt& operator=(OptionContempt&&) = default;
		void set(std::string value) override;
		void set(int value) noexcept override;
	};

	class OptionHash : public OptionSpin {
	public:
		OptionHash() noexcept : OptionSpin(OPTION_HASH, 32, 1, 262144) { };
		virtual ~OptionHash() { };
		OptionHash(const OptionHash&) = default;
		OptionHash& operator=(const OptionHash&) = default;
		OptionHash(OptionHash&&) = default;
		OptionHash& operator=(OptionHash&&) = default;
		void set(std::string value) override;
		void set(int value)  noexcept override;
	};

	class Options : public std::map<std::string, Option*> {
	public:
		Options() noexcept;
		virtual ~Options();
		Options(const Options&) = default;
		Options& operator=(const Options&) = default;
		Options(Options&&) = default;
		Options& operator=(Options&&) = default;
		void printUCI();
		void printInfo();
		void read(std::vector<std::string>& tokens);
		int getInt(std::string key);
		bool getBool(std::string key);
		std::string getString(std::string key);
		void set(std::string key, std::string value);
		void set(std::string key, int value);
		void set(std::string key, bool value);
	private:
		void initialize();
	};

	extern Options options;

}