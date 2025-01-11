# ubuntu 24.04 lts
# build test success 10:05 PM 1/1/2025
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential cmake libssl-dev
sudo apt install -y build-essential cmake libssl-dev libpq-dev postgresql-16 postgresql-contrib

mkdir build
cd build


cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQUICKFIX_SHARED_LIBS=OFF -DQUICKFIX_TESTS=OFF -DQUICKFIX_EXAMPLES=OFF -DCMAKE_CXX_FLAGS="-O1" -DHAVE_SSL=ON -DHAVE_POSTGRESQL=ON -DCMAKE_INSTALL_PREFIX="/usr/local/" ..
make -j4
make install


#link with quickfix
# Set QuickFix root directory and find the OpenSSL package

set(QUICKFIX_LIBRARIES /usr/local/lib/libquickfix.a)
set(QUICKFIX_INCLUDE_DIR /usr/local/include/quickfix)

if(QUICKFIX_LIBRARIES AND QUICKFIX_INCLUDE_DIR)
    message(STATUS "QuickFix Include Directory: ${QUICKFIX_INCLUDE_DIR}")
    message(STATUS "QuickFix Libraries: ${QUICKFIX_LIBRARIES}")
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${QUICKFIX_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${QUICKFIX_LIBRARIES})
else()
    message(FATAL_ERROR "QuickFix not found!")
endif()