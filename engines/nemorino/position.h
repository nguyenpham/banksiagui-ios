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
#include <string>
#include <vector>
#include "types.h"
#include "board.h"
#include "material.h"
#include "hashtables.h"
#include "network.h"

extern nnue::Network network;

struct Evaluation;

const MoveGenerationType generationPhases[26] = { HASHMOVE, NON_LOOSING_CAPTURES, KILLER, LOOSING_CAPTURES, QUIETS_POSITIVE, QUIETS_NEGATIVE, UNDERPROMOTION, NONE, //Main Search Phases
HASHMOVE, NON_LOOSING_CAPTURES, LOOSING_CAPTURES, NONE,                                   //QSearch Phases
HASHMOVE, CHECK_EVASION, UNDERPROMOTION, NONE, //Check Evasion
HASHMOVE, NON_LOOSING_CAPTURES, LOOSING_CAPTURES, QUIET_CHECKS, UNDERPROMOTION, NONE, //QSearch with Checks
REPEAT_ALL, NONE,
ALL, NONE };

//Indexed by StagedMoveGenerationType
const int generationPhaseOffset[] = { 0, //Main Search
8, //QSearch
12, //Check Evasion
16, //QSearch with Checks
22, //Repeat
24  //All moves
};
/* Represents a chess position and provides information about lots of characteristics of this position
   The position is represented by 8 Bitboards (2 for squares occupied by color, and 6 for each piece type)
   Further there is a redundant 64 byte array for fast lookup which piece is on a given square
   Each position contains a pointer to the previous position, which is created when copying the position
   Applying a move is done by first copying the position and then calling ApplyMove
*/
struct Position
{
public:
	//Creates the starting position 
	Position();
	//Creates a position based on the given FEN string
	explicit Position(std::string fen);
	Position(uint8_t * data);
	/*Creates a copy of a position.
	  ATTENTION: Only parts of the position are copied. To have a full copy a further call to copy() method is needed
	*/
	Position(Position &pos);
	~Position();
	Position& operator=(const Position&) = default;
	Position(Position&&) = default;
	Position& operator=(Position&&) = default;

