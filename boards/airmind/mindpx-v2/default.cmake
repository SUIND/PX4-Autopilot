
px4_add_board(
	PLATFORM nuttx
	TOOLCHAIN arm-none-eabi
	ARCHITECTURE cortex-m4
	CONSTRAINED_MEMORY
	ROMFSROOT px4fmu_common
	UAVCAN_INTERFACES 1
	SERIAL_PORTS
		GPS1:/dev/ttyS3
		TEL1:/dev/ttyS1
		TEL2:/dev/ttyS2
	DRIVERS
		adc/ads1115
		adc/board_adc
		barometer # all available barometer drivers
		batt_smbus
		camera_capture
		camera_trigger
		differential_pressure # all available differential pressure drivers
		distance_sensor # all available distance sensor drivers
		#dshot
		gps
		#heater
		#imu # all available imu drivers
		imu/invensense/icm20948 # required for ak09916 mag
		imu/l3gd20
		imu/lsm303d
		imu/invensense/mpu6000
		imu/invensense/mpu6500
		irlock
		lights # all available light drivers
		magnetometer # all available magnetometer drivers
		optical_flow # all available optical flow drivers
		osd
		pca9685
		pca9685_pwm_out
		power_monitor/ina226
		#protocol_splitter
		pwm_input
		pwm_out_sim
		pwm_out
		rc_input
		roboclaw
		rpm
		smart_battery/batmon
		telemetry # all available telemetry drivers
		tone_alarm
		uavcan
	MODULES
		airspeed_selector
		attitude_estimator_q
		battery_status
		camera_feedback
		commander
		dataman
		ekf2
		esc_battery
		events
		flight_mode_manager
		fw_att_control
		#fw_autotune_attitude_control
		fw_pos_control_l1
		gyro_calibration
		gyro_fft
		land_detector
		landing_target_estimator
		load_mon
		local_position_estimator
		logger
		mag_bias_estimator
		mavlink
		mc_att_control
		#mc_autotune_attitude_control
		mc_hover_thrust_estimator
		mc_pos_control
		mc_rate_control
		#micrortps_bridge
		navigator
		rc_update
		manual_control
		rover_pos_control
		sensors
		sih
		temperature_compensation
		uuv_att_control
		uuv_pos_control
		vmount
		vtol_att_control
	SYSTEMCMDS
		bl_update
		#dmesg
		dumpfile
		esc_calib
		gpio
		hardfault_log
		i2cdetect
		led_control
		mft
		mixer
		motor_ramp
		motor_test
		mtd
		nshterm
		param
		perf
		pwm
		reboot
		reflect
		sd_bench
		serial_test
		system_time
		top
		topic_listener
		tune_control
		uorb
		usb_connected
		ver
		work_queue
	EXAMPLES
		fake_gps
		#fake_imu
		#fake_magnetometer
		#fixedwing_control # Tutorial code from https://px4.io/dev/example_fixedwing_control
		#hello
		#hwtest # Hardware test
		#matlab_csv_serial
		#px4_mavlink_debug # Tutorial code from http://dev.px4.io/en/debug/debug_values.html
		#px4_simple_app # Tutorial code from http://dev.px4.io/en/apps/hello_sky.html
		#rover_steering_control # Rover example app
		#uuv_example_app
		#work_item
	)
