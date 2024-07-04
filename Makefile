CXX       	?= g++
PREFIX	  	?= .
VARS  	  	?=
GUI_SUPPORT     ?= 0

DEBUG 		?= 1
PARSER_TEST 	?= 0
VENDOR_TEST 	?= 0
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build\debug
        CXXFLAGS := -ggdb3 -Wall -DDEBUG=1 $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
        BUILDDIR  = build\release
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

ifneq ($(OS),Windows_NT)
	LDFLAGS += -lmagic
endif

NAME		 = customfetch
TARGET		 = cufetch
VERSION    	 = 0.1.0
BRANCH     	 = main
SRC 	   	 = src\config.cpp src\display.cpp src\main.cpp src\parse.cpp src\util.cpp \
			src\query\windows\cpu.cpp src\query\windows\disk.cpp src\query\windows\gpu.cpp \
			src\query\windows\ram.cpp src\query\windows\system.cpp src\query\windows\user.cpp
ifeq ($(GUI_SUPPORT), 1)
        SRC 	 += src\gui.cpp
endif
OBJ 	   	 = $(SRC:.cpp=.o)
LDFLAGS   	+= -L.\$(BUILDDIR)\fmt -lfmt
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -Wno-ignored-attributes -funroll-all-loops -fvisibility=hidden -Iinclude -std=c++17 $(VARS) -DVERSION=\"$(VERSION)\" -DBRANCH=\"$(BRANCH)\"

all: fmt toml $(TARGET)

fmt:
ifeq ($(wildcard $(BUILDDIR)\fmt\libfmt.a),)
	if not exist $(BUILDDIR)\fmt md $(BUILDDIR)\fmt
	make -C src\fmt BUILDDIR=$(BUILDDIR)\fmt
endif

toml:
ifeq ($(wildcard $(BUILDDIR)\toml++\toml.o),)
	if not exist $(BUILDDIR)\toml++ md $(BUILDDIR)\toml++
	make -C src\toml++ BUILDDIR=$(BUILDDIR)\toml++
endif

$(TARGET): fmt toml $(OBJ)
	if not exist $(BUILDDIR) md $(BUILDDIR)
	$(CXX) $(OBJ) $(BUILDDIR)\toml++\toml.o -o $(BUILDDIR)\$(TARGET) $(LDFLAGS) $(CXXFLAGS)

clean:
	del $(BUILDDIR)\$(TARGET) $(OBJ)

.PHONY: $(TARGET) fmt toml all
