CC = gcc
CXX = g++
CFLAGS = -ggdb3 -Wall -Wextra -fPIC -std=c99
CXXFLAGS = -ggdb3 -Wall -Wextra -fPIC -std=c++17
LDFLAGS = -shared

# Build C plugin
example_c_plugin.so: example_c_plugin.c ../libcufetch/libcufetch_c.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I../include $^ -o $@

clean:
	rm -f *.so

.PHONY: clean
