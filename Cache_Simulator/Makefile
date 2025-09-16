CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.cc)
OBJDIR = obj
BINDIR = bin
OBJ = $(SRC:$(SRC_DIR)/%.cc=$(OBJDIR)/%.o)
DEPS = $(wildcard $(SRC_DIR)/*.h)
TARGET = $(BINDIR)/cache_sim

all: $(OBJDIR) $(BINDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRC_DIR)/%.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
