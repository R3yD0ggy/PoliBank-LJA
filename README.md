# 🏦 POLI BANK

**Sistema de Cuentas y Transacciones Bancarias en C**

Un simulador de banco por consola construido desde cero en C puro, con persistencia en archivos, control de acceso por roles, horario de atención real y una bitácora diaria que se genera sola. Nacido como proyecto académico de Programación I — pensado para crecer más allá del aula.

[![Lenguaje](https://img.shields.io/badge/lenguaje-C-blue.svg)](https://es.wikipedia.org/wiki/C_(lenguaje_de_programaci%C3%B3n))
[![Plataforma](https://img.shields.io/badge/plataforma-Windows-lightgrey.svg)]()
[![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow.svg)]()
[![Licencia](https://img.shields.io/badge/licencia-académica-green.svg)]()

---

## 📖 Tabla de contenidos

- [¿Qué es POLI BANK?](#-qué-es-poli-bank)
- [Características principales](#-características-principales)
- [Roles del sistema](#-roles-del-sistema)
- [Arquitectura del proyecto](#-arquitectura-del-proyecto)
- [Modelo de datos](#-modelo-de-datos)
- [Cómo funciona por dentro](#-cómo-funciona-por-dentro)
- [Cómo compilar y ejecutar](#-cómo-compilar-y-ejecutar)
- [Persistencia y archivos generados](#-persistencia-y-archivos-generados)
- [Fundamentos académicos aplicados](#-fundamentos-académicos-aplicados)
- [Roadmap: hacia dónde va este proyecto](#-roadmap-hacia-dónde-va-este-proyecto)
- [Equipo](#-equipo)

---

## 🚀 ¿Qué es POLI BANK?

POLI BANK simula, dentro de una terminal, el funcionamiento real de un banco: las personas se registran, abren una cuenta, depositan, retiran y transfieren dinero — todo respaldado por archivos que sobreviven a cada ejecución del programa, no por datos que se pierden al cerrar la consola.

No es un CRUD de juguete. Cada decisión de diseño busca imitar una restricción real:

- 🕗 El banco **solo opera de 8:00 a 20:00**, igual que una entidad financiera real.
- 🔐 Antes de mover un solo dólar, el sistema **te vuelve a pedir la contraseña**.
- 📁 Cada movimiento del día queda registrado **automáticamente** en una bitácora de texto, sin que nadie tenga que acordarse de generarla.
- 👥 Ni el administrador ni el moderador pueden tocar el saldo de un cliente — **el dinero solo lo mueve su dueño**.

Es la segunda entrega de un proyecto en evolución: la primera versión vivía solo en memoria; esta ya piensa como un sistema real.

---

## ✨ Características principales

| Módulo | Qué hace |
|---|---|
| **Registro de clientes** | Valida cédula ecuatoriana (módulo 10), verifica mayoría de edad y usuario único, campo por campo — si algo está mal, te lo dice *al instante*, no al final. |
| **Autenticación por roles** | Un mismo punto de entrada reconoce si eres cliente, administrador, moderador o cajero, y te lleva a un menú distinto según tus permisos. |
| **Operaciones bancarias** | Depósitos, retiros y transferencias, todos protegidos por horario de atención y reconfirmación de contraseña. |
| **Búsqueda binaria** | Ubicar una cuenta entre cientos de clientes no requiere recorrerlos uno por uno: la lista se mantiene siempre ordenada para buscar en tiempo logarítmico. |
| **Reportes con Quicksort** | Ranking de clientes por saldo, generado ordenando una copia de la lista sin tocar el orden original (necesario para que la búsqueda binaria nunca se rompa). |
| **Bóveda automática** | Ya no es un cajero manual: es una bitácora que se escribe sola, movimiento por movimiento, y se cierra con un resumen calculado por recursividad. |
| **Exportación a CSV** | Balance y listado de usuarios exportables directamente a Excel o Google Sheets. |
| **Mensajería clara** | Todo resultado se anuncia sin ambigüedad: `[Éxito]` o `[Error]`, nunca un silencio incómodo. |

---

## 👥 Roles del sistema

POLI BANK no le da a todo el mundo las mismas llaves. Cada rol puede hacer exactamente lo que le corresponde — ni más, ni menos:

| Rol | Puede | No puede |
|---|---|---|
| 🙋 **Cliente** | Consultar saldo, depositar, retirar y transferir (con contraseña reconfirmada, en horario) | Ver datos de otros clientes, operar fuera de horario |
| 🛠️ **Administrador** | Ver y registrar clientes, generar reportes | Modificar el saldo de un cliente, acceder a la Bóveda |
| 🧾 **Moderador** | Corregir nombre, cédula, fecha de nacimiento, usuario y restablecer contraseña | Modificar el saldo de un cliente, acceder a la Bóveda |
| 🏧 **Cajero (Bóveda)** | Registrar depósitos y retiros a nombre de un cliente, en horario | Generar reportes, registrar clientes |

> 💡 Este diseño no es casualidad: separa deliberadamente "quién administra datos" de "quién mueve dinero", para que ningún rol tenga, ni directa ni indirectamente, un camino hacia el saldo ajeno.

---

## 🏗️ Arquitectura del proyecto

El sistema está organizado en cinco pares de archivos fuente/cabecera, cada uno con una única responsabilidad — bajo acoplamiento, alta cohesión:

```
POLI-BANK/
├── Polibank-LJA.c      → Punto de entrada: main(), menú principal, arranque y cierre
├── funkcii.c / .h      → Núcleo del sistema: struct Cliente, validaciones, registro,
│                          inicio de sesión, menús de cliente/admin/moderador,
│                          horario bancario y bitácora diaria
├── transferencia.c/.h  → Búsqueda binaria de cuentas y lógica de transferencias
├── reportes.c / .h     → Quicksort (ranking de saldos), recursividad (saldo total),
│                          exportación a CSV
└── boveda.c / .h       → Módulo del cajero: depósitos/retiros a nombre de un cliente
                           y cierre recursivo de la bitácora diaria
```

`funkcii.c` no sabe cómo se exporta un reporte, y `reportes.c` no sabe qué es una transferencia: cada módulo resuelve su propio problema y expone solo lo necesario a través de su `.h`.

---

## 🗃️ Modelo de datos

La entidad central del sistema es `struct Cliente`:

| Campo | Tipo | Descripción |
|---|---|---|
| `nombresCompletos` | `char[100]` | Nombre completo del cliente |
| `cedula` | `char[11]` | Cédula de identidad (10 dígitos) |
| `usuario` | `char[50]` | Nombre de usuario, único en el sistema |
| `contrasena` | `char[50]` | Contraseña de acceso |
| `fechaNacimiento` | `FechaNacimiento` | Sub-estructura con `dia`, `mes`, `anio` |
| `numeroCuenta` | `long long` | Fecha de nacimiento + dígitos aleatorios + dígito verificador de Luhn |
| `saldo` | `double` | Saldo disponible en la cuenta |

La lista completa de clientes se mantiene **siempre ordenada ascendentemente por número de cuenta** mediante inserción ordenada — es la condición que hace posible que la búsqueda binaria funcione correctamente en cada operación.

---

## ⚙️ Cómo funciona por dentro

**Registro de un cliente nuevo:**

```
Solicitar datos → Validar cédula, usuario y fecha (campo por campo)
                → Generar número de cuenta con dígito de Luhn
                → Insertar ordenado por número de cuenta
                → Guardar en clientes.bin
```

**Una operación de dinero (depósito, retiro o transferencia):**

```
¿Estamos en horario bancario (8:00–20:00)?  → No: se rechaza antes de pedir el monto
¿La contraseña reconfirmada es correcta?     → No: se cancela la operación
Leer y validar el monto
Actualizar saldo(s)
Guardar clientes.bin + registrar en la bitácora del día + generar comprobante
```

**Búsqueda binaria de una cuenta** (usada en transferencias y en reportes): descarta la mitad de la lista en cada comparación, en lugar de recorrerla cliente por cliente — `O(log n)` en vez de `O(n)`.

---

## 🔧 Cómo compilar y ejecutar

El proyecto está pensado para Windows (usa `windows.h` y `conio.h` para la consola y la lectura de contraseña oculta).

```bash
gcc -o polibank.exe Polibank-LJA.c funkcii.c transferencia.c reportes.c boveda.c
polibank.exe
```

También puede compilarse en Visual Studio Code con la extensión de C/C++ y un compilador MinGW instalado.

---

## 💾 Persistencia y archivos generados

Nada vive solo en memoria. Cada ejecución interactúa con:

| Archivo | Contenido |
|---|---|
| `clientes.bin` | Todos los clientes, en binario (`fwrite`/`fread`) |
| `estado_<numeroCuenta>.txt` | Comprobante de cada operación de un cliente |
| `bitacora_DDMMAAAA.txt` | Bitácora automática del día: cada movimiento, y un resumen recursivo al cerrar el programa |
| `balance_cuentas.csv` / `usuarios_polibank.csv` | Exportaciones para abrir en Excel o Google Sheets |

---

## 🎓 Fundamentos académicos aplicados

Este proyecto no usa estos conceptos de adorno — cada uno resuelve un problema real dentro del sistema:

- **Programación estructurada y modular**: 10 archivos `.c`/`.h`, cada uno con una responsabilidad.
- **Paso de parámetros por referencia**: `Cliente *cliente`, `int *cantidad`, para modificar datos originales sin copias innecesarias.
- **Persistencia en archivos**: `fopen`, `fread`, `fwrite`, `fclose`, en binario y en texto plano.
- **Búsqueda binaria**: localización de cuentas en `O(log n)`, posible gracias a mantener la lista siempre ordenada.
- **Quicksort**: ordenamiento recursivo por "dividir y conquistar" para el ranking de saldos.
- **Recursividad**: cálculo del saldo total del banco y de los totales de la bitácora diaria.
- **Validación por dígito verificador**: módulo 10 para cédulas ecuatorianas, algoritmo de Luhn para números de cuenta.
- **Control de acceso basado en roles (RBAC)**: cada rol solo puede ejecutar las operaciones estrictamente necesarias para su función.

---

## 🗺️ Roadmap: hacia dónde va este proyecto

Esto es solo la segunda entrega. Ya identificamos, con autocrítica, por dónde sigue creciendo:

- [ ] **Memoria dinámica real** — reemplazar el arreglo estático (`MAX_CLIENTES = 100`) por una lista dinámica (`malloc`/`realloc` o lista enlazada), eliminando el límite fijo de clientes.
- [ ] **Seguridad de credenciales** — aplicar una función de hash a las contraseñas antes de guardarlas (hoy viajan en texto plano dentro de `clientes.bin`).
- [ ] **Portabilidad multiplataforma** — quitar la dependencia de `windows.h`/`conio.h` para compilar también en Linux y macOS.
- [ ] **Interés compuesto** — incorporar una función recursiva dedicada a proyectar el crecimiento de un capital.
- [ ] **Pruebas automatizadas** — un set mínimo de pruebas unitarias sobre validaciones (cédula, Luhn, horario bancario).
- [ ] **Integridad del archivo binario** — checksum o respaldo de `clientes.bin` ante un cierre inesperado.

Un proyecto pequeño, con una lista de próximos pasos así de concreta, es la mejor señal de que sabe exactamente hacia dónde va. 🚀

---

## 👨‍💻 Equipo

Proyecto desarrollado para la asignatura **Programación I** — Escuela Politécnica Nacional, Facultad de Ingeniería en Sistemas (GR2CC).

- Jorge Aguirre
- Alisson Gualancañay
- Lionel Vásquez

**Docente:** Ing. Thomás Borja, MSc.

---

<p align="center">Quito, Ecuador — 2026</p>
