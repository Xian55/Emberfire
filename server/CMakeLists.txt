# emberfire_server — FUTURE from-scratch game server. Placeholder so the tree builds end to end.
# Intent + the reverse-engineered server spec: see server/README.md and docs/SERVER_BACKEND.md.
add_executable(emberfire_server src/main.cpp)
target_link_libraries(emberfire_server PRIVATE emberfire::shared)
target_compile_features(emberfire_server PRIVATE cxx_std_20)   # new code can use C++20
