
INCLUDES	:= -I$${AIO_LIB_DIR}
override CFLAGS	+= -D_GNU_SOURCE -I. $(INCLUDES) -I/usr/include/libusb-1.0 -std=gnu99 -g


FILES	:= $(wildcard *.c)
MOCKS	:= $(FILES:%.c=lib%.so)
GREEN	:= $(shell tput setaf 2)
RESET	:= $(shell tput sgr0 )
RED	:= $(shell tput setaf 1 )


all:	$(MOCKS)

lib%.so:%.c
	@cmake -E cmake_echo_color --green "Building $@"
	@$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -shared -fPIC -o $@ $< -ldl



clean:
	rm -f *.so
