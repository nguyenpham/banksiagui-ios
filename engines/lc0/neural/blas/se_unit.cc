/*
 This file is part of Leela Chess Zero.
 Copyright (C) 2018 The LCZero Authors

 Leela Chess is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Leela Chess is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Leela Chess.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "neural/blas/se_unit.h"
#include "neural/blas/fully_connected_layer.h"

#include <cmath>

namespace lczero {
namespace {
constexpr int kWidth = 8;
constexpr int kHeight = 8;
constexpr int kSquares = kWidth * kHeight;
}  // namespace

static void global_avg_pooling(const size_t channels, const float* input,
                               float* output) {
  for (auto c = size_t{0}; c < channels; c++) {
    auto acc = 0.0f;
    for (auto i = size_t{0}; i < kSquares; i++) {
      acc += input[c * kSquares + i];
    }
    output[c] = acc / kSquares;
  }
}

static void apply_se(const size_t channels, const size_t batch_size,
                     const float* input, const float* res, const float* scale,
                     float* output) {
  const auto lambda_ReLU = [](const auto val) {
    return (val > 0.0f) ? val : 0;
  };

  const auto lambda_sigmoid = [](const auto val) {
    return 1.0f / (1.0f + std::exp(-val));
  };

  for (auto c = size_t{0}; c < channels * batch_size; c++) {
    auto batch = c / channels;
    auto gamma = lambda_sigmoid(scale[c + batch * channels]);
    auto beta = scale[c + batch * channels + channels];
    for (auto i = size_t{0}; i < kSquares; i++) {
      output[c * kSquares + i] = lambda_ReLU(gamma * input[c * kSquares + i] +
                                             beta + res[c * kSquares + i]);
    }
  }
}

template <bool use_eigen>
void ApplySEUnit(const size_t batch_size, const size_t channels,
                 const size_t se_fc_outputs, const float* input,
                 const float* residual, const float* weights_w1,
                 const float* weights_b1, const float* weights_w2,
                 const float* weights_b2, float* output) {
  std::vector<float> pool(2 * channels * batch_size);
  std::vector<float> fc_out1(batch_size * se_fc_outputs);

  global_avg_pooling(channels * batch_size, input, pool.data());

  FullyConnectedLayer<use_eigen>::Forward1D(batch_size, channels, se_fc_outputs,
                                            pool.data(), weights_w1, weights_b1,
                                            true,  // Relu On
                                            fc_out1.data());

  FullyConnectedLayer<use_eigen>::Forward1D(batch_size, se_fc_outputs,
                                            2 * channels, fc_out1.data(),
                                            weights_w2, weights_b2,
                                            false,  // Relu Off
                                            pool.data());

  // Sigmoid, scale and add residual
  apply_se(channels, batch_size, input, residual, pool.data(), output);
}

template void ApplySEUnit<true>(const size_t batch_size, const size_t channels,
                                const size_t se_fc_outputs, const float* input,
                                const float* residual, const float* weights_w1,
                                const float* weights_b1,
                                const float* weights_w2,
                                const float* weights_b2, float* output);
#ifdef USE_BLAS
template void ApplySEUnit<false>(const size_t batch_size, const size_t channels,
                                 const size_t se_fc_outputs, const float* input,
                                 const float* residual, const float* weights_w1,
                                 const float* weights_b1,
                                 const float* weights_w2,
                                 const float* weights_b2, float* output);
#endif
}  // namespace lczero
