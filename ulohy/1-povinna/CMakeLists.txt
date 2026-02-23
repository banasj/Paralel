## Úloha 1: Maticové násobenie

# Sekvenčná verzia (čisté C++)
add_executable(u1_seq src/main_seq.cpp)
target_include_directories(u1_seq PRIVATE include)

# Paralelná verzia (TBB)
add_executable(u1_tbb src/main_tbb.cpp)
target_include_directories(u1_tbb PRIVATE include)
target_link_libraries(u1_tbb PRIVATE TBB::tbb)
