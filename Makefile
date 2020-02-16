ARCH ?= amd64

ifeq ($(ARCH),armhf)
CXX = arm-linux-gnueabihf-g++
COMPILE_FLAGS_ARCH = -Wno-psabi
LINKER_FLAGS_ARCH =	-L build/armhf/cross-openssl/compiled-openssl/lib \
  -L build/armhf/cross-poco/compiled-poco/lib \
	-L build/armhf/cross-yamlcpp/compiled-yamlcpp/lib \
	-L build/armhf/cross-fmt/compiled-fmt/lib
INCLUDES_ARCH = -I./build/armhf/cross-openssl/compiled-openssl/include \
 -I./build/armhf/cross-poco/compiled-poco/include \
 -I./build/armhf/cross-yamlcpp/compiled-yamlcpp/include \
 -I./build/armhf/cross-fmt/compiled-fmt/include 
else ifeq ($(ARCH),amd64)
CXX = g++
else
# Probably we should support ia32, but I don't need that platform atm.
$(error error is "Unknown or unsupported ARCH $(ARCH)")
endif

SRC_PATH = ./src
BUILD_PATH = build/$(ARCH)
BIN_PATH = $(BUILD_PATH)/bin
VIEWS_PATH = $(realpath ./views)
BIN_NAME = property-services-monitor
SRC_EXT = cpp

SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPS = $(OBJECTS:.o=.d)

# flags #
COMPILE_FLAGS = -std=c++17 -Wall -Wextra $(COMPILE_FLAGS_ARCH) # -g
INCLUDES = -I./include/ $(INCLUDES_ARCH)
LIBS = -lfmt -lyaml-cpp -lPocoNet -lPocoNetSSL -lPocoFoundation -lPocoCrypto

.PHONY: default_target
default_target: release

.PHONY: release
release: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS)
release: dirs
	@$(MAKE) all

.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(dir $(OBJECTS))
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	# TODO: We should probably delete the whole build, not just the build/$(ARCH)..
	@echo "Deleting $(BIN_NAME) symlink"
	@$(RM) $(BIN_NAME)
	@echo "Deleting directories"
	@$(RM) -r $(BUILD_PATH)
	@$(RM) -r $(BIN_PATH)

# checks the executable and symlinks to the output
.PHONY: all
all: $(BIN_PATH)/$(BIN_NAME)
	@echo "Making symlink: $(BIN_NAME) -> $<"
	@$(RM) $(BIN_NAME)
	@if [ ! -d "$(BIN_PATH)/views" ]; then ln -s $(VIEWS_PATH) $(BIN_PATH)/views; fi
	#TODO: don't do this on a cross compile:
	@ln -s $(BIN_PATH)/$(BIN_NAME) $(BIN_NAME)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	@echo "Linking: $@"
	$(CXX) $(LINKER_FLAGS_ARCH) $(OBJECTS) $(LIBS) -o $@

# Add dependency files, if they exist
-include $(DEPS)

# Source file rules
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@
