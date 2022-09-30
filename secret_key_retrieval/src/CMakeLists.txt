cmake_minimum_required(VERSION 3.10)

project(GaussianElimination)

find_package (Eigen3 3.3 REQUIRED NO_MODULE)
#find_package(ortools CONFIG REQUIRED)
include_directories(${ARMADILLO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES("/usr/local/include/eigen3/")
# Typically you don't care so much for a third party library's tests to be
# run from your own project's code.
set(JSON_BuildTests OFF CACHE INTERNAL "")

# Don't use include(nlohmann_json/CMakeLists.txt) since that carries with it
# unintended consequences that will break the build.  It's generally
# discouraged (although not necessarily well documented as such) to use
# include(...) for pulling in other CMake projects anyways.
add_subdirectory(nlohmann_json)

file(GLOB_RECURSE CFILES "${CMAKE_SOURCE_DIR}/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX )


#ORtools
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${ORTOOLS_ROOT})
find_package(ortools REQUIRED)

include_directories(
    ${ORTOOLS_INCLUDE_DIRS}
)
add_definitions(-DUSE_GLOP)

set(DILITHIUM_SRCS ref/sign.c ref/packing.c ref/polyvec.c ref/poly.c ref/ntt.c ref/reduce.c ref/rounding.c ref/symmetric-shake.c ref/randombytes.c ref/global_variables.cpp)
set(GaussianElimination_SRCS main.cpp integer_lwe.cpp ilp_solver.cpp side_channel_attack.cpp  )
set(GenerateData_SRCS generate_data.cpp integer_lwe.cpp ilp_solver.cpp side_channel_attack.cpp data_generator.cpp)
set(ExecuteAttack_SRCS  execute_attack.cpp  integer_lwe.cpp ilp_solver.cpp side_channel_attack.cpp  data_generator.cpp)
set(FIPS202_SRCS ref/fips202.c)
add_library(fips202_ref ${FIPS202_SRCS})
add_library(dilithium_lib ${DILITHIUM_SRCS} )
add_executable(main ${GaussianElimination_SRCS})
add_executable(generate_data ${GenerateData_SRCS})
add_executable(execute_attack ${ExecuteAttack_SRCS})
target_compile_definitions(main PUBLIC DILITHIUM_MODE=2 DILITHIUM_PARALLEL=1)

target_compile_definitions(generate_data PUBLIC DILITHIUM_MODE=2 DILITHIUM_PARALLEL=1)
target_compile_definitions(dilithium_lib PUBLIC DILITHIUM_MODE=2 DILITHIUM_PARALLEL=1)
target_compile_definitions(execute_attack PUBLIC DILITHIUM_MODE=2 DILITHIUM_PARALLEL=1)


target_link_libraries(main PUBLIC dilithium_lib ntl m gmp pthread Eigen3::Eigen ${ARMADILLO_LIBRARIES} ${ORTOOLS_LIBRARIES})
target_link_libraries(dilithium_lib INTERFACE fips202_ref)
target_link_libraries(generate_data PUBLIC dilithium_lib ntl m gmp pthread ${ARMADILLO_LIBRARIES})
target_link_libraries(generate_data PRIVATE nlohmann_json::nlohmann_json ${ARMADILLO_LIBRARIES} ${ORTOOLS_LIBRARIES})
target_link_libraries(execute_attack PUBLIC dilithium_lib ntl m gmp pthread ${ARMADILLO_LIBRARIES} )
target_link_libraries(execute_attack PRIVATE nlohmann_json::nlohmann_json ${ARMADILLO_LIBRARIES})
target_link_libraries(execute_attack PUBLIC ${ORTOOLS_LIBRARIES})
