CXX       	?= g++
PREFIX	  	?= /usr
MANPREFIX	?= $(PREFIX)/share/man
APPPREFIX 	?= $(PREFIX)/share/applications
LOCALEDIR	?= $(PREFIX)/share/locale
VARS  	  	?= -DENABLE_NLS=1

DEBUG 		?= 1
GUI_MODE        ?= 0
VENDOR_TEST 	?= 0
DEVICE_TEST     ?= 0

USE_DCONF	?= 1
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb3 -Wall -Wextra -Wpedantic -Wno-unused-parameter -DDEBUG=1 $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
	# Check if an optimization flag is not already set
	ifneq ($(filter -O%,$(CXXFLAGS)),)
    		$(info Keeping the existing optimization flag in CXXFLAGS)
	else
    		CXXFLAGS := -O3 $(CXXFLAGS)
	endif
        BUILDDIR  = build/release
endif

ifeq ($(VENDOR_TEST), 1)
	VARS += -DVENDOR_TEST=1
endif

ifeq ($(DEVICE_TEST), 1)
	VARS += -DDEVICE_TEST=1
endif

ifeq ($(GUI_MODE), 1)
        VARS 	 += -DGUI_MODE=1
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

NAME		= customfetch
TARGET		= $(NAME)
OLDVERSION	= 0.10.1
VERSION    	= 0.10.2
BRANCH     	= $(shell git rev-parse --abbrev-ref HEAD)
SRC 	   	= $(wildcard src/*.cpp src/query/linux/*.cpp src/query/android/*.cpp src/query/linux/utils/*.cpp)
OBJ 	   	= $(SRC:.cpp=.o)
LDFLAGS   	+= -L./$(BUILDDIR)/fmt -lfmt -ldl
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -fvisibility=hidden -Iinclude -std=c++20 $(VARS) -DVERSION=\"$(VERSION)\" -DBRANCH=\"$(BRANCH)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

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

$(TARGET): fmt toml $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CXX) $(OBJ) $(BUILDDIR)/toml++/toml.o -o $(BUILDDIR)/$(TARGET) $(LDFLAGS)
	cd $(BUILDDIR)/ && ln -sf $(TARGET) cufetch

android_app:
ifeq ($(DEBUG), 1)
	./android/gradlew assembleDebug --project-dir=./android
else
	./android/gradlew assembleRelease --project-dir=./android
endif
	@if [ $$? -eq 0 ]; then\
		echo "APK build successfully. Get it in $(CURDIR)/android/app/build/outputs/apk path and choose which to install (debug/release)";\
	fi

locale:
	scripts/make_mo.sh locale/

dist: $(TARGET) locale
ifeq ($(GUI_MODE), 1)
	bsdtar -zcf $(NAME)-v$(VERSION).tar.gz LICENSE $(TARGET).desktop locale/ $(TARGET).1 assets/ascii/ -C $(BUILDDIR) $(TARGET)
else
	bsdtar -zcf $(NAME)-v$(VERSION).tar.gz LICENSE $(TARGET).1 locale/ assets/ascii/ -C $(BUILDDIR) $(TARGET)
endif

usr-dist: $(TARGET) locale
	mkdir -p ./usr/bin usr/share/man/man1 ./usr/share/$(NAME) ./usr/share/locale ./usr/share/licenses/$(NAME)
	cp -f $(BUILDDIR)/$(TARGET) ./usr/bin/
	cp -f LICENSE ./usr/share/licenses/$(NAME)/
	sed -e "s/@VERSION@/$(VERSION)/g" -e "s/@BRANCH@/$(BRANCH)/g" < $(TARGET).1 > ./usr/share/man/man1/$(TARGET).1
	cp -rf locale/* ./usr/share/locale/
	cp -rf assets/ascii/ ./usr/share/$(NAME)/
ifeq ($(GUI_MODE), 1)
        mkdir -p ./usr/share/applications
        cp -f $(TARGET).desktop ./usr/share/applications/$(TARGET).desktop
endif
	bsdtar -zcf $(NAME)-v$(VERSION).tar.gz usr/

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(BUILDDIR)/libcustomfetch.a $(OBJ)

distclean:
	rm -rf $(BUILDDIR) ./tests/$(BUILDDIR) $(OBJ)
	find . -type f -name "*.tar.gz" -exec rm -rf "{}" \;
	find . -type f -name "*.o" -exec rm -rf "{}" \;
	find . -type f -name "*.a" -exec rm -rf "{}" \;

install: $(TARGET) locale
	install $(BUILDDIR)/$(TARGET) -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	cd $(DESTDIR)$(PREFIX)/bin/ && ln -sf $(TARGET) cufetch
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1/
	sed -e "s/@VERSION@/$(VERSION)/g" -e "s/@BRANCH@/$(BRANCH)/g" < $(TARGET).1 > $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	cd assets/ && find ascii/ -type f -exec install -Dm 644 "{}" "$(DESTDIR)$(PREFIX)/share/customfetch/{}" \;
	find locale/ -type f -exec install -Dm 755 "{}" "$(DESTDIR)$(PREFIX)/share/{}" \;
ifeq ($(GUI_MODE), 1)
	mkdir -p $(DESTDIR)$(APPPREFIX)
	cp -f $(TARGET).desktop $(DESTDIR)$(APPPREFIX)/$(TARGET).desktop
endif

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/$(TARGET) $(DESTDIR)$(PREFIX)/bin/cufetch
	rm -f  $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	rm -f  $(DESTDIR)$(APPPREFIX)/$(TARGET).desktop
	rm -rf $(DESTDIR)$(PREFIX)/share/customfetch/

remove: uninstall
delete: uninstall

updatever:
	sed -i "s#$(OLDVERSION)#$(VERSION)#g" $(wildcard .github/workflows/*.yml) compile_flags.txt

.PHONY: $(TARGET) updatever remove uninstall delete dist distclean fmt toml install all locale
