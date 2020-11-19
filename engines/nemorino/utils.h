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

//copied from SF
enum NemorionoSyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, NemorionoSyncCout);

#define sync_cout std::cout << IO_LOCK
#define sync_endl std::endl << IO_UNLOCK

/// Under Windows it is not possible for a process to run on more than one
/// logical processor group. This usually means to be limited to use max 64
/// cores. To overcome this, some special platform specific API should be
/// called to set group affinity for each thread. Original code from Texel by
/// Peter Österlund.
namespace NemorinoWinProcGroup {
	void bindThisThread(size_t idx);
}

namespace utils {

	void debugInfo(std::string info);
	void debugInfo(std::string info1, std::string info2);
	std::vector<std::string> split(const std::string str, char sep = ' ');
	void replaceExt(std::string& s, const std::string& newExt);
	std::string TrimLeft(const std::string& s);
	std::string TrimRight(const std::string& s);
	std::string Trim(const std::string& s);

	uint64_t MurmurHash2A(uint64_t input, uint64_t seed = 0x4f4863d5038ea3a3) noexcept;

	inline uint64_t Hash(uint64_t input) noexcept { return input * 14695981039346656037ull; }

	inline std::string bool2String(bool val) { if (val) return "true"; else return "false"; }

	template <class T> T clamp(T value, T lowerBound, T upperBound) { return std::max(lowerBound, std::min(value, upperBound)); }

	extern double K;
	inline double sigmoid(Value score) noexcept { return 1.0 / (1.0 + std::pow(10.0, K * (int)score)); }

	inline double interpolatedResult(double result, int ply, int totalPlies) noexcept {
		return 0.5 + ((result - 0.5) * ply) / totalPlies;
	}
	inline double winExpectation(Value score, double scale = 1.305) noexcept { return 1 / (1 + std::pow(10, -scale / 400 * (int)score)); }

	std::string mirrorFenVertical(std::string fen);

	double TexelTuneError(const char* argv[], int argc);

	double TexelTuneError(std::string data, std::string parameter);

	double calculateScaleFactor(std::string epd);

	//Copied from https://github.com/nodchip/Stockfish
// Class that handles bitstream
// useful when doing aspect encoding
	struct BitStream
	{
		// Set the memory to store the data in advance.
		// Assume that memory is cleared to 0.
		inline void set_data(uint8_t* data_) { data = data_; reset(); }

		// Get the pointer passed in set_data().
		inline uint8_t* get_data() const { return data; }

		// Get the cursor.
		inline int get_cursor() const { return bit_cursor; }

		// reset the cursor
		inline void reset() { bit_cursor = 0; }

		// Write 1bit to the stream.
		// If b is non-zero, write out 1. If 0, write 0.
		inline void write_one_bit(int b)
		{
			if (b)
				data[bit_cursor / 8] |= 1 << (bit_cursor & 7);

			++bit_cursor;
		}

		// Get 1 bit from the stream.
		inline int read_one_bit()
		{
			int b = (data[bit_cursor / 8] >> (bit_cursor & 7)) & 1;
			++bit_cursor;

			return b;
		}

		// write n bits of data
		// Data shall be written out from the lower order of d.
		inline void write_n_bit(int d, int n)
		{
			for (int i = 0; i < n; ++i)
				write_one_bit(d & (1 << i));
		}

		// read n bits of data
		// Reverse conversion of write_n_bit().
		inline int read_n_bit(int n)
		{
			int result = 0;
			for (int i = 0; i < n; ++i)
				result |= read_one_bit() ? (1 << i) : 0;

			return result;
		}

	private:
		// Next bit position to read/write.
		int bit_cursor;

		// data entity
		uint8_t* data;
	};

	struct HuffmanedPiece
	{
		int code; // how it will be coded
		int bits; // How many bits do you have
	};

	constexpr HuffmanedPiece huffman_table[] =
	{
		{0b1001,4}, // QUEEN
		{0b0111,4}, // ROOK
		{0b0101,4}, // BISHOP
		{0b0011,4}, // KNIGHT
	    {0b0001,4}, // PAWN
		{0b0000,1}, // KING
	  	{0b0000,1}, // NO_PIECE
	};

	struct PackedSfenValue
	{
		// phase
		std::array<uint8_t, 32> sfen;

		// Evaluation value returned from Learner::search()
		int16_t score;

		// PV first move
		// Used when finding the match rate with the teacher
		uint16_t move;

		// Trouble of the phase from the initial phase.
		uint16_t gamePly;

		// 1 if the player on this side ultimately wins the game. -1 if you are losing.
		// 0 if a draw is reached.
		// The draw is in the teacher position generation command gensfen,
		// Only write if LEARN_GENSFEN_DRAW_RESULT is enabled.
		int8_t game_result;

		// When exchanging the file that wrote the teacher aspect with other people
		//Because this structure size is not fixed, pad it so that it is 40 bytes in any environment.
		uint8_t padding;

		// 32 + 2 + 2 + 2 + 1 + 1 = 40bytes
	};


#define SCORE(score) score
#define SCORE_MDP(score) score
#define SCORE_TT(score) score
#define SCORE_TB(score) score
#define SCORE_RAZ(score) score
#define SCORE_BP(score) score
#define SCORE_NMP(score) score
#define SCORE_PC(score) score
#define SCORE_BC(score) score
#define SCORE_EXACT(score) score
#define SCORE_FINAL(score) score
#define SCORE_SP(score) score
#define SCORE_DP(score) score

}


