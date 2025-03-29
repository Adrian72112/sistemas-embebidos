import os
import re
import subprocess
import sys

# Configuración
BUILD_DIR = "build"
CMAKELISTS_FILE = "CMakeLists.txt"


def get_executable_name():
    """Obtiene el nombre del ejecutable desde CMakeLists.txt"""
    try:
        with open(CMAKELISTS_FILE, "r", encoding="utf-8") as file:
            content = file.read()
        match = re.search(r"add_executable\((\S+)", content)
        if match:
            return match.group(1)
        else:
            print("❌ Error: No se encontró 'add_executable()' en CMakeLists.txt.")
            sys.exit(1)
    except FileNotFoundError:
        print(f"❌ Error: No se encontró {CMAKELISTS_FILE}.")
        sys.exit(1)


def compile_project():
    """Compila el proyecto con CMake y mingw32-make"""
    if not os.path.exists(BUILD_DIR):
        os.mkdir(BUILD_DIR)

    os.chdir(BUILD_DIR)
    print("🔧 Ejecutando CMake...")
    subprocess.run(["cmake", "..", "-G", "MinGW Makefiles"], check=True)

    print("⚙️ Compilando con mingw32-make...")
    subprocess.run(["mingw32-make"], check=True)
    os.chdir("..")
    print("✅ Compilación completada.")


def clean_build():
    """Elimina el directorio build y lo recrea vacío"""
    if os.path.exists(BUILD_DIR):
        print("🧹 Limpiando el directorio build/...")
        subprocess.run(["rmdir", "/s", "/q", BUILD_DIR], shell=True)
    os.mkdir(BUILD_DIR)
    print("✅ Directorio limpio.")


def run_executable():
    """Ejecuta el programa compilado"""
    executable = get_executable_name()
    exe_path = os.path.join(BUILD_DIR, f"{executable}.exe")

    if os.path.exists(exe_path):
        print(f"🚀 Ejecutando {executable}...")
        subprocess.run([exe_path], check=True)
    else:
        print(f"❌ Error: No se encontró {exe_path}. Compila primero con 'python build.py compile'.")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("❌ Uso: python build.py {compile|clean|run}")
        sys.exit(1)

    command = sys.argv[1]

    if command == "compile":
        executable_name = get_executable_name()
        print(f"🏗️ Nombre del ejecutable detectado: {executable_name}")
        compile_project()
    elif command == "clean":
        clean_build()
    elif command == "run":
        run_executable()
    else:
        print("❌ Comando no reconocido. Uso: python build.py {compile|clean|run}")
        sys.exit(1)
