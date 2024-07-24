add_executable(
  parallel-kskycore
    main.cpp
    graph.cpp
    graph-structural-operations.cpp
    group-skyline-concepts.cpp
    ns-functions.cpp
    listing-danisch.cpp
    listing-cousins-first.cpp
    sky-layers.cpp
    spatial.cpp
    zhang19.cpp
)

add_executable(
  listing-comparison
    comparison-listing.cpp
    graph.cpp
    graph-structural-operations.cpp
    group-skyline-concepts.cpp
    listing-cousins-first.cpp
    sky-layers.cpp
    spatial.cpp
)

set_target_properties(
  parallel-kskycore
    PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "../lib/"
      LIBRARY_OUTPUT_DIRECTORY "../lib/"
      RUNTIME_OUTPUT_DIRECTORY "../"
)

set_target_properties(
  listing-comparison
    PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "../lib/"
      LIBRARY_OUTPUT_DIRECTORY "../lib/"
      RUNTIME_OUTPUT_DIRECTORY "../"
)

target_link_libraries(
  parallel-kskycore
)

target_link_libraries(
  listing-comparison
     ${Boost_LIBRARIES}
)
