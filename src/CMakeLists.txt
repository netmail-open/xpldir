add_library(xpldir SHARED
  dir.c
)


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_CFLAGS} ${PTHREAD_CFLAGS} ${OPENSSL_CFLAGS}")

find_library(MEMMGR_LIBRARY
  NAMES libmemmgr memmgr
)

target_link_libraries(xpldir
	memmgr${BIT}
)

install(TARGETS xpldir DESTINATION /usr/lib)
