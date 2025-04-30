# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -g # Use C++17, enable warnings, link pthreads, add debug symbols
LDFLAGS = -pthread # Ensure linker knows about pthreads too

# Executable names
SERVER_EXEC = server
CLIENT_EXEC = client

# Source files
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

# Object files (optional, but good practice for larger projects)
# SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
# CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Rule to build the server
$(SERVER_EXEC): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $(SERVER_EXEC) $(LDFLAGS)

# Rule to build the client
$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $(CLIENT_EXEC) $(LDFLAGS)

# Rule for object files (if used)
# %.o: %.cpp
#	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC) *.o # Remove executables and any object files

# Phony targets (targets that don't represent files)
.PHONY: all clean