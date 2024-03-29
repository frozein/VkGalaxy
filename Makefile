# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/linuxbrew/.linuxbrew/Cellar/cmake/3.27.4/bin/cmake

# The command to remove a file.
RM = /home/linuxbrew/.linuxbrew/Cellar/cmake/3.27.4/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/daniel/dev/VkGalaxy

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/daniel/dev/VkGalaxy

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --cyan "Running CMake cache editor..."
	/home/linuxbrew/.linuxbrew/Cellar/cmake/3.27.4/bin/ccmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --cyan "Running CMake to regenerate build system..."
	/home/linuxbrew/.linuxbrew/Cellar/cmake/3.27.4/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/daniel/dev/VkGalaxy/CMakeFiles /home/daniel/dev/VkGalaxy//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/daniel/dev/VkGalaxy/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -P /home/daniel/dev/VkGalaxy/CMakeFiles/VerifyGlobs.cmake
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named vkgalaxy

# Build rule for target.
vkgalaxy: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 vkgalaxy
.PHONY : vkgalaxy

# fast build rule for target.
vkgalaxy/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/build
.PHONY : vkgalaxy/fast

#=============================================================================
# Target rules for targets named vkgalaxy_shaders

# Build rule for target.
vkgalaxy_shaders: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 vkgalaxy_shaders
.PHONY : vkgalaxy_shaders

# fast build rule for target.
vkgalaxy_shaders/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy_shaders.dir/build.make CMakeFiles/vkgalaxy_shaders.dir/build
.PHONY : vkgalaxy_shaders/fast

src/draw.o: src/draw.cpp.o
.PHONY : src/draw.o

# target to build an object file
src/draw.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/draw.cpp.o
.PHONY : src/draw.cpp.o

src/draw.i: src/draw.cpp.i
.PHONY : src/draw.i

# target to preprocess a source file
src/draw.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/draw.cpp.i
.PHONY : src/draw.cpp.i

src/draw.s: src/draw.cpp.s
.PHONY : src/draw.s

# target to generate assembly for a file
src/draw.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/draw.cpp.s
.PHONY : src/draw.cpp.s

src/game.o: src/game.cpp.o
.PHONY : src/game.o

# target to build an object file
src/game.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/game.cpp.o
.PHONY : src/game.cpp.o

src/game.i: src/game.cpp.i
.PHONY : src/game.i

# target to preprocess a source file
src/game.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/game.cpp.i
.PHONY : src/game.cpp.i

src/game.s: src/game.cpp.s
.PHONY : src/game.s

# target to generate assembly for a file
src/game.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/game.cpp.s
.PHONY : src/game.cpp.s

src/libs/vkh/vkh.o: src/libs/vkh/vkh.c.o
.PHONY : src/libs/vkh/vkh.o

# target to build an object file
src/libs/vkh/vkh.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/libs/vkh/vkh.c.o
.PHONY : src/libs/vkh/vkh.c.o

src/libs/vkh/vkh.i: src/libs/vkh/vkh.c.i
.PHONY : src/libs/vkh/vkh.i

# target to preprocess a source file
src/libs/vkh/vkh.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/libs/vkh/vkh.c.i
.PHONY : src/libs/vkh/vkh.c.i

src/libs/vkh/vkh.s: src/libs/vkh/vkh.c.s
.PHONY : src/libs/vkh/vkh.s

# target to generate assembly for a file
src/libs/vkh/vkh.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/libs/vkh/vkh.c.s
.PHONY : src/libs/vkh/vkh.c.s

src/main.o: src/main.cpp.o
.PHONY : src/main.o

# target to build an object file
src/main.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/main.cpp.o
.PHONY : src/main.cpp.o

src/main.i: src/main.cpp.i
.PHONY : src/main.i

# target to preprocess a source file
src/main.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/main.cpp.i
.PHONY : src/main.cpp.i

src/main.s: src/main.cpp.s
.PHONY : src/main.s

# target to generate assembly for a file
src/main.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/vkgalaxy.dir/build.make CMakeFiles/vkgalaxy.dir/src/main.cpp.s
.PHONY : src/main.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... vkgalaxy_shaders"
	@echo "... vkgalaxy"
	@echo "... src/draw.o"
	@echo "... src/draw.i"
	@echo "... src/draw.s"
	@echo "... src/game.o"
	@echo "... src/game.i"
	@echo "... src/game.s"
	@echo "... src/libs/vkh/vkh.o"
	@echo "... src/libs/vkh/vkh.i"
	@echo "... src/libs/vkh/vkh.s"
	@echo "... src/main.o"
	@echo "... src/main.i"
	@echo "... src/main.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -P /home/daniel/dev/VkGalaxy/CMakeFiles/VerifyGlobs.cmake
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

