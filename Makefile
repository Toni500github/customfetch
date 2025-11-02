CXX       	?= g++
TAR		?= bsdtar
PREFIX	  	?= /usr
MANPREFIX	?= $(PREFIX)/share/man
APPPREFIX 	?= $(PREFIX)/share/applications
LOCALEDIR	?= $(PREFIX)/share/locale
ICONPREFIX 	?= $(PREFIX)/share/pixmaps
VARS  	  	?= -DENABLE_NLS=1
CXXSTD		?= c++20

DEBUG 		?= 1
GUI_APP         ?= 0
USE_DCONF	?= 1

COMPILER := $(shell $(CXX) --version | head -n1)

ifeq ($(findstring g++,$(COMPILER)),g++)
	export LTO_FLAGS = -flto=auto -ffat-lto-objects
else ifeq ($(findstring clang,$(COMPILER)),clang)
	export LTO_FLAGS = -flto=thin
else
    $(warning Unknown compiler: $(COMPILER). No LTO flags applied.)
endif

# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  := build/debug
	LTO_FLAGS  = -fno-lto
        CXXFLAGS  := -ggdb3 -Wall -Wextra -pedantic -Wno-unused-parameter -fsanitize=address \
			-DDEBUG=1 -fno-omit-frame-pointer $(DEBUG_CXXFLAGS) $(CXXFLAGS)
        LDFLAGS	  += -fsanitize=address -fno-lto -Wl,-rpath,$(BUILDDIR)
else
	# Check if an optimization flag is not already set
	ifneq ($(filter -O%,$(CXXFLAGS)),)
    		$(info Keeping the existing optimization flag in CXXFLAGS)
	else
    		CXXFLAGS := -O3 $(CXXFLAGS)
	endif
	LDFLAGS   += $(LTO_FLAGS)
        BUILDDIR  := build/release
endif

ifeq ($(GUI_APP), 1)
	TARGET	  = $(NAME)-gui
	VARS 	 += -DGUI_APP=1
	LDFLAGS	 += `pkg-config --libs gtkmm-3.0`
	CXXFLAGS += `pkg-config --cflags gtkmm-3.0`
endif

ifeq ($(USE_DCONF), 1)
        ifeq ($(shell pkg-config --exists glib-2.0 dconf && echo 1), 1)
                CXXFLAGS += -DUSE_DCONF=1 `pkg-config --cflags glib-2.0 dconf`
        else
                CXXFLAGS += -DUSE_DCONF=0
        endif
endif

