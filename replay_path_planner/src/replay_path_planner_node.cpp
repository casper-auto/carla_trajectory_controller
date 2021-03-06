#include <iostream>

// ros
#include <ros/ros.h>
#include <tf/LinearMath/Matrix3x3.h>
#include <tf/transform_datatypes.h>

// msg
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>
#include <std_msgs/Float32.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>

#include <casper_auto_msgs/Waypoint.h>
#include <casper_auto_msgs/WaypointArray.h>

#include "replay_path_planner.h"

using namespace std;

vector<double> current_pose(3);
double current_speed;
vector<vector<double>> global_route;
double cruise_speed = 5.0;

double mps2kmph(double velocity_mps) { return (velocity_mps * 60 * 60) / 1000; }

void currentOdometryCallback(const nav_msgs::Odometry::ConstPtr &msg) {
  ROS_INFO("Received the current odometry in replay path planner ...");

  geometry_msgs::Quaternion geo_quat = msg->pose.pose.orientation;

  // the incoming geometry_msgs::Quaternion is transformed to a tf::Quaterion
  tf::Quaternion quat;
  tf::quaternionMsgToTF(geo_quat, quat);

  // the tf::Quaternion has a method to acess roll pitch and yaw
  double roll, pitch, yaw;
  tf::Matrix3x3(quat).getRPY(roll, pitch, yaw);

  current_pose[0] = msg->pose.pose.position.x;
  current_pose[1] = msg->pose.pose.position.y;
  current_pose[2] = yaw;

  double vx = msg->twist.twist.linear.x;
  double vy = msg->twist.twist.linear.y;

  current_speed = sqrt(vx*vx + vy*vy);

  // ROS_INFO("Current pose: x: %.2f, y: %.2f, yaw: %.2f", current_pose[0],
  // current_pose[1], current_pose[2]);
}

void cruiseSpeedCallback(const std_msgs::Float32::ConstPtr &msg) {
  cruise_speed = msg->data;
}

