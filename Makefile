CXX ?= g++

prefix ?= /usr/

VERSION = 0.1
LIB_PATH = ./lib
BUILD_PATH = build
BIN_PATH = $(BUILD_PATH)/bin
VIEWS_PATH = $(realpath ./views)
BIN_NAME = property-services-monitor
SRC_EXT = cpp

LIB_SOURCES = $(shell find $(LIB_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
LIB_OBJECTS = $(LIB_SOURCES:$(LIB_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPS = $(LIB_OBJECTS:.o=.d)

TARGET_ARCH ?= $(shell uname -m)
ifeq (arm,$(findstring arm,$(TARGET_ARCH)))
	CXXFLAGS := $(CXXFLAGS) -Wno-psabi
endif                                      

# flags #
CXXFLAGS := $(CXXFLAGS) -std=c++17 -Wall -Wextra -DPREFIX=\"$(prefix)\" # -g 
INCLUDES = -I./include/
SHARED_LIBS = -lfmt -lyaml-cpp -lPocoNet -lPocoNetSSL -lPocoFoundation \
	-lPocoCrypto -lstdc++fs

.PHONY: default_target
default_target: release

.PHONY: release
release: dirs
	@$(MAKE) all

.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(dir $(LIB_OBJECTS))
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@echo "Deleting $(BIN_NAME) symlink"
	@$(RM) $(BIN_NAME)
	@echo "Deleting directories"
	@$(RM) -r $(BUILD_PATH)
	@$(RM) -r $(BIN_PATH)

distclean: clean

.PHONY: install
install:
	install -D build/bin/$(BIN_NAME) $(DESTDIR)$(prefix)/bin/$(BIN_NAME)
	install -m 644 -D build/bin/$(BIN_NAME).1.gz \
		$(DESTDIR)$(prefix)/share/man/man1/$(BIN_NAME).1.gz
	install -m 644 -D build/bin/views/notify.plain.inja \
		$(DESTDIR)$(prefix)/share/$(BIN_NAME)/views/notify.plain.inja
	install -m 644 -D build/bin/views/notify.html.inja \
		$(DESTDIR)$(prefix)/share/$(BIN_NAME)/views/notify.html.inja
	install -m 644 -D build/bin/views/notify_body.html.inja \
		$(DESTDIR)$(prefix)/share/$(BIN_NAME)/views/notify_body.html.inja
	install -m 644 -D build/bin/views/images/home.jpg \
		$(DESTDIR)$(prefix)/share/$(BIN_NAME)/views/images/home.jpg

.PHONY: uninstall
uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/$(BIN_NAME)
	-rm -rf $(DESTDIR)$(prefix)/share/$(BIN_NAME)

.PHONY: package
package:
	mkdir -p build/dpkg
	tar --exclude-vcs --exclude="*.yml" --exclude="*.swp" --exclude "*.txt" \
		--exclude "build" --transform "s,^.,$(BIN_NAME)," \
		--exclude configs \
		-czvf build/dpkg/propertyservicesmonitor-$(VERSION).tar.gz ./
	cd build/dpkg; debmake -a propertyservicesmonitor-$(VERSION).tar.gz
	cd build/dpkg/propertyservicesmonitor-$(VERSION); dpkg-buildpackage -us -uc

# checks the executable and symlinks to the output
.PHONY: all
all: $(BIN_PATH)/$(BIN_NAME)
	@echo "Making symlink: $(BIN_NAME) -> $<"
	@$(RM) $(BIN_NAME)
	@if [ ! -d "$(BIN_PATH)/views" ]; then ln -s $(VIEWS_PATH) $(BIN_PATH)/views; fi
	@gzip -k $(BIN_NAME).1.man -c > $(BIN_PATH)/$(BIN_NAME).1.gz
	@ln -s $(BIN_PATH)/$(BIN_NAME) $(BIN_NAME)

.PHONY: manpage
manpage: release
	help2man -n "A lightweight service availability checking tool." \
		--version-string=$(VERSION) build/bin/$(BIN_NAME) \
		> $(BIN_NAME).1.man

# Creation of the main executable
$(BIN_PATH)/$(BIN_NAME): $(LIB_OBJECTS) $(BUILD_PATH)/$(BIN_NAME).o
	@echo "Linking: $@"
	$(CXX) $(LDFLAGS) $(LIB_OBJECTS) $(BUILD_PATH)/$(BIN_NAME).o $(SHARED_LIBS) -o $@

# Creation of the test executable
$(BIN_PATH)/test-$(BIN_NAME): $(LIB_OBJECTS) $(BUILD_PATH)/test-$(BIN_NAME).o
	@echo "Linking: $@"
	$(CXX) $(LDFLAGS) $(LIB_OBJECTS) $(BUILD_PATH)/test-$(BIN_NAME).o $(SHARED_LIBS) -o $@

# Add dependency files, if they exist
-include $(DEPS) $(BIN_NAME).d test-$(BIN_NAME).d

# Source file rules
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies
$(BUILD_PATH)/%.o: $(LIB_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

# This is the main() program object itself:
$(BUILD_PATH)/$(BIN_NAME).o: src/$(BIN_NAME).$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

# This is the test program main() object :
$(BUILD_PATH)/test-$(BIN_NAME).o: src/test.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

.PHONY: test
test: dirs $(BIN_PATH)/test-$(BIN_NAME)
	@echo "Building test executable..."
	#@$(MAKE) $(BUILD_PATH)/bin/test-$(BIN_NAME)
	@$(BUILD_PATH)/bin/test-$(BIN_NAME)

