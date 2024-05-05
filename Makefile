# Makefile totally not taken from https://github.com/BurntRanch/TabAUR
# lmao

CXX	?= g++
SRC 	 = $(sort $(wildcard src/*.cpp))
OBJ 	 = $(SRC:.cpp=.o)
LDFLAGS  = -L./src/fmt -lfmt
TARGET   = cuftech
CPPFLAGS = -ggdb -pedantic -funroll-all-loops -march=native -Iinclude -Wall -std=c++20

all: cpr fmt toml $(TARGET)

fmt:
ifeq (,$(wildcard ./src/fmt/libfmt.a))
	make -C src/fmt
endif

toml:
ifeq (,$(wildcard ./src/toml++/toml.o))
	make -C src/toml++
endif

$(TARGET): fmt toml ${OBJ}
	${CXX} $(OBJ) src/toml++/toml.o $(CPPFLAGS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(TARGET) $(OBJ)
#	make -C src/fmt clean

.PHONY: $(TARGET) clean fmt toml all
