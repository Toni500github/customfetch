CXX	?= g++
SRC 	 = toml.cpp
TARGET   = toml.o
CXXFLAGS = -I../../../include/ -fvisibility-inlines-hidden -fvisibility=default -fPIC -std=c++20

all: $(TARGET)

$(TARGET):
	$(CXX) $(SRC) $(CXXFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../../$(BUILDDIR)/toml++/$(TARGET)

.PHONY: $(TARGET) clean all
