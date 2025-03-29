add_library(my_lib_1 my_lib_1.c)

# Especificar el directorio de los headers para esta librería
target_include_directories(my_lib_1 PUBLIC include)
