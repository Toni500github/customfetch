CXX	?= g++
SRC 	 = toml.cpp
TARGET   = toml.o
CXXFLAGS = -I../../../include/cufetch -fPIE -std=c++20

all: $(TARGET)

$(TARGET):
	${CXX} $(SRC) $(CXXFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../../$(BUILDDIR)/toml++/$(TARGET)

.PHONY: $(TARGET) clean all
