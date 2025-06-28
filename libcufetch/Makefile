CXX		?= g++
CXX_INCLUDES 	= -I../include
CXX_FLAGS 	= -shared -fPIC
SRC		= $(wildcard *.cc)
OBJ		= $(SRC:.cc=.o)
FMT_STATIC	= ../$(BUILDDIR)/libfmt.a

all: libcufetch

%.o: %.cc
	$(CXX) $(CXX_INCLUDES) $(CXX_FLAGS) -c -o $@ $<

libcufetch: $(OBJ) $(FMT_STATIC)
	$(CXX) $(CXX_FLAGS) $(OBJ) $(CXX_INCLUDES) \
		-Wl,-Bstatic $(FMT_STATIC) -Wl,-Bdynamic \
		-o libcufetch.so
	mv -f libcufetch.so ../$(BUILDDIR)/$@.so

clean:
	rm -f *.o *.so *.a ../$(BUILDDIR)/libcufetch.so

.PHONY: clean all libcufetch