	//Moves already played within a game before searched position is reached
	static int AppliedMovesBeforeRoot;
	//Access methods to the positions bitboards
	Bitboard PieceBB(const PieceType pt, const Color c) const noexcept;
	Bitboard ColorBB(const Color c) const noexcept;
	Bitboard ColorBB(const int c) const noexcept;
	Bitboard PieceTypeBB(const PieceType pt) const noexcept;
	Bitboard OccupiedBB() const noexcept;
	inline Bitboard MajorPieceBB(const Color c) const noexcept { return (OccupiedByPieceType[QUEEN] | OccupiedByPieceType[ROOK]) & OccupiedByColor[static_cast<int>(c)]; }
	inline Bitboard MinorPieceBB(const Color c) const noexcept { return (OccupiedByPieceType[BISHOP] | OccupiedByPieceType[KNIGHT]) & OccupiedByColor[static_cast<int>(c)]; }
	inline Bitboard NonPawnMaterial(const Color c) const noexcept { return (OccupiedByPieceType[QUEEN] | OccupiedByPieceType[ROOK] | OccupiedByPieceType[BISHOP] | OccupiedByPieceType[KNIGHT]) & OccupiedByColor[static_cast<int>(c)]; }
	//debug helpers
	std::string print();
	std::string printGeneratedMoves();
	//returns current position as FEN string
	std::string fen(bool only_epd = false) const;
	//initializes the position from given FEN string
	void setFromFEN(const std::string& fen);
	//Applies a pseudo-legal move and returns true if move is legal
	bool ApplyMove(Move move) noexcept;
	//"Undo move" by returning pointer to previous position
	inline Position * Previous() const noexcept { return previous; }
	//Generate moves and store it in moves array
	template<MoveGenerationType MGT> ValuatedMove * GenerateMoves();
	//returns Zobrist Hash key of position
	inline uint64_t GetHash() const noexcept { return Hash; }
	inline MaterialKey_t GetMaterialKey() const  noexcept { return MaterialKey; }
	inline uint64_t GetMaterialHash() const noexcept { return MaterialKey != MATERIAL_KEY_UNUSUAL ? MaterialKey * 14695981039346656037ull : GetMaterialHashUnusual(); }
	inline PawnKey_t GetPawnKey() const noexcept { return PawnKey; }
	inline Eval GetPsqEval() const noexcept { return PsqEval; }
	/* The position struct provides staged move generation. To make use of it the staged move generation has to be initialized first by calling InitializeMoveIterator.
	   Then every call to NextMove() will return the next move until MOVE_NONE is returned */
	   //Initialize staged move generation, by providing the necessary information for move ordering
	template<StagedMoveGenerationType SMGT> void InitializeMoveIterator(HistoryManager *history, MoveSequenceHistoryManager *counterMoveHistory, MoveSequenceHistoryManager *followupHistory, killer::Manager * km, Move counter, Move hashmove = MOVE_NONE) noexcept;
	//Get next move. If MOVE_NONE is returned end of move list is reached
	Move NextMove();
	//SEE (Static Exchange Evaluation): The implementation is copied from Chess Programming Wiki (https://chessprogramming.wikispaces.com/SEE+-+The+Swap+Algorithm)
	Value SEE(Move move) const noexcept;
	//SEE method, which returns without exact value, when it's sure that value is positive (then VALUE_KNOWN_WIN is returned)
	Value SEE_Sign(Move move) const noexcept;
	//returns true if SideTo Move is in check. Must not be called when it's unclear whether opponent's attack map is already determined
	inline bool Checked() const noexcept { return (attackedByThem & PieceBB(KING, SideToMove)) != EMPTY; }
	//Static evaluation function for unusual material (no pre-calculated material values available in Material Table)
	friend Evaluation evaluateFromScratch(Position &pos);
	//Calls the static evaluation function (it will call the evaluation even, if the StaticEval value is already different from VALUE_NOTYETEVALUATED)
	inline Value evaluate();
	//Just for debugging
	std::string printEvaluation();
	std::string printDbgEvaluation();
	//Evaluates final positions
	inline Value evaluateFinalPosition() noexcept;
	inline int GeneratedMoveCount() const noexcept { return movepointer - 1; }
	//Returns the number of plies applied from the root position of the search
	inline int GetPliesFromRoot() const noexcept { return pliesFromRoot; }
	inline Color GetSideToMove() const noexcept { return SideToMove; }
	inline Piece GetPieceOnSquare(Square square) const noexcept { return Board[square]; }
	inline Square GetEPSquare() const noexcept { return EPSquare; }
	//gives access to the moves of the currently processed stage
	inline ValuatedMove * GetMovesOfCurrentPhase() noexcept { return &moves[phaseStartIndex]; }
	//Within staged move generation this method returns the index of the current move within the current stage. This is needed for
	//updating the history table
	inline int GetMoveNumberInPhase() const noexcept { return moveIterationPointer; }
	inline Value GetMaterialScore() const noexcept { return material->Score(); }
	inline MaterialTableEntry * GetMaterialTableEntry() const noexcept { return material; }
	inline pawn::Entry * GetPawnEntry() const noexcept { return pawn; }
	inline Value GetPawnScore() const noexcept { return pawn->Score.getScore(material->Phase); }
	inline void InitMaterialPointer() noexcept { material = probe(MaterialKey); }
	inline Eval PawnStructureScore() const noexcept { return pawn->Score; }
	//checks if the position is final and returns the result
	Result GetResult();
	//for xboard protocol support it's helpful, to not only know that a position is a DRAW but also why it's a draw. Therefore this additional
	//method will return a more detailed result value
	DetailedResult GetDetailedResult();
	//returns a bitboard indicating the squares attacked by the piece on the given square 
	inline Bitboard GetAttacksFrom(Square square) const noexcept { return attacks[square]; }
	inline void SetPrevious(Position &pos) noexcept { previous = &pos; }
	inline void SetPrevious(Position *pos) noexcept { previous = pos; }
	//Should be called before search starts
	inline void ResetPliesFromRoot() noexcept { pliesFromRoot = 0; }
	inline Bitboard AttacksByPieceType(Color color, PieceType pieceType) const noexcept;
	inline Bitboard AttacksExcludingPieceType(Color color, PieceType excludedPieceType) const noexcept;
	inline Bitboard AttacksByColor(Color color) const noexcept { return (SideToMove == color) * attackedByUs + (SideToMove != color) * attackedByThem; }
	inline Bitboard AttackedByThem() const noexcept { return attackedByThem; }
	inline Bitboard AttackedByUs() const noexcept { return attackedByUs; }
	//checks if the position is already repeated (if one of the ancestors has the same zobrist hash). This is no check for 3-fold repetition!
	bool checkRepetition() const noexcept;
	//checks if there are any repetitions in prior moves
	bool hasRepetition() const noexcept;
	inline void SwitchSideToMove() noexcept { SideToMove = Color(SideToMove ^ 1); Hash ^= ZobristMoveColor; }
	inline unsigned char GetDrawPlyCount() const noexcept { return DrawPlyCount; }
	//applies a null move to the given position (there is no copy/make for null move), the EPSquare is the only information which has to be restored afterwards
	void NullMove(Square epsquare = OUTSIDE, Move lastApplied = MOVE_NONE) noexcept;
	//delete all ancestors of the current positions and frees the assigned memory
	void deleteParents();
	//returns the last move applied, which lead to this position
	inline Move GetLastAppliedMove() const noexcept { return lastAppliedMove; }
	//get's the piece, which moved in the last applied move
	inline Piece GetPreviousMovingPiece() const noexcept { if (previous && lastAppliedMove != MOVE_NONE) return previous->GetPieceOnSquare(to(lastAppliedMove)); else return BLANK; }
	//returns the piece, which has been captured by the last applied move
	inline Piece getCapturedInLastMove() const noexcept { if (lastAppliedMove == MOVE_NONE) return Piece::BLANK; else return capturedInLastMove; }
	//checks if a move is quiet (move is neither capture, nor promotion)
	inline bool IsQuiet(const Move move) const noexcept { return (Board[to(move)] == BLANK) && (type(move) == NORMAL || type(move) == CASTLING); }
	//checks if a move is quiet and not a castling move (used in search for pruning decisions)
	inline bool IsQuietAndNoCastles(const Move move) const noexcept { return type(move) == NORMAL && Board[to(move)] == BLANK; }
	//checks if a move is a tactical (cpture or promotion) move
	inline bool IsTactical(const Move& move) const noexcept { return Board[to(move)] != BLANK || type(move) == ENPASSANT || type(move) == PROMOTION; }
	inline bool IsTactical(const ValuatedMove& move) const noexcept { return IsTactical(move.move); }
	inline bool IsCapture(const Move& move) const noexcept { return Board[to(move)] != BLANK || type(move) == ENPASSANT; }
	//returns the current value of StaticEval - doesn't check if evaluate has been executed
	inline Value GetStaticEval() const noexcept { return StaticEval; }
	inline void SetStaticEval(Value evaluation) noexcept { StaticEval = evaluation; }
	inline PieceType GetMostValuablePieceType(Color col) const noexcept;
	inline PieceType GetMostValuableAttackedPieceType() const noexcept;
	inline bool PawnOn7thRank()  noexcept { return (PieceBB(PAWN, SideToMove) & RANKS[6 - 5 * SideToMove]) != 0; } //Side to Move has pawn on 7th Rank
	inline bool IsAdvancedPawnPush(Move move) const noexcept { return GetPieceType(Board[from(move)]) == PAWN && (SideToMove == WHITE ? to(move) >= A5 : to(move) <= H4); }
	/*Make a full copy of the current position. To get a full copy first the copy constructor has to be called and then method copy has to be called:
		position copiedPosition(pos);
		copiedPosition.copy(pos);
	  This method should always be used when a copy is neede without applying a move */
	void copy(const Position &pos) noexcept;
	inline bool CastlingAllowed(CastleFlag castling) const noexcept { return (CastlingOptions & castling) != 0; }
	inline unsigned GetCastles() const noexcept { return CastlingOptions & 15; }
	inline CastleFlag GetCastlesForColor(Color color) const noexcept { return static_cast<CastleFlag>(CastlingOptions & ((W0_0 | W0_0_0) << (2 * color))); }
	inline bool HasCastlingLost(const Color col) const noexcept { return (CastlingOptions & (CastleFlag::W_LOST_CASTLING << (int)col)) != 0; }
	//creates the SAN (standard algebraic notation) representation of a move
	std::string toSan(Move move);
	//parses a move in SAN notation
	Move parseSan(std::string move);
	Move GetCounterMove(Move(&counterMoves)[12][64]) noexcept;
	inline bool Improved() noexcept { return previous == nullptr || previous->previous == nullptr || StaticEval >= previous->previous->StaticEval; }
	inline bool Worsening() noexcept { return previous != nullptr && previous->previous != nullptr && StaticEval <= previous->previous->StaticEval - Value(10); }
	//During staged move generation first only queen promotions are generated. When all other moves are generated and processed under promotions will be added
	void AddUnderPromotions() noexcept;
	inline ValuatedMove * GetMoves(int & moveCount) noexcept { moveCount = movepointer - 1; return moves; }
	inline void ResetMoveGeneration() noexcept { movepointer = 0; moves[0].move = MOVE_NONE; moves[0].score = VALUE_NOTYETDETERMINED; }
	//Some moves (like moves from transposition tables) have to be validated (checked for legality) before being applied
	bool validateMove(Move move) noexcept;
	//For pruning decisions it's necessary to identify whether or not all special movee (like killer,..) are already returned
	inline bool QuietMoveGenerationPhaseStarted() const noexcept { return generationPhases[generationPhase] >= QUIETS_POSITIVE; }
	inline bool MoveGenerationPhasePassed(MoveGenerationType phase) const noexcept { return (processedMoveGenerationPhases & (1 << (int)phase)) != 0; }
	inline MoveGenerationType GetMoveGenerationPhase() const noexcept { return generationPhases[generationPhase]; }
	//Validate a move and return it if validated, else return another valid move
	Move validMove(Move proposedMove) noexcept;
	//Checks if a move gives check
	bool givesCheck(Move move) noexcept;
	//Get Pinned Pieces
	inline Bitboard PinnedPieces(Color colorOfKing) const noexcept { return bbPinned[colorOfKing]; }
	void CalculatePinnedPieces() noexcept;
	inline Square KingSquare(Color color) const noexcept { return kingSquares[color]; }
	//Check for opposite colored bishops
	bool oppositeColoredBishops() const noexcept;
	//Check if there is a mate threat (a possibility that there might be a quiet move giving mate)
	bool mateThread() const noexcept;
	//Bitboard of squares attacked by more than one piece
	inline Bitboard dblAttacks(Color color) const noexcept { return dblAttacked[color]; }
	//CHeck if kings are on opposed wings 
	inline bool KingOnOpposedWings() const noexcept { return ((CastlingOptions & 15) == 0) && std::abs((kingSquares[WHITE] & 7) - (kingSquares[BLACK] & 7)) > 2; }
	Bitboard BatteryAttacks(Color attacking_color) const noexcept;
	inline bool IsKvK() const noexcept { return MaterialKey == MATERIAL_KEY_KvK; }
	void pack(uint8_t* data, int move_count = 1) const;
	void unpack(uint8_t* data);
	int16_t NNScore() const;
	inline void ClearNNSet() const { nnInput.set = false; }

private:
	//These are the members, which are copied by the copy constructor and contain the full information
	mutable nnue::InputAdapter nnInput;
	Bitboard OccupiedByColor[2];
	Bitboard OccupiedByPieceType[6];
	//Zobrist hash hey
	uint64_t Hash = ZobristMoveColor;
	//Material table key
	MaterialKey_t MaterialKey;
	//Pawn hash key
	PawnKey_t PawnKey;
	Square EPSquare;
	//Castling Options (see enum in types.h)
	unsigned char CastlingOptions;
	unsigned char DrawPlyCount;
	Color SideToMove;
	int pliesFromRoot;
	Piece Board[64];
	Eval PsqEval;
	//King Squares
	Square kingSquares[2];


