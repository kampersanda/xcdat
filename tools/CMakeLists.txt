set(XCDAT_FILES
    "xcdat_build"
    "xcdat_lookup"
    "xcdat_decode"
    "xcdat_prefix_search"
    "xcdat_predictive_search"
    "xcdat_enumerate"
)

foreach(XCDAT_FILE ${XCDAT_FILES})
    add_executable(${XCDAT_FILE} ${XCDAT_FILE}.cpp)
    install(TARGETS ${XCDAT_FILE} RUNTIME DESTINATION bin)
endforeach(XCDAT_FILE)
