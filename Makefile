# Ripples Makefile
# Directory setup
HDR_DIR  := include
SRC_DIR  := src
LIB_DIR  := libs
OBJ_DIR  := obj
BIN_DIR  := build/bin
TEST_DIR := test_c
LIBS_DIR := build/docs
DOC_DIR  := build/docs

# LIBLFDS
LFDS           := lfds
LFDS_VERSION   := liblfds711
LFDS_BUILD_DIR := $(LIB_DIR)/liblfds/$(LFDS_VERSION)/build/gcc_gnumake
LFDS_INC       := $(LIB_DIR)/liblfds/$(LFDS_VERSION)/inc
LFDS_LIB       := $(LIB_DIR)/liblfds/$(LFDS_VERSION)/bin/$(LFDS_VERSION).a


# Test Framework
TEST_FMK := libs/munit/munit.c

# Executable produced
EXE := $(BIN_DIR)/ripples

# Documentation
DOXYGEN := doxygen

# Compiler options
CC       := clang
INC      := -I$(HDR_DIR) -Ilibs -I$(LFDS_INC)
#CFLAGS   := -Wall -Wno-unknown-pragmas -std=gnu17 -march=native -O3 -D_GNU_SOURCE
CFLAGS   := -g -Wall -Wno-unknown-pragmas -std=gnu17 -D_GNU_SOURCE
LDFLAGS  := -Llib
LDLIBS   := -lm -lpthread

#Source and object files
SRC           := $(wildcard $(SRC_DIR)/*.c)
OBJ           := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_LINK_OBJ := $(subst obj/main.o,,${OBJ})
TEST_SRC      := $(wildcard $(TEST_DIR)/test_*.c)
TEST_BIN      := $(TEST_DIR)/ctests

# Targets
.PHONY: default all clean ripples
default: help
all: ripples ripples_debug test doc
ripples: $(EXE)
ripples_debug: $(EXE)_debug
lfds: $(LFDS)

clean: clean_ripples clean_debug clean_obj clean_test clean_doc clean_lfds

$(LFDS):
	$(MAKE) -C $(LFDS_BUILD_DIR)

$(EXE)_debug: $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) $(LFDS_LIB) -o $@

$(EXE): $(OBJ) | $(BIN_DIR) 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) $(LFDS_LIB) -o $@

$(OBJ): $(LFDS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) 
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

test: $(TEST_LINK_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -lcriterion $(LDLIBS) $(TEST_LINK_OBJ) $(TEST_SRC) $(INC) $(LFDS_LIB) -o $(TEST_BIN)
	$(TEST_BIN)

clean_test:
	@$(RM) $(TEST_BIN)

clean_ripples:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

clean_obj:
	@$(RM) -rv $(OBJ_DIR)

clean_lfds:
	$(MAKE) -C $(LFDS_BUILD_DIR) clean

doc:
	@$(DOXYGEN) docs/Doxygen.config

clean_doc:
	@$(RM) -rv $(DOC_DIR)

debug: CFLAGS +=  -DDEBUG

debug: ripples_debug

clean_debug:
	@$(RM) -rv $(EXE)_debug

help:
	@echo "Usage: make \033[0;32m<command>\033[0m\n\nCommands:\n"
	@echo "  \033[0;32mhelp\033[0m           Prints his help message.\n"
	@echo "  \033[0;32mall\033[0m            Builds all targets: ripples, ripples_debug, test, and doc.\n"
	@echo "  \033[0;32mripples\033[0m        Builds the ripples application. Binary file will be build/bin/ripples.\n"
	@echo "  \033[0;32mripples_debug\033[0m  Builds the ripples application with debug output compiled in.\n\
	                 See documentation for details on debug output.\n\
	                 Binary file will be build/bin/ripples_debug.\n"
	@echo "  \033[0;32mtest\033[0m           Builds and runs unit tests in \"test_c\" directory. Binary for\n\
	                 unit tests is test_c/ctest. You can execute it as is or run\n\
	                 \"test_c/ctests -h\" to see options it supports.\n\
	                 Unit tests require criterion library to be installed on\n\
	                 the system. On Ubuntu Linux you can do so via:\n\
	                 apt install libcriterion-dev\n"
	@echo "  \033[0;32mdoc\033[0m            Build documentation using doxygen. You need doxygen installed\n\
	                 on the system. Documentation will be in build/docs/html\n\
	                 directory. Once built, open index.html file in a browser.\n"
	@echo "  \033[0;32mlfds\033[0m           Build liblfds only.\n"
	@echo "  \033[0;32mclean\033[0m          Cleans all builds.\n"
	@echo "  \033[0;32mclean_ripples\033[0m  Cleans ripples build.\n"
	@echo "  \033[0;32mclean_debug\033[0m    Removes ripples_debug binary.\n"
	@echo "  \033[0;32mclean_test\033[0m     Removes test/ctests binary.\n"
	@echo "  \033[0;32mclean_doc\033[0m      Removes built documentation.\n"
