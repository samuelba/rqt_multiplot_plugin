#!/usr/bin/env python3
"""Write a ROS dynamic RPM spec with bloom's library.

bloom-generate rosdynrpm in bloom 0.14.4 raises TypeError before it writes
rpm/, and that entry point does not rosify the package name.
"""

import os
import sys

from bloom.generators.dynrpm.generator import generate_substitutions_from_package
from bloom.generators.dynrpm.generator import place_template_files
from bloom.generators.dynrpm.generator import process_template_files
from catkin_pkg.packages import find_packages


def generate(ros_distro, package_path):
    os.chdir(package_path)
    packages = find_packages(".")
    if len(packages) != 1:
        raise SystemExit(f"expected one package in {package_path}, found {len(packages)}")

    _relative_path, package = next(iter(packages.items()))
    substitutions = generate_substitutions_from_package(
        package,
        ros_distro,
        installation_prefix=f"/opt/ros/{ros_distro}",
    )
    substitutions["Rosdistro"] = ros_distro
    substitutions["Package"] = "ros-{}-{}".format(
        ros_distro,
        package.name.replace("_", "-"),
    )
    place_template_files(".", package.get_build_type())
    for template in process_template_files(".", substitutions):
        if os.path.isfile(template):
            os.remove(template)


def main():
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} ROS_DISTRO PACKAGE_PATH")
    generate(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    main()
