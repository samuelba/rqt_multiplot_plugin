#!/usr/bin/env bash
set -euo pipefail

readonly VALID_TARGETS=(
  jazzy lyrical rolling
  el9-jazzy el10-lyrical el10-rolling fc43-lyrical fc44-rolling
)

usage() {
  echo "Usage: $0 <target>" >&2
  echo "  target: ${VALID_TARGETS[*]}" >&2
  exit 1
}

target="${1:-}"
if [[ -z "${target}" ]]; then
  usage
fi

case "${target}" in
  jazzy | lyrical | rolling)
    ros_distro="${target}"
    base_image="ros:${target}"
    ;;
  el9-*)
    ros_distro="${target#el9-}"
    base_image="almalinux:9"
    ;;
  el10-*)
    ros_distro="${target#el10-}"
    base_image="almalinux:10"
    ;;
  fc43-*)
    ros_distro="${target#fc43-}"
    base_image="fedora:43"
    ;;
  fc44-*)
    ros_distro="${target#fc44-}"
    base_image="fedora:44"
    ;;
  *)
    echo "Unknown target: ${target}" >&2
  usage
    ;;
esac

readonly container_name="rqt-multiplot-${target}"
readonly image_name="rqt-multiplot:${target}"
readonly distrobox_path="$(dirname "$(realpath "$0")")"
readonly current_path="$(pwd)"
readonly start_time="$(date +%s)"

processes="$(pgrep distrobox-enter || true)"
has_running_processes=false
for process in ${processes}; do
  command="$(ps -p "${process}" -o cmd=)"
  if [[ "${command}" =~ distrobox-enter( -n)?\ ${container_name} ]]; then
    has_running_processes=true
    echo "Process still running (${process}): '${command}'"
    echo "Killing process ${process}: '${command}'"
    kill -s 15 "${process}"
  fi
done

if [[ "${has_running_processes}" == true ]]; then
  echo "Stop running processes!" >&2
  exit 1
fi

if docker ps -a --format '{{.Names}}' | grep -qw "${container_name}"; then
  distrobox stop --yes "${container_name}" || true
  echo "Stop container '${container_name}'"
  docker stop "${container_name}" || true
  echo "Delete container '${container_name}'"
  docker rm -f "${container_name}" || true
fi

cd "${distrobox_path}"

echo "Pull base image '${base_image}'"
docker pull "${base_image}"

echo "Build image '${image_name}'"
docker buildx bake --allow=fs.read=.. --builder default --load "${target}"

host_rqt_config="${HOME}/.config/ros.org-${target}"
mkdir -p "${host_rqt_config}"

echo "Write Distrobox config file '${target}.ini'"
cat <<EOF > "${target}.ini"
[${container_name}]
image=${image_name}
nvidia=false
start_now=true
init=false
volume="/run/dbus/system_bus_socket:/run/dbus/system_bus_socket "
volume="\$HOME/.config/ros.org-${target}:\$HOME/.config/ros.org"
hostname=\$HOSTNAME
additional_flags="--device /dev/dri "
additional_flags="--env ROS_VERSION=2 "
additional_flags="--env ROS_DISTRO=${ros_distro} "
additional_flags="--env QT_QPA_PLATFORM=xcb "
unshare_process=true
EOF

echo "Create container '${container_name}'"
distrobox assemble create --file "${target}.ini"

end_time="$(date +%s)"
echo "Done. Execution time: $((end_time - start_time)) seconds"

cd "${current_path}"
