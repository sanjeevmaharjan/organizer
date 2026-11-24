cmake_minimum_required(VERSION 3.21)

project(FolderOrganizer
    VERSION 1.0.0
    DESCRIPTION "A desktop application for organizing files by type"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

add_library(organizer_core
    src/core/Organizer.cpp
    src/core/Organizer.h
)
target_include_directories(organizer_core PUBLIC src)
target_compile_features(organizer_core PUBLIC cxx_std_17)

add_executable(folder-organizer
    src/app/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)
set_target_properties(folder-organizer PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Folder Organizer"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.folderorganizer.desktop"
)
target_link_libraries(folder-organizer PRIVATE organizer_core Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Test)

    add_executable(organizer_tests
        tests/core/OrganizerTests.cpp
    )
    target_link_libraries(organizer_tests PRIVATE organizer_core Qt6::Test)
    add_test(NAME organizer_tests COMMAND organizer_tests)
endif()
