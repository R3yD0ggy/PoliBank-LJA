#ifndef FUNKCII_H
#define FUNKCII_H

#include <stddef.h>
#include <stdio.h>

/* ======================================================================
   ENCABEZADO: FUNKCII.H
   
   Define la interfaz pública de FUNCIONES UTILITARIAS reutilizables.
   Estos son helpers usados por todos los módulos del banco (boveda,
   transferencia, reportes, etc.) para evitar duplicar código.
   
   CATEGORÍAS:
   1. Entrada/Salida: lectura y validación de datos desde teclado
   2. Persistencia: guardar/cargar datos en archivos binarios
   3. Validaciones: cédula, fecha, número de cuenta (Luhn)
   4. Autenticación: login de cliente, admin, moderador
   5. Menús: interfaces interactivas para cada tipo de usuario
   6. Utilidades: búsqueda, cálculos, actualizaciones de saldo
   
   CONVENCIÓN:
   - Funciones estáticas en .c (static) = internas, no exponemos aquí
   - Funciones públicas = declaradas aquí en .h
   - Las que retornan 0/1 son "booleans" (0=falso, 1=verdadero)
   ====================================================================== */

/* Numero maximo de clientes que puede guardar el sistema en memoria. */
#define MAX_CLIENTES 100

/* ======================================================================
   ESTRUCTURA: FechaNacimiento
   
   Representa una fecha de nacimiento de un cliente (día/mes/año).
   Se utiliza para:
   - Validación de mayoría de edad (mínimo 18 años)
   - Generación del número de cuenta (DDMMAAAA + 3 aleatorios + Luhn)
   - Auditoría y reportes
   
   NOTAS SOBRE NOMBRES:
   - Evitamos ñ (eñe) en nombres de variables C: no todas las plataformas
     las aceptan (Windows vs. Linux puede ser diferente)
   - En cambio, los textos que VE el usuario sí usan tildes/ñ normalmente
   ====================================================================== */
typedef struct FechaNacimiento
{
    int dia;
    int mes;
    int anio;
} FechaNacimiento;

/* ======================================================================
   ESTRUCTURA: Cliente
   
   Representa UN CLIENTE DEL BANCO con todos sus datos:
   - Identificación: nombre, cédula
   - Autenticación: usuario, contraseña
   - Datos biográficos: fecha de nacimiento
   - Datos bancarios: número de cuenta único, saldo
   
   ORDENAMIENTO CRÍTICO:
   La lista se mantiene SIEMPRE ordenada ascendentemente por numeroCuenta.
   Esto es esencial para que la búsqueda binaria funcione.
   
   PERSISTENCIA:
   Se guarda completa en clientes.bin usando fwrite/fread (binario puro).
   ====================================================================== */
typedef struct Cliente
{
    char nombresCompletos[100];
    char cedula[11];          /* 10 dígitos + '\0' terminal */
    char usuario[50];         /* Username para login */
    char contrasena[50];      /* Contraseña (hardcodeada en demo) */
    FechaNacimiento fechaNacimiento;
    long long numeroCuenta;   /* 16 dígitos: DDMMAAAA + 3 aleatorios + Luhn */
    double saldo;             /* Dinero en la cuenta */
} Cliente;

/* ==================================================================
   HORARIO BANCARIO Y BITÁCORA DIARIA DE MOVIMIENTOS

   El banco ahora solo opera durante un horario de atención definido,
   dividido en dos jornadas (tal como pide el documento del proyecto):
   - Jornada MATUTINA:   08:00 a 11:59
   - Jornada VESPERTINA: 12:00 a 19:59
   Fuera de ese rango (20:00 a 07:59) el banco está CERRADO y no se
   permite ningún depósito, retiro o transferencia (consultar saldo,
   iniciar sesión y registrarse SÍ siguen disponibles siempre).

   Además, TODO movimiento de dinero que ocurra en el sistema (ya sea
   hecho por el cliente desde su propio menú, o por el cajero en el
   módulo Bóveda) se registra de inmediato en un archivo de texto
   "bitacora_DDMMAAAA.txt" correspondiente al día en curso. Si el
   programa se cierra y se vuelve a abrir el MISMO día, los nuevos
   movimientos se agregan (append) a ese mismo archivo en vez de crear
   uno nuevo, y al cerrar el programa se agrega un resumen con los
   totales del día (ver cerrarBitacoraDelDia en boveda.h).
   ================================================================== */
