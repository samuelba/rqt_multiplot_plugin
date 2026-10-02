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
  -j 1 \
  "${filters[@]}" \
  --exclude-lines-by-pattern 'Q_OBJECT|Q_ENUM' \
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

# Report files under 90% lines. The tree is still short of that floor, so this
# list does not fail the run. Codecov status is informational for the same reason.
python3 - "${report}/coverage.xml" "${package}" <<'PY' | tee -a "${report}/summary.txt"
import os
import re
import sys
import xml.etree.ElementTree as ET

xml_path, package = sys.argv[1], sys.argv[2]
excluded = {"multiplot_main.cpp", "MultiplotPlugin.cpp"}
macro_line = re.compile(r"^\s*(Q_OBJECT|Q_ENUM\b.*)\s*$")
short = []
for cls in ET.parse(xml_path).getroot().iter("class"):
    filename = cls.get("filename") or ""
    if os.path.basename(filename) in excluded:
        continue
    source_path = os.path.join(package, filename)
    try:
        source_lines = open(source_path, encoding="utf-8", errors="replace").read().splitlines()
    except OSError:
        source_lines = []
    total = 0
    hit = 0
    for line in cls.iter("line"):
        if line.get("hits") is None:
            continue
        number = int(line.get("number"))
        text = source_lines[number - 1] if 0 < number <= len(source_lines) else ""
        if macro_line.match(text):
            continue
        total += 1
        if int(line.get("hits")) > 0:
            hit += 1
    if total == 0:
        continue
    percent = 100.0 * hit / total
    if percent < 90.0:
        short.append((percent, hit, total, filename))

if short:
    print(f"{len(short)} files under 90% lines:")
    for percent, hit, total, filename in sorted(short):
        print(f"  {percent:5.1f}%  {hit}/{total}  {filename}")
else:
    print("every measured file is at or above 90% lines")
PY

echo "HTML report: ${report}/index.html"
