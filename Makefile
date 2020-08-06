.PHONY: debugbuild reldeb release clean run runreldeb runrel coverage-build coverage-run coverage

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

coverage-build:
	mkdir -p build/covBuild && \
	    cd build/covBuild && \
	    cmake -DCMAKE_BUILD_TYPE=Debug -DMINIWEB_COVERAGE_BUILD=1 -GNinja ../.. && \
	    ninja

coverage-run:
	cd build/covBuild && \
	    LLVM_PROFILE_FILE="miniweb.profraw" ./test/miniweb.t && \
	    llvm-profdata merge miniweb.profraw -o miniweb.profdata && \
	    llvm-cov report ./test/miniweb.t --instr-profile=miniweb.profdata \
		--ignore-filename-regex="vcpkg/*"

coverage: coverage-build coverage-run

clean:
	rm -rf build
