CC := gcc
exe := main
src := $(wildcard *.c)
obj := $(src:.c=.o) # 解釋：將 src 中的 .c 換成 .o
CFLAGS := -Wall -g
LDFLAGS := -lm

$(exe): $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(exe)

