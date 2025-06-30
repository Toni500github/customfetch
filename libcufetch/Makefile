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
		-Wl,--export-dynamic \
		-Wl,-soname,libcufetch.so.1 \
		-o libcufetch.so.1.0.0
	ln -sf libcufetch.so.1.0.0 libcufetch.so.1
	ln -sf libcufetch.so.1.0.0 libcufetch.so
	mv -f libcufetch.so* ../$(BUILDDIR)/

clean:
	rm -f *.o *.so *.a ../$(BUILDDIR)/libcufetch.so

.PHONY: clean all libcufetch
