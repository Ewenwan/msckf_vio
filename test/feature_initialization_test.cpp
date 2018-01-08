/*
 *This file is part of msckf_vio
 *
 *    msckf_vio is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    msckf_vio is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with msckf_vio.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <vector>
#include <map>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <Eigen/StdVector>

#include <gtest/gtest.h>
#include <random_numbers/random_numbers.h>

#include <msckf_vio/cam_state.h>
#include <msckf_vio/feature.hpp>


using namespace std;
using namespace Eigen;
using namespace msckf_vio;

TEST(FeatureInitializeTest, sphereDistribution) {
  // Set the real feature at the origin of the world frame.
  Vector3d feature(0.5, 0.0, 0.0);

  // Add 6 camera poses, all of which are able to see the
  // feature at the origin. For simplicity, the six camera
  // view are located at the six intersections between a
  // unit sphere and the coordinate system. And the z axes
  // of the camera frames are facing the origin.
  vector<Isometry3d> cam_poses(6);
  // Positive x axis.
  cam_poses[0].linear() << 0.0,  0.0, -1.0,
    1.0,  0.0,  0.0, 0.0, -1.0,  0.0;
  cam_poses[0].translation() << 1.0,  0.0,  0.0;
  // Positive y axis.
  cam_poses[1].linear() << -1.0,  0.0,  0.0,
     0.0,  0.0, -1.0, 0.0, -1.0,  0.0;
  cam_poses[1].translation() << 0.0,  1.0,  0.0;
  // Negative x axis.
  cam_poses[2].linear() << 0.0,  0.0,  1.0,
    -1.0,  0.0,  0.0, 0.0, -1.0,  0.0;
  cam_poses[2].translation() << -1.0,  0.0,  0.0;
  // Negative y axis.
  cam_poses[3].linear() << 1.0,  0.0,  0.0,
     0.0,  0.0,  1.0, 0.0, -1.0,  0.0;
  cam_poses[3].translation() << 0.0, -1.0,  0.0;
  // Positive z axis.
  cam_poses[4].linear() << 0.0, -1.0,  0.0,
    -1.0,  0.0,  0.0, 0.0, 0.0, -1.0;
  cam_poses[4].translation() << 0.0,  0.0,  1.0;
  // Negative z axis.
  cam_poses[5].linear() << 1.0,  0.0,  0.0,
     0.0,  1.0,  0.0, 0.0,  0.0,  1.0;
  cam_poses[5].translation() << 0.0,  0.0, -1.0;

  // Set the camera states
  CamStateServer cam_states;
  for (int i = 0; i < 6; ++i) {
    CAMState new_cam_state;
    new_cam_state.id = i;
    new_cam_state.time = static_cast<double>(i);
    new_cam_state.orientation = rotationToQuaternion(
        Matrix3d(cam_poses[i].linear().transpose()));
    new_cam_state.position = cam_poses[i].translation();
    cam_states[new_cam_state.id] = new_cam_state;
  }

  // Compute measurements.
  random_numbers::RandomNumberGenerator noise_generator;
  vector<Vector2d, aligned_allocator<Vector2d> > measurements(6);
  for (int i = 0; i < 6; ++i) {
    Isometry3d cam_pose_inv = cam_poses[i].inverse();
    Vector3d p = cam_pose_inv.linear()*feature + cam_pose_inv.translation();
    double u = p(0) / p(2) + noise_generator.gaussian(0.0, 0.01);
    double v = p(1) / p(2) + noise_generator.gaussian(0.0, 0.01);
    //double u = p(0) / p(2);
    //double v = p(1) / p(2);
    measurements[i] = Vector2d(u, v);
  }

  for (int i = 0; i < 6; ++i) {
    cout << "pose " << i << ":" << endl;
    cout << "orientation: " << endl;
    cout << cam_poses[i].linear() << endl;
    cout << "translation: "  << endl;
    cout << cam_poses[i].translation().transpose() << endl;
    cout << "measurement: " << endl;
    cout << measurements[i].transpose() << endl;
    cout << endl;
  }

  // Initialize a feature object.
  Feature feature_object;
  for (int i = 0; i < 6; ++i)
    feature_object.observations[i] = measurements[i];

  // Compute the 3d position of the feature.
  feature_object.initializePosition(cam_states);

  // Check the difference between the computed 3d
  // feature position and the groud truth.
  cout << "ground truth position: " << feature.transpose() << endl;
  cout << "estimated position: " << feature_object.position.transpose() << endl;
  Eigen::Vector3d error = feature_object.position - feature;
  EXPECT_NEAR(error.norm(), 0, 0.05);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

