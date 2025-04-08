# SISTEMAS EMBEBIDOS

**Integrantes:**  
- Sofia Nicoletti  
- Pierina Borsieri  
- Agustina Bacigalupe  
- Adrián Tesore

---

## Descripción General

En esta sección ("Parte 2") se implementa una lista enlazada para almacenar y manipular información de estudiantes. Se incluye además un menú interactivo que permite:
- **Ingresar un estudiante**: Registrar un nuevo estudiante en la lista.
- **Eliminar un estudiante**: Borrar un estudiante de la lista, mediante el número de cédula (CI).
- **Mostrar estudiantes**: Visualizar los datos de los estudiantes registrados.

Para garantizar la calidad del código, se han creado tests unitarios que validan el funcionamiento de los métodos implementados en **parte_2.h**.

---

## Uso del Programa

### Compilar y Ejecutar

## 🛠️ Compilación y Ejecución

Este proyecto utiliza **CMake** como sistema de compilación. Para compilar y ejecutar el programa, puedes utilizar el script `build.cmd` (en **Windows**) con los siguientes comandos:

---

### 🔧 Compilar el Proyecto

```bat
.\build.cmd compile
```

Este comando:
- Crea el directorio de compilación (`build`)
- Ejecuta CMake
- Compila el ejecutable principal y los tests

---

### ▶️ Ejecutar el Menú Principal

```bat
.\build.cmd run
```

Al ejecutar el programa principal, se mostrará un menú interactivo con las siguientes opciones:

#### 📌 1. Agregar Estudiante
Se solicitarán los datos del estudiante (nombre, apellido, CI, grado y promedio de calificación).  
- Si la lista está vacía, se creará el primer nodo.  
- Si ya hay estudiantes, el nuevo se agregará al final.

#### ❌ 2. Eliminar Estudiante
Se pedirá la **CI** del estudiante a eliminar.  
La función correspondiente:
- Buscará el nodo
- Lo eliminará
- Actualizará la cabeza de la lista si es necesario

#### 📋 3. Mostrar Estudiantes
Podrás filtrar por: **nombre**, **apellido**, **CI**, **grado** o **promedio**.  
- Si el filtro es inválido, se mostrará toda la información.

#### 🚪 4. Salir
Finaliza la ejecución del programa.

---

### ✅ Ejecutar los Tests

Para correr los tests y validar el funcionamiento de los métodos implementados en `parte_2.h`, utiliza:

```bat
.\build.cmd test
```

Esto ejecutará **CTest** desde el directorio de compilación.

## Documentación de Métodos (`parte_2.h`)

A continuación se ofrece una breve explicación de cada método definido en `parte_2.h`:

### 1. `nodo_estudiante_t *crear_lista_de_estudiantes(const estudiante_t *estudiante);`

**Descripción:**  
Crea el primer nodo (cabeza) de la lista de estudiantes utilizando los datos proporcionados en un objeto `estudiante_t`.

**Uso:**  
Se utiliza cuando la lista está vacía para inicializarla.

---

### 2. `void agregar_estudiante(nodo_estudiante_t *lista, const estudiante_t *estudiante);`

**Descripción:**  
Recorre la lista enlazada y agrega un nuevo nodo al final, conteniendo los datos del nuevo estudiante.

**Uso:**  
Se llama cuando ya existe al menos un estudiante en la lista y se desea insertar uno adicional.

---

### 3. `void eliminar_estudiante(nodo_estudiante_t **lista, const char *ci);`

**Descripción:**  
Elimina de la lista el nodo correspondiente al estudiante que posee la CI especificada. Utiliza doble puntero para poder actualizar la cabeza de la lista en caso de que se elimine el primer nodo.

**Uso:**  
Se utiliza para borrar un estudiante de la lista, tanto en posición intermedia, final o como cabeza de la lista.

---

### 4. `void mostrar_estudiantes(const nodo_estudiante_t *lista, int filter);`

**Descripción:**  
Recorre la lista y muestra la información de los estudiantes. El parámetro `filter` permite especificar qué atributo mostrar (nombre, apellido, CI, grado o promedio de calificación). En caso de un filtro inválido, se muestra la información completa.

**Uso:**  
Para visualizar los datos de todos los estudiantes de la lista, con opción a filtrar la salida.

---

### 5. `void liberarLista(nodo_estudiante_t *lista);`

**Descripción:**  
Libera la memoria asignada para la lista enlazada de estudiantes, liberando cada nodo de forma iterativa.

**Uso:**  
Se utiliza al finalizar la ejecución del programa o en los tests, para evitar fugas de memoria.
