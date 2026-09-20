cmake_minimum_required( VERSION 3.28 )
project( psi_pimpl LANGUAGES CXX )

add_library( psi_pimpl INTERFACE )
add_library( Psi::Pimpl ALIAS psi_pimpl )
target_include_directories( psi_pimpl INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include" )
target_compile_features( psi_pimpl INTERFACE cxx_std_20 )

include( module.cmake )

if ( PROJECT_IS_TOP_LEVEL )
    enable_testing()
    add_subdirectory( test )
endif()
