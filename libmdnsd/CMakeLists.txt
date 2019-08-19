add_library (mdnsd STATIC
    1035.c
    log.c
    mdnsd.c
    sdtxt.c
    xht.c
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -std=gnu99 -W -Wall -Wextra -Wno-unused-parameter \
    -Wno-unused-function"
    )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE")

