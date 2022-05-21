OBJS_DIR     = .objs
TEST_DIR     = tests
SRC_DIR      = src
LIBS_SRC_DIR = libs

EXE_SERVER = server
EXE_CLIENT = client
EXE_MAIN   = main

# set up compiler and linker
CC = clang
LD = clang

# define all the compilation flags
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter -Wmissing-declarations -Wmissing-variable-declarations
INC = -I./includes/
CFLAGS_COMMON = $(WARNINGS) $(INC) -std=c99 -c -MMD -MP -D_GNU_SOURCE
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g -DDEBUG

# Find object files for libraries
LIBS_SRC_FILES:=$(wildcard $(LIBS_SRC_DIR)/*.c)
LIBS_OBJS:=$(patsubst $(LIBS_SRC_DIR)/%.c,%.o,$(LIBS_SRC_FILES))

TEST_SRC_FILES:=$(wildcard $(TEST_DIR)/*.c)
TEST_EXES:=$(patsubst $(TEST_DIR)/%.c,%,$(TEST_SRC_FILES))

OBJS_COMMON =common.o format.o
OBJS_CLIENT =$(EXE_CLIENT).o $(OBJS_COMMON) $(LIBS_OBJS)
OBJS_SERVER =$(EXE_SERVER).o $(OBJS_COMMON) $(LIBS_OBJS)
OBJS_MAIN   =$(EXE_MAIN).o $(LIBS_OBJS)

.PHONY: all
all: release

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

.PHONY: print 
print:
	echo $(TEST_SRC_FILES)
	echo $(TEST_OBJS)
	echo $(TEST_EXES)

# build types
.PHONY: release
.PHONY: debug
.PHONY: test

release: $(EXES_STUDENT)
debug:   clean $(EXE_SERVER)-debug
test: 	 $(TEST_EXES)

# include dependencies
-include $(OBJS_DIR)/*.d

# Patterns to create objects: keep the debug and release postfix for object files so that we can always separate them correctly
$(OBJS_DIR)/%-debug.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# Define rules to compile object files for libraries
$(OBJS_DIR)/%-debug.o: $(LIBS_SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: $(LIBS_SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# Define rules to compile object files for tests
$(OBJS_DIR)/%-debug.o: $(TEST_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

# exes
$(EXE_SERVER): $(OBJS_SERVER:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER)-debug: $(OBJS_SERVER:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_CLIENT): $(OBJS_CLIENT:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ -o $@

$(EXE_CLIENT)-debug: $(OBJS_CLIENT:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ -o $@

$(EXE_MAIN): $(OBJS_MAIN:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ -o $@

# Rules to link test executables
$(TEST_EXES): %: $(OBJS_DIR)/%-debug.o $(LIBS_OBJS:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ -o $(notdir $@)

.PHONY: clean
clean:
	rm -rf .objs $(TEST_EXES)

build:
	docker build -t neilk3/linux-dev-env .

start:
	docker run -it --rm -p 49500:49500 -v `pwd`:/mount --hostname sp22-cs241-206.cs.illinois.edu neilk3/linux-dev-env
