CXX       	?= g++
PREFIX	  	?= /usr
VARS  	  	?=
GUI_SUPPORT     ?= 1

DEBUG 		?= 1
PARSER_TEST 	?= 0
VENDOR_TEST 	?= 0
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb3 -Wall -DDEBUG=1 $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
        BUILDDIR  = build/release
endif

ifeq ($(PARSER_TEST), 1)
	VARS += -DPARSER_TEST=1
endif

ifeq ($(VENDOR_TEST), 1)
	VARS += -DVENDOR_TEST=1
endif

ifeq ($(GUI_SUPPORT), 1)
        VARS 	 += -DGUI_SUPPORT=1
	LDFLAGS	 += `pkg-config --libs gtkmm-3.0`
	CXXFLAGS += `pkg-config --cflags gtkmm-3.0`
endif

NAME		 = customfetch
TARGET		 = cufetch
VERSION    	 = 0.1.0
BRANCH     	 = main
SRC 	   	 = $(sort $(wildcard src/*.cpp src/query/*.cpp))
OBJ 	   	 = $(SRC:.cpp=.o)
LDFLAGS   	+= -lmagic -lpci -L./$(BUILDDIR)/fmt -lfmt
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -02 -Wno-ignored-attributes -funroll-all-loops -Iinclude -std=c++17 $(VARS) -DVERSION=\"$(VERSION)\" -DBRANCH=\"$(BRANCH)\"

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
