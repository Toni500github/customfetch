CXX		?= g++
CXX_INCLUDES 	= -I../../include
CXX_FLAGS 	= -shared -fPIC

all: linux-system

linux-system: linux/system.cc
	$(CXX) $(CXX_INCLUDES) $(CXX_FLAGS) linux/system.cc -o linux-system-mod.so
	mv -f linux-system-mod.so ../../$(BUILDDIR)/linux-system-mod.so

clean:
	rm -f *.o *.so *.a ../../$(BUILDDIR)/core-plugin/.*so

.PHONY: clean all cufetch
