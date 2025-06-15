CXX       	?= g++
VARS  	  	?= -DENABLE_NLS=1

DEBUG 		?= 1
GUI_APP         ?= 0
VENDOR_TEST 	?= 0
DEVICE_TEST     ?= 0

USE_DCONF	?= 1
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

ifeq ($(VENDOR_TEST), 1)
	VARS += -DVENDOR_TEST=1
endif

ifeq ($(DEVICE_TEST), 1)
	VARS += -DDEVICE_TEST=1
endif

ifeq ($(GUI_APP), 1)
	TARGET	  = $(NAME)-gui
	VARS 	 += -DGUI_APP=1
	LDFLAGS	 += `pkg-config --libs gtkmm-3.0`
	CXXFLAGS += `pkg-config --cflags gtkmm-3.0`
endif

ifeq ($(USE_DCONF), 1)
        ifeq ($(shell pkg-config --exists glib-2.0 dconf && echo 1), 1)
                CXXFLAGS += -DUSE_DCONF=1 `pkg-config --cflags glib-2.0 dconf`
        else
                CXXFLAGS += -DUSE_DCONF=0
        endif
endif

SRC_DIR = ../src
TEST_DIR = .

NAME		 = customfetch
TARGET		?= $(NAME)
OLDVERSION	 = 0.10.2
VERSION    	 = 1.0.0
SRC              = $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/query/linux/*.cpp $(SRC_DIR)/query/android/*.cpp $(SRC_DIR)/query/macos/*.cpp $(SRC_DIR)/query/linux/utils/*.cpp))
OBJ              = $(patsubst $(SRC_DIR)/%.cpp, $(BUILDDIR)/%.o, $(SRC))
TESTS 		 = $(patsubst $(TEST_DIR)/%.cpp, $(TEST_DIR)/%, $(wildcard $(TEST_DIR)/test*.cpp))
LDFLAGS   	+= -L./$(BUILDDIR)/fmt -lfmt -ldl
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -fvisibility=hidden -I$(SRC_DIR)/../include -std=c++20 $(VARS) -DVERSION=\"$(VERSION)\"

all: fmt toml catch2 bin

fmt:
ifeq ($(wildcard $(BUILDDIR)/fmt/libfmt.a),)
	mkdir -p $(BUILDDIR)/fmt
	make -C $(SRC_DIR)/fmt BUILDDIR=tests/$(BUILDDIR)/fmt
endif

toml:
ifeq ($(wildcard $(BUILDDIR)/toml++/toml.o),)
	mkdir -p $(BUILDDIR)/toml++
	make -C $(SRC_DIR)/toml++ BUILDDIR=tests/$(BUILDDIR)/toml++
endif

catch2:
#ifeq ($(wildcard $(BUILDDIR)/catch2/catch.o),)
ifneq ($(shell test -e $(BUILDDIR)/catch2/catch.o && echo -n yes), yes)
	mkdir -p $(BUILDDIR)/catch2
	$(CXX) -std=c++20 catch2/catch_amalgamated.cpp -c -o $(BUILDDIR)/catch2/catch.o
endif

locale:
	scripts/make_mo.sh locale/

$(BUILDDIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(BUILDDIR)/query/linux/utils $(BUILDDIR)/query/macos $(BUILDDIR)/query/android
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TEST_DIR)/test_%: $(BUILDDIR)/catch2/catch.o $(OBJ) $(TEST_DIR)/test_%.cpp
	mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(BUILDDIR)/toml++/toml.o $^ -o $@ $(LDFLAGS)

bin: $(TESTS)

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(BUILDDIR)/libcustomfetch.a $(OBJ)

distclean:
	rm -rf $(BUILDDIR) ./tests/$(BUILDDIR) $(OBJ)
	find . -type f -name "*.tar.gz" -exec rm -rf "{}" \;
	find . -type f -name "*.o" -exec rm -rf "{}" \;
	find . -type f -name "*.a" -exec rm -rf "{}" \;

.PHONY: distclean clean catch2 fmt toml locale bin all
