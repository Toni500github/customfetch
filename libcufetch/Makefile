CXX		?= g++
CXX_INCLUDES 	= -I../include
CXX_FLAGS 	= -shared -fPIC
SRC		= $(wildcard *.cc)
OBJ		= $(SRC:.cc=.o)

all: libcufetch

%.o: %.cc
	$(CXX) $(CXX_INCLUDES) $(CXX_FLAGS) -c -o $@ $<

libcufetch: $(OBJ)
	$(CXX) $(OBJ) $(CXX_INCLUDES) $(CXX_FLAGS) -o libcufetch.so -L../$(BUILDDIR)/ -lfmt
	mv -f libcufetch.so ../$(BUILDDIR)/$@.so

clean:
	rm -f *.o *.so *.a ../$(BUILDDIR)/libcufetch.so

.PHONY: clean all libcufetch
