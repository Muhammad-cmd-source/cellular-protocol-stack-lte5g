CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -g

all: bin/stack_sim

bin/stack_sim: src/phy/phy_layer.o src/mac/mac_layer.o src/rlc/rlc_layer.o src/pdcp/pdcp_layer.o src/rrc/rrc_layer.o src/nas/nas_layer.o src/stack_sim.o
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/stack_sim $^

src/phy/phy_layer.o: src/phy/phy_layer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/mac/mac_layer.o: src/mac/mac_layer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/rlc/rlc_layer.o: src/rlc/rlc_layer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/pdcp/pdcp_layer.o: src/pdcp/pdcp_layer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/rrc/rrc_layer.o: src/rrc/rrc_layer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/nas/nas_layer.o: src/nas/nas_layer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/stack_sim.o: src/stack_sim.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test: bin/test_runner
	./bin/test_runner

bin/test_runner: src/phy/phy_layer.o src/mac/mac_layer.o src/rlc/rlc_layer.o src/pdcp/pdcp_layer.o src/rrc/rrc_layer.o src/nas/nas_layer.o tests/test_all.o
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/test_runner $^

tests/test_all.o: tests/test_all.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o src/phy/*.o src/mac/*.o src/rlc/*.o src/pdcp/*.o src/rrc/*.o src/nas/*.o tests/*.o bin/stack_sim bin/test_runner
