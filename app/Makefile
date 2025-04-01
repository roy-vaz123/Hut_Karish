# app/Makefile

CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -g -I../include -Iinclude -MMD
TARGET := main

SRC_DIR := src
OBJ_DIR := obj
INCLUDE_DIR := include

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Include generated dependency files
-include $(OBJS:.o=.d)
