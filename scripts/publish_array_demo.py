#!/usr/bin/python3

import math

import rclpy
from geometry_msgs.msg import Pose, PoseArray, PoseWithCovariance
from rclpy.node import Node
from sensor_msgs.msg import JointState, LaserScan
from std_msgs.msg import Float32MultiArray, Header, MultiArrayDimension


class ArrayDemoPublisher(Node):
    def __init__(self):
        super().__init__("array_demo_publisher")
        self.tick = 0
        self.float_pub = self.create_publisher(Float32MultiArray, "/array_demo/floats", 10)
        self.joint_pub = self.create_publisher(JointState, "/array_demo/joints", 10)
        self.covariance_pub = self.create_publisher(PoseWithCovariance, "/array_demo/covariance", 10)
        self.pose_pub = self.create_publisher(PoseArray, "/array_demo/poses", 10)
        self.scan_pub = self.create_publisher(LaserScan, "/array_demo/scan", 10)
        self.create_timer(0.1, self.publish_all)

    def header(self):
        msg = Header()
        msg.stamp = self.get_clock().now().to_msg()
        msg.frame_id = "array_demo"
        return msg

    def publish_floats(self, phase):
        msg = Float32MultiArray()
        msg.layout.dim = [MultiArrayDimension(label="data", size=16, stride=16)]
        msg.data = [math.sin(phase + 0.35 * i) for i in range(16)]
        self.float_pub.publish(msg)

    def publish_joints(self, phase):
        length = 8 if (self.tick // 40) % 2 else 4
        msg = JointState()
        msg.header = self.header()
        msg.name = [f"joint_{i}" for i in range(length)]
        msg.position = [math.sin(phase + 0.5 * i) for i in range(length)]
        msg.velocity = [math.cos(phase + 0.5 * i) for i in range(length)]
        self.joint_pub.publish(msg)

    def publish_covariance(self, phase):
        msg = PoseWithCovariance()
        msg.pose.position.x = math.cos(phase)
        msg.pose.position.y = math.sin(phase)
        msg.covariance = [0.0] * 36
        for i in range(36):
            msg.covariance[i] = 0.25 * math.sin(phase + 0.2 * i) + 0.05 * i
        self.covariance_pub.publish(msg)

    def publish_poses(self, phase):
        msg = PoseArray()
        msg.header = self.header()
        count = 12
        radius = 1.5 + 0.4 * math.sin(phase)
        for i in range(count):
            angle = phase + (2.0 * math.pi * i / count)
            pose = Pose()
            pose.position.x = radius * math.cos(angle)
            pose.position.y = radius * math.sin(angle)
            pose.position.z = 0.1 * math.sin(phase + i)
            msg.poses.append(pose)
        self.pose_pub.publish(msg)

    def publish_scan(self, phase):
        msg = LaserScan()
        msg.header = self.header()
        msg.angle_min = -math.pi / 2.0
        msg.angle_max = math.pi / 2.0
        msg.angle_increment = math.pi / 720.0
        msg.time_increment = 0.0
        msg.scan_time = 0.1
        msg.range_min = 0.05
        msg.range_max = 10.0
        count = 720
        msg.ranges = [1.5 + 0.4 * math.sin(phase + 0.05 * i) + 0.15 * math.sin(3.0 * phase + 0.02 * i) for i in range(count)]
        self.scan_pub.publish(msg)

    def publish_all(self):
        phase = 0.15 * self.tick
        self.publish_floats(phase)
        self.publish_joints(phase)
        self.publish_covariance(phase)
        self.publish_poses(phase)
        self.publish_scan(phase)
        self.tick += 1


def main():
    rclpy.init()
    node = ArrayDemoPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
