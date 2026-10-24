find_package(Qt6 REQUIRED COMPONENTS Test)

set(NN_SRC ${CMAKE_SOURCE_DIR}/src)

# Only the sources needed to exercise ScintillaNext editing operations
qt_add_executable(tst_scintillanext
    tst_scintillanext.cpp
    ${NN_SRC}/Finder.cpp
    ${NN_SRC}/QRegexSearch.cpp
    ${NN_SRC}/RangeAllocator.cpp
    ${NN_SRC}/ScintillaCommenter.cpp
    ${NN_SRC}/ScintillaNext.cpp
    ${NN_SRC}/SelectionTracker.cpp
    ${NN_SRC}/UndoAction.cpp
)

target_include_directories(tst_scintillanext PRIVATE ${NN_SRC})

target_link_libraries(tst_scintillanext
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
        lexilla
        scintilla
        uchardet
)

add_test(NAME tst_scintillanext COMMAND tst_scintillanext)
set_tests_properties(tst_scintillanext PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
