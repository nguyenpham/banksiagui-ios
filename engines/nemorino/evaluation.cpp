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

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>
#include "evaluation.h"
#include "position.h"
#include "settings.h"

#ifdef STRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#endif

Value evaluateDefault(const Position& pos) {
	Evaluation result;
	result.Material = pos.GetMaterialTableEntry()->Evaluation;
	result.PsqEval = pos.GetPsqEval();
	if (settings::parameter.UseNNUE && std::abs((result.Material + result.PsqEval).egScore) < (static_cast<int>(settings::parameter.NNEvalLimit) * (8 + pos.GetDrawPlyCount()) / 8)
		&& (!Chess960 || pos.GetCastles() == 0)) {
		int v1 = pos.NNScore();
		return static_cast<Value>(v1);
	}
	else {
		result.Mobility = evaluateMobility(pos);
		result.KingSafety = evaluateKingSafety(pos);
		result.PawnStructure = pos.PawnStructureScore();
		result.Threats = 7 * (evaluateThreats<WHITE>(pos) - evaluateThreats<BLACK>(pos)) / 8;
		result.Pieces = evaluatePieces<WHITE>(pos) - evaluatePieces<BLACK>(pos);
#ifdef STRACE
		spdlog::info("{0}; ce \"{1}\"", pos.fen(true), result.GetScore(pos) * (1 - 2 * pos.GetSideToMove()));
#endif
		return result.GetScore(pos);
	}
}

std::string printDefaultEvaluation(const Position& pos) {
	std::stringstream ss;
	Evaluation result;
	result.Material = pos.GetMaterialTableEntry()->Evaluation;
	result.Mobility = evaluateMobility(pos);
	result.KingSafety = evaluateKingSafety(pos);
	result.PawnStructure = pos.PawnStructureScore();
	result.Threats = evaluateThreats<WHITE>(pos) - evaluateThreats<BLACK>(pos);
	result.Pieces = evaluatePieces<WHITE>(pos) - evaluatePieces<BLACK>(pos);
	result.PsqEval = pos.GetPsqEval();
	ss << "       Material: " << result.Material.print() << std::endl;
	ss << "       Mobility: " << result.Mobility.print() << std::endl;
	ss << "    King Safety: " << result.KingSafety.print() << std::endl;
	ss << " Pawn Structure: " << result.PawnStructure.print() << std::endl;
	ss << "Threats (White): " << evaluateThreats<WHITE>(pos).print() << std::endl;
	ss << "Threats (Black): " << evaluateThreats<BLACK>(pos).print() << std::endl;
	ss << " Pieces (White): " << evaluatePieces<WHITE>(pos).print() << std::endl;
	ss << " Pieces (Black): " << evaluatePieces<BLACK>(pos).print() << std::endl;
	ss << "           PSQT: " << pos.GetPsqEval().print() << std::endl;
	Value score = result.GetScore(pos);
	if (pos.GetSideToMove() == BLACK) score = -score;
	ss << "Total evaluation: " << static_cast<int>(score) << " (white side)" << std::endl;
	return ss.str();
}

std::string printDbgDefaultEvaluation(const Position& pos)
{
	std::stringstream ss;
	Evaluation result;
	result.Material = pos.GetMaterialTableEntry()->Evaluation;
	result.Mobility = evaluateMobility(pos);
	result.KingSafety = evaluateKingSafety(pos);
	result.PawnStructure = pos.PawnStructureScore();
	result.Threats = evaluateThreats<WHITE>(pos) - evaluateThreats<BLACK>(pos);
	result.Pieces = evaluatePieces<WHITE>(pos) - evaluatePieces<BLACK>(pos);
	result.PsqEval = pos.GetPsqEval();
	const int f = pos.GetSideToMove() == WHITE ? 1 : -1;
	ss << f * result.GetScore(pos) << "@";
	ss << "Material=" << result.Material.mgScore << ":" << result.Material.egScore << "@";
	ss << "Mobility=" << result.Mobility.mgScore << ":" << result.Mobility.egScore << "@";
	ss << "KingSafety=" << result.KingSafety.mgScore << ":" << result.KingSafety.egScore << "@";
	ss << "PawnStructure=" << result.PawnStructure.mgScore << ":" << result.PawnStructure.egScore << "@";
	ss << "Threats=" << result.Threats.mgScore << ":" << result.Threats.egScore << "@";
	ss << "Pieces=" << result.Pieces.mgScore << ":" << result.Pieces.egScore << "@";
	ss << "PSQT=" << result.PsqEval.mgScore << ":" << result.PsqEval.egScore;
	return ss.str();
}

