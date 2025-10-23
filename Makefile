# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./src
LDFLAGS = 

# Directories
SRC_DIR = src
OBJ_DIR = obj
APPS_DIR = $(SRC_DIR)/Apps

# Target executable
TARGET = main

# Source files
SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/waydroid.cpp \
       $(APPS_DIR)/SVT.cpp \
       $(APPS_DIR)/EON.cpp

# Object files
OBJS = $(OBJ_DIR)/main.o \
       $(OBJ_DIR)/waydroid.o \
       $(OBJ_DIR)/Apps/SVT.o \
       $(OBJ_DIR)/Apps/EON.o

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile main.cpp
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.cpp $(SRC_DIR)/waydroid.h $(SRC_DIR)/channels.h
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile waydroid.cpp
$(OBJ_DIR)/waydroid.o: $(SRC_DIR)/waydroid.cpp $(SRC_DIR)/waydroid.h $(SRC_DIR)/channels.h $(SRC_DIR)/App.h
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Apps/SVT.cpp
$(OBJ_DIR)/Apps/SVT.o: $(APPS_DIR)/SVT.cpp $(APPS_DIR)/SVT.h $(SRC_DIR)/App.h $(SRC_DIR)/channels.h
	@mkdir -p $(OBJ_DIR)/Apps
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Apps/EON.cpp
$(OBJ_DIR)/Apps/EON.o: $(APPS_DIR)/EON.cpp $(APPS_DIR)/EON.h $(SRC_DIR)/App.h $(SRC_DIR)/channels.h
	@mkdir -p $(OBJ_DIR)/Apps
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR)/*.o $(OBJ_DIR)/Apps/*.o $(TARGET)

# Rebuild everything
rebuild: clean all

# Run the program (requires sudo for keyboard unbinding)
run: $(TARGET)
	sudo ./$(TARGET)

.PHONY: all clean rebuild run