typedef enum Jornada
{
    JORNADA_CERRADO = 0,   /* Banco cerrado: antes de 8:00 o desde las 20:00 */
    JORNADA_MATUTINA,      /* 08:00 - 11:59 */
    JORNADA_VESPERTINA     /* 12:00 - 19:59 */
} Jornada;

/* Determina en qué jornada estamos según la hora actual del sistema. */
Jornada obtenerJornadaActual(void);

/* Devuelve 1 si el banco está operando ahora mismo (jornada matutina o
   vespertina), 0 si está fuera de horario (JORNADA_CERRADO). Debe
   llamarse antes de permitir cualquier depósito, retiro o transferencia. */
int dentroDeHorarioBancario(void);

/* Devuelve en 'buffer' el nombre del archivo de bitácora del día de hoy,
   con formato "bitacora_DDMMAAAA.txt". Se usa tanto para registrar
   movimientos como para leerlos al cerrar el día. */
void obtenerNombreBitacoraHoy(char *buffer, size_t tamano);

/* Agrega UNA línea a la bitácora del día con los datos del movimiento:
   hora, jornada, tipo de operación, cuenta, monto y saldo resultante.
   Se llama desde cualquier punto del código que modifique un saldo
   (depósito/retiro del cliente, transferencias, y movimientos del
   cajero en Bóveda), por lo que afecta a todos los módulos del banco. */
void registrarEnBitacoraDiaria(const char *tipoOperacion, long long numeroCuenta, double monto, double saldoResultante);

/* Pide al cliente que reingrese su contraseña para CONFIRMAR una
   operación sensible (depósito, retiro o transferencia) y la compara
   con la contraseña almacenada en su cuenta. Retorna 1 si coincide,
   0 si no (en cuyo caso la operación debe cancelarse). */
int confirmarContrasenaCliente(const Cliente *cliente);

/* ==================================================================
   UTILIDADES DE ENTRADA / SALIDA POR CONSOLA
   Centralizan patrones que antes se repetian en cada archivo .c
   (leer texto, leer numeros, validar, limpiar el buffer de stdin, etc.)
   ================================================================== */

/* ======================================================================
   FUNCIÓN: limpiarBufferEntrada
   
   PROPÓSITO:
   Descarta caracteres pendientes en el buffer de stdin hasta el '\n'.
   
   PROBLEMA QUE RESUELVE:
   scanf("%d", &x) deja el '\n' en el buffer. Si luego haces fgets(),
   fgets() lee una línea vacía porque ve el '\n' que dejó scanf.
   
   SOLUCIÓN:
   Siempre que uses scanf(), llama limpiarBufferEntrada() después.
   
   PARÁMETROS: ninguno
   
   EFECTO: descarta todo hasta y incluyendo el próximo '\n' o EOF
   ====================================================================== */
void limpiarBufferEntrada(void);

/* ======================================================================
   FUNCIÓN: leerLinea
   
   Lee una línea de texto desde teclado (máximo 'tamano' bytes).
   Automáticamente quita el salto de línea final.
   
   VENTAJAS:
   - Segura: respeta límite de bytes (no hay buffer overflow)
   - Limpia: automáticamente quita '\n'
   - Simple: una función para toda entrada de texto
   
   PARÁMETROS:
   - *buffer: array donde guardar el texto
   - tamano: máximo de bytes a leer (para evitar overflow)
   
   RETORNA:
   - 1 si pudo leer correctamente
   - 0 si hubo error (EOF, fallo de fgets, etc)
   ====================================================================== */
int leerLinea(char *buffer, size_t tamano);

/* ======================================================================
   FUNCIÓN: leerContrasenaOculta
   
   Lee una contraseña OCULTA: muestra "*" en lugar del carácter real.
   Se usa para login (como en cajeros automáticos o bancos en línea).
   
   CARACTERÍSTICAS:
   - Usa _getch() en Windows para leer carácter por carácter sin echo
   - Muestra "*" por cada carácter tipeado
   - Maneja Enter y Backspace correctamente
   
   PARÁMETROS:
   - *buffer: array donde guardar la contraseña
   - tamano: máximo de bytes
   
   RETORNA:
   - 1 si pudo leer correctamente
   - 0 si error (buffer NULL o tamano 0)
   ====================================================================== */
