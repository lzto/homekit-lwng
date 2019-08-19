add_library (http-parser STATIC
    http_parser.c
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -std=gnu99 -W -Wall -Wextra -Wno-unused-parameter \
    -Wno-unused-function -DHTTP_PARSER_STRICT=0"
    )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE")

