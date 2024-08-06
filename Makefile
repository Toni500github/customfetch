CXX       	?= g++
PREFIX	  	?= /usr
VARS  	  	?=
GUI_MODE     	?= 0
CXX_STD		?= c++20

DEBUG 		?= 1
PARSER_TEST 	?= 0
VENDOR_TEST 	?= 0
DEVICE_TEST     ?= 0
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb3 -Wall -Wextra -Wpedantic -DDEBUG=1 $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
	CXXFLAGS := -O3 $(CXXFLAGS)
        BUILDDIR  = build/release
endif

ifeq ($(PARSER_TEST), 1)
	VARS += -DPARSER_TEST=1
endif

ifeq ($(VENDOR_TEST), 1)
	VARS += -DVENDOR_TEST=1
endif

ifeq ($(DEVICE_TEST), 1)
	VARS += -DDEVICE_TEST=1
endif

ifeq ($(GUI_MODE), 1)
        VARS 	 += -DGUI_MODE=1
	LDFLAGS	 += `pkg-config --libs gtkmm-3.0`
	CXXFLAGS += `pkg-config --cflags gtkmm-3.0`
endif

ifneq ($(OS),Windows_NT)
	LDFLAGS += -lprocps
endif

NAME		 = customfetch
TARGET		 = cufetch
VERSION    	 = 0.1.0
BRANCH     	 = windows
SRC 	   	 = $(wildcard src/*.cpp src/query/unix/*.cpp src/query/unix/utils/*.cpp src/query/windows/*.cpp)
OBJ 	   	 = $(SRC:.cpp=.o)
LDFLAGS   	+= -L./$(BUILDDIR)/fmt -lfmt
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -Wno-return-type -Wanalyzer-null-dereference -fvisibility=hidden -Iinclude -std=$(CXX_STD) $(VARS) -DVERSION=\"$(VERSION)\" -DBRANCH=\"$(BRANCH)\"

all: fmt toml $(TARGET)

fmt:
ifeq ($(wildcard $(BUILDDIR)/fmt/libfmt.a),)
	mkdir -p $(BUILDDIR)/fmt
	make -C src/fmt BUILDDIR=$(BUILDDIR)/fmt
endif

toml:
ifeq ($(wildcard $(BUILDDIR)/toml++/toml.o),)
	mkdir -p $(BUILDDIR)/toml++
	make -C src/toml++ BUILDDIR=$(BUILDDIR)/toml++
endif

$(TARGET): fmt toml $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CXX) $(OBJ) $(BUILDDIR)/toml++/toml.o -o $(BUILDDIR)/$(TARGET) $(LDFLAGS)

dist: $(TARGET)
	bsdtar -zcf $(NAME)-v$(VERSION).tar.gz LICENSE README.md -C $(BUILDDIR) $(TARGET)

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(OBJ)

distclean:
	rm -rf $(BUILDDIR) ./tests/$(BUILDDIR) $(OBJ)
	find . -type f -name "*.tar.gz" -exec rm -rf "{}" \;
	find . -type f -name "*.o" -exec rm -rf "{}" \;
	find . -type f -name "*.a" -exec rm -rf "{}" \;

install: $(TARGET)
	install $(BUILDDIR)/$(TARGET) -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	cd assets/ && find ascii/ -type f -exec install -Dm 755 "{}" "$(DESTDIR)$(PREFIX)/share/customfetch/{}" \;

.PHONY: $(TARGET) dist distclean fmt toml install all