int leerContrasenaOculta(char *buffer, size_t tamano);

/* ======================================================================
   FUNCIÓN: leerEntero
   
   Lee UN ENTERO desde teclado de forma segura.
   
   CARACTERÍSTICAS:
   - Imprime un mensaje antes de leer
   - Si el usuario escribe algo inválido (no es número), retorna 0
   - Limpia el buffer automáticamente para siguientes lecturas
   - Útil para menús (1. Opción, 2. Opción, etc.)
   
   PARÁMETROS:
   - *mensaje: string a mostrar ("Opción: ", "Cantidad: ", etc)
   - *valor: variable donde guardar el entero leído
   
   RETORNA:
   - 1 si el usuario escribió un número válido
   - 0 si escribió algo que no es un número
   ====================================================================== */
int leerEntero(const char *mensaje, int *valor);

/* ======================================================================
   FUNCIÓN: leerLongLong
   
   Lee UN NÚMERO GRANDE (long long) desde teclado.
   Se usa para números de cuenta (16 dígitos) que no caben en int.
   
   VALIDACIONES:
   - Valida que sea un número válido
   - Valida que sea > 0 (rechaza negativos y cero)
   
   PARÁMETROS:
   - *mensaje: string a mostrar
   - *valor: donde guardar el número
   
   RETORNA:
   - 1 si pudo leer un número > 0
   - 0 si escribió algo inválido o <= 0
   ====================================================================== */
int leerLongLong(const char *mensaje, long long *valor);

/* ======================================================================
   FUNCIÓN: leerDoublePositivo
   
   Lee UN NÚMERO DECIMAL POSITIVO para montos de dinero.
   
   CARACTERÍSTICAS:
   - Acepta TANTO punto COMO coma decimal
     (en Ecuador es común escribir 14,55 en lugar de 14.55)
   - Rechaza números <= 0 (no tiene sentido un monto negativo)
   - Valida formato correctamente
   
   EJEMPLO DE USO:
   leerDoublePositivo("Cantidad a transferir: $", &monto);
   Usuario escribe: 14.55 O 14,55 → funciona ambas
   
   PARÁMETROS:
   - *mensaje: text a mostrar
   - *valor: donde guardar el número
   
   RETORNA:
   - 1 si pudo leer un número > 0
   - 0 si error o <= 0
   ====================================================================== */
int leerDoublePositivo(const char *mensaje, double *valor);

/* ======================================================================
   FUNCIÓN: imprimirTitulo
   
   Imprime un encabezado bonito y consistente para todos los menús.
   
   FORMATO:
   ========================================================
                        MÓDULO BÓVEDA
   ========================================================
   
   VENTAJA:
   - Todos los menús se ven iguales (consistencia visual)
   - Código limpio: no repites formato en cada menú
   
   PARÁMETROS:
   - *titulo: text del encabezado
   ====================================================================== */
void imprimirTitulo(const char *titulo);

/* ======================================================================
   FUNCIÓN: imprimirSeparador
   
   Imprime una línea divisoria para separar visualmente bloques de texto.
   Útil cuando hay muchas líneas y necesitas claridad visual.
   
   EJEMPLO:
   "Ingrese sus datos"
   ----------------------------------------------------------
   "Nombre: "
   ====================================================================== */
void imprimirSeparador(void);

/* ==================================================================
   PERSISTENCIA DE CLIENTES (archivo binario clientes.bin)
   ================================================================== */

/* ======================================================================
   FUNCIÓN: guardarClientes
   
   Guarda TODO el arreglo de clientes al disco en formato BINARIO.
   
   FORMATO:
   - Archivo binario (no texto) con estructura Cliente repetida
   - Cada Cliente ocupa sizeof(Cliente) bytes
   - Se escriben 'cantidad' clientes consecutivos
   
   VENTAJAS DEL BINARIO:
   - Compacto: menos espacio en disco que CSV
   - Rápido: una sola operación fwrite() vs. iterar y printear
   - Preserva exactitud: no hay errores de formato (como CSV)
   
   PARÁMETROS:
   - lista[], cantidad: arreglo de clientes a guardar
   - *nombreArchivo: ruta donde guardar (ej. "clientes.bin")
   
   RETORNA:
   - 1 si se escribieron TODOS los registros correctamente
   - 0 si hubo error (no se pudo abrir, escribió parcial, etc)
   
   NOTA: Si falla, el archivo puede quedarse corrupto. Idealmente
         usar transacciones (escribir a temp, luego mover).
   ====================================================================== */
