CXX       = g++

WARNINGS  = -Wall
WARNINGS += -Wextra
WARNINGS += -Wpedantic
WARNINGS += -Wshadow
WARNINGS += -Wconversion
WARNINGS += -Wsign-conversion
WARNINGS += -Wold-style-cast
WARNINGS += -Wnon-virtual-dtor
WARNINGS += -Woverloaded-virtual
WARNINGS += -Wmissing-declarations
WARNINGS += -Wzero-as-null-pointer-constant
WARNINGS += -Wextra-semi

CXXFLAGS  = -std=c++17
CXXFLAGS += $(WARNINGS)
CXXFLAGS += -O2

CPPFLAGS  = -Ilexer/include
CPPFLAGS += -Iparser/include
CPPFLAGS += -Iutils/include
CPPFLAGS += -MMD
CPPFLAGS += -MP

ifeq ($(OS),Windows_NT)
  EXE = .exe
endif

PYTHON ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
TARGET  = build/octacc$(EXE)

SRCS    = main.cpp
SRCS   += $(wildcard lexer/src/*.cpp)
SRCS   += $(wildcard parser/src/*.cpp)
SRCS   += $(wildcard utils/src/*.cpp)

OBJS    = $(SRCS:%.cpp=build/%.o)
DEPS    = $(OBJS:.o=.d)

FORMAT  = main.cpp
FORMAT += $(wildcard lexer/*/*.hpp lexer/*/*.cpp)
FORMAT += $(wildcard parser/*/*.hpp parser/*/*.cpp)
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
