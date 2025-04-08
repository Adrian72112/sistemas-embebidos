cmake_minimum_required(VERSION 3.10)
project(MiProyecto C)

# Habilitar compilación con estándares modernos de C
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(component_lib "components")
set(include_lib "include")
set(lib_1 "my_lib_1")
set(lib_2 "parte_2_lib")

# Agregar subdirectorios de las librerías
add_subdirectory("${component_lib}/${lib_1}")
add_subdirectory("${component_lib}/${lib_2}")

# Definir el ejecutable principal
add_executable(mi_programa src/main.c)

# Vincular las bibliotecas al ejecutable
target_link_libraries(mi_programa "${lib_1}")
target_link_libraries(mi_programa "${lib_2}")

# Agregar los directorios de encabezados de las bibliotecas
target_include_directories(mi_programa PRIVATE 
    "${component_lib}/${lib_1}/${include_lib}"
    "${component_lib}/${lib_2}/${include_lib}"
)

# Agregar el ejecutable de pruebas para parte_2_lib
add_executable(test_estudiantes 
    ${CMAKE_CURRENT_SOURCE_DIR}/${component_lib}/${lib_2}/tests/test_estudiantes.c
)
# Vincular las bibliotecas a los tests
target_link_libraries(test_estudiantes PRIVATE ${lib_1} ${lib_2})

# Activar pruebas y registrar el test en CTest
enable_testing()
add_test(NAME TestEstudiantes COMMAND test_estudiantes)