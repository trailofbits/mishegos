SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

PROG = mish2jsonl

.PHONY: all
all: $(PROG)

$(PROG): $(OBJS) 

.PHONY: clean
clean:
	rm -rf $(PROG) $(OBJS)

