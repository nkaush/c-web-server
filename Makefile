OBJS_DIR = .objs

EXE_SERVER = server
EXE_CLIENT = client
EXE_TEST = set_test

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
LIBS_SRC_DIR=libs
LIBS_SRC_FILES:=$(wildcard $(LIBS_SRC_DIR)/*.c)
LIBS_OBJ_FILES:=$(patsubst $(LIBS_SRC_DIR)/%.c,%.o,$(LIBS_SRC_FILES))
LIBS_NAMES:=$(patsubst $(LIBS_SRC_DIR)/%.c,%,$(LIBS_SRC_FILES))

OBJS_CLIENT=$(EXE_CLIENT).o $(LIBS_OBJ_FILES)
OBJS_TEST=$(EXE_TEST).o $(LIBS_OBJ_FILES)

.PHONY: all
all: release

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

.PHONY: print 
print:
	echo $(LIBS_SRC_FILES)
	echo $(LIBS_OBJ_FILES)
	echo $(LIBS_NAMES)

# build types
.PHONY: release
.PHONY: debug

release: $(EXES_STUDENT)
debug:   clean $(EXE_TEST)-debug

# include dependencies
-include $(OBJS_DIR)/*.d

# Patterns to create objects: keep the debug and release postfix for object files so that we can always separate them correctly
$(OBJS_DIR)/%-debug.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# Define rules to compile object files for libraries
$(OBJS_DIR)/%-debug.o: $(LIBS_SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: $(LIBS_SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# exes
# $(EXE_SERVER): $(OBJS_SERVER:%.o=$(OBJS_DIR)/%-release.o)
# 	$(LD) $^ $(LDFLAGS) -o $@

# $(EXE_SERVER)-debug: $(OBJS_SERVER:%.o=$(OBJS_DIR)/%-debug.o)
# 	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_CLIENT): $(OBJS_CLIENT:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ -o $@

$(EXE_CLIENT)-debug: $(OBJS_CLIENT:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ -o $@

$(EXE_TEST): $(OBJS_TEST:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ -o $@ 

$(EXE_TEST)-debug: $(OBJS_TEST:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ -o $@ 

.PHONY: clean
clean:
	rm -rf .objs

build:
	docker build -t neilk3/linux-dev-env .

start:
	docker run -it --rm -p 49500:49500 -v `pwd`:/mount --hostname sp22-cs241-206.cs.illinois.edu neilk3/linux-dev-env
