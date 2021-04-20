#pragma once

#include <fstream>
#include <string>
#include <memory>

#include <eigen3/Eigen/Dense>

#include "messaging.hpp"
#include "common/params.h"
#include "common/util.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include "common/transformations/coordinates.hpp"
#include "common/transformations/orientation.hpp"
#include "selfdrive/sensord/sensors/constants.hpp"

#include "models/live_kf.h"

#define VISION_DECIMATION 2
#define SENSOR_DECIMATION 10

#define POSENET_STD_HIST_HALF 20

class Localizer {
public:
  Localizer();

  int locationd_thread();

  void reset_kalman(double current_time = NAN);
  void reset_kalman(double current_time, Eigen::VectorXd init_orient, Eigen::VectorXd init_pos);

  kj::ArrayPtr<capnp::byte> get_message_bytes(MessageBuilder& msg_builder, uint64_t logMonoTime,
    bool inputsOK, bool sensorsOK, bool gpsOK);
  void build_live_location(cereal::LiveLocationKalman::Builder& fix);

  Eigen::VectorXd get_position_geodetic();

  void handle_msg_bytes(const char *data, const size_t size);
  void handle_msg(const cereal::Event::Reader& log);
  void handle_sensors(double current_time, const capnp::List<cereal::SensorEventData, capnp::Kind::STRUCT>::Reader& log);
  void handle_gps(double current_time, const cereal::GpsLocationData::Reader& log);
  void handle_car_state(double current_time, const cereal::CarState::Reader& log);
  void handle_cam_odo(double current_time, const cereal::CameraOdometry::Reader& log);
  void handle_live_calib(double current_time, const cereal::LiveCalibrationData::Reader& log);

private:
  std::unique_ptr<LiveKalman> kf;

  Eigen::VectorXd calib;
  MatrixXdr device_from_calib;
  MatrixXdr calib_from_device;
  bool calibrated = false;

  int car_speed = 0;
  std::deque<double> posenet_stds;

  std::unique_ptr<LocalCoord> converter;

  int64_t unix_timestamp_millis = 0;
  double last_gps_fix = 0;
  bool device_fell = false;

  int gyro_counter = 0;
  int acc_counter = 0;
  int speed_counter = 0;
  int cam_counter = 0;
};
