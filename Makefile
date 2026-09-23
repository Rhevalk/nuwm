NAME    = nuwm

PREFIX  = /usr/local
CC      = cc

CFLAGS  = -Os -flto \
          -fomit-frame-pointer \
          -ffunction-sections -fdata-sections \
          -fno-asynchronous-unwind-tables \
          -fno-unwind-tables \
          -fstack-protector-strong \
          -D_FORTIFY_SOURCE=2 \
          -Wall -Wextra -Wpedantic -Wno-unused-result

LDFLAGS = -Wl,--gc-sections \
          -Wl,--as-needed \
          -Wl,-O1 \
          -Wl,-s \
          -Wl,-z,relro,-z,now

LIBS = -lxcb

all: $(NAME)

$(NAME): $(NAME).c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f $(NAME)

install: all
	install -Dm755 $(NAME) $(DESTDIR)$(PREFIX)/bin/$(NAME)
	size $(NAME)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)

.PHONY: all clean install uninstall
