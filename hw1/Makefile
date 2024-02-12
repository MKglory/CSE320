CC := gcc
SRCD := src
TSTD := tests
BLDD := build
BIND := bin
INCD := include

EXEC := philo
TEST_EXEC := $(EXEC)_tests

MAIN := $(BLDD)/main.o

ALL_SRCF := $(shell find $(SRCD) -type f -name *.c)
ALL_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(ALL_SRCF:.c=.o))
ALL_FUNCF := $(filter-out $(MAIN) $(AUX), $(ALL_OBJF))

TEST_ALL_SRCF := $(shell find $(TSTD) -type f -name *.c)
TEST_SRCF := $(filter-out $(TEST_REF_SRCF), $(TEST_ALL_SRCF))

INC := -I $(INCD)

CFLAGS := -Wall -Werror -Wno-unused-variable -Wno-unused-function -MMD -fcommon
COLORF := -DCOLOR
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=c99
TEST_LIB := -lcriterion
LIBS := $(LIB)

CFLAGS += $(STD)

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(BIND)/$(TEST_EXEC)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS) $(COLORF)
debug: all
	echo DEBUG

setup: $(BIND) $(BLDD)
	echo SETUP $(BIND)/$(EXEC) $(BIND)/$(TEST_EXEC)
	echo "ALL_FUNCF="$(ALL_FUNCF)
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)

$(BIND)/$(EXEC): $(MAIN) $(ALL_FUNCF)
	echo $(BIND)/$(EXEC)
	echo "ALL_FUNCF="$(ALL_FUNCF)
	$(CC) $(MAIN) $(ALL_FUNCF) -o $@ $(LIBS)

$(BIND)/$(TEST_EXEC): $(ALL_FUNCF) $(TEST_SRCF)
	echo $(BIND)/$(TEST_EXEC)
	$(CC) $(CFLAGS) $(INC) $(ALL_TESTF) $(ALL_FUNCF) $(TEST_SRCF) $(TEST_LIB) $(LIBS) -o $@

$(BLDD)/%.o: $(SRCD)/%.c
	echo BUILD
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	echo END_BUILD

clean:
	rm -rf $(BLDD) $(BIND)

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
