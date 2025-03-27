cmake_minimum_required(VERSION 3.10)
project(MiProyecto C)

# Habilitar compilación con estándares modernos de C
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(component_lib "components")
set(include_lib "include")
set(lib_1 "my_lib_1")
set(lib_2 "my_lib_2")

# Agregar subdirectorios de las librerías
add_subdirectory("${component_lib}/${lib_1}")
add_subdirectory("${component_lib}/${lib_2}")

# Definir el ejecutable principal
add_executable(mi_programa src/main.c)

# Vincular las bibliotecas al ejecutable
target_link_libraries(mi_programa "${lib_1}" "${lib_2}")

# Agregar los directorios de encabezados de las bibliotecas
target_include_directories(mi_programa PRIVATE 
    "${component_lib}/${lib_1}/${include_lib}"
    "${component_lib}/${lib_2}/${include_lib}"
)
