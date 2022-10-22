OBJS_DIR     = .objs
TEST_DIR     = tests
SRC_DIR      = src
APPS_DIR     = apps
LIBS_DIR     = libs
LIBS_SRC_DIR = c-data-structures

EXE_SERVER = server
EXE_CLIENT = client
EXE_MAIN   = main

# set up compiler and linker
CC = clang
LD = clang

PROVIDED_LIBRARIES:=$(shell find $(LIBS_DIR) -type f -name '*.a' 2>/dev/null)
PROVIDED_LIBRARIES:=$(PROVIDED_LIBRARIES:libs/lib%.a=%)
LDFLAGS = -Llibs/ $(foreach lib,$(PROVIDED_LIBRARIES),-l$(lib)) -lm

# define all the compilation flags
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter -Wmissing-declarations -Wmissing-variable-declarations
INC = -I./includes/ -I./c-data-structures/includes
CFLAGS_COMMON = $(WARNINGS) $(INC) -std=c99 -c -MMD -MP -D_GNU_SOURCE -D__LOG_REQUESTS__
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g -DDEBUG -pg

# Find object files for libraries
OBJS_SRC :=$(patsubst $(SRC_DIR)/%.c,%.o,$(wildcard $(SRC_DIR)/*.c))

TEST_SRC_FILES:=$(wildcard $(TEST_DIR)/*.c)
TEST_EXES:=$(patsubst $(TEST_DIR)/%.c,%,$(TEST_SRC_FILES))

OBJS_CLIENT = $(EXE_CLIENT).o $(OBJS_SRC)
OBJS_TEST   = $(OBJS_SRC)
OBJS_SERVER = $(EXE_SERVER)_main.o $(OBJS_SRC)
OBJS_MAIN   = $(EXE_MAIN).o route.o request.o response.o protocol.o format.o

.PHONY: all
all: release

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

.PHONY: libs
$(LIBS_DIR):
	@mkdir -p $(LIBS_DIR)
	$(MAKE) -C $(LIBS_SRC_DIR) LIBS_DIR=../$(LIBS_DIR)

.PHONY: print 
print:
	echo $(LDFLAGS)

# build types
.PHONY: release
.PHONY: debug
.PHONY: test

release: $(EXE_SERVER) $(TEST_EXES) $(EXE_MAIN)
debug:   clean $(EXE_SERVER)-debug
test: 	 $(TEST_EXES)

# include dependencies
-include $(OBJS_DIR)/*.d

################################################################################
#                          Patterns to Create Objects                          #
# keep debug & release postfix for object files so we can always separate them #
################################################################################
$(OBJS_DIR)/%-debug.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# Define rules to compile object files for apps
$(OBJS_DIR)/%-debug.o: $(APPS_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: $(APPS_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# Define rules to compile object files for tests
$(OBJS_DIR)/%-debug.o: $(TEST_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

################################################################################
#                          Rules to Link Executables                           #
################################################################################
$(EXE_SERVER): $(OBJS_SERVER:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER)-debug: $(OBJS_SERVER:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_CLIENT): $(OBJS_CLIENT:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_CLIENT)-debug: $(OBJS_CLIENT:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_MAIN): $(OBJS_MAIN:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

# Rules to link test executables
$(TEST_EXES): %: $(OBJS_DIR)/%-debug.o $(OBJS_TEST:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $(notdir $@)

################################################################################
#                              Bash Command Rules                              #
################################################################################
.PHONY: clean
clean:
	rm -rf .objs $(TEST_EXES) $(EXE_CLIENT) $(EXE_SERVER) $(EXE_CLIENT)-debug $(EXE_SERVER)-debug $(EXE_MAIN)

build:
	docker build -t neilk3/linux-dev-env .

build-release:
	docker build -t neilk3/web-server -f Dockerfile.release .

start: build
	docker run -it --rm -p 80:8000 -v `pwd`:/mount neilk3/linux-dev-env

start-release: build-release
	docker run -it --rm -p 80:8000 -v `pwd`/favicon.png:/service/favicon.png neilk3/web-server

trace: debug
	sudo dtrace -s trace/timing.d -c "./server-debug 80"

fcalls: debug
	sudo dtrace -s trace/fcalls.d -c "./server-debug 80"

valgrind: server-debug
	valgrind --leak-check=full --show-leak-kinds=all ./server-debug 8000

leaks: server-debug
	export MallocStackLogging=1 && ./server-debug 80