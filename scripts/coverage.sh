#!/usr/bin/env bash
# Line and branch coverage for src/ and include/. Run from the colcon workspace root.

set -euo pipefail

workspace=$(pwd)
package="${workspace}/src/rqt_multiplot_plugin"
if [[ ! -f "${package}/package.xml" ]]; then
  echo "run this script from the colcon workspace root (src/rqt_multiplot_plugin not found)" >&2
  exit 1
fi

if ! command -v colcon >/dev/null 2>&1; then
  echo "colcon is not on PATH; source the ROS setup first" >&2
  exit 1
fi
if ! command -v gcovr >/dev/null 2>&1; then
  echo "gcovr is not installed" >&2
  exit 1
fi

# gcc records the path colcon was given. Do not resolve the symlink: gcovr
# only strips --root from a report path when the strings match.
package_root=$(cd "${package}" && pwd -P)

regex_escape() {
  printf '%s' "$1" | sed -e 's/[.[\*^$()+?{|]/\\&/g'
}

filters=(
  --filter "$(regex_escape "${package}")/src/"
  --filter "$(regex_escape "${package}")/include/"
)
if [[ "${package}" != "${package_root}" ]]; then
  filters+=(
    --filter "$(regex_escape "${package_root}")/src/"
    --filter "$(regex_escape "${package_root}")/include/"
  )
fi

build_base="${workspace}/build_coverage"
install_base="${workspace}/install_coverage"
report="${workspace}/coverage"

colcon build \
  --packages-select rqt_multiplot \
  --build-base "${build_base}" \
  --install-base "${install_base}" \
  --cmake-args -DCMAKE_BUILD_TYPE=Debug -DRQT_MULTIPLOT_COVERAGE=ON \
  --event-handlers console_direct+

find "${build_base}/rqt_multiplot" -name '*.gcda' -delete

set +u
# shellcheck disable=SC1091
source "${install_base}/setup.bash"
set -u

colcon test \
  --packages-select rqt_multiplot \
  --build-base "${build_base}" \
  --install-base "${install_base}" \
  --event-handlers console_direct+ \
  --return-code-on-test-failure \
  --ctest-args " -L" "^unit_testing$"

colcon test-result --verbose --test-result-base "${build_base}"

rm -rf "${report}"
mkdir -p "${report}"

gcovr \
  --root "${package}" \
  --gcov-object-directory "${build_base}/rqt_multiplot" \
  "${filters[@]}" \
  --exclude-unreachable-branches \
  --exclude-throw-branches \
  --html-details "${report}/index.html" \
  --cobertura "${report}/coverage.xml" \
  --txt-summary \
  "${build_base}/rqt_multiplot" | tee "${report}/summary.txt"

# gcovr writes <source> as the workspace path of the package. Codecov matches
# filenames against the git checkout, whose root is the package.
python3 - "${report}/coverage.xml" <<'PY'
import sys

path = sys.argv[1]
with open(path, encoding="utf-8") as handle:
    text = handle.read()
if 'filename="/' in text:
    sys.exit("coverage.xml has absolute filenames; --root did not match the build path")
start = text.find("<source>")
end = text.find("</source>")
if start < 0 or end < 0:
    sys.exit("coverage.xml has no <source> element")
text = text[: start + len("<source>")] + "." + text[end:]
with open(path, "w", encoding="utf-8") as handle:
    handle.write(text)
PY

echo "HTML report: ${report}/index.html"
