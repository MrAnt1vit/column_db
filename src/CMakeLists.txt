add_library(columnar_core
    core/schema.cpp
    core/types.hpp
)
target_include_directories(columnar_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(columnar_utils
    utils/file_utils.cpp
)
target_include_directories(columnar_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(columnar_csv
    csv/parser.cpp
    csv/reader.cpp
    csv/writer.cpp
)
target_link_libraries(columnar_csv PUBLIC columnar_core columnar_utils)


add_library(columnar_format
    columnar/row_group.cpp
    columnar/writer.cpp
    columnar/reader.cpp
)
target_link_libraries(columnar_format PUBLIC columnar_core columnar_utils)


add_library(columnar_tools
    tools/converter.cpp
    tools/csv_compare.cpp
)
target_link_libraries(columnar_tools PUBLIC columnar_csv columnar_format columnar_core)

add_library(columnar_exec
    exec/scan.cpp
    exec/aggregate.cpp
    exec/filter.cpp
    exec/result_writer.cpp
    exec/count_distinct.cpp
    exec/hash_aggregate.cpp
    exec/sort.cpp
    exec/limit.cpp 
)
target_link_libraries(columnar_exec PUBLIC columnar_format columnar_core)

add_library(columnar_lib INTERFACE)
target_link_libraries(columnar_lib INTERFACE columnar_tools)
target_include_directories(columnar_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(columnar_query
    query/runner.cpp
)
target_link_libraries(columnar_query PUBLIC columnar_exec columnar_format columnar_core)
target_link_libraries(columnar_lib INTERFACE columnar_query)



add_executable(columnar_db main.cpp)
target_link_libraries(columnar_db PRIVATE columnar_lib)