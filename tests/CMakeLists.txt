find_package(Qt6 REQUIRED COMPONENTS Test)

set(NN_SRC ${CMAKE_SOURCE_DIR}/src)

# The Find dialog needs the whole application (main window, docks, settings...),
# so build every application source except main.cpp, which the test replaces.
# Generated sources (e.g. translations) live in the build directory and are left out.
get_target_property(NN_APP_SOURCES NotepadNext SOURCES)

set(NN_TEST_APP_SOURCES)
foreach(source IN LISTS NN_APP_SOURCES)
    if(IS_ABSOLUTE "${source}")
        continue()
    endif()

    if(NOT source MATCHES "(^|/)main\\.cpp$")
        list(APPEND NN_TEST_APP_SOURCES ${NN_SRC}/${source})
    endif()
endforeach()

qt_add_executable(tst_findinfiles
    tst_findinfiles.cpp
    ${NN_TEST_APP_SOURCES}
)

target_include_directories(tst_findinfiles
    PRIVATE
        ${NN_SRC}/decorators
        ${NN_SRC}/dialogs
        ${NN_SRC}/docks
        ${NN_SRC}/widgets
        ${NN_SRC}
)

target_link_libraries(tst_findinfiles
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::PrintSupport
        Qt6::Test
        qtadvanceddocking-qt6
        QSimpleUpdater
        SingleApplication::SingleApplication
        editorconfig-core-qt
        lua
        lexilla
        scintilla
        uchardet
)

target_compile_definitions(tst_findinfiles
    PRIVATE
        ADS_STATIC
        QSIMPLEUPDATER_STATIC
        QAPPLICATION_CLASS=QApplication
        APP_VERSION="${PROJECT_VERSION}"
        APP_COPYRIGHT="${APP_COPYRIGHT}"
        APP_DISTRIBUTION="${APP_DISTRIBUTION}"
)

add_test(NAME tst_findinfiles COMMAND tst_findinfiles)
set_tests_properties(tst_findinfiles PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
