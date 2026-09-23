#!/usr/bin/python3
"""Publish diagnostic_msgs/DiagnosticArray the way a robot does.

    ros2 run rqt_multiplot publish_diagnostics_demo.py

One topic, /diagnostics. Several timers publish onto it, each with its own
rate, the same way diagnostic_updater nodes share that topic. A message is
only the statuses that publisher owns. The next message is a different
subset, in a different order. The stable name of a number is

    status.name + KeyValue.key

Hardware id is optional. Leave it empty and any hardware id matches. Type one
and the status must have that hardware_id as well. In multiplot, set the axis
type to diagnostic_msgs/msg/DiagnosticArray, check Diagnostic value, and type
the status and key. X can be header/stamp (the time this array was published)
or message receipt time.

Message layout:

    DiagnosticArray
      header.stamp          time this array was published
      header.frame_id       empty
      status[]               DiagnosticStatus
        level                0 OK, 1 WARN, 2 ERROR, 3 STALE
        name                 status name, including aggregator-style prefixes
        message              short text, not a number
        hardware_id          defaults to the status name; the two Range statuses use front and rear
        values[]             KeyValue
          key                name of the reading
          value              always a string; only some of them are numbers

Plottable pairs (value is the whole string as a number):

    status                      key                rate     how it is sent
    /Power System/Battery       Voltage            1 Hz     with Charger, always
    /Power System/Battery       Current            1 Hz     with Charger, always
    /Power System/Battery       Temperature        1 Hz     with Charger, always
    /Power System/Battery       State of charge    1 Hz     with Charger, always
    /Power System/Charger       Output current     1 Hz     with Battery
    /Motors/Front Left          Position           10 Hz    with Front Right
    /Motors/Front Left          Velocity           10 Hz    with Front Right
    /Motors/Front Left          Winding temp       10 Hz    missing every 4th message
    /Motors/Front Right         Position           10 Hz    with Front Left
    /IMU                        Yaw rate           20 Hz    the only status in that message
    /Computer                   CPU load           0.5 Hz   with Memory
    /Computer                   Memory used        0.5 Hz   with CPU
    /GPS                        Satellites         0.33 Hz  alone, then absent until the next one
    /Sensors/Range              Distance           5 Hz     two statuses, hardware id front and rear

Not plottable, included so a wrong key stays empty:

    /Power System/Battery       State              "OK" or "WARN"
    /Power System/Battery       Voltage text       "24.1 V"
    /Motors/Front Left          Mode               "position"

Variations:

    Together   Battery and Charger share one array. CPU load and Memory used are two keys
               on one /Computer status. Front Left and Front Right share one array
               and swap order each message.
    Alone      IMU is a one-status array. GPS is a one-status array.
    Duplicate  Every 5th battery array appends a second /Power System/Battery whose
               Voltage is 999. Multiplot keeps the first.
    Gap        Winding temp is omitted every 4th motor message. GPS is absent between
               its publishes. A missing pair adds no point.
    Hardware   /Sensors/Range is published twice in one array. The name and the key
               Distance are the same. hardware_id is front or rear, and the distances
               differ. An empty hardware id plots the first. front or rear selects one.
    Level      Battery level becomes WARN while Voltage is below 23.7. Level is a
               byte on the status, not a key.
"""

import math

import rclpy
from diagnostic_msgs.msg import DiagnosticArray, DiagnosticStatus, KeyValue
from rclpy.node import Node


def key_value(key, value):
    item = KeyValue()
    item.key = key
    item.value = value
    return item


def status(name, level, pairs, hardware_id=None):
    item = DiagnosticStatus()
    item.level = level
    item.name = name
    item.message = "OK" if level == DiagnosticStatus.OK else "check values"
    item.hardware_id = name if hardware_id is None else hardware_id
    item.values = [key_value(key, value) for key, value in pairs]
    return item


def number(value):
    return f"{value:.4f}"


