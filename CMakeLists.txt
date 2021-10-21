# CMakeLists.txt for DistributedME

cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 11)

project(DistributedME)

set(This distributed-me)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(HEADERS
    include/distributed-me/config.h
    include/distributed-me/packet.h
    include/distributed-me/lamportclock.h
    include/distributed-me/messagequeue.h
    include/distributed-me/logger.h
    include/distributed-me/node/node.h
    include/distributed-me/node/server.h
    include/distributed-me/node/client.h
    include/distributed-me/CriticalSectionAlg/CriticalSectionAlgStrategy.h
    include/distributed-me/CriticalSectionAlg/RACriticalSectionAlgStrategy.h
    include/distributed-me/CriticalSectionAlg/MKWCriticalSectionAlgStrategy.h
)

set(SOURCES
    src/utils.cpp
    src/packet.cpp
    src/lamportclock.cpp
    src/messagequeue.cpp
    src/logger.cpp
    src/node/node.cpp
    src/node/server.cpp
    src/node/client.cpp
    src/CriticalSectionAlg/CriticalSectionAlgStrategy.cpp
    src/CriticalSectionAlg/RACriticalSectionAlgStrategy.cpp
    src/CriticalSectionAlg/MKWCriticalSectionAlgStrategy.cpp
)

add_library(${This} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${This} PUBLIC include)
target_link_libraries(${This} PRIVATE Threads::Threads)

add_executable(prog src/main.cpp ${SOURCES} ${HEADERS})
target_include_directories(prog PUBLIC include)
target_link_libraries(prog PRIVATE Threads::Threads)

add_executable(run_server src/run_server.cpp ${SOURCES} ${HEADERS})
target_include_directories(run_server PUBLIC include)
target_link_libraries(run_server PRIVATE Threads::Threads)

add_executable(run_client src/run_client.cpp ${SOURCES} ${HEADERS})
target_include_directories(run_client PUBLIC include)
target_link_libraries(run_client PRIVATE Threads::Threads)

#add_subdirectory(third_party)
#add_subdirectory(test)