# This is a Makefile to simplify compiling the Mili tests by the testing scripts
# It assumes various environment variables will be set before calling 'make'
#
ifeq ($(COMPILER_FAM), gnu)
COMPILER=gcc
else
COMPILER=icc
endif

INCLUDE_PATH = $(CODE_PATH)/include
LIBRARY_PATH = $(CODE_PATH)/lib

all: $(MDGTEST_MILI_TEST)

$(MDGTEST_MILI_TEST): $(MDGTEST_MILI_TEST).o
	$(COMPILER) $(MDGTEST_MILI_TEST).o -o $(MDGTEST_MILI_TEST) -g -I $(INCLUDE_PATH) -L $(LIBRARY_PATH) -l mili -l taurus -lm

$(MDGTEST_MILI_TEST).o: $(MDGTEST_MILI_TEST).c
	$(COMPILER) -g -I $(INCLUDE_PATH) -c $(MDGTEST_MILI_TEST).c

clean:
	rm -rf $(MDGTEST_MILI_TEST) $(MDGTEST_MILI_TEST).o