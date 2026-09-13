# FreeLinX TinyALSA (Non-GNU Audio Driver & Utilities) Makefile
CC      := /home/devuan/FreeLinX/toolchain/bin/clang
AR      := /home/devuan/FreeLinX/toolchain/bin/llvm-ar
SYSROOT := /home/devuan/FreeLinX/toolchain/x86_64-linux-musl
TARGET  := x86_64-linux-musl

CFLAGS  := -O2 -Wall -Wextra --target=$(TARGET) --sysroot=$(SYSROOT) -Iinclude -fPIC
LDFLAGS := -fuse-ld=lld --rtlib=compiler-rt -static

LIB_SRCS := src/mixer.c src/mixer_hw.c src/mixer_plugin.c src/pcm.c src/pcm_hw.c src/pcm_plugin.c src/snd_card_plugin.c src/limits.c
LIB_OBJS := $(LIB_SRCS:.c=.o)
LIB_STATIC := libtinyalsa.a

UTILS := tinymix tinyplay tinycap tinypcminfo

all: $(LIB_STATIC) $(UTILS)

$(LIB_STATIC): $(LIB_OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

tinymix: utils/tinymix.c $(LIB_STATIC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ utils/tinymix.c $(LIB_STATIC)

tinyplay: utils/tinyplay.c $(LIB_STATIC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ utils/tinyplay.c $(LIB_STATIC)

tinycap: utils/tinycap.c $(LIB_STATIC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ utils/tinycap.c $(LIB_STATIC)

tinypcminfo: utils/tinypcminfo.c $(LIB_STATIC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ utils/tinypcminfo.c $(LIB_STATIC)

clean:
	rm -f $(LIB_OBJS) $(LIB_STATIC) $(UTILS)

install: all
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/lib
	install -d $(DESTDIR)/usr/include/tinyalsa
	install -m 755 tinymix tinyplay tinycap tinypcminfo $(DESTDIR)/usr/bin/
	install -m 644 libtinyalsa.a $(DESTDIR)/usr/lib/
	install -m 644 include/tinyalsa/*.h $(DESTDIR)/usr/include/tinyalsa/

.PHONY: all clean install
