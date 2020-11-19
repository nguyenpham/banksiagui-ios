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
  This file contains a lot of code copied from Stockfish (see
  https://github.com/official-stockfish/Stockfish)
*/

#pragma once
#include <string>
#include <cstring>
#include <memory>
#include <vector>
#include <array>
#include <iostream>

#if defined(USE_AVX2)
#include <immintrin.h>

#elif defined(USE_SSE41)
#include <smmintrin.h>

#elif defined(USE_SSE2)
#include <emmintrin.h>


#endif

#if defined(USE_AVX2)
#if defined(__GNUC__ ) && (__GNUC__ < 9)
#define _mm256_loadA_si256  _mm256_loadu_si256
#define _mm256_storeA_si256 _mm256_storeu_si256
#else
#define _mm256_loadA_si256  _mm256_load_si256
#define _mm256_storeA_si256 _mm256_store_si256
#endif
#endif

namespace nnue {

	// Version of the evaluation file
	constexpr std::uint32_t kVersion = 0x7AF32F16u;

	// Constant used in evaluation value calculation
	constexpr int FV_SCALE = 16;
	constexpr int kWeightScaleBits = 6;

	// Size of cache line (in bytes)
	constexpr size_t kCacheLineSize = 64;

	// SIMD width (in bytes)
#if defined(USE_AVX2)
	constexpr size_t kSimdWidth = 32;

#elif defined(USE_SSE2)
	constexpr std::size_t kSimdWidth = 16;

#endif

	constexpr size_t kMaxSimdWidth = 32;

	// Type of input feature after conversion
	using TransformedFeatureType = uint8_t;
	using IndexType = std::uint16_t;

	// Round n up to be a multiple of base
	template <typename IntType>
	constexpr IntType CeilToMultiple(IntType n, IntType base) {
		return (n + base - 1) / base * base;
	}

	enum class Piece : uint8_t {
		WPAWN, BPAWN, WKNIGHT, BKNIGHT, WBISHOP, BBISHOP, WROOK, BROOK, WQUEEN, BQUEEN, WKING, BKING, NONE
	};

	inline Piece Invert(Piece p) { return static_cast<Piece>(static_cast<int>(p) ^ 1); }

	using PieceSquare = int;

	enum class Perspective { WHITE, BLACK };

	constexpr PieceSquare PieceSquareIndex(Piece piece, int square) {
		return static_cast<PieceSquare>(static_cast<int>(piece) * 64 + square);
	}

	constexpr int rotate(int square) {
		return square ^ 0x3F;
	}

	inline void* std_aligned_alloc(size_t alignment, size_t size) {
#if defined(POSIXALIGNEDALLOC)
		void* pointer;
		if (posix_memalign(&pointer, alignment, size) == 0)
			return pointer;
		return nullptr;
//#elif (defined(_WIN32) || (defined(__APPLE__) && !defined(_LIBCPP_HAS_C11_FEATURES)))
//		return _mm_malloc(size, alignment);
#else
		return aligned_alloc(alignment, size);
#endif
	}

	inline void std_aligned_free(void* ptr) {
#ifdef __ANDROID__
		return free(ptr);
#elif defined(__APPLE__)
		free(ptr);
#elif defined(_WIN32)
		_mm_free(ptr);
#else
		free(ptr);
#endif
	}

	template <typename T>
	struct AlignedDeleter {
		void operator()(T* ptr) const {
			ptr->~T();
			std_aligned_free(ptr);
		}
	};

	template <typename T>
	using AlignedPtr = std::unique_ptr<T, AlignedDeleter<T>>;

	using Board = Piece[64];

	//Input Layer Data (accumulator in SF code)
	using NNInput = std::int16_t[2][256];

	struct Delta {
		uint8_t square;
		Piece piece_before;
	};

	using Deltas = Delta[3];

	class alignas(32) InputAdapter {
	public:
		NNInput nninput;
		Piece* board;
		Deltas deltas;
		bool is_black_to_move;
		bool set = false;
		uint8_t kingSquares[2];

		inline bool IsDelta() {
			return set && deltas[0].square < 64 && (static_cast<int>(deltas[0].piece_before) & 0xFE) != static_cast<int>(Piece::WKING);
		}

		inline Piece PieceAfter(int deltaIndex) { return board[deltas[deltaIndex].square]; }
	};

	constexpr int kHalfDimensions = 256;
	constexpr int kInputDimensions = 40960;

	namespace layer {

		template <IndexType OutputDimensions>
		class InputSlice {
		public:

			// Output type
			using OutputType = TransformedFeatureType;

			// Output dimensionality
			static constexpr IndexType kOutputDimensions = OutputDimensions;

			// Size of forward propagation buffer used from the input layer to this layer
			static constexpr std::size_t kBufferSize = 0;

			// Hash value embedded in the evaluation file
			static constexpr std::uint32_t GetHashValue() {
				std::uint32_t hash_value = 0xEC42E90Du;
				hash_value ^= kOutputDimensions;
				return hash_value;
			}

			// Read network parameters
			bool ReadParameters(std::istream& stream) {
				stream.read(reinterpret_cast<char*>(&header), sizeof(header));
				stream.read(reinterpret_cast<char*>(biases_),
					OutputDimensions / 2 * sizeof(BiasType));
				stream.read(reinterpret_cast<char*>(weights_),
					OutputDimensions / 2 * kInputDimensions *
					sizeof(WeightType));
				return !stream.fail();
			}

			// Forward propagation
			const OutputType* Propagate(InputAdapter& input,
				TransformedFeatureType* output,
				char* /*buffer*/) const {
				//Calculate indices
				int stm = static_cast<int>(input.is_black_to_move);
				int ks = rotate(input.kingSquares[1]);
				if (input.IsDelta()) {
					int maxRIndex = 0;
					int maxAIndex = 0;
					IndexType Rindices[2][3];
					IndexType Aindices[2][3];
					//Calculate changed indices
					for (int dindx = 0; dindx < 3; ++dindx) {
						if (input.deltas[dindx].square > 63) break;
						if (input.deltas[dindx].piece_before != Piece::NONE) {
							Rindices[0][maxRIndex] = 640 * input.kingSquares[0] + PieceSquareIndex(input.deltas[dindx].piece_before, input.deltas[dindx].square);
							Rindices[1][maxRIndex] = 640 * ks + PieceSquareIndex(Invert(input.deltas[dindx].piece_before), rotate(input.deltas[dindx].square));
							maxRIndex++;
						}
						if (input.PieceAfter(dindx) != Piece::NONE) {
							Aindices[0][maxAIndex] = 640 * input.kingSquares[0] + PieceSquareIndex(input.PieceAfter(dindx), input.deltas[dindx].square);
							Aindices[1][maxAIndex] = 640 * ks + PieceSquareIndex(Invert(input.PieceAfter(dindx)), rotate(input.deltas[dindx].square));
							maxAIndex++;
						}
					}
					for (int p = 0; p < 2; ++p) {
#if defined(USE_AVX2)
						constexpr IndexType kNumChunks = kHalfDimensions / (kSimdWidth / 2);
						auto accumulation = reinterpret_cast<__m256i*>(&input.nninput[p][0]);

#elif defined(USE_SSE2)
						constexpr IndexType kNumChunks = kHalfDimensions / (kSimdWidth / 2);
						auto accumulation = reinterpret_cast<__m128i*>(&input.nninput[p][0]);
#endif
						for (int ii = 0; ii < maxRIndex; ++ii) {
							IndexType index = Rindices[p][ii];
							const uint32_t offset = kHalfDimensions * index;

#if defined(USE_AVX2)
							auto column = reinterpret_cast<const __m256i*>(&weights_[offset]);
							for (IndexType j = 0; j < kNumChunks; ++j) {
								accumulation[j] = _mm256_sub_epi16(accumulation[j], column[j]);
							}

#elif defined(USE_SSE2)
							auto column = reinterpret_cast<const __m128i*>(&weights_[offset]);
							for (IndexType j = 0; j < kNumChunks; ++j) {
								accumulation[j] = _mm_sub_epi16(accumulation[j], column[j]);
							}
#else
							for (IndexType j = 0; j < kHalfDimensions; ++j) {
								input.nninput[p][j] -=
									weights_[offset + j];
							}
#endif
						}
					
						{ // Difference calculation for the activated features
							for (int ii = 0; ii < maxAIndex; ++ii) {
								IndexType index = Aindices[p][ii];
								const uint32_t offset = kHalfDimensions * index;

#if defined(USE_AVX2)
								auto column = reinterpret_cast<const __m256i*>(&weights_[offset]);
								for (IndexType j = 0; j < kNumChunks; ++j) {
									accumulation[j] = _mm256_add_epi16(accumulation[j], column[j]);
								}

#elif defined(USE_SSE2)
								auto column = reinterpret_cast<const __m128i*>(&weights_[offset]);
								for (IndexType j = 0; j < kNumChunks; ++j) {
									accumulation[j] = _mm_add_epi16(accumulation[j], column[j]);
								}
#else
								for (IndexType j = 0; j < kHalfDimensions; ++j) {
									input.nninput[p][j] +=
										weights_[offset + j];
								}
#endif

							}
						}
					}
				}
				else {
					input.set = true;
					int maxIndex = 0;
					IndexType indices[2][30];
					for (int i = 0; i < 64; ++i) {
						if (static_cast<int>(input.board[i]) < static_cast<int>(Piece::WKING)) {
							indices[0][maxIndex] = 640 * input.kingSquares[0] + PieceSquareIndex(input.board[i], i);
							++maxIndex;
						}
					}
					maxIndex = 0;
					for (int i = 0; i < 64; ++i) {
						if (static_cast<int>(input.board[i]) < static_cast<int>(Piece::WKING)) {
							indices[1][maxIndex] = 640 * ks + PieceSquareIndex(Invert(input.board[i]), rotate(i));
							++maxIndex;
						}
					}
					assert(maxIndex <= 30);
					for (int i = 0; i < 2; ++i) {
						std::memcpy(input.nninput[i], biases_, kOutputDimensions / 2 * sizeof(BiasType));
						for (int ii = 0; ii < maxIndex; ++ii) {
							IndexType index = indices[i][ii];
							const uint32_t offset = kOutputDimensions / 2 * index;
#if defined(USE_AVX2)
							auto accumulation = reinterpret_cast<__m256i*>(&input.nninput[i][0]);
							auto column = reinterpret_cast<const __m256i*>(&weights_[offset]);
							constexpr IndexType kNumChunks = kOutputDimensions / 2 / (kSimdWidth / 2);
							for (IndexType j = 0; j < kNumChunks; ++j)
								_mm256_storeA_si256(&accumulation[j], _mm256_add_epi16(_mm256_loadA_si256(&accumulation[j]), column[j]));
#elif defined(USE_SSE2)
							auto accumulation = reinterpret_cast<__m128i*>(
								&input.nninput[i][0]);
							auto column = reinterpret_cast<const __m128i*>(&weights_[offset]);
							constexpr IndexType kNumChunks = kHalfDimensions / (kSimdWidth / 2);
							for (IndexType j = 0; j < kNumChunks; ++j)
								accumulation[j] = _mm_add_epi16(accumulation[j], column[j]);

#else
							for (IndexType j = 0; j < kOutputDimensions / 2; ++j)
								input.nninput[i][j] += weights_[offset + j];
#endif
						}
					}
				}
#if defined(USE_AVX2)
				constexpr IndexType kNumChunks = kHalfDimensions / kSimdWidth;
				constexpr int kControl = 0b11011000;
				const __m256i kZero = _mm256_setzero_si256();
#elif defined(USE_SSE2)
				constexpr IndexType kNumChunks = kHalfDimensions / kSimdWidth;

#ifdef USE_SSE41
				const __m128i kZero = _mm_setzero_si128();
#else
				const __m128i k0x80s = _mm_set1_epi8(-128);
#endif
#endif

				for (int i = 0; i < 2; ++i) {
					const IndexType offset = kOutputDimensions / 2 * i;
#if defined(USE_AVX2)
					auto out = reinterpret_cast<__m256i*>(&output[offset]);
					for (IndexType j = 0; j < kNumChunks; ++j) {
						__m256i sum0 = _mm256_loadA_si256(
							&reinterpret_cast<const __m256i*>(input.nninput[i ^ stm])[j * 2 + 0]);
						__m256i sum1 = _mm256_loadA_si256(
							&reinterpret_cast<const __m256i*>(input.nninput[i ^ stm])[j * 2 + 1]);
						_mm256_storeA_si256(&out[j], _mm256_permute4x64_epi64(_mm256_max_epi8(
							_mm256_packs_epi16(sum0, sum1), kZero), kControl));
					}
#elif defined(USE_SSE2)
					auto out = reinterpret_cast<__m128i*>(&output[offset]);
					for (IndexType j = 0; j < kNumChunks; ++j) {
						__m128i sum0 = _mm_load_si128(&reinterpret_cast<const __m128i*>(input.nninput[i ^ stm])[j * 2 + 0]);
						__m128i sum1 = _mm_load_si128(&reinterpret_cast<const __m128i*>(input.nninput[i ^ stm])[j * 2 + 1]);
						const __m128i packedbytes = _mm_packs_epi16(sum0, sum1);

						_mm_store_si128(&out[j],

#ifdef USE_SSE41
							_mm_max_epi8(packedbytes, kZero)
#else
							_mm_subs_epi8(_mm_adds_epi8(packedbytes, k0x80s), k0x80s)
#endif

						);
				}

#else
					for (IndexType j = 0; j < kOutputDimensions / 2; ++j) {
						BiasType sum = input.nninput[i ^ stm][j];
						output[offset + j] = static_cast<OutputType>(
							std::max<int>(0, std::min<int>(127, sum)));
					}
#endif
				}
				return output;
			}

		private:
			using BiasType = std::int16_t;
			using WeightType = std::int16_t;

			alignas(kCacheLineSize) BiasType biases_[kOutputDimensions];
			alignas(kCacheLineSize)
				WeightType weights_[kOutputDimensions * kInputDimensions];
			std::uint32_t header;
		};


		// Affine transformation layer
		template <typename PreviousLayer, IndexType OutputDimensions>
		class AffineTransform {
		public:
			// Input/output type
			using InputType = typename PreviousLayer::OutputType;
			using OutputType = std::int32_t;
			static_assert(std::is_same<InputType, std::uint8_t>::value, "");

			// Number of input/output dimensions
			static constexpr IndexType kInputDimensions =
				PreviousLayer::kOutputDimensions;
			static constexpr IndexType kOutputDimensions = OutputDimensions;
			static constexpr IndexType kPaddedInputDimensions =
				CeilToMultiple<IndexType>(kInputDimensions, kMaxSimdWidth);

			// Size of forward propagation buffer used in this layer
			static constexpr std::size_t kSelfBufferSize =
				CeilToMultiple(kOutputDimensions * sizeof(OutputType), kCacheLineSize);

			// Size of the forward propagation buffer used from the input layer to this layer
			static constexpr std::size_t kBufferSize =
				PreviousLayer::kBufferSize + kSelfBufferSize;

			// Hash value embedded in the evaluation file
			static constexpr std::uint32_t GetHashValue() {
				std::uint32_t hash_value = 0xCC03DAE4u;
				hash_value += kOutputDimensions;
				hash_value ^= PreviousLayer::GetHashValue() >> 1;
				hash_value ^= PreviousLayer::GetHashValue() << 31;
				return hash_value;
			}

			// Read network parameters
			bool ReadParameters(std::istream& stream) {
				if (!previous_layer_.ReadParameters(stream)) return false;
				if constexpr (std::is_same_v<PreviousLayer, InputSlice<PreviousLayer::kOutputDimensions>>) {
					stream.read(reinterpret_cast<char*>(&header), sizeof(header));
				}
				stream.read(reinterpret_cast<char*>(biases_),
					kOutputDimensions * sizeof(BiasType));
				stream.read(reinterpret_cast<char*>(weights_),
					kOutputDimensions * kPaddedInputDimensions *
					sizeof(WeightType));
				return !stream.fail();
			}

			// Forward propagation
			const OutputType* Propagate(InputAdapter& inputAdapter,
				TransformedFeatureType* transformed_features, char* buffer) const {
				const auto input = previous_layer_.Propagate(inputAdapter,
					transformed_features, buffer + kSelfBufferSize);
				const auto output = reinterpret_cast<OutputType*>(buffer);

#if defined(USE_AVX512)
				constexpr IndexType kNumChunks = kPaddedInputDimensions / (kSimdWidth * 2);
				const __m512i kOnes = _mm512_set1_epi16(1);
				const auto input_vector = reinterpret_cast<const __m512i*>(input);

#elif defined(USE_AVX2)
				constexpr IndexType kNumChunks = kPaddedInputDimensions / kSimdWidth;
				const __m256i kOnes = _mm256_set1_epi16(1);
				const auto input_vector = reinterpret_cast<const __m256i*>(input);

#endif

				for (IndexType i = 0; i < kOutputDimensions; ++i) {
					const IndexType offset = i * kPaddedInputDimensions;

#if defined(USE_AVX512)
					__m512i sum = _mm512_setzero_si512();
					const auto row = reinterpret_cast<const __m512i*>(&weights_[offset]);
					for (IndexType j = 0; j < kNumChunks; ++j) {

#if defined(__MINGW32__) || defined(__MINGW64__)
						__m512i product = _mm512_maddubs_epi16(_mm512_loadu_si512(&input_vector[j]), _mm512_load_si512(&row[j]));
#else
						__m512i product = _mm512_maddubs_epi16(_mm512_load_si512(&input_vector[j]), _mm512_load_si512(&row[j]));
#endif

						product = _mm512_madd_epi16(product, kOnes);
						sum = _mm512_add_epi32(sum, product);
					}
					output[i] = _mm512_reduce_add_epi32(sum) + biases_[i];

					// Note: Changing kMaxSimdWidth from 32 to 64 breaks loading existing networks.
					// As a result kPaddedInputDimensions may not be an even multiple of 64(512bit)
					// and we have to do one more 256bit chunk.
					if (kPaddedInputDimensions != kNumChunks * kSimdWidth * 2)
					{
						const auto iv_256 = reinterpret_cast<const __m256i*>(input);
						const auto row_256 = reinterpret_cast<const __m256i*>(&weights_[offset]);
						int j = kNumChunks * 2;

#if defined(__MINGW32__) || defined(__MINGW64__)  // See HACK comment below in AVX2.
						__m256i sum256 = _mm256_maddubs_epi16(_mm256_loadu_si256(&iv_256[j]), _mm256_load_si256(&row_256[j]));
#else
						__m256i sum256 = _mm256_maddubs_epi16(_mm256_load_si256(&iv_256[j]), _mm256_load_si256(&row_256[j]));
#endif

						sum256 = _mm256_madd_epi16(sum256, _mm256_set1_epi16(1));
						sum256 = _mm256_hadd_epi32(sum256, sum256);
						sum256 = _mm256_hadd_epi32(sum256, sum256);
						const __m128i lo = _mm256_extracti128_si256(sum256, 0);
						const __m128i hi = _mm256_extracti128_si256(sum256, 1);
						output[i] += _mm_cvtsi128_si32(lo) + _mm_cvtsi128_si32(hi);
					}

#elif defined(USE_AVX2)
					__m256i sum = _mm256_setzero_si256();
					const auto row = reinterpret_cast<const __m256i*>(&weights_[offset]);
					for (IndexType j = 0; j < kNumChunks; ++j) {
						__m256i product = _mm256_maddubs_epi16(

#if defined(__MINGW32__) || defined(__MINGW64__)
							// HACK: Use _mm256_loadu_si256() instead of _mm256_load_si256. Because the binary
							//       compiled with g++ in MSYS2 crashes here because the output memory is not aligned
							//       even though alignas is specified.
							_mm256_loadu_si256
#else
							_mm256_load_si256
#endif

							(&input_vector[j]), _mm256_load_si256(&row[j]));
						product = _mm256_madd_epi16(product, kOnes);
						sum = _mm256_add_epi32(sum, product);
					}
					sum = _mm256_hadd_epi32(sum, sum);
					sum = _mm256_hadd_epi32(sum, sum);
					const __m128i lo = _mm256_extracti128_si256(sum, 0);
					const __m128i hi = _mm256_extracti128_si256(sum, 1);
					output[i] = _mm_cvtsi128_si32(lo) + _mm_cvtsi128_si32(hi) + biases_[i];


#else
					OutputType sum = biases_[i];
					for (IndexType j = 0; j < kInputDimensions; ++j) {
						sum += weights_[offset + j] * input[j];
					}
					output[i] = sum;
#endif

				}
				return output;
			}

		private:
			using BiasType = OutputType;
			using WeightType = std::int8_t;

			PreviousLayer previous_layer_;

			alignas(kCacheLineSize) BiasType biases_[kOutputDimensions];
			alignas(kCacheLineSize)
				WeightType weights_[kOutputDimensions * kPaddedInputDimensions];
			std::uint32_t header;
		};

		template <typename PreviousLayer>
		class ClippedReLU {
		public:
			// Input/output type
			using InputType = typename PreviousLayer::OutputType;
			using OutputType = std::uint8_t;
			static_assert(std::is_same<InputType, std::int32_t>::value, "");

			// Number of input/output dimensions
			static constexpr IndexType kInputDimensions =
				PreviousLayer::kOutputDimensions;
			static constexpr IndexType kOutputDimensions = kInputDimensions;

			// Size of forward propagation buffer used in this layer
			static constexpr std::size_t kSelfBufferSize =
				CeilToMultiple(kOutputDimensions * sizeof(OutputType), kCacheLineSize);

			// Size of the forward propagation buffer used from the input layer to this layer
			static constexpr std::size_t kBufferSize =
				PreviousLayer::kBufferSize + kSelfBufferSize;

			// Hash value embedded in the evaluation file
			static constexpr std::uint32_t GetHashValue() {
				std::uint32_t hash_value = 0x538D24C7u;
				hash_value += PreviousLayer::GetHashValue();
				return hash_value;
			}

			// Read network parameters
			bool ReadParameters(std::istream& stream) {
				return previous_layer_.ReadParameters(stream);
			}

			// Forward propagation
			const OutputType* Propagate(InputAdapter& inputAdapter,
				TransformedFeatureType* transformed_features, char* buffer) const {
				const auto input = previous_layer_.Propagate(inputAdapter,
					transformed_features, buffer + kSelfBufferSize);
				const auto output = reinterpret_cast<OutputType*>(buffer);

#if defined(USE_AVX2)
				constexpr IndexType kNumChunks = kInputDimensions / kSimdWidth;
				const __m256i kZero = _mm256_setzero_si256();
				const __m256i kOffsets = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
				const auto in = reinterpret_cast<const __m256i*>(input);
				const auto out = reinterpret_cast<__m256i*>(output);
				for (IndexType i = 0; i < kNumChunks; ++i) {
					const __m256i words0 = _mm256_srai_epi16(_mm256_packs_epi32(

#if defined(__MINGW32__) || defined(__MINGW64__)
						// HACK: Use _mm256_loadu_si256() instead of _mm256_load_si256. Because the binary
						//       compiled with g++ in MSYS2 crashes here because the output memory is not aligned
						//       even though alignas is specified.
						_mm256_loadu_si256
#else
						_mm256_load_si256
#endif

						(&in[i * 4 + 0]),

#if defined(__MINGW32__) || defined(__MINGW64__)
						_mm256_loadu_si256
#else
						_mm256_load_si256
#endif

						(&in[i * 4 + 1])), kWeightScaleBits);
					const __m256i words1 = _mm256_srai_epi16(_mm256_packs_epi32(

#if defined(__MINGW32__) || defined(__MINGW64__)
						_mm256_loadu_si256
#else
						_mm256_load_si256
#endif

						(&in[i * 4 + 2]),

#if defined(__MINGW32__) || defined(__MINGW64__)
						_mm256_loadu_si256
#else
						_mm256_load_si256
#endif

						(&in[i * 4 + 3])), kWeightScaleBits);

#if defined(__MINGW32__) || defined(__MINGW64__)
					_mm256_storeu_si256
#else
					_mm256_store_si256
#endif

						(&out[i], _mm256_permutevar8x32_epi32(_mm256_max_epi8(
							_mm256_packs_epi16(words0, words1), kZero), kOffsets));
				}
				constexpr IndexType kStart = kNumChunks * kSimdWidth;

#else
				constexpr IndexType kStart = 0;
#endif

				for (IndexType i = kStart; i < kInputDimensions; ++i) {
					output[i] = static_cast<OutputType>(
						std::max(0, std::min(127, input[i] >> kWeightScaleBits)));
				}
				return output;
			}

		private:
			PreviousLayer previous_layer_;
			std::uint32_t header;
		};

		// Input layer

		using InputLayer = InputSlice<512>;
		using HiddenLayer1 = ClippedReLU<AffineTransform<InputLayer, 32>>;
		using HiddenLayer2 = ClippedReLU<AffineTransform<HiddenLayer1, 32>>;
		using OutputLayer = AffineTransform<HiddenLayer2, 1>;
	}

	using network = layer::OutputLayer;
	constexpr uint32_t ID_REDUCED_NETWORK = 0x1c4dfd36;

	class Network
	{
	public:
		Network() {
			std::cout << "Network instantiated.." << std::endl;
		}
		bool Load(std::string filename);
		int16_t score(InputAdapter& input) const;
		static std::string convert(std::string ifile);
		static std::string convert(std::string ifile, std::string ofile);
	private:
		AlignedPtr<network> nn;
		std::uint32_t version;
		std::uint32_t hash_value;
		std::string architecture;
	};
}