class DiagnosticsDemo(Node):
    def __init__(self):
        super().__init__("diagnostics_demo")
        self.publisher = self.create_publisher(DiagnosticArray, "/diagnostics", 10)
        self.battery_tick = 0
        self.motor_tick = 0
        self.create_timer(1.0, self.publish_power)
        self.create_timer(0.1, self.publish_motors)
        self.create_timer(0.05, self.publish_imu)
        self.create_timer(2.0, self.publish_computer)
        self.create_timer(3.0, self.publish_gps)
        self.create_timer(0.2, self.publish_ranges)

    def now(self):
        return self.get_clock().now().nanoseconds * 1e-9

    def publish(self, statuses):
        message = DiagnosticArray()
        message.header.stamp = self.get_clock().now().to_msg()
        message.status = statuses
        self.publisher.publish(message)

    def publish_power(self):
        t = self.now()
        voltage = 24.2 + 0.6 * math.sin(t * 0.4)
        level = DiagnosticStatus.OK if voltage >= 23.7 else DiagnosticStatus.WARN
        battery_pairs = [
            ("Voltage", number(voltage)),
            ("Current", number(1.8 + 0.7 * math.sin(t * 1.3))),
            ("Temperature", number(32.0 + 4.0 * math.sin(t * 0.15))),
            ("State of charge", number(70.0 + 15.0 * math.sin(t * 0.05))),
            ("State", "OK" if level == DiagnosticStatus.OK else "WARN"),
            ("Voltage text", f"{voltage:.1f} V"),
        ]
        battery = status("/Power System/Battery", level, battery_pairs)
        charger = status(
            "/Power System/Charger",
            DiagnosticStatus.OK,
            [("Output current", number(0.4 + 0.3 * math.sin(t * 0.8)))],
        )
        statuses = [battery, charger]
        self.battery_tick += 1
        if self.battery_tick % 5 == 0:
            statuses.append(status("/Power System/Battery", DiagnosticStatus.OK, [("Voltage", "999")]))
        self.publish(statuses)

    def publish_motors(self):
        t = self.now()
        left_pairs = [
            ("Position", number(math.sin(t * 2.0))),
            ("Velocity", number(2.0 * math.cos(t * 2.0))),
            ("Mode", "position"),
        ]
        if self.motor_tick % 4 != 0:
            left_pairs.insert(2, ("Winding temp", number(48.0 + 6.0 * math.sin(t * 0.3))))
        left = status("/Motors/Front Left", DiagnosticStatus.OK, left_pairs)
        right = status(
            "/Motors/Front Right",
            DiagnosticStatus.OK,
            [("Position", number(math.sin(t * 2.0 + 0.4)))],
        )
        self.motor_tick += 1
        order = [left, right] if self.motor_tick % 2 else [right, left]
        self.publish(order)

    def publish_imu(self):
        t = self.now()
        self.publish([status("/IMU", DiagnosticStatus.OK, [("Yaw rate", number(0.3 * math.sin(t * 3.0)))])])

    def publish_computer(self):
        t = self.now()
        self.publish(
            [
                status(
                    "/Computer",
                    DiagnosticStatus.OK,
                    [
                        ("CPU load", number(25.0 + 20.0 * math.sin(t * 0.2))),
                        ("Memory used", number(42.0 + 5.0 * math.sin(t * 0.07))),
                    ],
                )
            ]
        )

    def publish_gps(self):
        t = self.now()
        satellites = 8 + int(3 * math.sin(t * 0.1))
        self.publish([status("/GPS", DiagnosticStatus.OK, [("Satellites", str(satellites))])])

    def publish_ranges(self):
        t = self.now()
        self.publish(
            [
                status("/Sensors/Range", DiagnosticStatus.OK, [("Distance", number(1.2 + 0.3 * math.sin(t * 2.0)))], "front"),
                status("/Sensors/Range", DiagnosticStatus.OK, [("Distance", number(2.4 + 0.5 * math.sin(t * 1.3)))], "rear"),
            ]
        )


def main():
    rclpy.init()
    node = DiagnosticsDemo()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