	//Pointer to the previous position
	Position * previous = nullptr;

	//These members are only calculated when needed
	//Pointer to the relevant entry in the Material table
	MaterialTableEntry * material;
	//Pointer to the relevant entry in the pawn hash table
	pawn::Entry * pawn;

	//array to store the generated moves
	ValuatedMove moves[MAX_MOVE_COUNT];
	//number of generated moves in moves array
	int movepointer = 0;
	//Attack array - index is Square number, value is a bitboard indicating all squares attacked by a piece on that square
	Bitboard attacks[64];
	//Attack bitboard containing all squares attacked by the side not to move
	Bitboard attackedByThem;
	//Attack bitboard containing all squares attacked by the side to move
	Bitboard attackedByUs;
	//Attack bitboard containing all squares attacked by at least 2 pieces (indexed by attacking color)
	Bitboard dblAttacked[2] = { EMPTY, EMPTY };
	//Attack bitboard containing all attacks by a certain Piece Type
	Bitboard attacksByPt[12];
	//Bitboards of pieces pinned to king of given Color: bbPinned[0] contains white an black pieces "pinned" to white king
	Bitboard bbPinned[2] = { EMPTY, EMPTY };
	Bitboard bbPinner[2];

	//indices needed to manage staged move generation
	int moveIterationPointer;
	int phaseStartIndex;
	int generationPhase;
	//Result of position (value will be OPEN unless position is final)
	Result result = Result::RESULT_UNKNOWN;
	//Static evaluation of position
	Value StaticEval = VALUE_NOTYETDETERMINED;
	//Information needed for move ordering during staged move generation
	Move hashMove = MOVE_NONE;
	killer::Manager *killerManager;
	Move lastAppliedMove = MOVE_NONE;
	Piece capturedInLastMove = BLANK;
	HistoryManager * history;
	MoveSequenceHistoryManager * cmHistory;
	MoveSequenceHistoryManager * followupHistory;
	Move counterMove = MOVE_NONE;
	ValuatedMove * firstNegative;
	bool canPromote = false;
	uint32_t processedMoveGenerationPhases;
	//Place a piece on Squarre square and update bitboards and Hash key
	template<bool SquareIsEmpty> void set(const Piece piece, const Square square) noexcept;
	void remove(const Square square) noexcept;

