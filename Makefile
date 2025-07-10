CMAKE_FLAGS=-DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
BUILD_DIR=build


all:
	cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS)
	cmake --build $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
