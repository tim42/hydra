

# build LodePNG (create a static library)
add_library(lodePNG STATIC
    ./lodePNG/lodepng.cpp
    ./lodePNG/lodepng.h
)

target_include_directories(lodePNG PUBLIC ./lodePNG/)
