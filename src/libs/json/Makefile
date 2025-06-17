CXX	?= g++
SRC 	 = json.cpp
TARGET   = json.o
CXXFLAGS = -I../../../include -std=c++20

all: $(TARGET)

$(TARGET):
	${CXX} $(SRC) $(CXXFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../$(BUILDDIR)/json/$(TARGET)

.PHONY: $(TARGET) clean all
