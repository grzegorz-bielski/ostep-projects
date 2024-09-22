SRCS = main.c
TARG = wish
CC = gcc
OPTS = -Wall -Werror -O
LIBS = -lm
OBJS = $(SRCS:.c=.o)
all: $(TARG)

$(TARG): $(OBJS)
	$(CC) -o $(TARG) $(OBJS) $(LIBS)
%.o: %.c
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARG)

TEST_FOLDER_PATH = ../test-suite/processes-shell

test-copy: all
	cp $(TARG) $(TEST_FOLDER_PATH)/$(TARG)

test: test-copy
	cd $(TEST_FOLDER_PATH) && ./test-$(TARG).sh
