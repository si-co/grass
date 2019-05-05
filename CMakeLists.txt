#
#   CMakeLists for CLion integration.
#   The original Makefile was not modified and should be used for the final results
#

cmake_minimum_required (VERSION 3.12)
project (grass)

set(CMAKE_BUILD_TYPE DEBUG)
set(EXECUTABLE_OUTPUT_PATH "../bin")

    #
    # originally : "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11 -I $(INCLUDES)/ -m32")
    # But this causes CLion to include $HOME in header search path
    #
set(CMAKE_C_FLAGS           "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11")
set(CMAKE_C_FLAGS_DEBUG     "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11")
set(CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11")
set(CMAKE_CXX_FLAGS         "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11")


include_directories (include)

set (SOURCES_SHARED
        src/grass.cpp
        lib/sockutils.cpp
        lib/parser.cpp
        lib/commands.cpp
        lib/utils.cpp)
set (SOURCES_CLIENT
        src/client.cpp)
set (SOURCES_SERVER
        src/server.cpp)

add_executable (client ${SOURCES_SHARED} ${SOURCES_CLIENT})
add_executable (server ${SOURCES_SHARED} ${SOURCES_SERVER})

install( TARGETS server client
        RUNTIME DESTINATION bin/
)