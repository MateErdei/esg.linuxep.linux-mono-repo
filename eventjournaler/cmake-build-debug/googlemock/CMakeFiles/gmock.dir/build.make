# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /snap/clion/69/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/69/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pair/CLionProjects/sspl-tools/3rdparty-googletest

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug

# Include any dependencies generated for this target.
include googlemock/CMakeFiles/gmock.dir/depend.make

# Include the progress variables for this target.
include googlemock/CMakeFiles/gmock.dir/progress.make

# Include the compile flags for this target's objects.
include googlemock/CMakeFiles/gmock.dir/flags.make

googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o: googlemock/CMakeFiles/gmock.dir/flags.make
googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o: ../googlemock/src/gmock-all.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o"
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/gmock.dir/src/gmock-all.cc.o -c /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/googlemock/src/gmock-all.cc

googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/gmock.dir/src/gmock-all.cc.i"
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/googlemock/src/gmock-all.cc > CMakeFiles/gmock.dir/src/gmock-all.cc.i

googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/gmock.dir/src/gmock-all.cc.s"
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/googlemock/src/gmock-all.cc -o CMakeFiles/gmock.dir/src/gmock-all.cc.s

# Object files for target gmock
gmock_OBJECTS = \
"CMakeFiles/gmock.dir/src/gmock-all.cc.o"

# External object files for target gmock
gmock_EXTERNAL_OBJECTS =

googlemock/libgmockd.a: googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o
googlemock/libgmockd.a: googlemock/CMakeFiles/gmock.dir/build.make
googlemock/libgmockd.a: googlemock/CMakeFiles/gmock.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libgmockd.a"
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock && $(CMAKE_COMMAND) -P CMakeFiles/gmock.dir/cmake_clean_target.cmake
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gmock.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
googlemock/CMakeFiles/gmock.dir/build: googlemock/libgmockd.a

.PHONY : googlemock/CMakeFiles/gmock.dir/build

googlemock/CMakeFiles/gmock.dir/clean:
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock && $(CMAKE_COMMAND) -P CMakeFiles/gmock.dir/cmake_clean.cmake
.PHONY : googlemock/CMakeFiles/gmock.dir/clean

googlemock/CMakeFiles/gmock.dir/depend:
	cd /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pair/CLionProjects/sspl-tools/3rdparty-googletest /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/googlemock /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock /home/pair/CLionProjects/sspl-tools/3rdparty-googletest/cmake-build-debug/googlemock/CMakeFiles/gmock.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : googlemock/CMakeFiles/gmock.dir/depend