	inline uint64_t GetMaterialHashUnusual() const noexcept {
		uint64_t mhash = 0;
		for (int c = 0; c <= 1; ++c) {
			for (int pt = 0; pt <= 5; ++pt) {
				mhash ^= ZobristKeys[2 * pt + c][popcount(PieceBB(static_cast<PieceType>(pt), static_cast<Color>(c)))];
			}
		}
		return mhash;
	}
	inline void AddCastlingOption(const CastleFlag castleFlag) noexcept {
		Hash ^= ZobristCastles[CastlingOptions & 15]; 
		CastlingOptions |= castleFlag; 
		Hash ^= ZobristCastles[CastlingOptions & 15];
	}
	inline void RemoveCastlingOption(const CastleFlag castleFlag) noexcept { Hash ^= ZobristCastles[CastlingOptions & 15]; CastlingOptions &= ~castleFlag; Hash ^= ZobristCastles[CastlingOptions & 15]; }
	inline void SetCastlingLost(const Color col) noexcept { CastlingOptions |= CastleFlag::W_LOST_CASTLING << (int)col; }
	inline void SetEPSquare(const Square square) noexcept {
		if (EPSquare != square) {
			if (EPSquare != OUTSIDE) {
				Hash ^= ZobristEnPassant[EPSquare & 7];
				EPSquare = square;
				if (EPSquare != OUTSIDE)
					Hash ^= ZobristEnPassant[EPSquare & 7];
			}
			else {
				EPSquare = square;
				Hash ^= ZobristEnPassant[EPSquare & 7];
			}
		}
	}
	inline int PawnStep() const noexcept { return 8 - 16 * SideToMove; }
	//Add a move to the move list and increment movepointer
	inline void AddMove(Move move) noexcept {
		moves[movepointer].move = move;
		moves[movepointer].score = VALUE_NOTYETDETERMINED;
		++movepointer;
	}
	//Adds MOVE_NONE at the end of the move list
	inline void AddNullMove() noexcept { moves[movepointer].move = MOVE_NONE; moves[movepointer].score = VALUE_NOTYETDETERMINED; ++movepointer; }
	//Updates Castle Flags after a move from fromSquare to toSquare has been applied, must not be called for castling moves
	void updateCastleFlags(Square fromSquare, Square toSquare) noexcept;
	//Calculates the attack bitboards for all pieces of one side
	Bitboard calculateAttacks(Color color) noexcept;
	//Calculates Bitboards of pieces blocking a check. If colorOfBlocker = kingColor, these are the pinned pieces, else these are candidates for discovered checks
	Bitboard checkBlocker(Color colorOfBlocker, Color kingColor) noexcept;
	//Calculates the material key of this position
	MaterialKey_t calculateMaterialKey() const noexcept;
	//Calculates the Pawn Key of this position
	PawnKey_t calculatePawnKey() const noexcept;
	//different move evaluation methods (used for move ordering):
	void evaluateByCaptureScore(int startIndex = 0) noexcept;
	void evaluateBySEE(int startIndex) noexcept;
	void evaluateCheckEvasions(int startIndex);
	void evaluateByHistory(int startIndex) noexcept;
	//Get's the best evaluated move from the move list starting at start Index
	Move getBestMove(int startIndex) noexcept;
	void insertionSort(ValuatedMove* begin, ValuatedMove* end) noexcept;
	void shellSort(ValuatedMove* vm, int count) noexcept;
	PieceType getAndResetLeastValuableAttacker(Square toSquare, Bitboard stmAttackers, Bitboard& occupied, Bitboard& attackers, Bitboard& mayXray) const noexcept;
	//return a bitboard of squares with pieces attacking the targetField
	Bitboard AttacksOfField(const Square targetField, const Bitboard occupied) const noexcept;
	Bitboard AttacksOfField(const Square targetField, const Color attackingSide) const noexcept;
	//Checks is a oseudo-legal move is valid
	inline bool isValid(Move move) { Position next(*this); return next.ApplyMove(move); }
	//Checks if a move (e.g. from killer move list) is a valid move
	bool validateMove(ExtendedMove move) noexcept;
	//Checks if at least one valid move exists - ATTENTION must not be called on a newly initialized position where attackedByThem isn't calculated yet!!
	template<bool CHECKED> bool CheckValidMoveExists();
	//Checks for unusual Material (this means one side has more than one Queen or more than 2 rooks, knights or bishop)
	bool checkMaterialIsUnusual() const noexcept;
	//Generates quiet and tactical moves like forks
	ValuatedMove* GenerateForks(bool withChecks) noexcept;

	inline nnue::Piece TransformPiece(Piece p) const { return p == Piece::BLANK ? nnue::Piece::NONE : nnue::Invert(static_cast<nnue::Piece>(9 - p)); }

};

template<> inline ValuatedMove* Position::GenerateMoves<QUIET_CHECKS>();
template<> inline ValuatedMove* Position::GenerateMoves<LEGAL>();
template<> inline ValuatedMove* Position::GenerateMoves<FORKS>();
template<> inline ValuatedMove* Position::GenerateMoves<FORKS_NO_CHECKS>();

Move parseMoveInUCINotation(const std::string& uciMove, const Position& pos) noexcept;


inline Bitboard Position::PieceBB(const PieceType pt, const Color c) const noexcept { return OccupiedByColor[c] & OccupiedByPieceType[pt]; }
inline Bitboard Position::ColorBB(const Color c) const noexcept { return OccupiedByColor[c]; }
inline Bitboard Position::ColorBB(const int c) const noexcept { return OccupiedByColor[c]; }
inline Bitboard Position::OccupiedBB() const noexcept { return OccupiedByColor[WHITE] | OccupiedByColor[BLACK]; }
inline Bitboard Position::PieceTypeBB(const PieceType pt) const noexcept { return OccupiedByPieceType[pt]; }

inline PieceType Position::GetMostValuablePieceType(Color color) const noexcept {
	if (MaterialKey != MATERIAL_KEY_UNUSUAL) return material->GetMostExpensivePiece(color);
	else {
		for (PieceType pt = QUEEN; pt < KING; ++pt) {
			if (PieceBB(pt, color)) {
				assert(MaterialKey == MATERIAL_KEY_UNUSUAL || pt == material->GetMostExpensivePiece(color));
				return pt;
			}
		}
		return KING;
	}
}

inline PieceType Position::GetMostValuableAttackedPieceType() const noexcept {
	const Color col = Color(SideToMove ^ 1);
	const PieceType ptstart = MaterialKey != MATERIAL_KEY_UNUSUAL ? material->GetMostExpensivePiece(col) : QUEEN;
	for (PieceType pt = ptstart; pt < KING; ++pt) {
		if (PieceBB(pt, col) & attackedByUs) return pt;
	}
	return KING;
}

inline Value Position::evaluate() {
	if (StaticEval != VALUE_NOTYETDETERMINED)
		return StaticEval;
	if (GetResult() == Result::OPEN) {
		 return StaticEval = material->EvaluationFunction(*this) + settings::parameter.BONUS_TEMPO.getScore(material->Phase);
	}
	else if (result == Result::DRAW) return StaticEval = VALUE_DRAW;
	else
		return StaticEval = Value((2 - int(result)) * (VALUE_MATE - pliesFromRoot));
}

inline Value Position::evaluateFinalPosition() noexcept {
	if (result == Result::DRAW) return VALUE_DRAW;
	else
		return Value((2 - int(result)) * (VALUE_MATE - pliesFromRoot));
}

