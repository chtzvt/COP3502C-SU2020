# Grok AutoBuild - A Useful Makefile
# (c) 2018 Charlton Trezevant - MIT License

# Overview:
# 	The stated goal of Grok is to introduce the basic concepts of automation
# 	by providing a thin, useful layer of abstraction over the more clerical parts of
# 	the software development workflow.
#
# 	While intended for use in an academic context,where a developer might be learning
# 	a language or related tooling, Grok may be adapted for any purpose seen fit
# 	by its user, and such experimentation or extension is in fact encouraged.
#
# 	This Makefile will automatically detect and build any C source file
# 	in the ./src/ directory whose name ends with '_main.c'. It additionally provides
# 	a useful set of aliases for various development tools and procedures.
#
#	Each rule in this file has its dependencies properly defined, and sanity checks are
#	performed where appropriate. Should a rule encounter an error, you will recieve a
# 	straightforward error message. You will not have to worry about losing work due to
#	any process performed by this file.
#
# 	This file has been meticulously commented, for your ease of understanding.

# Usage:
#	make all or make <your project name> - Compiles your project
#		Note: compiled executables will be placed in the directory ./bin/
#	make run - Compiles and executes your project
#	make test - Compiles your project and runs tests agains it (see below)
#	make testfile or make etf - Creates a test input file and opens it in your $EDITOR
#		Note: Nether of these will overwrite an existing test input file, they will only open it
#	in your editor or create one if it does not exist.
#	make lint - Static code analysis via scan-build
#		Note: this requres you to have scan-build installed on your system
#	make clean - Removes the ./bin/ directory
#	make help - prints a short (read: useless) help message, as well as the version of this file

# Notes on automated testing
# 	In its current form, Grok is capable of performing rudimentary testing of
# 	your compiled code. This is useful for debugging purposes, provided that you
# 	place the inputs required for reaching a success state in the file ./test_input.txt.
# How it works
# 	Given the files ./test_input.txt (and optionally ./test_args.txt), Grok will interactively
#   test your program by executing it and piping the input provided in ./test_input.txt to your program.
#   If you supply arguments in test_args.txt, the contents of that file will be passed in as arguments
#   when your program is executed.
# What this means
# 	Given the set of inputs that will (or should) produce a "successful" run of
# 	your program, Grok can easily run repeated tests against your code. Rather than
# 	spend time manually executing your program, running 'make test' will command Grok
# 	to automatically recompile, execute, and run through your program. As it does so, Grok
# 	will read from the test input file, printing each line of it in order as your program prompts
# 	the shell for input. Unlike a human, however, Grok will complete this process instantly.
# What isn't tested
# 	Although Travis CI (if in use with your repository) will perform more extensive testing (including
# 	static analysis/linting of your code), the test rule in this makefile will not perform any of those
# 	checks. Rather, this file will only test the behavior of your program at runtime.
# 	For static analysis, please install the scan-build package and run 'make lint'.
# 	For information about the scan-build package, visit https://clang-analyzer.llvm.org/scan-build.html
# Creating the test_input file
# 	To create test_input.txt, simply fill the file with the inputs your program requires in the order
# 	in which your program expects or prompts for them. A working example is provided in this template.
# 	To easily create this file, use 'make testfile'

