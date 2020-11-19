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

#include "types.h"
#include "board.h"
#include "position.h"

int scaleEG(const Position& pos) noexcept;

inline Eval Contempt;

struct Evaluation
{
public:
	Eval Material = EVAL_ZERO;
	Eval Mobility = EVAL_ZERO;
	Eval Threats = EVAL_ZERO;
	Eval KingSafety = EVAL_ZERO;
	Eval Pieces = EVAL_ZERO;
	Eval PsqEval = EVAL_ZERO;
	//Eval Space = EVAL_ZERO;
	Eval PawnStructure = EVAL_ZERO;

	inline Value GetScore(const Position & pos) noexcept {
		Eval total = Material + Mobility + KingSafety + Threats + Pieces + PawnStructure + PsqEval + Contempt;
		total.egScore = Value((scaleEG(pos) * int(total.egScore)) / 128);
		return total.getScore(pos.GetMaterialTableEntry()->Phase) * (1 - 2 * pos.GetSideToMove());
	}
};

Value evaluateDraw(const Position& pos) noexcept;
Value evaluateFromScratch(const Position& pos);
Eval evaluateMobility(const Position& pos) noexcept;
Eval evaluateKingSafety(const Position& pos) noexcept;
Value evaluatePawnEnding(const Position& pos);
template <Color COL> Eval evaluateThreats(const Position& pos) noexcept;
template <Color COL> Eval evaluatePieces(const Position& pos) noexcept;
std::string printDefaultEvaluation(const Position& pos);
std::string printDbgDefaultEvaluation(const Position& pos);

const int PSQ_GoForMate[64] = {
	100, 90, 80, 70, 70, 80, 90, 100,
	90, 70, 60, 50, 50, 60, 70, 90,
	80, 60, 40, 30, 30, 40, 60, 80,
	70, 50, 30, 20, 20, 30, 50, 70,
	70, 50, 30, 20, 20, 30, 50, 70,
	80, 60, 40, 30, 30, 40, 60, 80,
	90, 70, 60, 50, 50, 60, 70, 90,
	100, 90, 80, 70, 70, 80, 90, 100
};