//Tries to find one valid move as fast as possible
template<bool CHECKED> bool Position::CheckValidMoveExists() {
	assert(attackedByThem); //should have been already calculated
	//Start with king (Castling need not be considered - as there is always another legal move available with castling
	//In Chess960 this might be different)
	const Square kingSquare = kingSquares[SideToMove];
	Bitboard kingTargets = KingAttacks[kingSquare] & ~OccupiedByColor[SideToMove] & ~attackedByThem;
	if (CHECKED && kingTargets) {
		if (popcount(kingTargets) > 2) return true; //unfortunately 8/5p2/5kp1/8/4p3/R4n2/1r3K2/4q3 w - - shows that king itself can even block 2 sliders
		else {
			//unfortunately king could be "blocker" e.g. in 8/8/1R2k3/K7/8/8/8/8 w square f6 is not attacked by white
			//however if king moves to f6 it's still check
			const Square to = lsb(kingTargets);
			kingTargets &= kingTargets - 1;
			if (isValid(createMove(kingSquare, to)) || (kingTargets && isValid(createMove(kingSquare, lsb(kingTargets))))) return true;
		}
	}
	else if (kingTargets) return true; //No need to check
	if (CHECKED) {
		const Bitboard checker = AttacksOfField(kingSquare, Color(SideToMove ^ 1));
		if (popcount(checker) != 1) return false; //double check and no king move => MATE
		//All valid moves are now either capturing the checker or blocking the check
		const Bitboard blockingSquares = checker | InBetweenFields[kingSquare][lsb(checker)];
		const Bitboard pinned = checkBlocker(SideToMove, SideToMove);
		//Sliders and knights can't move if pinned (as we are in check) => therefore only check the unpinned pieces
		Bitboard sliderAndKnight = OccupiedByColor[SideToMove] & ~OccupiedByPieceType[KING] & ~OccupiedByPieceType[PAWN] & ~pinned;
		while (sliderAndKnight) {
			if (attacks[lsb(sliderAndKnight)] & ~OccupiedByColor[SideToMove] & blockingSquares) return true;
			sliderAndKnight &= sliderAndKnight - 1;
		}
		//Pawns
		Bitboard singleStepTargets;
		Bitboard pawns = PieceBB(PAWN, SideToMove) & ~pinned;
		if (SideToMove == WHITE) {
			if ((singleStepTargets = ((pawns << 8) & ~OccupiedBB())) & blockingSquares) return true;
			if (((singleStepTargets & RANK3) << 8) & ~OccupiedBB() & blockingSquares) return true;
		}
		else {
			if ((singleStepTargets = ((pawns >> 8) & ~OccupiedBB())) & blockingSquares) return true;
			if (((singleStepTargets&RANK6) >> 8) & ~OccupiedBB() & blockingSquares) return true;
		}
		//Pawn captures
		while (pawns) {
			const Square from = lsb(pawns);
			if (checker & attacks[from]) return true;
			pawns &= pawns - 1;
		}
	}
	else {
		//Now we need the pinned pieces
		const Bitboard pinned = checkBlocker(SideToMove, SideToMove);
		//Now first check all unpinned pieces
		Bitboard sliderAndKnight = OccupiedByColor[SideToMove] & ~OccupiedByPieceType[KING] & ~OccupiedByPieceType[PAWN] & ~pinned;
		while (sliderAndKnight) {
			if (attacks[lsb(sliderAndKnight)] & ~OccupiedByColor[SideToMove]) return true;
			sliderAndKnight &= sliderAndKnight - 1;
		}
		//Pawns
		const Bitboard pawns = PieceBB(PAWN, SideToMove) & ~pinned;
		Bitboard pawnTargets;
		//normal pawn move
		if (SideToMove == WHITE) pawnTargets = (pawns << 8) & ~OccupiedBB(); else pawnTargets = (pawns >> 8) & ~OccupiedBB();
		if (pawnTargets) return true;
		//pawn capture
		if (SideToMove == WHITE) pawnTargets = (((pawns << 9) & NOT_A_FILE) | ((pawns << 7) & NOT_H_FILE)) & OccupiedByColor[SideToMove ^ 1];
		else pawnTargets = (((pawns >> 9) & NOT_H_FILE) | ((pawns >> 7) & NOT_A_FILE)) & OccupiedByColor[SideToMove ^ 1];
		if (pawnTargets) return true;
		//Now let's deal with pinned pieces
		Bitboard pinnedSlider = (PieceBB(QUEEN, SideToMove) | PieceBB(ROOK, SideToMove) | PieceBB(BISHOP, SideToMove)) & pinned;
		while (pinnedSlider) {
			const Square from = lsb(pinnedSlider);
			if (attacks[from] & (InBetweenFields[from][kingSquare] | ShadowedFields[kingSquare][from])) return true;
			pinnedSlider &= pinnedSlider - 1;
		}
		//pinned knights must not move and pinned kings don't exist => remains pinned pawns
		Bitboard pinnedPawns = PieceBB(PAWN, SideToMove) & pinned;
		const Bitboard pinnedPawnsAllowedToMove = pinnedPawns & FILES[kingSquare & 7];
		if (pinnedPawnsAllowedToMove) {
			if (SideToMove == WHITE && ((pinnedPawnsAllowedToMove << 8) & ~OccupiedBB())) return true;
			else if (SideToMove == BLACK && ((pinnedPawnsAllowedToMove >> 8) & ~OccupiedBB())) return true;
		}
		//Now there remains only pinned pawn captures
		while (pinnedPawns) {
			const Square from = lsb(pinnedPawns);
			pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from];
			while (pawnTargets) {
				if (isolateLSB(pawnTargets) & (InBetweenFields[from][kingSquare] | ShadowedFields[kingSquare][from])) return true;
				pawnTargets &= pawnTargets - 1;
			}
			pinnedPawns &= pinnedPawns - 1;
		}
	}
	//ep-captures are difficult as 3 squares are involved => therefore simply apply and check it
	Bitboard epAttacker;
	if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
		while (epAttacker) {
			if (isValid(createMove<ENPASSANT>(lsb(epAttacker), EPSquare))) return true;
			epAttacker &= epAttacker - 1;
		}
	}
	return false;
}


// Generates all legal moves (by first generating all pseudo-legal moves and then eliminating all invalid moves
//Should only be used at the root as implementation is slow!
template<> ValuatedMove* Position::GenerateMoves<LEGAL>() {
	GenerateMoves<ALL>();
	for (int i = 0; i < movepointer - 1; ++i) {
		if (!isValid(moves[i].move)) {
			moves[i] = moves[movepointer - 2];
			--movepointer;
			--i;
		}
	}
	return &moves[0];
}

