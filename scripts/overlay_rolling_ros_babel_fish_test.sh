#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
overlay=${script_dir}/overlay_rolling_ros_babel_fish.sh

# shellcheck source=overlay_rolling_ros_babel_fish.sh
source "${overlay}"

tmp=$(mktemp -d)
trap 'rm -rf "${tmp}"' EXIT

fixed=${tmp}/fixed.hpp
broken=${tmp}/broken.hpp
printf 'int callback_;\n' > "${fixed}"
printf 'SubscriptionTopicStatistics subscription_topic_statistics_;\n' > "${broken}"

distro_header_is_fixed "${fixed}"
if distro_header_is_fixed "${broken}"; then
  echo "broken header was treated as fixed" >&2
  exit 1
fi
if distro_header_is_fixed "${tmp}/missing.hpp"; then
  echo "missing header was treated as fixed" >&2
  exit 1
fi

if ROS_DISTRO=jazzy "${overlay}" --check-expiry >/dev/null 2>&1; then
  echo "check-expiry should fail when ROS_DISTRO is not rolling" >&2
  exit 1
fi

ROS_DISTRO=jazzy "${overlay}"

if ROS_DISTRO=jazzy "${overlay}" --nope >/dev/null 2>&1; then
  echo "bad flag should fail" >&2
  exit 1
fi

echo "overlay_rolling_ros_babel_fish_test: ok"