void globalRouteCallback(const nav_msgs::Path::ConstPtr &msg) {
  ROS_INFO("Received the global route  in replay path planner ...");
  global_route.resize(0);
  for (int i = 0; i < msg->poses.size(); i++) {
    double x = msg->poses[i].pose.position.x;
    double y = msg->poses[i].pose.position.y;
    double z = msg->poses[i].pose.position.z;

    geometry_msgs::Quaternion geo_quat = msg->poses[i].pose.orientation;
    // the incoming geometry_msgs::Quaternion is transformed to a tf::Quaterion
    tf::Quaternion quat;
    tf::quaternionMsgToTF(geo_quat, quat);
    // the tf::Quaternion has a method to acess roll pitch and yaw
    double roll, pitch, yaw;
    tf::Matrix3x3(quat).getRPY(roll, pitch, yaw);

    // skip overlapped points
    if (global_route.size() == 0) {
      global_route.push_back({x, y, z, yaw});
    } else if ((global_route.size() > 0) &&
               !(abs(x - global_route[global_route.size() - 1][0]) < 0.001 &&
                 abs(y - global_route[global_route.size() - 1][1]) < 0.001)) {
      global_route.push_back({x, y, z, yaw});
    }
  }
  ROS_INFO("Received %d valid points in the global route.",
           int(global_route.size()));
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "replay_path_planner");
  ros::NodeHandle n("~");

  //////////////////////////////////////////////////////////////////////////////
  // ROS Parameter server
  //////////////////////////////////////////////////////////////////////////////
  string role_name;
  n.param<string>("role_name", role_name, "ego_vehicle");

  //////////////////////////////////////////////////////////////////////////////
  // ROS Subscriber and Publisher
  //////////////////////////////////////////////////////////////////////////////
  ros::Subscriber current_odom_sub = n.subscribe(
      "/carla/" + role_name + "/odometry", 10, currentOdometryCallback);
  ros::Subscriber global_route_sub = n.subscribe(
      "/carla/" + role_name + "/waypoints", 10, globalRouteCallback);
  ros::Subscriber cruise_speed_sub =
      n.subscribe("/casper_auto/cruise_speed", 10, cruiseSpeedCallback);

  ros::Publisher final_path_pub =
      n.advertise<nav_msgs::Path>("/casper_auto/final_path", 10);
  ros::Publisher final_waypoints_pub =
      n.advertise<casper_auto_msgs::WaypointArray>("/casper_auto/final_waypoints", 10);
  ros::Publisher final_path_marker_pub =
      n.advertise<visualization_msgs::MarkerArray>("/casper_auto/final_path_marker", 10);

  ros::Rate rate(60);

  double path_length = 20;

  ReplayPathPlanner replay_path_planner(path_length);

  double prev_timestamp = ros::Time::now().toSec();
  vector<vector<double>> final_path =
      replay_path_planner.get_replayed_path(current_pose, global_route);

  while (ros::ok()) {
    ////////////////////////////////////////////////////////////////////////////
    // Run Replay Path Planner every single second
    ////////////////////////////////////////////////////////////////////////////
    double current_timestamp = ros::Time::now().toSec();
    if (current_timestamp - prev_timestamp > 1.0 && !global_route.empty()) {
      final_path =
          replay_path_planner.get_replayed_path(current_pose, global_route);
      // final_path = replay_path_planner.get_interpolated_path(final_path);
      prev_timestamp = current_timestamp;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Publish the ROS message containing the waypoints to the controller
    ////////////////////////////////////////////////////////////////////////////
    casper_auto_msgs::WaypointArray waypoints;
    waypoints.header.frame_id = "map";
    for (int i = 0; i < final_path.size(); i++) {
      casper_auto_msgs::Waypoint wp;
      wp.pose.pose.position.x = final_path[i][0];
      wp.pose.pose.position.y = final_path[i][1];
      wp.twist.twist.linear.x = cruise_speed;
      waypoints.waypoints.push_back(wp);
    }

    final_waypoints_pub.publish(waypoints);

    ////////////////////////////////////////////////////////////////////////////
    // Publish the ROS message containing the path
    ////////////////////////////////////////////////////////////////////////////
    nav_msgs::Path path;
    path.header.frame_id = "map";
    // path.header.stamp = ros::Time::now();
    for (int i = 0; i < final_path.size(); i++) {
      geometry_msgs::PoseStamped pose;
      pose.pose.position.x = final_path[i][0];
      pose.pose.position.y = final_path[i][1];
      pose.pose.position.z = final_path[i][2];

      geometry_msgs::Quaternion quat =
          tf::createQuaternionMsgFromYaw(final_path[i][2]);
      pose.pose.orientation = quat;
      path.poses.push_back(pose);
    }

    final_path_pub.publish(path);

    ////////////////////////////////////////////////////////////////////////////
    // Construct and Publish generated paths as visualization markers
    ////////////////////////////////////////////////////////////////////////////
    std_msgs::ColorRGBA waypoint_color;
    waypoint_color.a = 1.0;
    waypoint_color.g = 0.7;
    waypoint_color.b = 1.0;

    visualization_msgs::MarkerArray marker_array;

    visualization_msgs::Marker waypoint_marker;
    waypoint_marker.header.frame_id = "map";
    waypoint_marker.header.stamp = ros::Time::now();
    waypoint_marker.ns = "local_path_marker";
    waypoint_marker.id = 0;
    waypoint_marker.type = visualization_msgs::Marker::LINE_STRIP;
    waypoint_marker.action = visualization_msgs::Marker::ADD;
    waypoint_marker.scale.x = 0.2;
    waypoint_marker.color = waypoint_color;
    waypoint_marker.frame_locked = true;
    waypoint_marker.pose.orientation.w = 1.0;

    for (unsigned int i = 0; i < static_cast<int>(final_path.size()); i++) {
      geometry_msgs::Point point;
      point.x = final_path[i][0];
      point.y = final_path[i][1];
      point.z = final_path[i][2] + 0.2;
      waypoint_marker.points.push_back(point);
    }

    marker_array.markers.push_back(waypoint_marker);

    std_msgs::ColorRGBA velocity_color;
    velocity_color.a = 1.0;
    velocity_color.g = 0.7;
    velocity_color.b = 1.0;

    visualization_msgs::Marker velocity_marker;
    velocity_marker.header.frame_id = "map";
    velocity_marker.header.stamp = ros::Time::now();
    velocity_marker.ns = "local_waypoint_velocity";
    velocity_marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
    velocity_marker.action = visualization_msgs::Marker::ADD;
    velocity_marker.scale.z = 0.4;
    velocity_marker.color = velocity_color;
    velocity_marker.frame_locked = true;

    for (int i = 0; i < static_cast<int>(final_path.size()); i++) {
      velocity_marker.id = i;
      velocity_marker.pose.position.x = final_path[i][0];
      velocity_marker.pose.position.y = final_path[i][1];
      velocity_marker.pose.position.z = final_path[i][2] + 0.4;
      velocity_marker.pose.orientation.w = 1.0;

      // double to string
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << mps2kmph(cruise_speed);
      velocity_marker.text = oss.str();

      marker_array.markers.push_back(velocity_marker);
    }

    visualization_msgs::Marker waypoint_cube_marker;
    waypoint_cube_marker.header.frame_id = "map";
    waypoint_cube_marker.header.stamp = ros::Time::now();
    waypoint_cube_marker.ns = "local_point_marker";
    waypoint_cube_marker.id = 0;
    waypoint_cube_marker.type = visualization_msgs::Marker::CUBE_LIST;
    waypoint_cube_marker.action = visualization_msgs::Marker::ADD;
    waypoint_cube_marker.scale.x = 0.2;
    waypoint_cube_marker.scale.y = 0.2;
    waypoint_cube_marker.scale.z = 0.2;
    waypoint_cube_marker.color.r = 1.0;
    waypoint_cube_marker.color.a = 1.0;
    waypoint_cube_marker.frame_locked = true;
    waypoint_cube_marker.pose.orientation.w = 1.0;

    for (unsigned int i = 0; i < static_cast<int>(final_path.size()); i++) {
      geometry_msgs::Point point;
      point.x = final_path[i][0];
      point.y = final_path[i][1];
      point.z = final_path[i][2] + 0.6;
      waypoint_cube_marker.points.push_back(point);
    }
    marker_array.markers.push_back(waypoint_cube_marker);

    final_path_marker_pub.publish(marker_array);

    ////////////////////////////////////////////////////////////////////////////
    // ROS Spin
    ////////////////////////////////////////////////////////////////////////////
    ros::spinOnce();
    rate.sleep();
    // ROS_INFO("The iteration end.");
  }

  return 0;
}
