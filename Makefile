# Host runtime + example (Linux or other Unix with /dev/aac0 after loading driver).

PREFIX ?= /usr/local
CFLAGS += -Wall -Wextra -I$(CURDIR)/include
LDFLAGS ?=

LIB_SRC := runtime/host/runtime.c
EXAMPLE_SRC := examples/host_smoke/smoke.c

.PHONY: all clean install

all: libaac.a smoke

libaac.a: $(LIB_SRC) include/aac/ioctl.h include/aac/runtime.h include/aac/types.h
	$(CC) $(CFLAGS) -c $(LIB_SRC) -o runtime.o
	$(AR) rcs $@ runtime.o

smoke: $(EXAMPLE_SRC) libaac.a
	$(CC) $(CFLAGS) $(EXAMPLE_SRC) libaac.a $(LDFLAGS) -o $@

clean:
	rm -f libaac.a runtime.o smoke

install: libaac.a include/aac/types.h include/aac/runtime.h include/aac/ioctl.h
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/aac
	install -m 644 libaac.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 include/aac/types.h include/aac/runtime.h $(DESTDIR)$(PREFIX)/include/aac/
	install -m 644 include/aac/ioctl.h $(DESTDIR)$(PREFIX)/include/aac/