int guardarClientes(const Cliente lista[], int cantidad, const char *nombreArchivo);

/* ======================================================================
   FUNCIÓN: cargarClientes
   
   Carga clientes guardados en disco hacia memoria.
   
   FLUJO:
   1. Intenta abrir el archivo en modo lectura binaria
   2. Si no existe: retorna 0 y cantidad = 0 (primer inicio del banco)
   3. Si existe: lee hasta MAX_CLIENTES cliente y actualiza cantidad
   4. Cierra el archivo
   
   PARÁMETROS:
   - lista[]: array donde cargar los clientes
   - *cantidad: se rellena con el número de clientes leídos
   - *nombreArchivo: ruta del archivo (ej. "clientes.bin")
   
   RETORNA:
   - 1 si se pudo abrir el archivo
   - 0 si el archivo no existe (banco vacío, primer inicio)
   
   NOTA: Se llama automáticamente en main() al iniciar el programa
   ====================================================================== */
int cargarClientes(Cliente lista[], int *cantidad, const char *nombreArchivo);

/* ==================================================================
   REGISTRO Y VALIDACION DE CLIENTES
   ================================================================== */

/* ======================================================================
   FUNCIÓN: registrarCliente
   
   FLUJO COMPLETO de registro de un nuevo cliente.
   
   PASOS:
   1. Pide nombre completo
   2. Pide y valida cédula (10 dígitos, verificador correcto, provincia válida)
   3. Pide y valida fecha de nacimiento (real, no futura, >= 18 años)
   4. Pide usuario (único, no puede repetirse)
   5. Pide contraseña oculta
   6. Genera número de cuenta basado en fecha (DDMMAAAA + 3 aleatorios + Luhn)
   7. Inserta en la lista manteniendo orden ascendente
   8. Le asigna saldo inicial = 0
   
   PARÁMETROS:
   - lista[]: donde agregar el nuevo cliente
   - *cantidad: se incrementa si se registra exitosamente
   ====================================================================== */
void registrarCliente(Cliente lista[], int *cantidad);

/* ======================================================================
   FUNCIÓN: validarCedula
   
   Valida que una cédula ecuatoriana sea correcta.
   
   VALIDACIONES:
   - Exactamente 10 dígitos
   - Primeros 2 dígitos = provincia válida (01-24 o 30)
   - Tercer dígito = tipo persona (0-5)
   - Último dígito = verificador correcto (algoritmo Luhn modificado)
   
   PARÁMETROS:
   - *cedula: string de 10 caracteres dígitos
   
   RETORNA:
   - 1 si es válida
   - 0 si es inválida (formato, provincia, verificador, etc)
   ====================================================================== */
int validarCedula(const char *cedula);

/* ======================================================================
   ENUMERACIÓN: ResultadoValidacionFecha
   
   Códigos de error ESPECÍFICOS para validación de fecha.
   Permite al usuario saber EXACTAMENTE qué está mal (no solo "fecha inválida").
   
   VALORES:
   - FECHA_VALIDA (0): todo ok, se puede usar
   - FECHA_MES_INVALIDO: mes < 1 o > 12
   - FECHA_ANIO_INVALIDO: año < 1900 o > hoy
   - FECHA_DIA_INVALIDO: día no existe en ese mes
   - FECHA_ES_FUTURA: la fecha aún no ha llegado
   - FECHA_MENOR_DE_EDAD: persona tiene < 18 años
   ====================================================================== */
typedef enum ResultadoValidacionFecha
{
    FECHA_VALIDA = 0,
    FECHA_MES_INVALIDO,
    FECHA_ANIO_INVALIDO,
    FECHA_DIA_INVALIDO,
    FECHA_ES_FUTURA,
    FECHA_MENOR_DE_EDAD
} ResultadoValidacionFecha;

/* ======================================================================
   FUNCIÓN: validarFechaNacimiento
   
   Valida que una fecha sea REAL, NO FUTURA, y que la persona sea MAYOR DE EDAD.
   
   VALIDACIONES (en orden):
   1. Mes entre 1-12
   2. Año entre 1900 y hoy
   3. Día válido para ese mes (respetando bisiestos)
   4. Fecha no es futura (no puedes nacer mañana)
   5. Persona >= 18 años hoy
   
   PARÁMETROS:
   - dia, mes, anio: componentes de la fecha
   
   RETORNA: código de ResultadoValidacionFecha (indica qué está mal, si algo)
   ====================================================================== */
