CXX		?= g++
CXX_INCLUDES 	 = -I../../../include
CXX_FLAGS 	 = -O3 -DNDEBUG -fvisibility=hidden -fvisibility-inlines-hidden -fPIC

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Windows)
    CXXFLAGS 	+= -D_CRT_SECURE_NO_WARNINGS
    SRC 	+= process_win.cpp
    # Check if we're in MSYS environment
    ifneq (,$(findstring MSYS,$(shell uname -o)))
        CXXFLAGS += -DMSYS_PROCESS_USE_SH
    endif
    LIBNAME 	= tiny-process-library.lib
else
    CXXFLAGS 	+= -std=c++11 -Wall -Wextra
    SRC 	+= process_unix.cpp
    LIBNAME 	= libtiny-process-library.a
endif

SRC             += process.cpp
OBJ		= $(SRC:.cpp=.o)

all: $(LIBNAME)

$(LIBNAME): $(OBJ)
	ar rcs $@ $^
	mv -f $(LIBNAME) ../../../$(BUILDDIR)/$(LIBNAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(LIBNAME)

.PHONY: all clean
