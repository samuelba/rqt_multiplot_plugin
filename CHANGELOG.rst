^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rqt_multiplot
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.1.0 (2026-10-15)
------------------
* Migrate to ROS 2 (Jazzy, Lyrical, Rolling) with ament_cmake, rclcpp, and rosbag2
* Use Qt 6 on Lyrical and Rolling, and Qt 5 on Jazzy
* Subscribe to live topics through runtime type support, including diagnostics and array fields
* Import rosbag2 recordings (``.mcap``, ``.db3``, and bag directories), including several files
* Overlay 2 or 3 bags on the same plots, aligned at the start, at markers, or at a Y threshold
* Replace the fixed row/column layout with nested splits and tabs
* Add log scale, time windows, date-time axes, and rad/deg conversion
* Add curve filters, markers, data statistics, and topic metrics (rate, bandwidth, delay)
* Add a topic browser with drag and drop onto plots
* Add a light/dark theme, keyboard shortcuts, and plot export (PNG, SVG, PDF, TXT, CSV)
* Contributors: Samuel Bachmann

0.0.12 (2021-01-13)
-------------------
* CurveDataSequence::processMessage: Catch variant_topic_tools exception
* Mogrify all png to get rid of libpng warning
* update the readme for melodic and noetic
* Contributors: Wolfgang Merkt, tkazik, ConstGemm

0.0.11 (2021-01-06)
-------------------
* update for Ros noetic

0.0.10 (2018-12-13)
-------------------
* add missing dependency and use format 2 in package.xml
* Contributors: Samuel Bachmann

0.0.9 (2018-12-12)
------------------
* fix rosdep key
* Contributors: Samuel Bachmann

0.0.8 (2018-11-21)
------------------
* Make your library usable
* Contributors: Adriano Henrique Rossette Leite, Samuel Bachmann

0.0.7 (2017-05-08)
------------------
* fix issues when plotting constant values
* fix plotting with receipt time
* Contributors: Samuel Bachmann

0.0.6 (2016-12-28)
------------------
* reduce min plot size
* run all plots on startup
* flexible command line url input for config files
* arg parsing for the command line script
* add time frame as a new curve data type
  This gives the option to plot e.g. only the last 10 seconds and
  remove older data.
* fix plot resizing
* Contributors: Daniel Stonier, Samuel Bachmann

0.0.5 (2016-08-05)
------------------
* fixes `#2 <https://github.com/anybotics/rqt_multiplot_plugin/issues/2>`_ a QT API change
* rename two tooltips (copy and past curve) in PlotConfigWidget
* qt5 ready
* Contributors: Samuel Bachmann

0.0.4 (2016-05-23)
------------------
* add install command for resource icons
* Contributors: Samuel Bachmann

0.0.3 (2016-05-22)
------------------
* fixes for release
* Contributors: Samuel Bachmann

0.0.2 (2016-05-20)
------------------
* Fixes related to OS X clang compatibility and added support for Qwt >= 6.1
* Added copy and paste for curve configurations
* Fixed segmentation fault when removing curves
* * Refactored configuration classes
  * Added drag and drop feature for copying and moving curves from the
  legend of one plot to another plot
  * Added possibility to double-click legend items for editing curve
  properties
* * Implemented match filter combo box as class
  * Added match filter completer to type combo box
* Added match filter completer to topic combo box
* * Curve axis topic and type are now automatically filled if initially empty
  * Added feature to copy between axes configurations
* Bag reader is now using topic queries to reduce the number of processed messages
* Added adaptive-precision output format to tracker coordinate labels
* Fixed bag reader to not block event processing
* Implemented bag import feature
* * Curve axis config now starts with no type selected
  * Fixed validations in curve axis config widget
* Fixed message field completer to correctly handle dynamic arrays
* Bug fixing and stability improvements on the configuration and subscriber backend
* * Added feature for cleaning the configuration file history
  * Fixed threads to terminate on destruction
  * Added package and message type registry updates to main widget
  constructor
  * Fixed curve config widget components to initialize correctly
* Added feature to specify custom axis titles
* Fixed configuration file loading/saving bug, now producing ROS console output
* Plot widgets cannot change state anymore if only one plot is available in the plot table
* Fixed a graphical bug which was caused by bad masks for the plot cursor tracked point labels
* Implemented topic interpolation feature
* About to get topic interpolation right
* Added feature to export plots
* Added feature to maximize/restore a plot
* Added axis titles and legend
* * Added curve styles
  * Revised plot controls
  * Efficient curve data storage
* Plot scaling works
* Enhanced plot table configuration and widget
* Added configuration file handling
* Contributors: Ralf Kaestner