ResultadoValidacionFecha validarFechaNacimiento(int dia, int mes, int anio);

/* ======================================================================
   FUNCIÓN: existeUsuario
   
   Verifica si un usuario (username) ya está siendo usado por otro cliente.
   
   PROPÓSITO:
   - Exigir que cada usuario sea único
   - Prevenir que dos clientes tengan el mismo login
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes registrados
   - *usuario: username a buscar
   
   RETORNA:
   - 1 si el usuario ya existe (está en uso)
   - 0 si el usuario está disponible
   ====================================================================== */
int existeUsuario(const Cliente lista[], int cantidad, const char *usuario);

/* ======================================================================
   FUNCIÓN: asignarNumeroCuentaLuhn
   
   Genera un NÚMERO DE CUENTA ÚNICO para un cliente nuevo.
   
   FORMATO (16 dígitos):
   - Primeros 8 dígitos: DDMMAAAA (fecha de nacimiento)
   - Siguientes 3 dígitos: aleatorios (previene colisiones)
   - Último 1 dígito: verificador de Luhn (previene números inventados)
   
   PROCESO:
   1. Crea base de 11 dígitos (DDMMAAAA + 3 aleatorios)
   2. Calcula dígito de Luhn para esos 11
   3. Verifica que el número NO exista ya (colisión check)
   4. Si existe, reintenta con otros 3 aleatorios
   
   PARÁMETROS:
   - *numeroCuenta: salida - número generado
   - lista[], cantidad: clientes existentes (para no repetir)
   - *fecha: fecha de nacimiento del cliente nuevo
   ====================================================================== */
void asignarNumeroCuentaLuhn(long long *numeroCuenta, const Cliente lista[], int cantidad, const FechaNacimiento *fecha);

/* ======================================================================
   FUNCIÓN: verificarNumeroCuentaLuhn
   
   Valida que un número de cuenta cumpla con el algoritmo de Luhn.
   
   PROPÓSITO:
   - Rechazar de inmediato números inventados/incorrectos
   - Prevenir que alguien escriba un número de cuenta "a ojo"
   
   PARÁMETROS:
   - numeroCuenta: número a validar (16 dígitos)
   
   RETORNA:
   - 1 si el verificador de Luhn es correcto
   - 0 si es inválido
   ====================================================================== */
int verificarNumeroCuentaLuhn(long long numeroCuenta);

/* ==================================================================
   SESION Y MENUS PRINCIPALES
   ================================================================== */

/* ======================================================================
   ENUMERACIÓN: TipoSesion
   
   Representa el tipo de usuario autenticado (su rol en el banco).
   Controla qué menú se abre después del login exitoso.
   
   VALORES:
   - SESION_NINGUNA (0): credenciales incorrectas, no autenticado
   - SESION_CLIENTE: usuario regular que puede transferir, consultar
   - SESION_ADMINISTRADOR: gestiona clientes y reportes (NUNCA dinero;
     no tiene acceso al módulo Bóveda, que es un punto de entrada aparte)
   - SESION_MODERADOR: acceso de lectura y corrección de datos (sin dinero)
   ====================================================================== */
typedef enum TipoSesion
{
    SESION_NINGUNA = 0,
    SESION_CLIENTE,
    SESION_ADMINISTRADOR,
    SESION_MODERADOR
} TipoSesion;

/* ======================================================================
   FUNCIÓN: iniciarSesion
   
   FLUJO DE AUTENTICACIÓN COMPLETO:
   1. Pide usuario y contraseña oculta
   2. Compara contra credenciales de admin y moderador (hardcodeadas)
   3. Si no coinciden, busca el usuario entre clientes
   4. Devuelve el tipo de sesión y posición si es cliente
   
   CREDENCIALES FIJAS (para educación; en producción = base de datos):
   - Admin: "R3yD0ggy" / "admin123"
   - Moderador: "moderador1" / "moder2024"
   - Clientes: se crean durante el registro interactivo
   
   PARÁMETROS:
   - lista[], cantidad: lista de clientes del banco
   - *posicion: se rellena con índice de cliente si la sesión es SESION_CLIENTE
   
   RETORNA: TipoSesion (tipo de usuario autenticado)
   ====================================================================== */