Value evaluateDraw(const Position& pos) noexcept {
	return Contempt.getScore(pos.GetMaterialTableEntry()->Phase) * (1 - 2 * pos.GetSideToMove());;
}

Eval evaluateKingSafety(const Position& pos) noexcept {
	if (pos.GetMaterialTableEntry()->Phase > PHASE_LIMIT_ENDGAME) return EVAL_ZERO;
	Eval result;
	int attackerCount[2] = { 0, 0 };
	int attackWeight[2] = { 0, 0 };
	int kingZoneAttacks[2] = { 0, 0 };
	const Bitboard kingRing[2] = { pos.PieceBB(KING, WHITE) | KingAttacks[pos.KingSquare(WHITE)] , pos.PieceBB(KING, BLACK) | KingAttacks[pos.KingSquare(BLACK)] };
	Bitboard kingZone[2] = {
		(pos.PieceBB(KING, WHITE) & RANK1) != EMPTY ? kingRing[0] | (kingRing[0] << 8) : kingRing[0],
		(pos.PieceBB(KING, BLACK) & RANK8) != EMPTY ? kingRing[1] | (kingRing[1] >> 8) : kingRing[1]
	};
	if ((pos.KingSquare(WHITE) & 7) == 7) kingZone[0] |= kingRing[0] >> 1;  else if ((pos.KingSquare(WHITE) & 7) == 0)  kingZone[0] |= kingRing[0] << 1;
	if ((pos.KingSquare(BLACK) & 7) == 7)  kingZone[1] |= kingRing[1] >> 1; else if ((pos.KingSquare(BLACK) & 7) == 0) kingZone[1] |= kingRing[1] << 1;
	const Bitboard bbExclQueen = pos.OccupiedBB() & ~pos.PieceTypeBB(QUEEN);
	for (int c = static_cast<int>(Color::WHITE); c <= static_cast<int>(Color::BLACK); ++c) {
		const Color color_attacker = static_cast<Color>(c);
		for (PieceType pt = PieceType::QUEEN; pt <= PieceType::KNIGHT; ++pt) {
			Bitboard pieceBB = pos.PieceBB(pt, color_attacker);
			while (pieceBB) {
				const Square s = lsb(pieceBB);
				Bitboard bbKingZoneAttacks = pos.GetAttacksFrom(s);
				Bitboard bbPinned;
				if (pt < KNIGHT &&
					(bbPinned = pos.GetAttacksFrom(s) & pos.PinnedPieces(static_cast<Color>(c ^ 1))) != EMPTY &&
					(InBetweenFields[s][pos.KingSquare(static_cast<Color>(c ^ 1))] & bbPinned) != EMPTY) {
					bbKingZoneAttacks |= InBetweenFields[s][pos.KingSquare(static_cast<Color>(c ^ 1))];
				}
				bbKingZoneAttacks &= kingZone[c ^ 1];
				if (bbKingZoneAttacks) {
					++attackerCount[c];
					attackWeight[c] += settings::parameter.ATTACK_WEIGHT[pt];
					kingZoneAttacks[c] += popcount(bbKingZoneAttacks);
				}
				pieceBB &= pieceBB - 1;
			}
		}
		attackerCount[c] += popcount(kingZone[c ^ 1] & pos.AttacksByPieceType(color_attacker, PAWN));
	}
	int attackScore[2] = { 0, 0 };
	Value attackVals[2] = { VALUE_ZERO, VALUE_ZERO };
	for (int c = static_cast<int>(Color::WHITE); c <= static_cast<int>(Color::BLACK); ++c) {
		const Color color_defender = static_cast<Color>(c);
		const Color color_attacker = static_cast<Color>(c ^ 1);
		//if (attackerCount[c ^ 1] + static_cast<int>(pos.PieceBB(QUEEN, color_attacker) != EMPTY) < 2) continue;
		const Bitboard bbWeak = pos.AttacksByColor(color_attacker) & KingAttacks[pos.KingSquare(color_defender)] & ~pos.dblAttacks(color_defender);
		const Bitboard bbUndefended = pos.AttacksByColor(color_attacker) & ~pos.AttacksByColor(color_defender) & kingZone[color_defender] & ~pos.ColorBB(color_attacker);
		attackScore[c] = attackerCount[c ^ 1] * attackWeight[c ^ 1];
		attackScore[c] += settings::parameter.KING_RING_ATTACK_FACTOR * kingZoneAttacks[c ^ 1];
		attackScore[c] += settings::parameter.WEAK_SQUARES_FACTOR * popcount(bbWeak | bbUndefended);
		attackScore[c] += settings::parameter.PINNED_FACTOR * popcount(pos.PinnedPieces(color_defender) & pos.ColorBB(color_defender) & ~pos.PieceTypeBB(PAWN));
		//attackScore[c] += settings::parameter.PINNED_FACTOR * popcount(pos.PinnedPieces(color_defender));
		attackScore[c] -= settings::parameter.ATTACK_WITH_QUEEN * (pos.GetMaterialTableEntry()->GetMostExpensivePiece(color_attacker) != QUEEN);
		//Safe checks
		Bitboard bbSafe = (~pos.AttacksByColor(color_defender) | (bbWeak & pos.dblAttacks(color_attacker))) & ~pos.ColorBB(color_attacker);
		const Bitboard bbRookAttacks = RookTargets(pos.KingSquare(color_defender), bbExclQueen);
		const Bitboard bbBishopAttacks = BishopTargets(pos.KingSquare(color_defender), bbExclQueen);
		if ((bbRookAttacks | bbBishopAttacks) & pos.AttacksByPieceType(color_attacker, QUEEN) & bbSafe)
			attackScore[c] += settings::parameter.SAFE_CHECK[QUEEN];
		bbSafe |= pos.dblAttacks(color_attacker) & ~(pos.dblAttacks(color_defender) | pos.ColorBB(color_attacker)) & pos.AttacksByPieceType(color_defender, QUEEN);
		if (bbRookAttacks & pos.AttacksByPieceType(color_attacker, ROOK) & bbSafe)
			attackScore[c] += settings::parameter.SAFE_CHECK[ROOK];
		if (bbBishopAttacks & pos.AttacksByPieceType(color_attacker, BISHOP) & bbSafe)
			attackScore[c] += settings::parameter.SAFE_CHECK[BISHOP];
		const Bitboard bbKnightAttacks = KnightAttacks[pos.KingSquare(color_defender)] & pos.AttacksByPieceType(color_attacker, KNIGHT);
		if (bbKnightAttacks & bbSafe)
			attackScore[c] += settings::parameter.SAFE_CHECK[KNIGHT];
		if (attackScore[c] > 0) {
			attackVals[c] = static_cast<Value>((attackScore[c] * attackScore[c]) >> settings::parameter.KING_DANGER_SCALE);
		}
	}
	result.mgScore += attackVals[BLACK] - attackVals[WHITE];
	const Bitboard bbWhite = pos.PieceBB(PAWN, WHITE);
	const Bitboard bbBlack = pos.PieceBB(PAWN, BLACK);
	//Pawn shelter/storm
	Eval pawnStorm;
	if (pos.PieceBB(KING, WHITE) & SaveSquaresForKing & HALF_OF_WHITE) { //Bonus only for castled king
		Bitboard bbShelter = bbWhite & ((kingRing[0] & ShelterPawns2ndRank) | (kingZone[0] & ShelterPawns3rdRank) | ((kingZone[0] << 8) & ShelterPawns4thRank));
		Eval shelter;
		while (bbShelter) {
			const Square s = lsb(bbShelter);
			shelter += settings::parameter.PAWN_SHELTER[PawnShieldIndex[s]];
			bbShelter &= bbShelter - 1;
		}
		pawnStorm += shelter;
		const bool kingSide = (pos.KingSquare(WHITE) & 7) > 3;
		const Bitboard pawnStormArea = kingSide ? bbKINGSIDE : bbQUEENSIDE;
		Bitboard stormPawns = pos.PieceBB(PAWN, BLACK) & pawnStormArea & (HALF_OF_WHITE | RANK5);
		while (stormPawns) {
			const Square sq = lsb(stormPawns);
			stormPawns &= stormPawns - 1;
			if ((pos.GetAttacksFrom(sq) & pos.PieceBB(PAWN, WHITE)) != EMPTY) {
				const int rank = sq >> 3;
				if (rank > 1 && rank < 4) pawnStorm -= Eval(settings::parameter.BONUS_LEVER_ON_KINGSIDE,0); //lever
			}
			else {
				const Piece blocker = pos.GetPieceOnSquare(static_cast<Square>(sq - 8));
				if (blocker == WKING || GetPieceType(blocker) == PAWN) continue;
			}
			pawnStorm -= settings::parameter.PAWN_STORM[(sq >> 3) - 1];
		}
	}
	if (pos.PieceBB(KING, BLACK) & SaveSquaresForKing & HALF_OF_BLACK) {
		Bitboard bbShelter = bbBlack & ((kingRing[1] & ShelterPawns2ndRank) | (kingZone[1] & ShelterPawns3rdRank) | ((kingZone[1] >> 8) & ShelterPawns4thRank));
		Eval shelter;
		while (bbShelter) {
			const Square s = lsb(bbShelter);
			shelter += settings::parameter.PAWN_SHELTER[PawnShieldIndex[s]];
			bbShelter &= bbShelter - 1;
		}
		pawnStorm -= shelter;
		const bool kingSide = (pos.KingSquare(BLACK) & 7) > 3;
		const Bitboard pawnStormArea = kingSide ? bbKINGSIDE : bbQUEENSIDE;
		Bitboard stormPawns = pos.PieceBB(PAWN, WHITE) & pawnStormArea & (HALF_OF_BLACK | RANK4);
		while (stormPawns) {
			const Square sq = lsb(stormPawns);
			stormPawns &= stormPawns - 1;
			if ((pos.GetAttacksFrom(sq) & pos.PieceBB(PAWN, BLACK)) != EMPTY) {
				const int rank = sq >> 3;
				if (rank > 3 && rank < 6) pawnStorm += Eval(settings::parameter.BONUS_LEVER_ON_KINGSIDE, 0); //lever
			}
			else {
				const Piece blocker = pos.GetPieceOnSquare(static_cast<Square>(sq + 8));
				if (blocker == BKING || GetPieceType(blocker) == PAWN) continue;
			}
			pawnStorm += settings::parameter.PAWN_STORM[6 - (sq >> 3)];
		}
	}
	result += pawnStorm;
	return result;
}

