CXX       	?= g++
PREFIX	  	?= /usr
MANPREFIX	?= $(PREFIX)/share/man
VARS  	  	?= -DENABLE_NLS=1

DEBUG 		?= 1
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb3 -Wall -Wextra -Wpedantic -Wno-unused-parameter -DDEBUG=1 -fsanitize=address $(DEBUG_CXXFLAGS) $(CXXFLAGS)
        LDFLAGS	 += -fsanitize=address
else
	# Check if an optimization flag is not already set
	ifneq ($(filter -O%,$(CXXFLAGS)),)
    		$(info Keeping the existing optimization flag in CXXFLAGS)
	else
    		CXXFLAGS := -O3 $(CXXFLAGS)
	endif
        BUILDDIR  = build/release
endif

NAME		 = cufetchpm
TARGET		?= $(NAME)
OLDVERSION	 = 0.0.0
VERSION    	 = 0.0.1
SRC 	   	 = $(wildcard src/*.cpp src/manager/*.cpp  ../src/util.cpp)
OBJ 	   	 = $(SRC:.cpp=.o)
LDLIBS   	+= $(BUILDDIR)/libfmt.a $(BUILDDIR)/libtiny-process-library.a
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -fvisibility=hidden -I../include -I../include/libs/ -Iinclude -std=c++20 $(VARS) -DVERSION=\"$(VERSION)\"

all: fmt toml getopt-port tpl $(TARGET)

fmt:
ifeq ($(wildcard $(BUILDDIR)/libfmt.a),)
	mkdir -p $(BUILDDIR)
	make -C ../src/libs/fmt BUILDDIR=cufetchpm/$(BUILDDIR)
endif

toml:
ifeq ($(wildcard $(BUILDDIR)/toml.o),)
	mkdir -p $(BUILDDIR)
	make -C ../src/libs/toml++ BUILDDIR=cufetchpm/$(BUILDDIR)
endif

getopt-port:
ifeq ($(wildcard $(BUILDDIR)/getopt.o),)
	mkdir -p $(BUILDDIR)
	make -C ../src/libs/getopt_port BUILDDIR=$(BUILDDIR)
endif

tpl:
ifeq ($(wildcard $(BUILDDIR)/libtiny-process-library.a),)
	mkdir -p $(BUILDDIR)
	make -C ../src/libs/tiny-process-library BUILDDIR=cufetchpm/$(BUILDDIR)
endif

$(TARGET): fmt toml getopt-port tpl $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CXX) $(OBJ) $(BUILDDIR)/*.o -o $(BUILDDIR)/$(TARGET) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(OBJ)

distclean:
	find .. -type f -name "*.tar.gz" -exec rm -rf "{}" \;
	find .. -type f -name "*.o" -exec rm -rf "{}" \;
	find .. -type f -name "*.a" -exec rm -rf "{}" \;

.PHONY: $(TARGET) fmt toml all
