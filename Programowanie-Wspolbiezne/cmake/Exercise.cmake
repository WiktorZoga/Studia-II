include_guard(GLOBAL)

find_package(Threads REQUIRED)

function(configure_concurrency_target target standard)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD ${standard}
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)
    target_link_libraries(${target} PRIVATE Threads::Threads)

    if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        if(USE_TSAN)
            target_compile_options(${target} PRIVATE -fsanitize=thread -g)
            target_link_options(${target} PRIVATE -fsanitize=thread)
        else()
            target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
            target_link_options(${target} PRIVATE -fsanitize=address)
        endif()
    endif()
endfunction()

function(add_concurrency_exercise target source standard)
    add_executable(${target} ${source})
    configure_concurrency_target(${target} ${standard})
endfunction()

function(add_concurrency_component target source standard)
    add_library(${target} OBJECT ${source})
    configure_concurrency_target(${target} ${standard})
endfunction()
