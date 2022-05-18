# Credit to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := main

BUILD_DIR := ./build
SRC_DIRS := ./src

# Find c++ files
SRCS := $(shell find $(SRC_DIRS) -name '*.cc')

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP -std=c++20

FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))
MAIN_OBJS := $(call FILTER_OUT,test, $(OBJS))

# The final build step.
main: $(OBJS)
	$(CXX) $(MAIN_OBJS) -o $(BUILD_DIR)/main $(LDFLAGS)

TESTS_OBJS := $(call FILTER_OUT,main, $(OBJS))

tests: $(TESTS_OBJS) 
	$(CXX) $(TESTS_OBJS) -o $(BUILD_DIR)/tests $(LDFLAGS)

# Build step for C++ source
$(BUILD_DIR)/%.cc.o: %.cc
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
