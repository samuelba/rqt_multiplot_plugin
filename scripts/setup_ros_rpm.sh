#!/usr/bin/env bash
# Enable the ROS 2 RPM repositories, install bloom, and initialize rosdep.
# ENABLE_ROS_TESTING=1 also enables the ros2-testing repository.

set -euo pipefail

: "${ROS_DISTRO:?ROS_DISTRO is required}"

# shellcheck disable=SC1091
. /etc/os-release

dnf install -y ca-certificates
if ! command -v curl >/dev/null 2>&1; then
  dnf install -y curl
fi

# grep -q in a pipeline exits before dnf finishes, and pipefail turns that SIGPIPE into a miss.
dnf_version=$(dnf --version 2>&1 || true)

case "${ID}" in
  fedora)
    rpm_channel=fc
    # codecs.fedoraproject.org is unreachable from some CI networks, and openh264
    # is only pulled in as a weak dependency.
    if [[ -f /etc/yum.repos.d/fedora-cisco-openh264.repo ]]; then
      if [[ ${dnf_version} == *dnf5* ]]; then
        dnf config-manager setopt fedora-cisco-openh264.enabled=0
      else
        dnf config-manager --set-disabled fedora-cisco-openh264
      fi
    fi
    ;;
  almalinux | rocky | centos | rhel)
    rpm_channel=el
    dnf install -y epel-release dnf-plugins-core
    if [[ ${dnf_version} == *dnf5* ]]; then
      dnf config-manager setopt crb.enabled=1
    else
      dnf config-manager --set-enabled crb
    fi
    ;;
  *)
    echo "unsupported OS: ${ID}" >&2
    exit 1
    ;;
esac

release_url=$(curl -fsSL -o /dev/null -w '%{url_effective}' \
  https://github.com/ros-infrastructure/ros-apt-source/releases/latest)
ros_apt_source_version=${release_url##*/}
if [[ -z ${ros_apt_source_version} || ${ros_apt_source_version} == */* ]]; then
  echo "could not resolve ros-apt-source release from ${release_url}" >&2
  exit 1
fi

rpm_url="https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ros_apt_source_version}/ros2-release-${ros_apt_source_version}-1.${rpm_channel}.noarch.rpm"
dnf install -y "${rpm_url}"

if [[ ${ENABLE_ROS_TESTING:-0} == 1 ]]; then
  testing_repo=/etc/yum.repos.d/ros2-testing.repo
  if [[ ! -f ${testing_repo} ]]; then
    echo "ros2-testing.repo not found" >&2
    exit 1
  fi
  tmp=$(mktemp)
  awk '
    /^\[/ { section = $0 }
    section == "[ros2-testing]" && /^enabled=/ {
      print "enabled=1"
      next
    }
    { print }
  ' "${testing_repo}" > "${tmp}"
  mv "${tmp}" "${testing_repo}"
fi

dnf upgrade -y
dnf install -y git ros-dev-tools python3-pip

pip_args=()
if python3 -m pip install --help 2>/dev/null | grep -q -- '--break-system-packages'; then
  pip_args+=(--break-system-packages)
fi
python3 -m pip install "${pip_args[@]}" 'bloom==0.14.4'

if [[ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]]; then
  rosdep init
fi
rosdep update --rosdistro "${ROS_DISTRO}"

if [[ -z ${ROS_OS_OVERRIDE:-} ]]; then
  case ${ID} in
    almalinux | rocky | centos | rhel)
      ROS_OS_OVERRIDE="rhel:$(rpm -E %rhel)"
      ;;
  esac
fi

if [[ -n ${ROS_OS_OVERRIDE:-} ]]; then
  export ROS_OS_OVERRIDE
  printf 'export ROS_OS_OVERRIDE=%q\n' "${ROS_OS_OVERRIDE}" > /etc/profile.d/ros-os-override.sh
  if [[ -n ${GITHUB_ENV:-} ]]; then
    echo "ROS_OS_OVERRIDE=${ROS_OS_OVERRIDE}" >> "${GITHUB_ENV}"
  fi
fi
