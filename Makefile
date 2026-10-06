.DEFAULT_GOAL := all
CXX ?= g++
BUILD ?= build-haiku
RC ?= rc
XRES ?= xres
MIMESET ?= mimeset
CPPFLAGS += -Isrc $(CROSS_CPPFLAGS)
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wno-multichar -Wno-unused-parameter
APP_CPPFLAGS = -I/boot/system/develop/headers/private/interface
APP_SRC = $(wildcard src/*.cpp)
APP_OBJ = $(APP_SRC:%.cpp=$(BUILD)/%.o)
.PHONY: all check clean package
all: $(BUILD)/Clipper $(BUILD)/Clipper_filter $(BUILD)/Clipper_device
$(BUILD)/Clipper: $(APP_OBJ) resources/Clipper.rdef resources/clipper.hvif
	$(CXX) $(APP_LDFLAGS) -o $@.new $(APP_OBJ) -lbe -ltracker -ltranslation -Wl,--export-dynamic $(APP_LDEND)
	$(RC) -o $(BUILD)/Clipper.rsrc resources/Clipper.rdef
	$(XRES) -o $@.new $(BUILD)/Clipper.rsrc
	$(MIMESET) -f $@.new
	mv $@.new $@
$(BUILD)/Clipper_filter: $(BUILD)/filter/ClipperFilter.o
	$(CXX) $(FILTER_LDFLAGS) -shared -o $@ $^ -lbe $(FILTER_LDEND)
$(BUILD)/Clipper_device: $(BUILD)/device/PasteDevice.o
	$(CXX) $(FILTER_LDFLAGS) -shared -o $@ $^ -lbe $(FILTER_LDEND)
$(BUILD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(APP_CPPFLAGS) $(CXXFLAGS) -fPIC -MMD -MP -c $< -o $@
$(BUILD)/history_tests: tests/HistoryTests.cpp $(BUILD)/src/History.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_LDFLAGS) -o $@ $^ -lbe $(APP_LDEND)
check: $(BUILD)/history_tests
	$(BUILD)/history_tests
package: all
	bash tools/package-haiku.sh
clean:
	rm -rf $(BUILD)
-include $(APP_OBJ:.o=.d) $(BUILD)/filter/ClipperFilter.d $(BUILD)/device/PasteDevice.d
$(BUILD)/fixture: tests/Fixture.cpp $(BUILD)/src/History.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_LDFLAGS) -o $@ $^ -lbe -ltracker -ltranslation $(APP_LDEND)
$(BUILD)/paste_integration: tests/PasteIntegration.cpp $(BUILD)/src/History.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_LDFLAGS) -o $@ $^ -lbe $(APP_LDEND)

.PHONY: check-ui icon
icon:
	python3 tools/make-icon.py
$(BUILD)/ui_tests: tests/UITests.cpp $(BUILD)/src/HistoryWindow.o $(BUILD)/src/History.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_LDFLAGS) -o $@ $^ -lbe -ltranslation $(APP_LDEND)
check-ui: $(BUILD)/ui_tests
	$(BUILD)/ui_tests
