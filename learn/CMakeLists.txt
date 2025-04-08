cmake_minimum_required(VERSION 3.30)
project(LEARN)

add_executable(BASE_LEARN base.cc)
add_executable(MUTEX_LEARN mutex.cc
        hierarchical_mutex.hpp
        threadsafe_stack.hpp
        threadsafe_queue.hpp
        threadsafe_raw_queue.hpp
        threadsafe_lookup_table.hpp
        threadsafe_list.hpp)


