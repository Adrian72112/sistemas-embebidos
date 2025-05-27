# CMake generated Testfile for 
# Source directory: C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1
# Build directory: C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(TestEstudiantes "C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/build/test_estudiantes.exe")
set_tests_properties(TestEstudiantes PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/CMakeLists.txt;31;add_test;C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/CMakeLists.txt;0;")
add_test(TestLib1 "C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/build/test_my_lib_1.exe")
set_tests_properties(TestLib1 PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/CMakeLists.txt;32;add_test;C:/Users/pierina borsieri/OneDrive/Documentos/GitHub/sistemas-embebidos/lab_1/CMakeLists.txt;0;")
subdirs("components/my_lib_1")
subdirs("components/parte_2_lib")
