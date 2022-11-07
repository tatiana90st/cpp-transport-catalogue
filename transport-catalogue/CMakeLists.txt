cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto graph.proto transport_router.proto)

set(TRCAT_FILES main.cpp transport_router.h transport_router.cpp json_reader.h json_reader.cpp
request_handler.h request_handler.cpp json_builder.h json_builder.cpp map_renderer.h map_renderer.cpp
domain.h domain.cpp transport_catalogue.h transport_catalogue.cpp graph.h ranges.h router.h
svg.h svg.cpp json.h json.cpp geo.h geo.cpp serialization.h serialization.cpp transport_catalogue.proto map_renderer.proto graph.proto transport_router.proto)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRCAT_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)