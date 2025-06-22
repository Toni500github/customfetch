CXX		?= g++
CXX_INCLUDES 	= -I../../include
CXX_FLAGS 	= -ggdb3 -shared -fPIC -fvisibility=hidden -fvisibility-inlines-hidden -DNDEBUG -shared -Wl,-soname,libcufetch-core-modules.so.1

SRC		= $(wildcard linux/*.cc)
OBJ		= $(SRC:.cc=.o)

all: linux-core-modules

linux-core-modules:
	$(CXX) $(CXX_INCLUDES) $(CXX_FLAGS) linux/*.cc linux/utils/*.cpp ../util.cpp -o $@.so
	mv -f $@.so ../../$(BUILDDIR)/core-plugins/$@.so

clean:
	rm -f *.o *.so *.a ../../$(BUILDDIR)/core-plugins/.*so

.PHONY: clean all linux-core-modules
