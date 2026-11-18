CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
SRC      = main.cpp Inventory.cpp Logistics.cpp

erlhub: $(SRC) Exceptions.h Inventory.h Logistics.h PriorityDispatchQueue.h
	$(CXX) $(CXXFLAGS) $(SRC) -o erlhub

run: erlhub
	./erlhub

clean:
	rm -f erlhub *.bin *.tmp
