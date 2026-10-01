CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Ilexer/include

ifeq ($(OS),Windows_NT)
  EXE = .exe
endif

PYTHON ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
TARGET = build/lexer$(EXE)
SRCS   = lexer/main.cpp $(wildcard lexer/src/*.cpp)
OBJS   = $(patsubst %.cpp,build/%.o,$(notdir $(SRCS)))
HDRS   = $(wildcard lexer/include/*.hpp)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@

build/main.o: lexer/main.cpp $(HDRS) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: lexer/src/%.cpp $(HDRS) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

# make test ARGS=-v lists every error in each failing case
test: $(TARGET)
	$(PYTHON) -m pytest tests/lexer $(ARGS)

clean:
	rm -rf build
