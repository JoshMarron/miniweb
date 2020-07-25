.PHONY: debugbuild reldeb release clean run runreldeb runrel

.DEFAULT_GOAL := debug

debug:
	mkdir -p build/debugBuild && \
	    cd build/debugBuild && \
	    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -GNinja ../.. && \
	    ninja

reldeb:
	mkdir -p build/relDebBuild && \
	    cd build/relDebBuild && \
	    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja ../.. && \
	    ninja

release:
	mkdir -p build/releaseBuild && \
	    cd build/releaseBuild && \
	    cmake -DCMAKE_BUILD_TYPE=Release -GNinja ../.. && \
	    ninja

clean:
	rm -rf build