TipoSesion iniciarSesion(Cliente lista[], int cantidad, int *posicion);

/* ======================================================================
   FUNCIÓN: mostrarMenuCliente
   
   MENÚ PRINCIPAL para un cliente autenticado.
   
   OPCIONES DISPONIBLES:
   1. Consultar saldo actual
   2. Depositar dinero
   3. Retirar dinero
   4. Transferir dinero a otra cuenta
   5. Simular interés compuesto de su propia cuenta (tasa fija 5% anual,
      calculado recursivamente sobre el saldo que tiene guardado; es solo
      una proyección informativa, no modifica el saldo real)
   6. Cerrar sesión
   
   REGLAS DE SEGURIDAD (opciones 2, 3 y 4):
   - Solo se permiten dentro del horario de atención del banco
     (dentroDeHorarioBancario): 08:00-12:00 (matutina) y 12:00-20:00
     (vespertina). Fuera de ese rango se rechaza la operación.
   - Antes de ejecutarse, se le vuelve a pedir la contraseña al cliente
     (confirmarContrasenaCliente) para confirmar que es él quien opera.
   - Cada movimiento exitoso se registra también en la bitácora diaria
     del banco (registrarEnBitacoraDiaria), además del estado de cuenta
     individual del cliente.
   
   OPERACIONES:
   - Depósito/Retiro: el cliente los hace directamente desde su cuenta
   - Transferencia: búsqueda binaria de cuenta, validación Luhn, saldo
   - Consulta: simple lectura de datos (disponible en cualquier horario)
   
   PARÁMETROS:
   - *cliente: referencia al cliente autenticado
   - lista[], cantidad: todos los clientes (para búsqueda binaria)
   - posicion: índice del cliente autenticado en la lista
   ====================================================================== */
void mostrarMenuCliente(Cliente *cliente, Cliente lista[], int cantidad, int posicion);

/* ======================================================================
   FUNCIÓN: mostrarMenuAdmin
   
   MENÚ ADMINISTRATIVO del ADMINISTRADOR del banco.
   
   OPCIONES DISPONIBLES:
   1. Listar todos los clientes con sus datos
   2. Registrar cliente nuevo
   3. Ver reportes y análisis (rankings, exportar CSV, saldo total)
   4. Salir (logout)
   
   RESTRICCIONES IMPORTANTES:
   - El admin NO tiene NINGUNA opción para tocar el saldo de un cliente
   - El admin NO puede acceder al módulo Bóveda: ese acceso es un punto
     de entrada aparte en el menú principal, con credenciales propias
     de cajero, para que el admin no pueda mover dinero ni siquiera
     entrando a Bóveda con otra identidad
   - El admin tiene acceso a todos los datos (auditoría)
   - Esta función es muy poderosa → usar en ambiente seguro
   
   PARÁMETROS:
   - lista[], *cantidad: lista de clientes (cantidad puede cambiar si registra nuevo)
   ====================================================================== */
void mostrarMenuAdmin(Cliente lista[], int *cantidad);

/* ======================================================================
   FUNCIÓN: mostrarMenuModerador
   
   MENÚ PARA EL MODERADOR (corrige datos, pero nunca dinero).
   
   OPCIONES DISPONIBLES:
   1. Buscar y corregir datos de un cliente (nombre, cédula, fecha de
      nacimiento, usuario, restablecer contraseña)
   2. Ver lista de clientes
   3. Volver
   
   RESTRICCIONES:
   - NO puede crear clientes nuevos
   - NO puede acceder al módulo Bóveda
   - NO puede modificar el saldo de ninguna cuenta
   - Perfecto para supervisores que NO manejan dinero
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes (solo lectura, no se modifica)
   ====================================================================== */
void mostrarMenuModerador(Cliente lista[], int cantidad);

/* ==================================================================
   OPERACIONES SOBRE EL SALDO DE UN CLIENTE
   ================================================================== */

/* ======================================================================
   FUNCIÓN: depositarSaldo
   
   Suma un monto al saldo actual de un cliente (operación de depósito).
   
   PARÁMETROS:
   - *cliente: cliente cuyo saldo se incrementará
   - monto: cantidad a agregar (debe ser > 0)
   
   NOTAS:
   - Esta función NO valida que monto > 0 (lo debe hacer el llamador)
   - Es una operación simple: cliente->saldo += monto
   - Se llama desde registrarMovimiento() en bóveda.c
   ====================================================================== */
