# Build everything:   make
# Run:                make run
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iinclude

all: bin/attack_demo bin/monte_carlo bin/test_pkes

bin/%: src/%.cpp include/pkes.hpp include/aes128.hpp
	@mkdir -p bin results
	$(CXX) $(CXXFLAGS) $< -o $@

bin/test_pkes: tests/test_pkes.cpp include/pkes.hpp include/aes128.hpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	./bin/test_pkes
	./bin/attack_demo
	./bin/monte_carlo

clean:
	rm -rf bin
