ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
EXE := .exe
LDLIBS := -lpsapi
RM := del /Q
RUN_BENCH := bench.exe
RUN_TESTS := run_tests.exe
RUN_QUICK := quick.exe
else
EXE :=
LDLIBS :=
RM := rm -f
RUN_BENCH := ./bench
RUN_TESTS := ./run_tests
RUN_QUICK := ./quick
endif

CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -static-libstdc++ -static-libgcc

.PHONY: all run test quick clean

all: bench$(EXE) run_tests$(EXE)

bench$(EXE): main.cpp search_algos.h benchmark.h os_metrics.h memory_tracker.h text_generator.h input_utils.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp $(LDLIBS)

run_tests$(EXE): tests.cpp search_algos.h
	$(CXX) $(CXXFLAGS) -o $@ tests.cpp

run: bench$(EXE)
	$(RUN_BENCH)

test: run_tests$(EXE)
	$(RUN_TESTS)

quick$(EXE): test_simple.cpp search_algos.h
	$(CXX) $(CXXFLAGS) -o $@ test_simple.cpp

quick: quick$(EXE)
	$(RUN_QUICK)

clean:
	-$(RM) bench$(EXE)
	-$(RM) run_tests$(EXE)
	-$(RM) quick$(EXE)
