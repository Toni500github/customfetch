CXX	?= g++
SRC 	 = json.cpp
TARGET   = json.o
CXXFLAGS = -I../../../include/libs -flto=auto -ffat-lto-objects -std=c++20 -fPIC

all: $(TARGET)

$(TARGET):
	$(CXX) $(SRC) $(CXXFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../$(BUILDDIR)/json/$(TARGET)

.PHONY: $(TARGET) clean all
