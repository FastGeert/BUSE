TARGET		:= busexmp loopback pythontest pythonbd
LIBOBJS 	:= buse.o
OBJS		:= $(TARGET:=.o) $(LIBOBJS)
STATIC_LIB	:= libbuse.a

CC		:= /usr/bin/gcc
CFLAGS		:= -g -pedantic -Wall -Wextra -std=c99 -I/usr/include/python3.5m -I/usr/include/python3.5m  -Wno-unused-result -Wsign-compare -g -fstack-protector-strong -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes
LDFLAGS   := -L. -lbuse -L/usr/lib/python3.5/config-3.5m-x86_64-linux-gnu -L/usr/lib -lpython3.5m -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions

.PHONY: all clean
all: $(TARGET)

$(TARGET): %: %.o $(STATIC_LIB)
	$(CC) -o $@ $< $(LDFLAGS)

$(TARGET:=.o): %.o: %.c buse.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(STATIC_LIB): $(LIBOBJS)
	ar rcu $(STATIC_LIB) $(LIBOBJS)

$(LIBOBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS) $(STATIC_LIB)
