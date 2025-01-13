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
# Specify the PostgreSQL root (optional)
set(PostgreSQL_ROOT "/usr")

# Find PostgreSQL
# Find PostgreSQL and explicitly link the static libpq.a
find_package(PostgreSQL REQUIRED)

if(PostgreSQL_FOUND)
    message(STATUS "PostgreSQL Include Directory: ${PostgreSQL_INCLUDE_DIRS}")
    message(STATUS "PostgreSQL Library Path: ${PostgreSQL_LIBRARY_DIR}")

    # Include PostgreSQL headers
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    
    # Locate the static library explicitly
    find_library(PostgreSQL_STATIC_LIB NAMES libpq.a PATHS ${PostgreSQL_LIBRARY_DIR})
    if(PostgreSQL_STATIC_LIB)
        message(STATUS "PostgreSQL Static Library: ${PostgreSQL_STATIC_LIB}")
        
        # Link the static library and its dependencies
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${PostgreSQL_STATIC_LIB} -lpthread -lz -lssl -lcrypto -ldl)
    else()
        message(FATAL_ERROR "Static PostgreSQL library (libpq.a) not found!")
    endif()
else()
    message(FATAL_ERROR "PostgreSQL not found!")
endif()


# Locate and include QuickFIX static library
set(QUICKFIX_LIBRARY /usr/local/lib/libquickfix.a)
set(QUICKFIX_INCLUDE_DIR /usr/local/include/quickfix)

if(EXISTS ${QUICKFIX_LIBRARY} AND EXISTS ${QUICKFIX_INCLUDE_DIR})
    message(STATUS "QuickFix Include Directory: ${QUICKFIX_INCLUDE_DIR}")
    message(STATUS "QuickFix Static Library: ${QUICKFIX_LIBRARY}")
    
    # Include QuickFIX headers
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${QUICKFIX_INCLUDE_DIR})

    # Link QuickFIX static library
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${QUICKFIX_LIBRARY})
    
else()
    message(FATAL_ERROR "QuickFix static library or include directory not found!")
endif()
