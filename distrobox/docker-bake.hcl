group "default" {
  targets = ["jazzy", "kilted", "lyrical", "rolling"]
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

target "kilted" {
  inherits = ["rqt-multiplot"]
  args = {
    ROS_DISTRO = "kilted"
  }
  tags = ["rqt-multiplot:kilted"]
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
