# SPDX-FileCopyrightText: Copyright 2026 Lichii Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(hortensia-updater main.cpp ../frontend_common/update_installer.cpp)
target_include_directories(hortensia-updater PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_features(hortensia-updater PRIVATE cxx_std_20)
target_link_libraries(hortensia-updater PRIVATE user32 shell32)
set_target_properties(hortensia-updater PROPERTIES WIN32_EXECUTABLE ON)

# The helper runs from a temporary folder, away from the DLLs next to hortensia.exe.
if (MSVC)
    set_property(TARGET hortensia-updater PROPERTY MSVC_RUNTIME_LIBRARY
                 "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_link_options(hortensia-updater PRIVATE -static)
endif()
