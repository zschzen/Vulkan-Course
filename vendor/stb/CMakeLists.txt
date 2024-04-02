# stb
set(STB_HEADER_FILES
  ./stb_image.h
)

add_library(stb STATIC ${STB_HEADER_FILES})
target_include_directories(stb PUBLIC .)

set_target_properties(stb PROPERTIES LINKER_LANGUAGE C)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${STB_HEADER_FILES})

