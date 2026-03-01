## Úloha 2: K-means klasterizácia

# Sekvenčná verzia
add_executable(u2_seq src/main_seq.cpp)
target_include_directories(u2_seq PRIVATE include)

# Paralelná verzia (TBB)
add_executable(u2_tbb src/main_tbb.cpp)
target_include_directories(u2_tbb PRIVATE include)
target_link_libraries(u2_tbb PRIVATE TBB::tbb)
