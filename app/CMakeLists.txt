
cmake_minimum_required(VERSION 3.4.1)
set(libs_DIR src/main/jnilib/${ANDROID_ABI})
link_directories(${libs_DIR})

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp )

target_link_libraries( # Specifies the target library.
                       native-lib
                       test2
                       xhook
                        )

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )


target_link_libraries( # Specifies the target library.
                       native-lib
                       ${log-lib}
                        )