//Generates all quiet moves giving check
template<> ValuatedMove* Position::GenerateMoves<QUIET_CHECKS>() {
	movepointer -= (movepointer != 0);
	ValuatedMove * firstMove = &moves[movepointer];
	//There are 2 options to give check: Either give check with the moving piece, or a discovered check by
	//moving a check blocking piece
	const Square opposedKingSquare = kingSquares[SideToMove ^ 1];
	//1. Discovered Checks
	const Bitboard discoveredCheckCandidates = checkBlocker(SideToMove, Color(SideToMove ^ 1));
	const Bitboard targets = ~OccupiedBB();
	//1a Sliders
	Bitboard sliders = (PieceBB(ROOK, SideToMove) | PieceBB(QUEEN, SideToMove) | PieceBB(BISHOP, SideToMove)) & discoveredCheckCandidates;
	while (sliders) {
		const Square from = lsb(sliders);
		Bitboard sliderTargets = attacks[from] & targets & (~InBetweenFields[opposedKingSquare][from] & ~ShadowedFields[opposedKingSquare][from]);
		while (sliderTargets) {
			AddMove(createMove(from, lsb(sliderTargets)));
			sliderTargets &= sliderTargets - 1;
		}
		sliders &= sliders - 1;
	}
	//1b Knights
	Bitboard knights = PieceBB(KNIGHT, SideToMove) & discoveredCheckCandidates;
	while (knights) {
		const Square from = lsb(knights);
		Bitboard knightTargets = KnightAttacks[from] & targets;
		while (knightTargets) {
			AddMove(createMove(from, lsb(knightTargets)));
			knightTargets &= knightTargets - 1;
		}
		knights &= knights - 1;
	}
	//1c Kings
	const Square kingSquare = kingSquares[SideToMove];
	if (discoveredCheckCandidates & PieceBB(KING, SideToMove)) {
		Bitboard kingTargets = KingAttacks[kingSquare] & ~attackedByThem & targets & (~InBetweenFields[opposedKingSquare][kingSquare] & ~ShadowedFields[opposedKingSquare][kingSquare]);
		while (kingTargets) {
			AddMove(createMove(kingSquare, lsb(kingTargets)));
			kingTargets &= kingTargets - 1;
		}
	}
	//1d Pawns
	const Bitboard pawns = PieceBB(PAWN, SideToMove) & discoveredCheckCandidates;
	if (pawns) {
		Bitboard singleStepTargets;
		Bitboard doubleStepTargets;
		if (SideToMove == WHITE) {
			singleStepTargets = (pawns << 8) & ~RANK8 & targets;
			doubleStepTargets = ((singleStepTargets & RANK3) << 8) & targets;
		}
		else {
			singleStepTargets = (pawns >> 8) & ~RANK1 & targets;
			doubleStepTargets = ((singleStepTargets & RANK6) >> 8) & targets;
		}
		while (singleStepTargets) {
			const Square to = lsb(singleStepTargets);
			const Square from = Square(to - PawnStep());
			const Bitboard stillBlocking = InBetweenFields[from][opposedKingSquare] | ShadowedFields[opposedKingSquare][from];
			if (!(stillBlocking & ToBitboard(to))) AddMove(createMove(from, to));
			singleStepTargets &= singleStepTargets - 1;
		}
		while (doubleStepTargets) {
			const Square to = lsb(doubleStepTargets);
			const Square from = Square(to - 2 * PawnStep());
			const Bitboard stillBlocking = InBetweenFields[from][opposedKingSquare] | ShadowedFields[opposedKingSquare][from];
			if (!(stillBlocking & ToBitboard(to))) AddMove(createMove(from, to));
			doubleStepTargets &= doubleStepTargets - 1;
		}
	}
	//2. Normal checks
	//2a "Rooks"
	const Bitboard rookAttackstoKing = RookTargets(opposedKingSquare, OccupiedBB()) & targets;
	Bitboard rooks = (PieceBB(ROOK, SideToMove) | PieceBB(QUEEN, SideToMove)) & ~discoveredCheckCandidates;
	while (rooks) {
		const Square from = lsb(rooks);
		Bitboard rookTargets = attacks[from] & rookAttackstoKing;
		while (rookTargets) {
			AddMove(createMove(from, lsb(rookTargets)));
			rookTargets &= rookTargets - 1;
		}
		rooks &= rooks - 1;
	}
	//2b "Bishops"
	const Bitboard bishopAttackstoKing = BishopTargets(opposedKingSquare, OccupiedBB()) & targets;
	Bitboard bishops = (PieceBB(BISHOP, SideToMove) | PieceBB(QUEEN, SideToMove)) & ~discoveredCheckCandidates;
	while (bishops) {
		const Square from = lsb(bishops);
		Bitboard bishopTargets = attacks[from] & bishopAttackstoKing;
		while (bishopTargets) {
			AddMove(createMove(from, lsb(bishopTargets)));
			bishopTargets &= bishopTargets - 1;
		}
		bishops &= bishops - 1;
	}
	//2c Knights
	const Bitboard knightAttacksToKing = KnightAttacks[opposedKingSquare];
	knights = PieceBB(KNIGHT, SideToMove) & ~discoveredCheckCandidates;
	while (knights) {
		const Square from = lsb(knights);
		Bitboard knightTargets = KnightAttacks[from] & knightAttacksToKing & targets;
		while (knightTargets) {
			AddMove(createMove(from, lsb(knightTargets)));
			knightTargets &= knightTargets - 1;
		}
		knights &= knights - 1;
	}
	//2d Pawn
	Bitboard pawnTargets;
	Bitboard pawnFrom;
	Bitboard dblPawnFrom;
	if (SideToMove == WHITE) {
		pawnTargets = targets & (((PieceBB(KING, Color(SideToMove ^ 1)) >> 9) & NOT_H_FILE) | ((PieceBB(KING, Color(SideToMove ^ 1)) >> 7) & NOT_A_FILE));
		pawnFrom = pawnTargets >> 8;
		dblPawnFrom = ((pawnFrom & targets & RANK3) >> 8) & PieceBB(PAWN, WHITE);
		pawnFrom &= PieceBB(PAWN, WHITE);
	}
	else {
		pawnTargets = targets & (((PieceBB(KING, Color(SideToMove ^ 1)) << 7) & NOT_H_FILE) | ((PieceBB(KING, Color(SideToMove ^ 1)) << 9) & NOT_A_FILE));
		pawnFrom = pawnTargets << 8;
		dblPawnFrom = ((pawnFrom & targets & RANK6) << 8) & PieceBB(PAWN, BLACK);
		pawnFrom &= PieceBB(PAWN, BLACK);
	}
	while (pawnFrom) {
		const Square from = lsb(pawnFrom);
		AddMove(createMove(from, from + PawnStep()));
		pawnFrom &= pawnFrom - 1;
	}
	while (dblPawnFrom) {
		const Square from = lsb(dblPawnFrom);
		AddMove(createMove(from, from + 2 * PawnStep()));
		dblPawnFrom &= dblPawnFrom - 1;
	}
	//2e Castles
	if (CastlingOptions & 15 & CastlesbyColor[SideToMove]) //King is on initial square
	{
		//King-side castles
		if ((CastlingOptions & 15 & (1 << (2 * SideToMove))) //Short castle allowed
			&& (InitialRookSquareBB[2 * SideToMove] & PieceBB(ROOK, SideToMove)) //Rook on initial square
			&& !(SquaresToBeEmpty[2 * SideToMove] & OccupiedBB()) //Fields between Rook and King are empty
			&& !(SquaresToBeUnattacked[2 * SideToMove] & attackedByThem) //Fields passed by the king are unattacked
			&& (RookTargets(opposedKingSquare, ~targets & ~PieceBB(KING, SideToMove)) & RookSquareAfterCastling[2 * SideToMove])) //Rook is giving check after castling
		{
			if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove])); else AddMove(createMove<CASTLING>(kingSquare, Square(G1 + SideToMove * 56)));
		}
		//Queen-side castles
		if ((CastlingOptions & 15 & (1 << (2 * SideToMove + 1))) //Short castle allowed
			&& (InitialRookSquareBB[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
			&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
			&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem) //Fields passed by the king are unattacked
			&& (RookTargets(opposedKingSquare, ~targets & ~PieceBB(KING, SideToMove)) & RookSquareAfterCastling[2 * SideToMove + 1])) //Rook is giving check after castling
		{
			if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove + 1])); else AddMove(createMove<CASTLING>(kingSquare, Square(C1 + SideToMove * 56)));
		}
	}
	AddNullMove();
	return firstMove;
}

template<> inline ValuatedMove* Position::GenerateMoves<FORKS>() {
	return GenerateForks(true);
}

