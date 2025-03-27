add_library(my_lib_2 my_lib_2.c)

# Especificar el directorio de los headers para esta librería
target_include_directories(my_lib_2 PUBLIC include)