NAME		 = customfetch
TARGET		?= $(NAME)
OLDVERSION	 = 1.0.0
VERSION    	 = 2.0.0-beta1
SRC_CPP 	 = $(wildcard src/*.cpp)
SRC_CC  	 = $(wildcard src/core-modules/*.cc src/core-modules/linux/*.cc src/core-modules/linux/utils/*.cc src/core-modules/android/*.cc src/core-modules/macos/*.cc)
OBJ_CPP 	 = $(SRC_CPP:.cpp=.o)
OBJ_CC  	 = $(SRC_CC:.cc=.o)
OBJ		 = $(OBJ_CPP) $(OBJ_CC)
LDFLAGS   	+= -L$(BUILDDIR)
LDLIBS		+= $(BUILDDIR)/libfmt.a $(BUILDDIR)/libtiny-process-library.a -lcufetch -ldl
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += $(LTO_FLAGS) -fvisibility-inlines-hidden -fvisibility=hidden -Iinclude -Iinclude/libcufetch -Iinclude/libs -std=$(CXXSTD) $(VARS) -DVERSION=\"$(VERSION)\" -DLOCALEDIR=\"$(LOCALEDIR)\" -DICONPREFIX=\"$(ICONPREFIX)\"

all: genver fmt toml tpl getopt-port json libcufetch $(TARGET)

libcufetch: fmt tpl toml
ifeq ($(wildcard $(BUILDDIR)/libcufetch.so),)
	$(MAKE) -C libcufetch BUILDDIR=$(BUILDDIR) GUI_APP=$(GUI_APP) CXXSTD=$(CXXSTD) DEBUG=$(DEBUG)
endif

fmt:
ifeq ($(wildcard $(BUILDDIR)/libfmt.a),)
	mkdir -p $(BUILDDIR)
	$(MAKE) -C src/libs/fmt BUILDDIR=$(BUILDDIR) CXXSTD=$(CXXSTD)
endif

toml:
ifeq ($(wildcard $(BUILDDIR)/toml.o),)
	$(MAKE) -C src/libs/toml++ BUILDDIR=$(BUILDDIR) CXXSTD=$(CXXSTD)
endif

tpl:
ifeq ($(wildcard $(BUILDDIR)/libtiny-process-library.a),)
	$(MAKE) -C src/libs/tiny-process-library BUILDDIR=$(BUILDDIR) CXXSTD=$(CXXSTD)
endif

getopt-port:
ifeq ($(wildcard $(BUILDDIR)/getopt.o),)
	$(MAKE) -C src/libs/getopt_port BUILDDIR=$(BUILDDIR)
endif

json:
ifeq ($(wildcard $(BUILDDIR)/json.o),)
	$(MAKE) -C src/libs/json BUILDDIR=$(BUILDDIR) CXXSTD=$(CXXSTD)
endif

genver: ./scripts/generateVersion.sh
ifeq ($(wildcard include/version.h),)
	./scripts/generateVersion.sh
endif

$(TARGET): genver fmt toml tpl getopt-port json libcufetch $(OBJ)
	mkdir -p $(BUILDDIR)
	sh ./scripts/generateVersion.sh
	$(CXX) -o $(BUILDDIR)/$(TARGET) $(OBJ) $(BUILDDIR)/*.o $(LDFLAGS) $(LDLIBS)
	cd $(BUILDDIR)/ && ln -sf $(TARGET) cufetch

locale:
	scripts/make_mo.sh locale/

#dist: $(TARGET) locale
#ifeq ($(GUI_APP), 1)
#	$(TAR) -zcf $(NAME)-v$(VERSION).tar.gz LICENSE $(NAME).desktop locale/ $(NAME).1 assets/ascii/ -C $(BUILDDIR) $(TARGET)
#else
#	$(TAR) -zcf $(NAME)-v$(VERSION).tar.gz LICENSE $(NAME).1 locale/ assets/ascii/ -C $(BUILDDIR) $(TARGET)
#endif
dist:
	$(warning `make dist` is deprecated, use `make usr-dist` instead)

usr-dist: $(TARGET) locale
	mkdir -p $(PWD)/usr
	$(MAKE) install DESTDIR=$(PWD) PREFIX=/usr
	$(TAR) -zcf $(NAME)-v$(VERSION).tar.gz usr/
	rm -rf usr/

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(BUILDDIR)/libcufetch.so $(OBJ) libcufetch/*.o

distclean:
	rm -rf $(BUILDDIR) ./tests/$(BUILDDIR) $(OBJ)
	find . -type f -name "*.tar.gz" -delete
	find . -type f -name "*.o" -delete
	find . -type f -name "*.a" -delete

install: install-common $(TARGET)
	install $(BUILDDIR)/$(TARGET) -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	cd $(DESTDIR)$(PREFIX)/bin/ && ln -sf $(TARGET) cufetch

install-common: genver libcufetch locale
	$(MAKE) -C cufetchpm DEBUG=$(DEBUG) GUI_APP=$(GUI_APP) CXXSTD=$(CXXSTD)
	install cufetchpm/$(BUILDDIR)/cufetchpm -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/cufetchpm
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1/
	sed -e "s/@VERSION@/$(VERSION)/g" -e "s/@BRANCH@/$(BRANCH)/g" < docs/man/$(NAME).1 > $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1
	install LICENSE -Dm 644 $(DESTDIR)$(PREFIX)/share/licenses/$(NAME)/LICENSE
	cd assets/ && find ascii/ -type f -exec install -Dm 644 "{}" "$(DESTDIR)$(PREFIX)/share/$(NAME)/{}" \;
	cd assets/icons && find . -type f -exec install -Dm 644 "{}" "$(DESTDIR)$(ICONPREFIX)/$(NAME)/{}" \;
	find examples/ -type f -exec install -Dm 644 "{}" "$(DESTDIR)$(PREFIX)/share/$(NAME)/{}" \;
	find locale/ -type f -exec install -Dm 644 "{}" "$(DESTDIR)$(PREFIX)/share/{}" \;
	mkdir -p $(DESTDIR)$(PREFIX)/include/libcufetch/
	cd include/libcufetch && find . -type f -exec install -Dm 644 "{}" "$(DESTDIR)$(PREFIX)/include/libcufetch/{}" \;
	cd $(BUILDDIR) && find . -name "libcufetch*" -exec install -Dm 755 "{}" "$(DESTDIR)$(PREFIX)/lib/{}" \;
	install -Dm 755 $(BUILDDIR)/libfmt.a $(DESTDIR)$(PREFIX)/lib/libcufetch-fmt.a
ifeq ($(GUI_APP), 1)
	mkdir -p $(DESTDIR)$(APPPREFIX)
	cp -f assets/$(NAME).desktop $(DESTDIR)$(APPPREFIX)/$(NAME).desktop
endif

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/$(TARGET) $(DESTDIR)$(PREFIX)/bin/cufetch
	rm -f  $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1
	rm -f  $(DESTDIR)$(APPPREFIX)/$(NAME).desktop
	rm -rf $(DESTDIR)$(PREFIX)/share/licenses/$(NAME)/
	rm -rf $(DESTDIR)$(PREFIX)/share/$(NAME)/
	rm -rf $(DESTDIR)$(PREFIX)/include/cufetch/
	rm -rf $(DESTDIR)$(ICONPREFIX)/$(NAME)/
	find   $(DESTDIR)$(LOCALEDIR) -type f -path "$(DESTDIR)$(LOCALEDIR)/*/LC_MESSAGES/$(NAME).mo" -exec rm -f {} \;

remove: uninstall
delete: uninstall

updatever:
	sed -i "s#$(OLDVERSION)#$(VERSION)#g" $(wildcard .github/workflows/*.yml) compile_flags.txt
	sed -i "s#Project-Id-Version: $(NAME) $(OLDVERSION)#Project-Id-Version: $(NAME) $(VERSION)#g" po/*

.PHONY: $(TARGET) updatever remove uninstall delete dist distclean fmt toml libcufetch install all locale
