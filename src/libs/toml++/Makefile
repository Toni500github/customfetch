CXX	?= g++
CXXSTD	?= c++20
SRC 	 = toml.cpp
TARGET   = toml.o
CXXFLAGS = -I../../../include/libs -flto=auto -ffat-lto-objects -fvisibility-inlines-hidden -fvisibility=default -fPIC -std=$(CXXSTD)

all: $(TARGET)

$(TARGET):
	$(CXX) $(SRC) $(CXXFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../../$(BUILDDIR)/toml++/$(TARGET)

.PHONY: $(TARGET) clean all