template<> inline ValuatedMove* Position::GenerateMoves<FORKS_NO_CHECKS>() {
	return GenerateForks(false);
}

template<MoveGenerationType MGT> ValuatedMove * Position::GenerateMoves() {
	if (MGT == ALL || MGT == CHECK_EVASION) movepointer = 0; else movepointer -= (movepointer != 0);
	ValuatedMove * firstMove = &moves[movepointer];
	//Rooksliders
	Bitboard targets;
	if (MGT == ALL || MGT == TACTICAL || MGT == QUIETS || MGT == CHECK_EVASION) {
		const Square kingSquare = kingSquares[SideToMove];
		Bitboard checkBlocker = 0;
		bool doubleCheck = false;
		if (MGT == CHECK_EVASION) {
			const Bitboard checker = AttacksOfField(kingSquare, Color(SideToMove ^ 1));
			if (popcount(checker) == 1) {
				checkBlocker = checker | InBetweenFields[kingSquare][lsb(checker)];
			}
			else doubleCheck = true;
		}
		const Bitboard empty = ~ColorBB(BLACK) & ~ColorBB(WHITE);
		if (MGT == ALL) targets = ~ColorBB(SideToMove);
		else if (MGT == TACTICAL) targets = ColorBB(SideToMove ^ 1);
		else if (MGT == QUIETS) targets = empty;
		else if (MGT == CHECK_EVASION && !doubleCheck) targets = ~ColorBB(SideToMove) & checkBlocker;
		else targets = 0;
		//Kings
		Bitboard kingTargets;
		if (MGT == CHECK_EVASION) kingTargets = KingAttacks[kingSquare] & ~OccupiedByColor[SideToMove] & ~attackedByThem;
		else kingTargets = KingAttacks[kingSquare] & targets & ~attackedByThem;
		while (kingTargets) {
			AddMove(createMove(kingSquare, lsb(kingTargets)));
			kingTargets &= kingTargets - 1;
		}
		if (MGT == ALL || MGT == QUIETS) {
			if (CastlingOptions & 15 & CastlesbyColor[SideToMove]) //King is on initial square
			{
				//King-side castles
				if ((CastlingOptions & 15 & (1 << (2 * SideToMove))) //Short castle allowed
					&& (InitialRookSquareBB[2 * SideToMove] & PieceBB(PieceType::ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove] & OccupiedBB()) //Fields between Rook and King are empty
					&& !(SquaresToBeUnattacked[2 * SideToMove] & attackedByThem)) //Fields passed by the king are unattacked
				{
					if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove]));
					else AddMove(createMove<CASTLING>(kingSquare, Square(G1 + SideToMove * 56)));
				}
				//Queen-side castles
				if ((CastlingOptions & 15 & (1 << (2 * SideToMove + 1))) //Short castle allowed
					&& (InitialRookSquareBB[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
					&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem)) //Fields passed by the king are unattacked
				{
					if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove + 1]));
					else AddMove(createMove<CASTLING>(kingSquare, Square(C1 + SideToMove * 56)));
				}
			}
		}
		if (!doubleCheck) {
			Bitboard sliders = PieceBB(PieceType::ROOK, SideToMove) | PieceBB(PieceType::QUEEN, SideToMove) | PieceBB(PieceType::BISHOP, SideToMove);
			while (sliders) {
				const Square from = lsb(sliders);
				Bitboard sliderTargets = attacks[from] & targets;
				while (sliderTargets) {
					AddMove(createMove(from, lsb(sliderTargets)));
					sliderTargets &= sliderTargets - 1;
				}
				sliders &= sliders - 1;
			}
			//Knights
			Bitboard knights = PieceBB(KNIGHT, SideToMove);
			while (knights) {
				const Square from = lsb(knights);
				Bitboard knightTargets;
				if (MGT == CHECK_EVASION) knightTargets = attacks[from] & targets & checkBlocker; else knightTargets = attacks[from] & targets;
				while (knightTargets) {
					AddMove(createMove(from, lsb(knightTargets)));
					knightTargets &= knightTargets - 1;
				}
				knights &= knights - 1;
			}
			//Pawns
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			//Captures
			if (MGT == ALL || MGT == TACTICAL || MGT == CHECK_EVASION) {
				while (pawns) {
					const Square from = lsb(pawns);
					Bitboard pawnTargets;
					if (MGT == CHECK_EVASION) pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8 & checkBlocker; else pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8;
					while (pawnTargets) {
						AddMove(createMove(from, lsb(pawnTargets)));
						pawnTargets &= pawnTargets - 1;
					}
					//Promotion Captures
					if (MGT == CHECK_EVASION) pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8 & checkBlocker; else pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
					while (pawnTargets) {
						const Square to = lsb(pawnTargets);
						AddMove(createMove<PROMOTION>(from, to, QUEEN));
						canPromote = true;
						pawnTargets &= pawnTargets - 1;
					}
					pawns &= pawns - 1;
				}
			}
			Bitboard singleStepTarget = 0;
			Bitboard doubleSteptarget = 0;
			Bitboard promotionTarget = 0;
			if (SideToMove == WHITE) {
				if (MGT == ALL || MGT == QUIETS) {
					singleStepTarget = (PieceBB(PAWN, WHITE) << 8) & empty & ~RANK8;
					doubleSteptarget = ((singleStepTarget & RANK3) << 8) & empty;
				}
				else if (MGT == CHECK_EVASION) {
					singleStepTarget = (PieceBB(PAWN, WHITE) << 8) & empty & ~RANK8;
					doubleSteptarget = ((singleStepTarget & RANK3) << 8) & empty & checkBlocker;
					singleStepTarget &= checkBlocker;
				}
				if (MGT == ALL || MGT == TACTICAL) promotionTarget = (PieceBB(PAWN, WHITE) << 8) & empty & RANK8;
				else if (MGT == CHECK_EVASION) promotionTarget = (PieceBB(PAWN, WHITE) << 8) & empty & RANK8 & checkBlocker;
			}
			else {
				if (MGT == ALL || MGT == QUIETS) {
					singleStepTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & ~RANK1;
					doubleSteptarget = ((singleStepTarget & RANK6) >> 8) & empty;
				}
				else if (MGT == CHECK_EVASION) {
					singleStepTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & ~RANK1;
					doubleSteptarget = ((singleStepTarget & RANK6) >> 8) & empty & checkBlocker;
					singleStepTarget &= checkBlocker;
				}
				if (MGT == ALL || MGT == TACTICAL) promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & RANK1;
				else if (MGT == CHECK_EVASION) promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & RANK1 & checkBlocker;
			}
			if (MGT == ALL || MGT == QUIETS || MGT == CHECK_EVASION) {
				while (singleStepTarget) {
					const Square to = lsb(singleStepTarget);
					AddMove(createMove(to - PawnStep(), to));
					singleStepTarget &= singleStepTarget - 1;
				}
				while (doubleSteptarget) {
					const Square to = lsb(doubleSteptarget);
					AddMove(createMove(to - 2 * PawnStep(), to));
					doubleSteptarget &= doubleSteptarget - 1;
				}
			}
			//Promotions
			if (MGT == ALL || MGT == TACTICAL || MGT == CHECK_EVASION) {
				while (promotionTarget) {
					const Square to = lsb(promotionTarget);
					const Square from = Square(to - PawnStep());
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					canPromote = true;
					promotionTarget &= promotionTarget - 1;
				}
				Bitboard epAttacker;
				if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
					while (epAttacker) {
						AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
						epAttacker &= epAttacker - 1;
					}
				}
			}
		}
		if (MGT == ALL && canPromote) {
			const int moveCount = movepointer;
			for (int i = 0; i < moveCount; ++i) {
				const Move pmove = moves[i].move;
				if (type(pmove) == PROMOTION) {
					AddMove(createMove<PROMOTION>(from(pmove), to(pmove), KNIGHT));
					AddMove(createMove<PROMOTION>(from(pmove), to(pmove), ROOK));
					AddMove(createMove<PROMOTION>(from(pmove), to(pmove), BISHOP));
				}
			}
		}
	}
	else { //Winning, Equal and loosing captures
		Bitboard sliders;
		const Bitboard hanging = ColorBB(Color(SideToMove ^ 1)) & ~AttackedByThem();
		if (MGT == WINNING_CAPTURES || MGT == NON_LOOSING_CAPTURES) {
			Bitboard promotionTarget;
			if (SideToMove == WHITE)  promotionTarget = (PieceBB(PAWN, WHITE) << 8) & ~OccupiedBB() & RANK8; else promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & ~OccupiedBB() & RANK1;
			while (promotionTarget) {
				const Square to = lsb(promotionTarget);
				const Square from = Square(to - PawnStep());
				AddMove(createMove<PROMOTION>(from, to, QUEEN));
				canPromote = true;
				promotionTarget &= promotionTarget - 1;
			}
			//King Captures are always winning as kings can only capture uncovered pieces
			const Square kingSquare = kingSquares[SideToMove];
			Bitboard kingTargets = KingAttacks[kingSquare] & ColorBB(SideToMove ^ 1) & ~attackedByThem;
			while (kingTargets) {
				AddMove(createMove(kingSquare, lsb(kingTargets)));
				kingTargets &= kingTargets - 1;
			}
		}
		//Pawn Captures
		if (MGT == WINNING_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				const Square from = lsb(pawns);
				Bitboard pawnTargets = ColorBB(SideToMove ^ 1) & (~PieceBB(PAWN, Color(SideToMove ^ 1)) | hanging) & attacks[from] & ~RANK1and8;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				//Promotion Captures
				pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
				while (pawnTargets) {
					const Square to = lsb(pawnTargets);
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					canPromote = true;
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
		}
		else if (MGT == EQUAL_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				const Square from = lsb(pawns);
				Bitboard pawnTargets = PieceBB(PAWN, Color(SideToMove ^ 1)) & attacks[from] & ~hanging;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
			Bitboard epAttacker;
			if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
				while (epAttacker) {
					AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
					epAttacker &= epAttacker - 1;
				}
			}
		}
		else if (MGT == NON_LOOSING_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				const Square from = lsb(pawns);
				Bitboard pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				//Promotion captures
				pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
				while (pawnTargets) {
					const Square to = lsb(pawnTargets);
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					canPromote = true;
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
			Bitboard epAttacker;
			if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
				while (epAttacker) {
					AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
					epAttacker &= epAttacker - 1;
				}
			}
		}
		//Knight Captures
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1)) | hanging;
		else if (MGT == EQUAL_CAPTURES) targets = (PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1))) & ~hanging;
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(PAWN, Color(SideToMove ^ 1)) & ~hanging;
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1)) | hanging;
		else targets = 0;
		Bitboard knights = PieceBB(KNIGHT, SideToMove);
		while (knights) {
			const Square from = lsb(knights);
			Bitboard knightTargets = attacks[from] & targets;
			while (knightTargets) {
				AddMove(createMove(from, lsb(knightTargets)));
				knightTargets &= knightTargets - 1;
			}
			knights &= knights - 1;
		}
		//Bishop Captures
		sliders = PieceBB(BISHOP, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1)) | hanging;
		else if (MGT == EQUAL_CAPTURES) targets = (PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1))) & ~hanging;
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(PAWN, Color(SideToMove ^ 1)) & ~hanging;
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1)) | hanging;
		else targets = 0;
		while (sliders) {
			const Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		//Rook Captures
		sliders = PieceBB(ROOK, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | hanging;
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(ROOK, Color(SideToMove ^ 1)) & ~hanging;
		else if (MGT == LOOSING_CAPTURES) targets = (PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1))) & ~hanging;
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1)) | hanging;
		else targets = 0;
		while (sliders) {
			const Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		//Queen Captures
		sliders = PieceBB(QUEEN, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = hanging;
		if (MGT == EQUAL_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) & ~hanging;
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | hanging;
		else if (MGT == LOOSING_CAPTURES) targets = (PieceBB(ROOK, Color(SideToMove ^ 1)) | PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1))) & ~hanging;
		else targets = 0;
		while (sliders) {
			const Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
	}
	AddNullMove();
	return firstMove;
}


