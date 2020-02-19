CXX ?= g++

prefix ?= /usr/

VERSION = 0.1
SRC_PATH = ./src
BUILD_PATH = build
BIN_PATH = $(BUILD_PATH)/bin
VIEWS_PATH = $(realpath ./views)
BIN_NAME = property-services-monitor
SRC_EXT = cpp

SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPS = $(OBJECTS:.o=.d)

# flags #
COMPILE_FLAGS = -std=c++17 -Wall -Wextra -DPREFIX=\"$(prefix)\" # -g 
INCLUDES = -I./include/ 
LIBS = -lfmt -lyaml-cpp -lPocoNet -lPocoNetSSL -lPocoFoundation -lPocoCrypto -lstdc++fs

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
	@echo "Deleting $(BIN_NAME) symlink"
	@$(RM) $(BIN_NAME)
	@echo "Deleting directories"
	@$(RM) -r $(BUILD_PATH)
	@$(RM) -r $(BIN_PATH)

distclean: clean

.PHONY: install
install:
	install -D build/bin/property-services-monitor \
		$(DESTDIR)$(prefix)/bin/property-services-monitor
	install -m 644 -D build/bin/property-services-monitor.1.gz \
		$(DESTDIR)$(prefix)/share/man/man1/property-services-monitor.1.gz
	install -m 644 -D build/bin/views/notify.plain.inja \
		$(DESTDIR)$(prefix)/share/property-services-monitor/views/notify.plain.inja
	install -m 644 -D build/bin/views/notify.html.inja \
		$(DESTDIR)$(prefix)/share/property-services-monitor/views/notify.html.inja
	install -m 644 -D build/bin/views/notify.html.inja \
		$(DESTDIR)$(prefix)/share/property-services-monitor/views/notify_body.html.inja
	install -m 644 -D build/bin/views/images/home.jpg \
		$(DESTDIR)$(prefix)/share/property-services-monitor/views/images/home.jpg

.PHONY: uninstall
uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/property-services-monitor
	-rm -rf $(DESTDIR)$(prefix)/share/property-services-monitor

.PHONY: package
package:
	mkdir -p build/dpkg
	tar --exclude-vcs --exclude="*.yml" --exclude="*.swp" --exclude "*.txt" \
		--exclude "build" --transform "s,^.,property-services-monitor," \
		-czvf build/dpkg/propertyservicesmonitor-$(VERSION).tar.gz ./
	cd build/dpkg; debmake -a propertyservicesmonitor-$(VERSION).tar.gz
	cd build/dpkg/propertyservicesmonitor-$(VERSION); dpkg-buildpackage -us -uc
	cd build/dpkg; sbuild --host=armhf -d buster propertyservicesmonitor-$(VERSION)

# checks the executable and symlinks to the output
.PHONY: all
all: $(BIN_PATH)/$(BIN_NAME)
	@echo "Making symlink: $(BIN_NAME) -> $<"
	@$(RM) $(BIN_NAME)
	@if [ ! -d "$(BIN_PATH)/views" ]; then ln -s $(VIEWS_PATH) $(BIN_PATH)/views; fi
	@gzip -k property-services-monitor.1.man -c > $(BIN_PATH)/property-services-monitor.1.gz
	@ln -s $(BIN_PATH)/$(BIN_NAME) $(BIN_NAME)

.PHONY: manpage
manpage: release
	help2man -n "A lightweight service availability checking tool." \
		--version-string=$(VERSION) build/bin/property-services-monitor \
		> property-services-monitor.1.man

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	@echo "Linking: $@"
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

# Add dependency files, if they exist
-include $(DEPS)

# Source file rules
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@
