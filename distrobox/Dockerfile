# syntax=docker/dockerfile:1.4

ARG ROS_DISTRO
FROM ros:${ROS_DISTRO}

ARG ROS_DISTRO
ENV ROS_VERSION=2 \
    ROS_DISTRO=${ROS_DISTRO} \
    DEBIAN_FRONTEND=noninteractive \
    GTEST_COLOR=1 \
    CLANG_TIDY_VERSION=18

RUN echo 'APT::Install-Recommends "false";' > /etc/apt/apt.conf.d/99no-install-recommends && \
    echo 'APT::Install-Suggests "false";' >> /etc/apt/apt.conf.d/99no-install-recommends

RUN --mount=type=cache,id=apt-cache-${ROS_DISTRO},target=/var/cache/apt \
    apt-get update && \
    apt-get install -y \
    #
    # Distrobox packages.
    bash \
    apt-utils \
    bash-completion \
    bc \
    bzip2 \
    curl \
    dialog \
    diffutils \
    findutils \
    gnupg \
    gnupg2 \
    gpgsm \
    hostname \
    iproute2 \
    iputils-ping \
    keyutils \
    language-pack-en \
    less \
    libcap2-bin \
    libkrb5-3 \
    libnss-mdns \
    libnss-myhostname \
    libvte-2.9*-common \
    locales \
    lsof \
    man-db \
    manpages \
    mtr \
    ncurses-base \
    openssh-client \
    passwd \
    pigz \
    pinentry-curses \
    procps \
    rsync \
    sudo \
    tcpdump \
    time \
    traceroute \
    tree \
    tzdata \
    unzip \
    util-linux \
    wget \
    xauth \
    xz-utils \
    zip \
    libgl1 \
    #
    # Other packages.
    ca-certificates \
    git \
    libegl1 \
    libglx-mesa0 \
    libvulkan1 \
    mesa-vulkan-drivers \
    python3 \
    python3-pip

ARG CCACHE_VERSION=4.12.2
ARG CCACHE_SHA256=630c34ec94d451b200f5b14a6a25580d6a45bc80c394b7e0b93e33556eee5d32
RUN wget -q "https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz" && \
    echo "${CCACHE_SHA256}  ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz" | sha256sum -c - && \
    tar -xf "ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz" && \
    install -m 0755 "ccache-${CCACHE_VERSION}-linux-x86_64/ccache" /usr/local/bin/ccache && \
    rm -rf "ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz" "ccache-${CCACHE_VERSION}-linux-x86_64"

RUN mkdir -p /tmp/ws/src/rqt_multiplot
COPY package.xml /tmp/ws/src/rqt_multiplot/package.xml

RUN --mount=type=cache,id=apt-cache-${ROS_DISTRO},target=/var/cache/apt \
    rosdep update --rosdistro "${ROS_DISTRO}" && \
    rosdep install --from-paths /tmp/ws/src --ignore-src -y --rosdistro "${ROS_DISTRO}" \
    --dependency-types buildtool \
    --dependency-types build \
    --dependency-types build_export \
    --dependency-types exec \
    --dependency-types test && \
    apt-get install -y "ros-${ROS_DISTRO}-rmw-cyclonedds-cpp" && \
    rm -rf /tmp/ws

RUN --mount=type=cache,id=apt-cache-${ROS_DISTRO},target=/var/cache/apt \
    apt-get update && \
    apt-get install -y wget gnupg ca-certificates software-properties-common && \
    case "${ROS_DISTRO}" in \
      lyrical|rolling) \
        apt-get install -y clang-format-18 clang-tidy-18 ;; \
      *) \
        wget -q https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh && \
        chmod +x /tmp/llvm.sh && \
        /tmp/llvm.sh 18 && \
        apt-get install -y clang-format-18 clang-tidy-18 ;; \
    esac && \
    clang-format-18 --version && \
    clang-tidy-18 --version

RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" > /etc/profile.d/ros.sh && \
    echo 'export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"' >> /etc/profile.d/ros.sh && \
    printf '%s\n' \
      '# Distrobox forwards the host PATH. Host mise/asdf shims pick the wrong Python.' \
      '_path_filtered=""' \
      'IFS=":"' \
      'for _dir in $PATH; do' \
      '  case "${_dir}" in' \
      '    */.local/share/mise/*|*/mise/shims|*/.asdf/shims) ;;' \
      '    *)' \
      '      if [ -n "${_path_filtered}" ]; then' \
      '        _path_filtered="${_path_filtered}:${_dir}"' \
      '      else' \
      '        _path_filtered="${_dir}"' \
      '      fi' \
      '      ;;' \
      '  esac' \
      'done' \
      'unset IFS' \
      'export PATH="/usr/bin:/usr/local/bin:${_path_filtered}"' \
      'unset _dir _path_filtered' \
      > /etc/profile.d/zz-system-python.sh
