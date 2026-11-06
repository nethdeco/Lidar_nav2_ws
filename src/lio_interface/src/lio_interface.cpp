#include "lio_interface/lio_interface.hpp"

namespace lio_interface
{

LioInterface::LioInterface(const rclcpp::NodeOptions & options)
: Node("lio_interface", options){

    this->declare_parameter("odometry_sub", "Odometry");
    this->get_parameter("odometry_sub", odometry_sub_);

    base_frame_to_lidar_initialized_ = false;

    data_position_ = std::make_shared<lio_interface::data_>();

    // Initialize TF listener
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    pcd_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("/registered_scan", 5);
    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/registered_odometry", 5);

    pcd_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
        "/cloud_registered", rclcpp::SensorDataQoS(), 
        std::bind(&LioInterface::pointCloudCallback, this, std::placeholders::_1));

    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
        odometry_sub_, rclcpp::SensorDataQoS(),
        std::bind(&LioInterface::odometryCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), MAG "LioInterface node is START" RST);
}

LioInterface::~LioInterface(){
    RCLCPP_INFO(this->get_logger(), MAG "LioInterface node is OVER" RST);
}

/**
 * @brief Transform the point cloud from the lidar_odom frame into the odom frame.
 *
 * @param[in]  msg Input point cloud in the lidar_odom frame (sensor_msgs::PointCloud2)
 * @param[in]  tf_odom_to_lidar_odom_ Transform between odom and lidar_odom (tf2::Transform)
 * @param[out] out Point cloud in the odom frame (sensor_msgs::PointCloud2)
 *
 */
void LioInterface::pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg){

    auto out = std::make_shared<sensor_msgs::msg::PointCloud2>();
    pcl_ros::transformPointCloud("odom", tf_odom_to_lidar_odom_, *msg, *out);
    pcd_pub_->publish(*out);

}

/**
 * @brief Get the FAST-LIO camera_init -> body TF and the base_footprint -> lidar_frame TF;
 *        compute the odom -> lidar_frame TF, then publish the lidar_frame pose in odom
 *        on the /lidar_odometry topic.
 */
void LioInterface::odometryCallback(const nav_msgs::msg::Odometry::ConstSharedPtr msg){

    if (!base_frame_to_lidar_initialized_) {
    try {
        // Look up livox_frame pose in base_footprint from the TF tree (T_base_footprint_livox)
        auto tf_stamped_tf_base_frame_to_lidar_frame = tf_buffer_->lookupTransform(
            "base_footprint", 
            "livox_frame", 
            tf2::TimePointZero,      
            std::chrono::seconds(1)   
        ); 

        tf2::Transform tf_base_frame_to_lidar_frame;        // TF from base_frame to lidar_frame

        // tf2::fromMsg(tf_stamped_lidar_odom_to_lidar_frame.transform, tf_lidar_odom_to_lidar_frame);
        tf2::fromMsg(tf_stamped_tf_base_frame_to_lidar_frame.transform, tf_base_frame_to_lidar_frame);

        // At the initial pose, the static lidar_frame -> base_footprint transform equals odom -> lidar_odom
        tf_odom_to_lidar_odom_ = tf_base_frame_to_lidar_frame;

        base_frame_to_lidar_initialized_ = true;
    }   
    catch (const tf2::TransformException & ex) {
        RCLCPP_WARN(this->get_logger(), "TF lookup failed: %s Retrying...", ex.what());
        return;
    }
    }

    tf2::Transform tf_lidar_odom_to_lidar_frame;        // Transform from lidar_odom to lidar_frame
    tf2::Transform tf_odom_to_lidar;                    // Transform from odom to lidar_frame

    tf2::fromMsg(msg->pose.pose, tf_lidar_odom_to_lidar_frame);

    // Compute the odom -> lidar_frame transform
    tf_odom_to_lidar = tf_odom_to_lidar_odom_ * tf_lidar_odom_to_lidar_frame;

    nav_msgs::msg::Odometry out;
    out.header.stamp = msg->header.stamp;
    out.header.frame_id = "odom";
    out.child_frame_id = "livox_frame";

    const auto & origin = tf_odom_to_lidar.getOrigin();
    out.pose.pose.position.x = origin.x();
    out.pose.pose.position.y = origin.y();
    out.pose.pose.position.z = origin.z();
    out.pose.pose.orientation = tf2::toMsg(tf_odom_to_lidar.getRotation());

    odom_pub_->publish(out);

}

}   // namespace lio_interface

#include "rclcpp_components/register_node_macro.hpp"

RCLCPP_COMPONENTS_REGISTER_NODE(lio_interface::LioInterface)