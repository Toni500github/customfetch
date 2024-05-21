# Makefile totally not taken from https://github.com/BurntRanch/TabAUR
# lmao
# uncomment all the lines when you fix fmt/toml support

CXX	?= g++
SRC 	 = $(sort $(wildcard src/*.cpp))
OBJ 	 = $(SRC:.cpp=.o)
# LDFLAGS  = -L./src/fmt -lfmt
LDFLAGS  = -lfmt
TARGET   = cuftech
CPPFLAGS = -ggdb -pedantic -funroll-all-loops -march=native -Iinclude -Wall -std=c++20

# all: fmt toml $(TARGET)
all: $(TARGET)

# fmt:
# ifeq (,$(wildcard ./src/fmt/libfmt.a))
# 	make -C src/fmt
# endif
# 
# toml:
# ifeq (,$(wildcard ./src/toml++/toml.o))
# 	make -C src/toml++
# endif

# $(TARGET): fmt toml ${OBJ}
#	${CXX} $(OBJ) src/toml++/toml.o $(CPPFLAGS) -o $@ $(LDFLAGS)
$(TARGET): $(OBJ)
	${CXX} $(OBJ) $(CPPFLAGS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(TARGET) $(OBJ)
#	make -C src/fmt clean

#.PHONY: $(TARGET) clean fmt toml all
.PHONY: $(TARGET) clean all
