CXX	?= g++
SRC 	 = json.cpp
TARGET   = json.o
CXXSTD	?= c++20
CXXFLAGS = -I../../../include/libs $(LTO_FLAGS) -std=$(CXXSTD) -fPIC

all: $(TARGET)

$(TARGET):
	$(CXX) $(SRC) $(CXXFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../$(BUILDDIR)/json/$(TARGET)

.PHONY: $(TARGET) clean all