void depositarSaldo(Cliente *cliente, double monto);

/* ======================================================================
   FUNCIÓN: retirarSaldo
   
   Resta un monto del saldo actual de un cliente (operación de retiro).
   
   VALIDACIÓN:
   - Verifica que el cliente tenga saldo suficiente
   - Si no, NO hace ningún cambio (fail-safe)
   
   PARÁMETROS:
   - *cliente: cliente cuyo saldo se reducirá
   - monto: cantidad a restar
   
   RETORNA:
   - 1 si el retiro se realizó (saldo suficiente)
   - 0 si no hay fondos (saldo se mantiene sin cambios)
   ====================================================================== */
int retirarSaldo(Cliente *cliente, double monto);

/* ======================================================================
   FUNCIÓN: buscarClientePorNumeroCuenta
   
   Busca UN cliente por su número de cuenta usando BÚSQUEDA BINARIA.
   
   PRECONDICIÓN:
   - La lista debe estar SIEMPRE ordenada ascendentemente por numeroCuenta
   - insertarClienteOrdenado() garantiza esto
   
   COMPLEJIDAD:
   - O(log n): muy eficiente incluso con millones de clientes
   
   PARÁMETROS:
   - lista[], cantidad: lista de clientes (debe estar ordenada)
   - numeroCuenta: número a buscar
   - *posicion: salida - dónde guardar el índice si encuentra
   
   RETORNA:
   - 1 si encontró (*posicion se rellena)
   - 0 si no existe (*posicion = -1)
   ====================================================================== */
int buscarClientePorNumeroCuenta(const Cliente lista[], int cantidad, long long numeroCuenta, int *posicion);

/* ======================================================================
   FUNCIÓN: mostrarDatosCliente
   
   Imprime en pantalla los datos básicos de un cliente:
   - Nombre completo
   - Cédula
   - Usuario (login)
   - Fecha de nacimiento
   - Número de cuenta
   - Saldo actual
   
   PARÁMETROS:
   - *cliente: cliente cuyos datos mostrar
   ====================================================================== */
void mostrarDatosCliente(const Cliente *cliente);

/* ======================================================================
   FUNCIÓN: calcularSaldoTotalRecursivo
   
   Suma RECURSIVAMENTE los saldos de TODOS los clientes del banco.
   
   ESTRUCTURA RECURSIVA:
   - Caso base: si cantidad <= 0, retorna 0 (fin)
   - Caso recursivo: saldo del último cliente + calcular para el resto
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes
   
   RETORNA: suma total de todos los saldos (auditoría del banco)
   ====================================================================== */
double calcularSaldoTotalRecursivo(const Cliente lista[], int cantidad);

/* ======================================================================
   FUNCIÓN: generarEstadoCuenta
   
   Crea un archivo de "estado de cuenta" (recibo) para un cliente.
   Se guarda como estado_<numeroCuenta>.txt
   
   CONTENIDO DEL ARCHIVO:
   - Fecha/hora de la operación
   - Tipo de operación (DEPÓSITO, RETIRO, TRANSFERENCIA, etc)
   - Monto
   - Saldo resultante
   - Datos del cliente
   
   PARÁMETROS:
   - *cliente: cliente que realiza la operación
   - *tipoOperacion: "DEPOSITO", "RETIRO", "TRANSFERENCIA ENVIADA", etc
   - monto: cantidad operada
   - saldoResultante: saldo final después de la operación
   ====================================================================== */
void generarEstadoCuenta(const Cliente *cliente, const char *tipoOperacion, double monto, double saldoResultante);

/* ======================================================================
   FUNCIÓN: modificarCliente (función auxiliar)
   
   Permite CORREGIR datos de un cliente existente:
   - Nombre completo
   - Cédula
   - Fecha de nacimiento
   - Usuario (login)
   - Contraseña
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes
   
   NOTAS:
   - Solo el moderador puede acceder a esta función
   - NO modifica saldo (eso es exclusivo de bóveda/cajero)
   - Útil si el cliente se registró con datos equivocados
   ====================================================================== */
void modificarCliente(Cliente lista[], int cantidad);

#endif
