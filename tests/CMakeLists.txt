find_package(Qt6 REQUIRED COMPONENTS Test)

set(NN_SRC ${CMAKE_SOURCE_DIR}/src)

qt_add_executable(tst_indentationdetector
    tst_indentationdetector.cpp
    ${NN_SRC}/IndentationDetector.cpp
)

target_include_directories(tst_indentationdetector PRIVATE ${NN_SRC})

target_link_libraries(tst_indentationdetector
    PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME tst_indentationdetector COMMAND tst_indentationdetector)
