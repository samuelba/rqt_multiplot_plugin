#!/usr/bin/python3
"""Publish a one-sided FFT for a log-log spectrum plot.

    ros2 run rqt_multiplot publish_fft_demo.py

Topic /fft_demo/spectrum, type sensor_msgs/JointState. An array curve needs
both axes on the same topic. JointState is a stock message with two float
arrays, so the names are not frequency and magnitude:

    position[i]   frequency in Hz. Bin 0 is 0 Hz, the DC bin.
    velocity[i]   magnitude, linear, always > 0.

256 real samples at 1 kHz give 129 bins from 0 to 500 Hz. The shape is a 1/f
floor, a large DC bin, tones at 50 Hz and 120 Hz, and one tone that drifts.
The DC frequency is 0, so a log X axis draws that bin below the axis.

In the plot dialog set X to position/*, Y to velocity/*, and turn Log on for
both axes.
"""

import math

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState

SAMPLE_RATE_HZ = 1000.0
# 257 bins serialize to 4160 bytes. CycloneDDS then blocks forever in send()
# because the kernel does not release that UDP memory. 129 bins is about 2 KB.
FFT_SIZE = 256
BIN_COUNT = FFT_SIZE // 2 + 1
BIN_HZ = SAMPLE_RATE_HZ / FFT_SIZE
PEAK_WIDTH_HZ = 1.2 * BIN_HZ


def tone(frequency_hz, center_hz, amplitude):
    delta = (frequency_hz - center_hz) / PEAK_WIDTH_HZ
    return amplitude * math.exp(-0.5 * delta * delta)


def spectrum(time_s):
    drift_hz = 240.0 + 40.0 * math.sin(0.3 * time_s)
    mains = 0.12 * (1.0 + 0.15 * math.sin(2.0 * time_s))
    tones = (
        (0.0, 0.8),
        (50.0, mains),
        (120.0, 0.035),
        (drift_hz, 0.06),
    )
    frequency = []
    magnitude = []
    for index in range(BIN_COUNT):
        frequency_hz = index * BIN_HZ
        floor = 2.0e-4 * (1.0 + (50.0 / max(frequency_hz, BIN_HZ)))
        magnitude_value = floor
        for center_hz, amplitude in tones:
            magnitude_value += tone(frequency_hz, center_hz, amplitude)
        frequency.append(frequency_hz)
        magnitude.append(magnitude_value)
    return frequency, magnitude


class FftDemoPublisher(Node):
    def __init__(self):
        super().__init__("fft_demo_publisher")
        self.publisher = self.create_publisher(JointState, "/fft_demo/spectrum", 10)
        self.start = self.get_clock().now()
        self.create_timer(0.2, self.publish_spectrum)

    def publish_spectrum(self):
        time_s = (self.get_clock().now() - self.start).nanoseconds * 1.0e-9
        frequency, magnitude = spectrum(time_s)
        msg = JointState()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "fft_demo"
        msg.position = frequency
        msg.velocity = magnitude
        self.publisher.publish(msg)


def main():
    rclpy.init()
    node = FftDemoPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
