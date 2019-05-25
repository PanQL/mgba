# this is required
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /opt/aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc)
SET(CMAKE_CXX_COMPILER  /opt/aarch64-linux-musl-cross/bin/aarch64-linux-musl-g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /opt/aarch64-linux-musl-cross /opt/aarch64-linux-musl-cross/aarch64-linux-musl)

# search for programs in the build host directories (not necessary)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# configure Boost and Qt
#SET(QT_QMAKE_EXECUTABLE /opt/qt-embedded/qmake)
#SET(BOOST_ROOT /opt/boost_arm)
