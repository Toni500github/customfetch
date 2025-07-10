UNAME_S := $(shell uname -s)

# is macos?
ifeq ($(UNAME_S),Darwin)
    LIBNAME      := libcufetch.dylib
    INSTALL_NAME := -Wl,-install_name,@rpath/$(LIBNAME)
    SHARED_FLAG  := -dynamiclib
    SONAME_FLAGS :=
else
    LIBNAME      := libcufetch.so.1.0.0
    INSTALL_NAME := -Wl,-soname,libcufetch.so.1
    SHARED_FLAG  := -shared
    SONAME_FLAGS := -Wl,--export-dynamic
endif

CXX		?= g++
GUI_APP         ?= 0
SRC             = $(wildcard *.cc)
OBJ             = $(SRC:.cc=.o) ../$(BUILDDIR)/toml.o
FMT_STATIC      = ../$(BUILDDIR)/libfmt.a
CXXFLAGS 	+= -ggdb3 -I../include -fPIC -DGUI_APP=$(GUI_APP)
OUTPUT		:= ../$(BUILDDIR)/$(LIBNAME)

all: $(OUTPUT)
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		ln -sf libcufetch.so.1.0.0 ../$(BUILDDIR)/libcufetch.so.1; \
		ln -sf libcufetch.so.1.0.0 ../$(BUILDDIR)/libcufetch.so; \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		ln -sf libcufetch.dylib ../$(BUILDDIR)/libcufetch.1.dylib; \
		ln -sf libcufetch.dylib ../$(BUILDDIR)/libcufetch.1.0.0.dylib; \
	fi

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTPUT): $(OBJ) $(FMT_STATIC)
	$(CXX) $(SHARED_FLAG) $(CXXFLAGS) $(OBJ) \
		$(FMT_STATIC) \
		$(SONAME_FLAGS) $(INSTALL_NAME) \
		-o $@

clean:
	rm -f *.o *.so *.a ../$(BUILDDIR)/libcufetch.so

.PHONY: clean all libcufetch
