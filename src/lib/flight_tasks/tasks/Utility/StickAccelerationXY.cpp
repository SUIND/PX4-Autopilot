/****************************************************************************
 *
 *   Copyright (c) 2020 PX4 Development Team. All rights reserved.
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
 *    without spec{fic prior written permission.
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
 * @file StickAccelerationXY.cpp
 */

#include "StickAccelerationXY.hpp"

#include <ecl/geo/geo.h>
#include "Sticks.hpp"

StickAccelerationXY::StickAccelerationXY(ModuleParams *parent) :
	ModuleParams(parent)
{
	_brake_boost_filter.reset(1.f);
}

void StickAccelerationXY::resetAcceleration(const matrix::Vector2f &acceleration)
{
	_acceleration_slew_rate_x.setForcedValue(acceleration(0));
	_acceleration_slew_rate_x.setForcedValue(acceleration(1));
}

void StickAccelerationXY::generateSetpoints(Vector2f stick_xy, const float yaw, const float yaw_sp, const Vector3f &pos,
		const float dt, Vector3f &pos_sp, Vector3f &vel_sp, Vector3f &acc_sp)
{
	// maximum commanded acceleration and velocity
	Vector2f acceleration_scale(_param_mpc_acc_hor.get(), _param_mpc_acc_hor.get());
	Vector2f velocity_scale(_param_mpc_vel_manual.get(), _param_mpc_vel_manual.get());

	acceleration_scale *= 2.f; // because of drag the average aceleration is half

	// Map stick input to acceleration
	Sticks::limitStickUnitLengthXY(stick_xy);
	Sticks::rotateIntoHeadingFrameXY(stick_xy, yaw, yaw_sp);
	Vector2f acceleration_xy = stick_xy.emult(acceleration_scale);
	applyFeasibilityLimit(acceleration_xy, dt);

	// Add drag to limit speed and brake again
	acceleration_xy -= calculateDrag(acceleration_scale.edivide(velocity_scale), dt, stick_xy, vel_sp);

	applyTiltLimit(acceleration_xy);
	acc_sp.xy() = acceleration_xy;

	// Generate velocity setpoint by forward integrating commanded acceleration
	Vector2f velocity_xy(vel_sp);
	velocity_xy += Vector2f(acc_sp.xy()) * dt;
	vel_sp.xy() = velocity_xy;

	lockPosition(vel_sp, pos, dt, pos_sp);
}

void StickAccelerationXY::applyFeasibilityLimit(Vector2f &acceleration, const float dt)
{
	// Apply jerk limit - acceleration slew rate
	_acceleration_slew_rate_x.setSlewRate(_param_mpc_jerk_max.get());
	_acceleration_slew_rate_y.setSlewRate(_param_mpc_jerk_max.get());
	acceleration(0) = _acceleration_slew_rate_x.update(acceleration(0), dt);
	acceleration(1) = _acceleration_slew_rate_y.update(acceleration(1), dt);
}

Vector2f StickAccelerationXY::calculateDrag(Vector2f drag_coefficient, const float dt, const Vector2f &stick_xy,
		const Vector3f &vel_sp)
{
	_brake_boost_filter.setParameters(dt, .8f);

	if (stick_xy.norm_squared() < FLT_EPSILON) {
		_brake_boost_filter.update(2.f);

	} else {
		_brake_boost_filter.update(1.f);
	}

	drag_coefficient *= _brake_boost_filter.getState();

	return drag_coefficient.emult(vel_sp.xy());
}

void StickAccelerationXY::applyTiltLimit(Vector2f &acceleration)
{
	// Check if acceleration would exceed the tilt limit
	const float acc = acceleration.length();
	const float acc_tilt_max = tanf(M_DEG_TO_RAD_F * _param_mpc_tiltmax_air.get()) * CONSTANTS_ONE_G;

	if (acc > acc_tilt_max) {
		acceleration *= acc_tilt_max / acc;
	}
}

void StickAccelerationXY::lockPosition(const Vector3f &vel_sp, const Vector3f &pos, const float dt, Vector3f &pos_sp)
{
	if (Vector2f(vel_sp).norm_squared() < FLT_EPSILON) {
		Vector2f position_xy(pos_sp);

		if (!PX4_ISFINITE(position_xy(0))) {
			position_xy = Vector2f(pos);
		}

		position_xy += Vector2f(vel_sp.xy()) * dt;
		pos_sp.xy() = position_xy;

	} else {
		pos_sp.xy() = NAN;

	}
}