const int BonusDistance[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

template <Color WinningSide> Value easyMate(const Position& pos) noexcept {
	Value result = pos.GetMaterialScore();
	if (WinningSide == WHITE) {
		result += VALUE_KNOWN_WIN;
		result += Value(PSQ_GoForMate[pos.KingSquare(BLACK)]);
		result += Value(BonusDistance[ChebishevDistance(pos.KingSquare(BLACK), pos.KingSquare(WHITE))]);
	}
	else {
		result -= VALUE_KNOWN_WIN;
		result -= Value(PSQ_GoForMate[pos.KingSquare(WHITE)]);
		result -= Value(BonusDistance[ChebishevDistance(pos.KingSquare(BLACK), pos.KingSquare(WHITE))]);
	}

	return result * (1 - 2 * pos.GetSideToMove());
}

const int PSQ_MateInCorner[64] = {
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};


template <Color COL> Eval evaluateThreats(const Position& pos) noexcept {
	enum { Defended, Weak };
	enum { Minor, Major };
	Bitboard b, weak, defended;
	Eval result = EVAL_ZERO;
	const Color OTHER = Color(COL ^ 1);
	// Non-pawn enemies defended by a pawn
	defended = (pos.ColorBB(OTHER) ^ pos.PieceBB(PAWN, OTHER)) & pos.AttacksByPieceType(OTHER, PAWN);
	// Add a bonus according to the kind of attacking pieces
	if (defended)
	{
		b = defended & (pos.AttacksByPieceType(COL, KNIGHT) | pos.AttacksByPieceType(COL, BISHOP));
		while (b) {
			result += settings::parameter.Threat[Defended][Minor][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
		b = defended & pos.AttacksByPieceType(COL, ROOK);
		while (b) {
			result += settings::parameter.Threat[Defended][Major][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
	}
	// Enemies not defended by a pawn and under our attack
	weak = pos.ColorBB(OTHER)
		& ~pos.AttacksByPieceType(OTHER, PAWN)
		& pos.AttacksByColor(COL);
	// Add a bonus according to the kind of attacking pieces
	if (weak)
	{
		b = weak & (pos.AttacksByPieceType(COL, KNIGHT) | pos.AttacksByPieceType(COL, BISHOP));
		while (b) {
			result += settings::parameter.Threat[Weak][Minor][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
		b = weak & (pos.AttacksByPieceType(COL, ROOK) | pos.AttacksByPieceType(COL, QUEEN));
		while (b) {
			result += settings::parameter.Threat[Weak][Major][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
		b = weak & ~pos.AttacksByColor(OTHER);
		if (b) result += settings::parameter.HANGING * popcount(b);
		b = weak & pos.AttacksByPieceType(COL, KING);
		if (b) result += (b & (b - 1)) ? settings::parameter.KING_ON_MANY : settings::parameter.KING_ON_ONE;
	}
	return result;
}

template <Color COL> Eval evaluatePieces(const Position& pos) noexcept {
	const Color OTHER = Color(COL ^ 1);
		//Rooks
	const Bitboard rooks = pos.PieceBB(ROOK, COL);
	Eval bonusRook = EVAL_ZERO;
	if (rooks) {
		//bonusRook = popcount(rooks & seventhRank) * ROOK_ON_7TH;
		const Bitboard bbHalfOpen = FileFill(pos.GetPawnEntry()->halfOpenFiles[COL]);
		const Bitboard rooksOnSemiOpen = bbHalfOpen & rooks;
		bonusRook += popcount(rooksOnSemiOpen) * settings::parameter.ROOK_ON_SEMIOPENFILE;
		bonusRook += 2 * popcount(FileFill(pos.GetPawnEntry()->openFiles) & rooks) * settings::parameter.ROOK_ON_OPENFILE;
		const Bitboard bbRookRays = FileFill(pos.PieceBB(ROOK, COL));
		bonusRook += popcount(bbRookRays & (pos.PieceBB(QUEEN, OTHER) | pos.PieceBB(KING, OTHER))) * Eval(5, 0);
	}
	//Passed Pawns (passed pawn bonus is already assigned statically in pawn::probe. Nevertheless all aspects related to position of other pieces have to be 
	//evaluated here) dynamically 
	const Square ownKingSquare = pos.KingSquare(COL);
	const Square opponentKingSquare = pos.KingSquare(OTHER);
	//Deplaced Knights (far away from all kings)
#pragma warning(suppress: 26496)
	Eval bonusKnight = EVAL_ZERO;
	Bitboard knights = pos.PieceBB(KNIGHT, COL);
	while (knights) {
		const Square knightSquare = lsb(knights);
		const int distance = std::min(Distance[knightSquare][ownKingSquare]-2, Distance[knightSquare][opponentKingSquare]-4);
		if (distance >= 1) {
			bonusKnight.mgScore -= static_cast<Value>(distance * settings::parameter.MALUS_KNIGHT_DISLOCATED);
		}
		knights &= knights - 1;
	}
	Eval bonusPassedPawns = EVAL_ZERO;
	Bitboard passedPawns = pos.GetPawnEntry()->passedPawns & pos.ColorBB(COL);
	while (passedPawns) {
		const Square pawnSquare = lsb(passedPawns);
		const Square blockSquare = COL == WHITE ? Square(pawnSquare + 8) : Square(pawnSquare - 8);
		const uint8_t dtc = MovesToConversion<COL>(pawnSquare);
		const int dtcSquare = (6 - dtc) * (6 - dtc);
		bonusPassedPawns.egScore += Value(2 * dtcSquare * ChebishevDistance(opponentKingSquare, blockSquare));
		bonusPassedPawns.egScore -= Value(1 * dtcSquare * ChebishevDistance(ownKingSquare, blockSquare));
		if ((pos.ColorBB(OTHER) & ToBitboard(blockSquare))!= EMPTY) bonusPassedPawns -= settings::parameter.MALUS_BLOCKED[dtc-1];
		passedPawns &= passedPawns - 1;
	}
	const Eval malusLostCastles = (pos.GetMaterialTableEntry()->Phase < 128 && pos.HasCastlingLost(COL)) ? settings::parameter.MALUS_LOST_CASTLING : EVAL_ZERO;
	return bonusPassedPawns + bonusKnight + bonusRook - malusLostCastles;
}
