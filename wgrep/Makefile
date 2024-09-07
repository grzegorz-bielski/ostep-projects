# specify all source files here
SRCS = main.c
# specify target here (name of executable)
TARG = wgrep
# specify compiler, compile flags, and needed libs
CC = gcc
OPTS = -Wall -Werror -O
LIBS = -lm
# this translates .c files in src list to .o’s
OBJS = $(SRCS:.c=.o)
# all is not really needed, but is used to generate the target
all: $(TARG)
# this generates the target executable
$(TARG): $(OBJS)
	$(CC) -o $(TARG) $(OBJS) $(LIBS)
# this is a generic rule for .o files
%.o: %.c
	$(CC) $(OPTS) -c $< -o $@
# and finally, a clean line
clean:
	rm -f $(OBJS) $(TARG)

TEST_FOLDER_PATH = ../test-suite/initial-utilities/wgrep

test-copy: all
	cp wgrep $(TEST_FOLDER_PATH)/wgrep

test: test-copy
	cd $(TEST_FOLDER_PATH) && ./test-wgrep.sh
