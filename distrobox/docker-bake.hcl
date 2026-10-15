group "default" {
  targets = ["jazzy", "lyrical", "rolling"]
}

target "rqt-multiplot" {
  context    = ".."
  dockerfile = "distrobox/Dockerfile"
  cache-to   = ["type=inline"]
}

target "jazzy" {
  inherits = ["rqt-multiplot"]
  args = {
    ROS_DISTRO = "jazzy"
  }
  tags = ["rqt-multiplot:jazzy"]
}

target "lyrical" {
  inherits = ["rqt-multiplot"]
  args = {
    ROS_DISTRO = "lyrical"
  }
  tags = ["rqt-multiplot:lyrical"]
}

target "rolling" {
  inherits = ["rqt-multiplot"]
  args = {
    ROS_DISTRO = "rolling"
  }
  tags = ["rqt-multiplot:rolling"]
}

target "rpm-image" {
  context    = ".."
  dockerfile = "distrobox/Dockerfile.rpm"
  cache-to   = ["type=inline"]
}

target "el9-jazzy" {
  inherits = ["rpm-image"]
  args = {
    BASE_IMAGE          = "almalinux:9"
    ROS_DISTRO          = "jazzy"
    ENABLE_ROS_TESTING  = "0"
  }
  tags = ["rqt-multiplot:el9-jazzy"]
}

target "el10-lyrical" {
  inherits = ["rpm-image"]
  args = {
    BASE_IMAGE          = "almalinux:10"
    ROS_DISTRO          = "lyrical"
    ENABLE_ROS_TESTING  = "0"
  }
  tags = ["rqt-multiplot:el10-lyrical"]
}

target "el10-rolling" {
  inherits = ["rpm-image"]
  args = {
    BASE_IMAGE          = "almalinux:10"
    ROS_DISTRO          = "rolling"
    ENABLE_ROS_TESTING  = "1"
  }
  tags = ["rqt-multiplot:el10-rolling"]
}

target "fc43-lyrical" {
  inherits = ["rpm-image"]
  args = {
    BASE_IMAGE          = "fedora:43"
    ROS_DISTRO          = "lyrical"
    ENABLE_ROS_TESTING  = "1"
  }
  tags = ["rqt-multiplot:fc43-lyrical"]
}

target "fc44-rolling" {
  inherits = ["rpm-image"]
  args = {
    BASE_IMAGE          = "fedora:44"
    ROS_DISTRO          = "rolling"
    ENABLE_ROS_TESTING  = "1"
  }
  tags = ["rqt-multiplot:fc44-rolling"]
}
