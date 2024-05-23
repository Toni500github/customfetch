CXX       	?= g++
PREFIX	  	?= /usr
LOCALEDIR 	?= $(PREFIX)/share/locale
VARS  	  	?= -DENABLE_NLS=1

DEBUG 		?= 1
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb -pedantic -Wall $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
        BUILDDIR  = build/release
        CXXFLAGS := -O2 $(CXXFLAGS)
endif

TARGET		 = cufetch
VERSION    	 = 0.0.1
BRANCH     	 = main
SRC 	   	 = $(sort $(wildcard src/*.cpp))
OBJ 	   	 = $(SRC:.cpp=.o)
LDFLAGS   	+= -L./$(BUILDDIR)/fmt -lfmt
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS	+= -funroll-all-loops -Iinclude -std=c++20 $(VARS) -DVERSION=\"$(VERSION)\" -DBRANCH=\"$(BRANCH)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

all: fmt toml $(TARGET)

fmt:
ifeq ($(wildcard $(BUILDDIR)/fmt/libfmt.a),)
	mkdir -p $(BUILDDIR)/fmt
	make -C src/fmt BUILDDIR=$(BUILDDIR)/fmt
endif

toml:
ifeq ($(wildcard $(BUILDDIR)/toml++/toml.o),)
	mkdir -p $(BUILDDIR)/toml++
	make -C src/toml++ BUILDDIR=$(BUILDDIR)/toml++
endif

locale:
	scripts/make_mo.sh locale/

$(TARGET): fmt toml $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CXX) $(OBJ) $(BUILDDIR)/toml++/toml.o -o $(BUILDDIR)/$(TARGET) $(LDFLAGS)

dist: $(TARGET) locale
	bsdtar --zstd -cf TabAUR-v$(VERSION).tar.zst LICENSE README.md locale/ -C $(BUILDDIR) $(TARGET)

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(OBJ)

distclean:
	rm -rf $(BUILDDIR) ./tests/$(BUILDDIR) $(OBJ) cpr/build
	find . -type f -name "*.tar.zst" -exec rm -rf "{}" \;
	find . -type f -name "*.o" -exec rm -rf "{}" \;
	find . -type f -name "*.a" -exec rm -rf "{}" \;
	#make -C tests/ clean

install: $(TARGET) locale
	install $(BUILDDIR)/$(TARGET) -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	find locale -type f -exec install -Dm 755 "{}" "$(DESTDIR)$(PREFIX)/share/{}" \;

test:
	make -C tests

.PHONY: $(TARGET) clean fmt toml locale install all
