# Variables
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = -lpackets
OBJ_DIR = obj
BIN = file_transfer
SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

# Rules
all: directories $(BIN)

directories: $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $(BIN) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN)
