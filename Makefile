
CXX       = g++

CXXFLAGS  = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
CXXFLAGS += -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS += -Wmissing-declarations -Wzero-as-null-pointer-constant -Wextra-semi

CPPFLAGS  = -Ilexer/include -Iparser/include -Iutils/include -MMD -MP

ifeq ($(OS),Windows_NT)
  EXE = .exe
endif

PYTHON ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
TARGET  = build/octacc$(EXE)

SRCS    = main.cpp $(wildcard lexer/src/*.cpp) $(wildcard parser/src/*.cpp)
SRCS   += $(wildcard utils/src/*.cpp)

OBJS    = $(SRCS:%.cpp=build/%.o)
DEPS    = $(OBJS:.o=.d)

FORMAT  = main.cpp $(wildcard lexer/*/*.hpp lexer/*/*.cpp parser/*/*.hpp parser/*/*.cpp)
FORMAT += $(wildcard utils/*/*.hpp utils/*/*.cpp)

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
