## Úloha 3: Producer-Consumer pipeline (so synchronizáciou)

# Sekvenčná verzia
add_executable(u3_seq src/main_seq.cpp)
target_include_directories(u3_seq PRIVATE include)

# Paralelná verzia (TBB)
add_executable(u3_tbb src/main_tbb.cpp)
target_include_directories(u3_tbb PRIVATE include)
target_link_libraries(u3_tbb PRIVATE TBB::tbb)
