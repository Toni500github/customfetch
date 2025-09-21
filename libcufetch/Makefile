CXX	?= g++
CXXSTD	?= c++20
VARS	?= -DENABLE_NLS=1

DEBUG	?= 1
GUI_APP ?= 0

# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  := build/debug
        LTO_FLAGS  = -fno-lto
        CXXFLAGS  := -ggdb3 -Wall -Wextra -pedantic -Wno-unused-parameter \
                        -DDEBUG=1 -fno-omit-frame-pointer $(DEBUG_CXXFLAGS) $(CXXFLAGS)
        LDFLAGS   += -fsanitize=address -fno-lto
else
        # Check if an optimization flag is not already set
        ifneq ($(filter -O%,$(CXXFLAGS)),)
                $(info Keeping the existing optimization flag in CXXFLAGS)
        else
                CXXFLAGS := -O3 $(CXXFLAGS)
        endif
        LDFLAGS   += $(LTO_FLAGS)
        BUILDDIR  := build/release
endif


UNAME_S := $(shell uname -s)

# is macos?
ifeq ($(UNAME_S),Darwin)
    LIBNAME      := libcufetch.dylib
    INSTALL_NAME := -Wl,-install_name,@rpath/$(LIBNAME)
    SHARED_FLAG  := -dynamiclib
    SONAME_FLAGS :=
else
    LIBNAME      := libcufetch.so.2.0.0
    INSTALL_NAME := -Wl,-soname,libcufetch.so.2
    SHARED_FLAG  := -shared
    SONAME_FLAGS := -Wl,--export-dynamic
endif

SRC              = $(wildcard *.cc)
OBJ              = $(SRC:.cc=.o) ../$(BUILDDIR)/toml.o
LDLIBS	        := ../$(BUILDDIR)/libfmt.a ../$(BUILDDIR)/libtiny-process-library.a
OUTPUT		:= ../$(BUILDDIR)/$(LIBNAME)
CXXFLAGS 	+= -fvisibility-inlines-hidden -fvisibility=hidden -std=$(CXXSTD) -I../include -I../include/libs -fPIC -DGUI_APP=$(GUI_APP)

all: $(OUTPUT)
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		ln -sf libcufetch.so.2.0.0 ../$(BUILDDIR)/libcufetch.so.2; \
		ln -sf libcufetch.so.2.0.0 ../$(BUILDDIR)/libcufetch.so; \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		ln -sf libcufetch.dylib ../$(BUILDDIR)/libcufetch.2.dylib; \
		ln -sf libcufetch.dylib ../$(BUILDDIR)/libcufetch.2.0.0.dylib; \
	fi

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTPUT): $(OBJ) $(LDLIBS)
	$(CXX) $(SHARED_FLAG) $(CXXFLAGS) $(OBJ) \
		$(SONAME_FLAGS) $(INSTALL_NAME) \
		-o $@ $(LDLIBS)

clean:
	rm -f *.o *.so *.a ../$(BUILDDIR)/libcufetch.so

.PHONY: clean all libcufetch
