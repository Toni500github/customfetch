CXX	?= g++
SRC 	 = toml.cpp
TARGET   = toml.o
CPPFLAGS = -I../../include -std=c++20

all: $(TARGET)

$(TARGET):
	${CXX} $(SRC) $(CPPFLAGS) -c -o ../../$(BUILDDIR)/$@

clean:
	rm -rf ../../$(BUILDDIR)/toml++/$(TARGET)

.PHONY: $(TARGET) clean all