# Grok script version number
AB_VERSION=1.4
CPYEAR=2020
# The full path to the current project's main source file
PROJSRC=$(wildcard ${CURDIR}/src/*_main.c)
# The "human" name of the current project (e.g. foo_main.c becomes foo)
PROJNAME=$(subst _main.c,,$(subst ${CURDIR}/src/,,$(PROJSRC)))


# Default Make rules to build your project.
# Run any of these (make all or make projectname or make build, even just 'make') if
# you want to compile your project without also running tests
# These rules are effectively aliases to the 'build' rule, and do the same thing
all:build
# make <your project name> (e.g. for a project foo_main.c, use 'make foo')
$(PROJNAME):build


# Make rule to compile your project
# When calling 'make test', this rule will run first to compile the project before testing.
# In fact, any rule in this Makefile which requires your project to have been built first
# will, in fact, call this rule before running.
build:$(PROJSRC)
# If the location of the project's source file is valid
ifneq ("$(PROJSRC)","")
	$(info (Grok - Compile): Building "${PROJNAME}" from $^...)
# Create a directory for our compiled binaries (./bin/)
# -p tells makedir not to complain if that directory already exists
	@mkdir -p ${CURDIR}/bin/
# Call GCC to compile your source(s)
# This will produce a binary in ./bin/, with the name of your project
# $@ is the project name foo, $^ is the full path to the source file foo_main.c
# Note: the flag -std=c99 tells GCC to compile for C99 syntax (ISO/IEC 9899:1999)
	gcc -Wall -g -std=c99 -o ${CURDIR}/bin/$(PROJNAME) $^
# Set the execuable bit on the binary produced by GCC
# In some cases, this may not always be necessary. But we'll do it anyways.
	@chmod +x ${CURDIR}/bin/$(PROJNAME)
else
# Otherwise, print an error message
	$(error (Grok - Compile): ERROR: Could not find the main source file! \
	Please ensure that your source code is located inside of ${CURDIR}, and that \
	your source's filename ends with '_main.c'.)
endif
# Prints an empty line (newline)
	@echo ""

# This rule provides an alias to easily compile and execute your project.
# Rather than typing the full path to the executable at your shell, you can
# simply use 'make run'.

# Support for passing arguments to "make run"
# i.e. make run [your arguments here]
# Many thanks to https://stackoverflow.com/a/14061796
ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif

run:all
	$(info (Grok - Run Project): Running "${PROJNAME}"...)
	${CURDIR}/bin/$(PROJNAME) $(RUN_ARGS)

# Rule for running tests
# When called with 'make test', your project will be compiled and then tested
# against sample input provided in the file ./test_input.txt.
# If the test input file does not exist, an error message will be displayed and
# Grok will not attempt to run tests.
# See the top of this file for documentation regarding the test input file.
test:$(PROJNAME)
ifneq ("$(wildcard ${CURDIR}/tests/test_input.txt)","")
	$(info (Grok - Test): Testing "${PROJNAME}" with provided input from ${CURDIR}/tests/test_input.txt...)

# Depending on whether or not arguments are provided for the project in ./test_args.txt, Grok will automatically
# pass those in when your program is executed
ifneq ("$(wildcard ${CURDIR}/tests/test_args.txt)","")
	$(info (Grok - Test): Will test "${PROJNAME}" using arguments provided in ${CURDIR}/tests/test_args.txt...)
	${CURDIR}/bin/$(PROJNAME) `cat ${CURDIR}/tests/test_args.txt` < ${CURDIR}/tests/test_input.txt
else
	${CURDIR}/bin/$(PROJNAME) < ${CURDIR}/tests/test_input.txt
endif
else
	$(error (Grok - Test): Will not test "${PROJNAME}", as no test input is available! (${CURDIR}/tests/test_input.txt doesn't exist) \
	Use 'make testfile' or 'make etf' to create one. \
	For detailed documentation, please run 'cat ${CURDIR}/makefile | more' in your terminal.)
endif
	@echo ""

# Rule for running Valgrind
# When called with 'make valgrind', your project will be compiled and then tested
# in the same manner as 'make test', but it will be tested for memory leaks and other
# errors using Valgrind as well.
valgrind:$(PROJNAME)
ifeq (, $(shell which valgrind))
	$(error (Grok - Valgrind): Cannot test "${PROJNAME}", as valgrind does not appear to be installed. Please install valgrind, or check your PATH. \
	For detailed documentation, please run 'cat ${CURDIR}/makefile | more' in your terminal.)
endif
ifneq ("$(wildcard ${CURDIR}/tests/test_input.txt)","")
	$(info (Grok - Valgrind): Testing "${PROJNAME}" with provided input from ${CURDIR}/tests/test_input.txt...)
# Depending on whether or not arguments are provided for the project in ./test_args.txt, Grok will automatically
# pass those in when your program is executed
ifneq ("$(wildcard ${CURDIR}/tests/test_args.txt)","")
	$(info (Grok - Valgrind): Will test "${PROJNAME}" using arguments provided in ${CURDIR}/tests/test_args.txt...)
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes -v ${CURDIR}/bin/$(PROJNAME) `cat ${CURDIR}/tests/test_args.txt` < ${CURDIR}/tests/test_input.txt
else
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes -v ${CURDIR}/bin/$(PROJNAME) < ${CURDIR}/tests/test_input.txt
endif
else
	$(info (Grok - Valgrind): Will not test "${PROJNAME}", as no test input is available! (${CURDIR}/tests/test_input.txt doesn't exist))
	$(info (Grok - Valgrind): Use 'make testfile' or 'make etf' to create one.)
	$(info (Grok - Valgrind): For detailed documentation, please run 'cat ${CURDIR}/makefile | more' in your terminal.)
endif
	@echo ""

# This rule ('make testfile') creates an empty test_input.txt for you, and opens it
# in your $EDITOR
testfile:
	$(info (Grok - Edit Test Input File): Creating and opening a test_input.txt file for "${PROJNAME}"...)
	@mkdir -p ${CURDIR}/tests/
	@touch ${CURDIR}/tests/test_input.txt
	${EDITOR} ${CURDIR}/tests/test_input.txt
	
# This rule ('make testargs') creates an empty test_args.txt for you, and opens it
# in your $EDITOR
testargs:
	$(info (Grok - Edit Test Arguments): Creating and opening a test_args.txt file for "${PROJNAME}"...)
	@mkdir -p ${CURDIR}/tests/
	@touch ${CURDIR}/tests/test_args.txt
	${EDITOR} ${CURDIR}/tests/test_args.txt

# This rule ('make etf') creates an empty test_input.txt for you.
etf:
	$(info (Grok - Create Test File): Creating empty test_input.txt and test_args.txt files for "${PROJNAME}"...)
	mkdir -p ${CURDIR}/tests/
	touch ${CURDIR}/tests/test_input.txt
	touch ${CURDIR}/tests/test_args.txt

# Static code analysis
# This rule will run scan-build against your project's source code, which will alert
# you to a variety of errors in your syntax or program structure, should these exist.
# You may invoke scan-build with 'make lint', or alternatively 'scan-build make <any other rule>'
lint:
ifneq (, $(shell which scan-build))
	$(info (Grok - Lint): Linting "${PROJNAME}" source file: ${PROJSRC} (using scan-build)...)
# Invoke scan-build on your project's source files
	@mkdir -p ${CURDIR}/bin/scan-build/
	scan-build gcc -Wall -g -std=c99 -c ${PROJSRC} -o ${CURDIR}/bin/scan-build/${PROJNAME}
else
	$(error (Grok - Lint): Cannot test "${PROJNAME}", as scan-build does not appear to be installed. Please install scan-build, or check your PATH. \
	For detailed documentation, please run 'cat ${CURDIR}/makefile | more' in your terminal.)
endif
	@echo ""

# Rule for (brief) cleanup
# When called with 'make clean', Grok will remove the ./bin/ directory
# This effectively scrubs any files Grok may have generated from the current
# directory
clean:
	$(info (Grok - Clean): Removing ${CURDIR}/bin/...)
# Remove ./bin/, without printing errors if the directory doesn't exist
	rm -rf ${CURDIR}/bin/
	@echo ""

help:
	$(info Grok Makefile - Version ${AB_VERSION})
	$(info (c) ${CPYEAR} Charlton Trezevant - MIT License)
	$(info For detailed documentation, please run 'cat ${CURDIR}/makefile | more' in your terminal.)
	@echo ""
