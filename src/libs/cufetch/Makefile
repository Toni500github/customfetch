CXX		?= g++
CXX_INCLUDES 	= -I../../../include
CXX_FLAGS 	= -shared -fPIC

all: cufetch

cufetch: cufetch.cc
	$(CXX) $(CXX_INCLUDES) $(CXX_FLAGS) cufetch.cc -o libcufetch.so
	mv -f libcufetch.so ../../../$(BUILDDIR)/libcufetch.so

clean:
	rm -f *.o *.so *.a ../../../$(BUILDDIR)/cufetch/.*so

.PHONY: clean all cufetch
