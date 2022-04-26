/****************************************************************************
 *
 *   Copyright (c) 2020-2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file zero_order_thrust_ekf.cpp
 *
 * @author Mathieu Bresciani 	<brescianimathieu@gmail.com>
 */

#include "zero_order_hover_thrust_ekf.hpp"

using matrix::sign;
using D = matrix::Dual<float, 1>;

void ZeroOrderHoverThrustEkf::predict(const float dt)
{
	// State is constant
	// Predict state covariance only
	_state_var += _process_var * dt * dt;
	_dt = dt;
}

void ZeroOrderHoverThrustEkf::fuseAccZ(const float acc_z, const float thrust)
{
	D predicted_meas = computePredictedAccZ(D(_hover_thr, 0), thrust);
	float &H = predicted_meas.derivative(0);
	const float innov_var = computeInnovVar(H);
	const float innov = acc_z - predicted_meas.value;
	const float K = _state_var * H / innov_var;
	const float innov_test_ratio = innov * innov / (_gate_size * _gate_size *
				       innov_var); // ratio between the Normalized Innovation Squared (NIS) and its gate size.

	float residual = innov;

	if (innov_test_ratio < 1.f) {
		_hover_thr = math::constrain(_hover_thr + K * innov, _hover_thr_min, _hover_thr_max);
		_state_var = math::constrain((1.f - K * H) * _state_var, 1e-10f, 1.f);

		// Update residual and observation jacobian since the hover thrust changed
		predicted_meas = computePredictedAccZ(D(_hover_thr, 0), thrust);
		residual = acc_z - predicted_meas.value;

	} else if (fabsf(_signed_innov_test_ratio_lpf) > 0.2f) {
		// Rejecting all the measurements for some time,
		// it means that the hover thrust suddenly changed or that the EKF
		// is diverging. To recover, we bump the state variance
		_state_var += 1e3f * _process_var * _dt * _dt;
	}

	const float signed_innov_test_ratio = sign(innov) * innov_test_ratio;
	updateLpf(residual, signed_innov_test_ratio);
	updateMeasurementNoise(residual, H);

	// save for logging
	_innov = innov;
	_innov_var = innov_var;
	_innov_test_ratio = innov_test_ratio;
}

inline float ZeroOrderHoverThrustEkf::computeInnovVar(const float H) const
{
	const float R = _acc_var * _acc_var_scale;
	const float P = _state_var;
	return math::max(H * P * H + R, R);
}

D ZeroOrderHoverThrustEkf::computePredictedAccZ(const D &hover_thrust, float thrust) const
{
	return CONSTANTS_ONE_G * thrust / hover_thrust - CONSTANTS_ONE_G;
}

inline void ZeroOrderHoverThrustEkf::updateLpf(const float residual, const float signed_innov_test_ratio)
{
	const float alpha = _dt / (_lpf_time_constant + _dt);
	_residual_lpf = (1.f - alpha) * _residual_lpf + alpha * residual;
	_signed_innov_test_ratio_lpf = (1.f - alpha) * _signed_innov_test_ratio_lpf + alpha * math::constrain(
					       signed_innov_test_ratio, -1.f, 1.f);
}

inline void ZeroOrderHoverThrustEkf::updateMeasurementNoise(const float residual, const float H)
{
	const float alpha = _dt / (_noise_learning_time_constant + _dt);
	const float res_no_bias = residual - _residual_lpf;
	const float P = _state_var;
	_acc_var = math::constrain((1.f - alpha) * _acc_var  + alpha * (res_no_bias * res_no_bias + H * P * H), 1.f, 400.f);
}
