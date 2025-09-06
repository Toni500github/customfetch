CC	?= cc
SRC 	 = getopt.c
TARGET   = getopt.o
CFLAGS 	 = -I../../../include/libs $(LTO_FLAGS) -fvisibility=hidden -fPIC

all: $(TARGET)

$(TARGET):
	$(CC) $(SRC) $(CFLAGS) -c -o ../../../$(BUILDDIR)/$@

clean:
	rm -rf ../../../$(BUILDDIR)/getopt_port/$(TARGET)

.PHONY: $(TARGET) clean all
