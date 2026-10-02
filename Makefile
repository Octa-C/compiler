CXX      = g++
WARNINGS = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
           -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual \
           -Wmissing-declarations -Wzero-as-null-pointer-constant -Wextra-semi
CXXFLAGS = -std=c++17 $(WARNINGS) -O2
CPPFLAGS = -Ilexer/include -Iutils/include -MMD -MP

ifeq ($(OS),Windows_NT)
  EXE = .exe
endif

PYTHON ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
TARGET = build/octacc$(EXE)
SRCS   = main.cpp $(wildcard lexer/src/*.cpp) $(wildcard utils/src/*.cpp)
OBJS   = $(SRCS:%.cpp=build/%.o)
DEPS   = $(OBJS:.o=.d)
FORMAT = main.cpp $(wildcard lexer/*/*.hpp lexer/*/*.cpp utils/*/*.hpp utils/*/*.cpp)

.PHONY: all test format clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# make test ARGS=-v lists every error in each failing case
test: $(TARGET)
	$(PYTHON) -m pytest tests $(ARGS)

format:
	clang-format -i $(FORMAT)

clean:
	rm -rf build

-include $(DEPS)