template<StagedMoveGenerationType SMGT> void Position::InitializeMoveIterator(HistoryManager * historyStats, MoveSequenceHistoryManager * counterHistoryStats, MoveSequenceHistoryManager * followupHistoryStats, killer::Manager * km, Move counter, Move hashmove) noexcept {
	processedMoveGenerationPhases = 0;
	if (SMGT == REPETITION) {
		moveIterationPointer = 0;
		generationPhase = generationPhaseOffset[SMGT];
		return;
	}
	if (SMGT == ALL_MOVES) {
		moveIterationPointer = -1;
		generationPhase = generationPhaseOffset[SMGT];
		return;
	}
	if (SMGT == MAIN_SEARCH) killerManager = km; else killerManager = nullptr;
	counterMove = counter;
	moveIterationPointer = -1;
	movepointer = 0;
	phaseStartIndex = 0;
	history = historyStats;
	cmHistory = counterHistoryStats;
	followupHistory = followupHistoryStats;
	hashMove = hashmove;
	if (Checked()) generationPhase = generationPhaseOffset[CHECK] + (hashMove == MOVE_NONE);
	else generationPhase = generationPhaseOffset[SMGT] + (hashMove == MOVE_NONE);
}

inline Bitboard Position::AttacksByPieceType(Color color, PieceType pieceType) const noexcept {
	return attacksByPt[GetPiece(pieceType, color)];
}

inline Bitboard Position::AttacksExcludingPieceType(Color color, PieceType excludedPieceType) const noexcept
{
	Bitboard bb = EMPTY;
	for (PieceType p = QUEEN; p <= KING; ++p) {
		if (p != excludedPieceType) bb |= AttacksByPieceType(color, p);
	}
	return bb;
}