Eval evaluateMobility(const Position& pos) noexcept {
	Eval result;
	//Create attack bitboards
	const Bitboard abbWPawn = pos.AttacksByPieceType(WHITE, PAWN);
	const Bitboard abbBPawn = pos.AttacksByPieceType(BLACK, PAWN);

	//Leichtfiguren (N+B)
	const Bitboard abbWMinor = abbWPawn | pos.AttacksByPieceType(WHITE, KNIGHT) | pos.AttacksByPieceType(WHITE, BISHOP);
	const Bitboard abbBMinor = abbBPawn | pos.AttacksByPieceType(BLACK, KNIGHT) | pos.AttacksByPieceType(BLACK, BISHOP);
	//Rooks
	const Bitboard abbWRook = abbWMinor | pos.AttacksByPieceType(WHITE, ROOK);
	const Bitboard abbBRook = abbBMinor | pos.AttacksByPieceType(BLACK, ROOK);

	const Bitboard bbBlockedPawns[2] = { (pos.PieceBB(PAWN, BLACK) >> 8) & pos.PieceBB(PAWN, WHITE), (pos.PieceBB(PAWN, WHITE) << 8) & pos.PieceBB(PAWN, BLACK) };
	const Bitboard allowedWhite = ~((pos.PieceBB(PAWN, WHITE) & (RANK2 | RANK3)) | bbBlockedPawns[WHITE] | pos.PieceBB(KING, WHITE));
	const Bitboard allowedBlack = ~((pos.PieceBB(PAWN, BLACK) & (RANK6 | RANK7)) | bbBlockedPawns[BLACK] | pos.PieceBB(KING, BLACK));

	//Bitboard bbBattery[2] = { pos.BatteryAttacks(WHITE), pos.BatteryAttacks(BLACK) };
	//Now calculate Mobility
	//Queens can move to all unattacked squares and if protected to all squares attacked by queens or kings
	Bitboard pieceBB = pos.PieceBB(QUEEN, WHITE);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBRook;
		result += settings::parameter.MOBILITY_BONUS_QUEEN[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(QUEEN, BLACK);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWRook;
		result -= settings::parameter.MOBILITY_BONUS_QUEEN[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Rooks can move to all unattacked squares and if protected to all squares attacked by rooks or less important pieces
	pieceBB = pos.PieceBB(ROOK, WHITE);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBMinor;
		result += settings::parameter.MOBILITY_BONUS_ROOK[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(ROOK, BLACK);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWMinor;
		result -= settings::parameter.MOBILITY_BONUS_ROOK[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Leichtfiguren
	pieceBB = pos.PieceBB(BISHOP, WHITE);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBPawn;
		result += settings::parameter.MOBILITY_BONUS_BISHOP[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(BISHOP, BLACK);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWPawn;
		result -= settings::parameter.MOBILITY_BONUS_BISHOP[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(KNIGHT, WHITE);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBPawn;
		result += settings::parameter.MOBILITY_BONUS_KNIGHT[popcount(targets)];
		result += popcount(targets & EXTENDED_CENTER) * settings::parameter.MOBILITY_CENTER_EXTENDED_KNIGHT;
		result += popcount(targets & CENTER) * settings::parameter.MOBILITY_CENTER_KNIGHT;
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(KNIGHT, BLACK);
	while (pieceBB) {
		const Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWPawn;
		result -= settings::parameter.MOBILITY_BONUS_KNIGHT[popcount(targets)];
		result -= popcount(targets & EXTENDED_CENTER) * settings::parameter.MOBILITY_CENTER_EXTENDED_KNIGHT;
		result -= popcount(targets & CENTER) * settings::parameter.MOBILITY_CENTER_KNIGHT;
		pieceBB &= pieceBB - 1;
	}
	//Pawn mobility
	Bitboard pawnTargets = abbWPawn & pos.ColorBB(BLACK);
	pawnTargets |= (pos.PieceBB(PAWN, WHITE) << 8) & ~pos.OccupiedBB();
	result += Eval(10, 10) * popcount(pawnTargets);
	pawnTargets = abbBPawn & pos.ColorBB(WHITE);
	pawnTargets |= (pos.PieceBB(PAWN, BLACK) >> 8) & ~pos.OccupiedBB();
	result -= Eval(10, 10) * popcount(pawnTargets);
	return result;
}

Value evaluateFromScratch(const Position& pos) {
	Evaluation result;
	MaterialTableEntry * material = pos.GetMaterialTableEntry();
	for (PieceType pt = QUEEN; pt <= PAWN; ++pt) {
		const int diff = popcount(pos.PieceBB(pt, WHITE)) - popcount(pos.PieceBB(pt, BLACK));
		material->Evaluation += diff * settings::parameter.PieceValues[pt];
	}
	const Phase_t phase = Phase(popcount(pos.PieceBB(QUEEN, WHITE)), popcount(pos.PieceBB(QUEEN, BLACK)),
		popcount(pos.PieceBB(ROOK, WHITE)), popcount(pos.PieceBB(ROOK, BLACK)),
		popcount(pos.PieceBB(BISHOP, WHITE)), popcount(pos.PieceBB(BISHOP, BLACK)),
		popcount(pos.PieceBB(KNIGHT, WHITE)), popcount(pos.PieceBB(KNIGHT, BLACK)));
	material->Phase = phase;
	result.Material = material->Evaluation;
	material->EvaluationFunction = &evaluateDefault;
	return result.GetScore(pos);
}

int scaleEG(const Position & pos) noexcept
{

	if (pos.GetMaterialTableEntry()->NeedsScaling() && pos.oppositeColoredBishops()) {
		if (pos.PieceTypeBB(ROOK) == EMPTY) {
			if (popcount(pos.PieceTypeBB(PAWN)) <= 1) return 18; return 62;
		}
		else return 92;
	}
	return 128;
}

Value evaluatePawnEnding(const Position& pos) {
	Value ppEval = VALUE_ZERO;
	if (pos.GetPawnEntry()->passedPawns) {
		std::array<int, 2> min_distance = { 999,999 };
		std::array<int, 2> distant = { 0, 0 };
		std::array<int, 2> distcount = { 0, 0 };
		std::array<uint8_t, 2> filesets = { 0,0 };
		for (int c = 0; c < 2; ++c) {
			const int pawnstep = 8 - c * 16;
			const Color col = static_cast<Color>(c);
			const Color ocol = static_cast<Color>(c ^ 1);
			Bitboard bbPassed = pos.GetPawnEntry()->passedPawns & pos.PieceBB(PAWN, col);
			filesets.at(c) = Fileset(bbPassed);
			while (bbPassed) {
				const Square s = lsb(bbPassed);
				if (PawnUncatcheable(s, pos.KingSquare(ocol), PromotionSquare(s, col), pos.GetSideToMove() == ocol)) {
					min_distance.at(c) = std::min(min_distance.at(c), static_cast<int>(relativeRank(ocol, s >> 3)));
				}
				else {
					distcount.at(c)++;
					const Square blockSquare = static_cast<Square>(static_cast<int>(s) + pawnstep);
					const int distToConv = std::min(static_cast<int>(relativeRank(ocol, s >> 3)), 5);
					const int dtcSquare = (6 - distToConv) * (6 - distToConv);
					const bool is_protected = ToBitboard(s) & pos.AttacksByPieceType(col, PAWN);
					int distPoints = is_protected ? 2 * dtcSquare * ChebishevDistance(pos.KingSquare(ocol), blockSquare)
						: 2 * dtcSquare * ChebishevDistance(pos.KingSquare(ocol), blockSquare) - dtcSquare * ChebishevDistance(pos.KingSquare(col), blockSquare);
					distant.at(c) += distPoints;
				}
				bbPassed &= bbPassed - 1;
			}
		}
		min_distance.at(static_cast<int>(pos.GetSideToMove()) ^ 1) += 1;
		const int diff = min_distance.at(0) - min_distance.at(1);
		if (diff < -1) {
			ppEval += Value(settings::parameter.PieceValues[QUEEN].egScore - settings::parameter.PieceValues[PAWN].egScore - 100 * min_distance.at(0));
		}
		else if (diff > 1) {
			ppEval -= Value(settings::parameter.PieceValues[QUEEN].egScore - settings::parameter.PieceValues[PAWN].egScore - 100 * min_distance.at(1));
		}
		ppEval += static_cast<Value>(11 * (distant.at(0) * distcount.at(0) - distant.at(1) * distcount.at(1)) / 5);
	}
	return Value((pos.GetMaterialScore() + pos.GetPawnEntry()->Score.egScore + pos.GetPsqEval().egScore + ppEval + Contempt.egScore) * (1 - 2 * pos.GetSideToMove()));
}

