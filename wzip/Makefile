SRCS = main.c
TARG = wzip
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

TEST_FOLDER_PATH = ../test-suite/initial-utilities/$(TARG)

test-copy: all
	cp $(TARG) $(TEST_FOLDER_PATH)/$(TARG)

test: test-copy
	cd $(TEST_FOLDER_PATH) && ./test-$(TARG).sh
