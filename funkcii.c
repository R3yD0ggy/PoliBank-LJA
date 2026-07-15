#include "funkcii.h"
#include "transferencia.h"
#include "reportes.h"
#include "boveda.h"

#include <ctype.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

/* ======================================================================
   MÓDULO FUNCII - FUNKCII.C
   
   PROPÓSITO:
   Centraliza funciones reutilizables para NO repetir código en otros
   módulos (boveda.c, reportes.c, transferencia.c). Incluye:
   - Utilidades de I/O (leer texto, números, contraseñas ocultas)
   - Persistencia (guardar/cargar clientes en binario)
   - Validación (cédulas, fechas, números de cuenta)
   - Algoritmos de seguridad (Luhn)
   - Búsqueda de datos (binaria)
   - Menús interactivos (cliente, admin, moderador)
   
   DISEÑO:
   - Cada función hace UNA cosa y la hace bien (Single Responsibility)
   - Las funciones estáticas (static) son internas; públicas están en .h
   - Patrón: leer, validar, reintentar si error
   ====================================================================== */

#define USUARIO_ADMIN "R3yD0ggy"        /* Credencial fija de administrador */
#define CLAVE_ADMIN "admin123"          /* Clave fija (educativo; usar BCrypt en producción) */
#define USUARIO_MODERADOR "moderador1"  /* Credencial fija de moderador */
#define CLAVE_MODERADOR "moder2024"     /* Clave fija de moderador */

/* Horario de atención del banco (ver funkcii.h para más detalle). */
#define HORA_APERTURA 8            /* 08:00 abre la jornada matutina */
#define HORA_CAMBIO_JORNADA 12     /* 12:00 termina matutina, empieza vespertina */
#define HORA_CIERRE 20             /* 20:00 cierra la jornada vespertina */

/* Tasa de interés compuesto fija que cualquier cliente puede simular
   sobre el saldo guardado en su propia cuenta (ver mostrarMenuCliente). */
#define TASA_INTERES_CLIENTE 0.05  /* 5% anual */

/* ======================================================================
   SECCIÓN 0: HORARIO BANCARIO Y BITÁCORA DIARIA

   Estas funciones son usadas por TODOS los módulos que mueven dinero
   (menú cliente aquí mismo, transferencia.c y boveda.c) para restringir
   las operaciones al horario de atención y dejar un registro persistente
   de cada movimiento del día.
   ====================================================================== */

Jornada obtenerJornadaActual(void)
{
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);

    if (t == NULL)
    {
        return JORNADA_CERRADO;
    }
    if (t->tm_hour >= HORA_APERTURA && t->tm_hour < HORA_CAMBIO_JORNADA)
    {
        return JORNADA_MATUTINA;
    }
    if (t->tm_hour >= HORA_CAMBIO_JORNADA && t->tm_hour < HORA_CIERRE)
    {
        return JORNADA_VESPERTINA;
    }
    return JORNADA_CERRADO;
}

int dentroDeHorarioBancario(void)
{
    return obtenerJornadaActual() != JORNADA_CERRADO;
}

/* Nombre legible de la jornada actual, usado para las líneas de la
   bitácora y para mensajes al usuario. */
static const char *nombreJornadaActual(Jornada jornada)
{
    switch (jornada)
    {
    case JORNADA_MATUTINA:
        return "MATUTINA";
    case JORNADA_VESPERTINA:
        return "VESPERTINA";
    default:
        return "CERRADO";
    }
}

void obtenerNombreBitacoraHoy(char *buffer, size_t tamano)
{
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);

    if (t == NULL)
    {
        snprintf(buffer, tamano, "bitacora_desconocida.txt");
        return;
    }
    /* Un archivo distinto por día (DDMMAAAA): si el programa se cierra y
       se reabre el mismo día, este nombre es el mismo y fopen(...,"a")
       simplemente sigue agregando líneas al archivo ya existente. */
    snprintf(buffer, tamano, "bitacora_%02d%02d%04d.txt",
             t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
}

void registrarEnBitacoraDiaria(const char *tipoOperacion, long long numeroCuenta, double monto, double saldoResultante)
{
    char nombreArchivo[64];
    FILE *archivo;
    time_t ahora;
    struct tm *fechaHora;

    obtenerNombreBitacoraHoy(nombreArchivo, sizeof(nombreArchivo));
    archivo = fopen(nombreArchivo, "a");
    if (archivo == NULL)
    {
        printf("[Error] No se pudo actualizar la bitácora diaria de movimientos.\n");
        return;
    }

    ahora = time(NULL);
    fechaHora = localtime(&ahora);

    if (fechaHora != NULL)
    {
        fprintf(archivo, "%02d:%02d|%s|%s|%lld|%.2f|%.2f\n",
                fechaHora->tm_hour, fechaHora->tm_min,
                nombreJornadaActual(obtenerJornadaActual()),
                tipoOperacion, numeroCuenta, monto, saldoResultante);
    }
    fclose(archivo);
}

int confirmarContrasenaCliente(const Cliente *cliente)
{
    char clave[50];

    if (cliente == NULL)
    {
        return 0;
    }

    printf("Por seguridad, reingrese su contraseña para confirmar la operación: ");
    leerContrasenaOculta(clave, sizeof(clave));

    if (strcmp(clave, cliente->contrasena) != 0)
    {
        printf("[Error] Contraseña incorrecta. Operación cancelada por seguridad.\n");
        return 0;
    }
    return 1;
}

/* ======================================================================
   SECCIÓN 1: UTILIDADES DE ENTRADA/SALIDA
   
   Centralizan el manejo de buffers, validación de entrada y límites.
   Evitan repetir el mismo bloque de código en cada menú.
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: limpiarBufferEntrada
   
   Descarta caracteres pendientes en stdin hasta encontrar '\n'.
   Necesario porque scanf() deja el salto de línea en el buffer.
   
   PROBLEMA SIN ESTA FUNCIÓN:
   scanf("%d", &x);        // Usuario escribe: 5<Enter>
   fgets(buffer, ...);     // Fgets lee "" porque hay '\n' pendiente
   
   SOLUCIÓN:
   scanf("%d", &x);
   limpiarBufferEntrada(); // Descarta el '\n' que dejó scanf
   fgets(buffer, ...);     // Ahora sí lee correctamente
   ====================================================================== */
void limpiarBufferEntrada(void)
{
    int c;
    /* Lee hasta encontrar '\n' (fin de línea) o EOF (fin de archivo) */
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

int leerLinea(char *buffer, size_t tamano)
{
    if (!fgets(buffer, (int)tamano, stdin))
    {
        buffer[0] = '\0';
        return 0;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

int leerContrasenaOculta(char *buffer, size_t tamano)
{
    size_t i = 0;
    int tecla;

    if (buffer == NULL || tamano == 0)
    {
        return 0;
    }

    while (i < tamano - 1)
    {
        tecla = _getch();

        if (tecla == '\r' || tecla == '\n')
        {
            break;
        }
        else if (tecla == '\b') /* retroceso: permite corregir sin mostrar la clave */
        {
            if (i > 0)
            {
                i--;
                printf("\b \b");
            }
        }
        else if (tecla >= 32 && tecla <= 126) /* caracter imprimible */
        {
            buffer[i++] = (char)tecla;
            printf("*");
        }
        /* teclas especiales (flechas, F1, etc.) se ignoran */
    }

    buffer[i] = '\0';
    printf("\n");
    return 1;
}

int leerEntero(const char *mensaje, int *valor)
{
    printf("%s", mensaje);
    if (scanf("%d", valor) != 1)
    {
        limpiarBufferEntrada();
        return 0;
    }
    limpiarBufferEntrada();
    return 1;
}

int leerLongLong(const char *mensaje, long long *valor)
{
    char entrada[32];

    printf("%s", mensaje);
    if (!leerLinea(entrada, sizeof(entrada)))
    {
        return 0;
    }
    return sscanf(entrada, "%lld", valor) == 1 && *valor > 0;
}

int leerDoublePositivo(const char *mensaje, double *valor)
{
    char entrada[64];
    char *fin = NULL;
    int i;

    printf("%s", mensaje);
    if (!fgets(entrada, sizeof(entrada), stdin))
    {
        return 0;
    }

    /* Se admite tanto "14.55" como "14,55": en Ecuador es comun escribir
       los decimales con coma, y no queremos que eso trunque el monto a
       su parte entera (ese era el bug que hacia que $14.55 se registrara
       como $14.00). */
    for (i = 0; entrada[i] != '\0'; i++)
    {
        if (entrada[i] == ',')
        {
            entrada[i] = '.';
        }
    }

    *valor = strtod(entrada, &fin);
    return fin != entrada && (*fin == '\n' || *fin == '\r' || *fin == '\0') && *valor > 0;
}

void imprimirTitulo(const char *titulo)
{
    const int ancho = 58;
    int longitud = (int)strlen(titulo);
    int espacios = (ancho - longitud) / 2;

    if (espacios < 0)
    {
        espacios = 0;
    }

    for (int i = 0; i < ancho; i++)
    {
        putchar('=');
    }
    printf("\n");
    printf("%*s%s%*s\n", espacios, "", titulo, ancho - espacios - longitud, "");
    for (int i = 0; i < ancho; i++)
    {
        putchar('=');
    }
    printf("\n");
}

void imprimirSeparador(void)
{
    printf("----------------------------------------------------------\n");
}

/* ==================================================================
   PERSISTENCIA DE CLIENTES (clientes.bin)
   Se guarda/lee el arreglo completo de struct Cliente de una sola vez
   con fwrite/fread: es simple y suficiente para el tamano del proyecto.
   ================================================================== */

int guardarClientes(const Cliente lista[], int cantidad, const char *nombreArchivo)
{
    FILE *archivo = fopen(nombreArchivo, "wb");
    if (archivo == NULL)
    {
        printf("[Error] No se pudo abrir el archivo para guardar los datos.\n");
        return 0;
    }

    size_t escritos = fwrite(lista, sizeof(Cliente), cantidad, archivo);
    fclose(archivo);

    /* 1 = se escribieron todos los registros, 0 = algo fallo a mitad de camino. */
    return (int)escritos == cantidad;
}

int cargarClientes(Cliente lista[], int *cantidad, const char *nombreArchivo)
{
    FILE *archivo = fopen(nombreArchivo, "rb");
    if (archivo == NULL)
    {
        *cantidad = 0;
        return 0;
    }

    *cantidad = (int)fread(lista, sizeof(Cliente), MAX_CLIENTES, archivo);
    fclose(archivo);
    return 1;
}

/* ======================================================================
   SECCIÓN 2: VALIDACIONES Y SEGURIDAD
   
   Funciones para validar documentos de identidad, fechas y números de
   cuenta. Incluyen algoritmos de seguridad como el dígito verificador
   de Luhn (usado en tarjetas de crédito y aquí en números de cuenta).
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: validarCedula
   
   Valida una CÉDULA ECUATORIANA usando el algoritmo oficial.
   
   REGLAS:
   1. Debe tener exactamente 10 dígitos
   2. Primeros 2 dígitos = provincia (01-24 o 30)
   3. Tercer dígito = tipo de persona (0-5)
   4. Último dígito = dígito verificador (calcula ldo con algoritmo Luhn)
   
   ALGORITMO DE VALIDACIÓN:
   - Multiplica dígitos 1-9 por coeficientes alternados (2,1,2,1,2,1,2,1,2)
   - Si resultado > 9, le resta 9
   - Suma todos los resultados
   - (10 - (suma mod 10)) mod 10 debe coincidir con dígito 10
   
   EJEMPLOS VÁLIDOS (ficticios):
   - 1234567890 (provincia 12, tipo 3, verificador calculado)
   - 1750123458 (provincia 17, tipo 5, verificador 8)
   
   PARÁMETROS:
   - cedula: string de 10 caracteres que deben ser dígitos
   
   RETORNA: 1 si la cédula es válida, 0 si no lo es
   
   NOTA: Esto valida FORMAT O y verificador, pero no que sea de una persona real
   ====================================================================== */
int validarCedula(const char *cedula)
{
    int coeficientes[9] = {2, 1, 2, 1, 2, 1, 2, 1, 2};
    int suma = 0;
    int i;
    int provincia;
    int digitoVerificador;
    int resultado;

    if (cedula == NULL || strlen(cedula) != 10)
    {
        return 0;
    }

    for (i = 0; i < 10; i++)
    {
        if (!isdigit((unsigned char)cedula[i]))
        {
            return 0;
        }
    }

    provincia = (cedula[0] - '0') * 10 + (cedula[1] - '0');
    if ((provincia < 1 || provincia > 24) && provincia != 30)
    {
        return 0;
    }
    if (cedula[2] - '0' >= 6)
    {
        return 0;
    }

    for (i = 0; i < 9; i++)
    {
        int valor = (cedula[i] - '0') * coeficientes[i];
        suma += (valor > 9) ? valor - 9 : valor;
    }

    digitoVerificador = cedula[9] - '0';
    resultado = (((suma + 9) / 10) * 10) - suma;
    if (resultado == 10)
    {
        resultado = 0;
    }

    return resultado == digitoVerificador;
}

/* ======================================================================
   FUNCIÓN: validarFechaNacimiento
   
   Valida que una fecha (día/mes/año) sea REAL, no futura, y que la
   persona sea MAYOR DE EDAD (18+ años).
   
   VALIDACIONES EN ORDEN:
   1. Mes debe estar entre 1 y 12
   2. Año debe ser razonable (entre 1900 y hoy)
   3. Día debe ser válido para ese mes (respetando bisiestos)
   4. La fecha no puede ser futura (no puedes registrarte mañana)
   5. La persona debe tener 18 años cumplidos (requisito legal bancario)
   
   BISIESTO:
   - Un año es bisiesto si:
     * Es divisible por 400, O
     * Es divisible por 4 Y no por 100
   - Febrero de bisiesto tiene 29 días
   
   CÓDIGOS DE ERROR:
   - FECHA_VALIDA (0): todo ok
   - FECHA_MES_INVALIDO: mes < 1 o > 12
   - FECHA_ANIO_INVALIDO: año < 1900 o > año actual
   - FECHA_DIA_INVALIDO: día no existe en ese mes
   - FECHA_ES_FUTURA: fecha es después de hoy
   - FECHA_MENOR_DE_EDAD: persona tiene < 18 años
   
   PARÁMETROS:
   - dia, mes, anio: componentes de la fecha de nacimiento
   
   RETORNA: código específico de ResultadoValidacionFecha
   ====================================================================== */
ResultadoValidacionFecha validarFechaNacimiento(int dia, int mes, int anio)
{
    time_t tiempoActual = time(NULL);
    struct tm *fechaActual = localtime(&tiempoActual);
    /* Array con días válidos para cada mes (índice 0-11) */
    int diasMes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int anioActual, mesActual, diaActual, maxDias;
    int edadReal, yaCumplioEsteAnio;

    if (fechaActual == NULL)
    {
        return FECHA_ANIO_INVALIDO;
    }

    if (mes < 1 || mes > 12)
    {
        return FECHA_MES_INVALIDO;
    }
    if (anio < 1900)
    {
        return FECHA_ANIO_INVALIDO;
    }

    anioActual = fechaActual->tm_year + 1900;
    mesActual = fechaActual->tm_mon + 1;
    diaActual = fechaActual->tm_mday;

    maxDias = diasMes[mes - 1];
    if (mes == 2 && ((anio % 400 == 0) || (anio % 4 == 0 && anio % 100 != 0)))
    {
        maxDias = 29;
    }
    if (dia < 1 || dia > maxDias)
    {
        return FECHA_DIA_INVALIDO;
    }

    if (anio > anioActual || (anio == anioActual && (mes > mesActual || (mes == mesActual && dia > diaActual))))
    {
        return FECHA_ES_FUTURA;
    }

    /* Edad real considerando si ya paso el cumpleanos de este anio. */
    edadReal = anioActual - anio;
    yaCumplioEsteAnio = (mes < mesActual) || (mes == mesActual && dia <= diaActual);
    if (!yaCumplioEsteAnio)
    {
        edadReal--;
    }

    if (edadReal < 18)
    {
        return FECHA_MENOR_DE_EDAD;
    }

    return FECHA_VALIDA;
}

/* ======================================================================
   FUNCIÓN: calcularDigitoVerificadorLuhn
   
   Calcula el DÍGITO VERIFICADOR de Luhn para un número de cuenta.
   Este dígito previene que alguien invente un número "a ojo".
   
   ALGORITMO DE LUHN (usado en tarjetas de crédito, números de cuenta, etc):
   
   1. Itera los dígitos de DERECHA a IZQUIERDA
   2. Duplica CADA SEGUNDO dígito (posición 1, 3, 5, 7... desde la derecha)
   3. Si el resultado > 9, le resta 9
   4. Suma todos los dígitos resultantes
   5. El dígito verificador = (10 - (suma mod 10)) mod 10
   
   EJEMPLO: número = "14034520"
   Posición desde derecha: 1 2 3 4 5 6 7 8
   Dígitos:                0 2 5 4 3 0 4 1
   Se duplican posiciones pares (2,4,6,8) desde derecha:
                           0 4 5 8 3 0 8 2
   Ajustar si > 9:        0 4 5 8 3 0 8 2 (5->5, 8->8 no necesitan ajuste)
   Suma: 0+4+5+8+3+0+8+2 = 30
   Verificador: (10 - (30 % 10)) % 10 = (10 - 0) % 10 = 0
   
   POR QUÉ FUNCIONA:
   - Si alguien cambia UN dígito, la suma cambia y el verificador ya no coincide
   - Si alguien intercambia dos dígitos, la suma también cambia
   - Es probabilístico: 90% de los números inventados al azar fallarán la prueba
   
   PARÁMETROS:
   - numero: string de dígitos (sin verificador aún)
   
   RETORNA: el dígito verificador (0-9)
   ====================================================================== */
static int calcularDigitoVerificadorLuhn(const char *numero)
{
    int longitud = (int)strlen(numero);
    int suma = 0;
    int i;

    for (i = 0; i < longitud; i++)
    {
        int posicionDesdeElFinal = longitud - 1 - i;
        int digito = numero[posicionDesdeElFinal] - '0';

        /* Duplica cada segundo dígito (posiciones impares desde la derecha) */
        if (i % 2 == 1)
        {
            digito *= 2;
            if (digito > 9)
            {
                digito -= 9;
            }
        }
        suma += digito;
    }

    /* Calcula complemento a 10 */
    return (10 - (suma % 10)) % 10;
}

/* Devuelve 1 si algun cliente de 'lista' ya tiene ese numero de cuenta
   (se usa para no generar dos cuentas iguales por coincidencia). */
static int numeroCuentaExiste(const Cliente lista[], int cantidad, long long numero)
{
    int i;
    for (i = 0; i < cantidad; i++)
    {
        if (lista[i].numeroCuenta == numero)
        {
            return 1;
        }
    }
    return 0;
}

/* El numero de cuenta ahora se arma con tres partes:
   1) la fecha de nacimiento del cliente: DDMMAAAA
   2) tres digitos aleatorios para evitar duplicados obvios
   3) un digito verificador calculado con el algoritmo de Luhn

   Se concatena la fecha y los 3 digitos aleatorios en una cadena base,
   luego se calcula el digito Luhn de esa base y se añade al final.
   Si el numero completo ya existe en la lista de clientes, se genera
   otra combinacion de los 3 digitos aleatorios hasta encontrar uno unico.
*/
void asignarNumeroCuentaLuhn(long long *numeroCuenta, const Cliente lista[], int cantidad, const FechaNacimiento *fecha)
{
    char base[16];
    char numeroCompleto[20];
    int aleatorio;

    do
    {
        aleatorio = rand() % 1000;
        snprintf(base, sizeof(base), "%02d%02d%04d%03d", fecha->dia, fecha->mes, fecha->anio, aleatorio);
        snprintf(numeroCompleto, sizeof(numeroCompleto), "%s%d", base, calcularDigitoVerificadorLuhn(base));
        *numeroCuenta = atoll(numeroCompleto);
    } while (numeroCuentaExiste(lista, cantidad, *numeroCuenta));
}

int verificarNumeroCuentaLuhn(long long numeroCuenta)
{
    char texto[32];
    int longitud;
    int digitoReal;

    snprintf(texto, sizeof(texto), "%lld", numeroCuenta);
    longitud = (int)strlen(texto);
    if (longitud < 2)
    {
        return 0;
    }

    digitoReal = texto[longitud - 1] - '0';
    texto[longitud - 1] = '\0'; /* separamos el digito verificador del resto del numero */

    return calcularDigitoVerificadorLuhn(texto) == digitoReal;
}

/* ==================================================================
   REGISTRO DE CLIENTES
   ================================================================== */

int existeUsuario(const Cliente lista[], int cantidad, const char *usuario)
{
    int i;
    for (i = 0; i < cantidad; i++)
    {
        if (strcmp(lista[i].usuario, usuario) == 0)
        {
            return 1;
        }
    }
    return 0;
}

/* Inserta 'nuevo' dentro de 'lista' respetando el orden ascendente por
   numero de cuenta (insercion ordenada), en vez de simplemente agregarlo
   al final. Esto es indispensable ahora que el numero de cuenta depende
   de la fecha de nacimiento y ya no crece de forma secuencial: si no se
   mantuviera el orden, la busqueda binaria (buscarClientePorNumeroCuenta,
   buscarCuentaBinaria) dejaria de funcionar correctamente. */
static void insertarClienteOrdenado(Cliente lista[], int *cantidad, const Cliente *nuevo)
{
    int i = *cantidad - 1;

    while (i >= 0 && lista[i].numeroCuenta > nuevo->numeroCuenta)
    {
        lista[i + 1] = lista[i];
        i--;
    }
    lista[i + 1] = *nuevo;
    (*cantidad)++;
}

void registrarCliente(Cliente lista[], int *cantidad)
{
    Cliente nuevo;
    char confirmacionClave[50];
    int dia = 0, mes = 0, anio = 0;
    int datoValido;
    ResultadoValidacionFecha resultadoFecha;

    if (*cantidad >= MAX_CLIENTES)
    {
        printf("\n[Error] Lo sentimos, Polibank alcanzó el límite de clientes que puede atender por ahora.\n");
        return;
    }

    imprimirTitulo("REGISTRO DE NUEVO CLIENTE - POLIBANK");
    printf("Vamos a crear su cuenta en 5 pasos cortos.\n");

    imprimirSeparador();
    printf("Paso 1/5 - Datos personales\n");
    printf("Nombres y apellidos completos: ");
    leerLinea(nuevo.nombresCompletos, sizeof(nuevo.nombresCompletos));

    do
    {
        printf("Número de cédula (10 dígitos): ");
        leerLinea(nuevo.cedula, sizeof(nuevo.cedula));
        datoValido = validarCedula(nuevo.cedula);
        if (!datoValido)
        {
            printf("[Error] Esa cédula no es válida. Verifique los 10 dígitos e intente de nuevo.\n");
        }
    } while (!datoValido);

    imprimirSeparador();
    printf("Paso 2/5 - Fecha de nacimiento (debe ser mayor de edad)\n");
    do
    {
        /* Cada campo se valida DE INMEDIATO apenas se ingresa: si el día
           está mal, se vuelve a pedir el día ahí mismo antes de siquiera
           preguntar por el mes (en vez de dejar que ingrese los 3 datos
           mal y recién ahí avisarle). */
        do
        {
            if (!leerEntero("Día de nacimiento (1-31): ", &dia) || dia < 1 || dia > 31)
            {
                printf("[Error] El día debe ser un número entre 1 y 31. Vuelva a ingresarlo.\n");
                dia = 0;
            }
        } while (dia == 0);

        do
        {
            if (!leerEntero("Mes de nacimiento (1-12): ", &mes) || mes < 1 || mes > 12)
            {
                printf("[Error] El mes debe ser un número entre 1 y 12. Vuelva a ingresarlo.\n");
                mes = 0;
            }
        } while (mes == 0);

        do
        {
            if (!leerEntero("Año de nacimiento (ej. 2001): ", &anio) || anio < 1900 || anio > 2100)
            {
                printf("[Error] Ingrese un año de nacimiento válido (desde 1900). Vuelva a ingresarlo.\n");
                anio = 0;
            }
        } while (anio == 0);

        /* Con los 3 campos ya válidos individualmente, se hace la
           validación completa: día real para ese mes, que no sea futura,
           y que la persona sea mayor de edad. */
        resultadoFecha = validarFechaNacimiento(dia, mes, anio);
        switch (resultadoFecha)
        {
        case FECHA_DIA_INVALIDO:
            printf("[Error] El día %d no existe para el mes %d. Vamos a corregir la fecha completa.\n", dia, mes);
            break;
        case FECHA_ES_FUTURA:
            printf("[Error] La fecha de nacimiento no puede ser en el futuro. Vamos a corregir la fecha completa.\n");
            break;
        case FECHA_MENOR_DE_EDAD:
            printf("[Error] Debe ser mayor de edad (18 años cumplidos) para abrir una cuenta. Vamos a corregir la fecha completa.\n");
            break;
        default:
            break;
        }
    } while (resultadoFecha != FECHA_VALIDA);

    imprimirSeparador();
    printf("Paso 3/5 - Usuario de acceso\n");
    do
    {
        printf("Cree un nombre de usuario: ");
        leerLinea(nuevo.usuario, sizeof(nuevo.usuario));
        datoValido = strlen(nuevo.usuario) > 0 && !existeUsuario(lista, *cantidad, nuevo.usuario);
        if (!datoValido)
        {
            printf("[Error] Ese usuario ya está en uso (o está vacío). Elija otro.\n");
        }
    } while (!datoValido);

    imprimirSeparador();
    printf("Paso 4/5 - Contraseña\n");
    do
    {
        printf("Cree una contraseña: ");
        leerContrasenaOculta(nuevo.contrasena, sizeof(nuevo.contrasena));
        printf("Confirme su contraseña: ");
        leerContrasenaOculta(confirmacionClave, sizeof(confirmacionClave));
        datoValido = strlen(nuevo.contrasena) > 0 && strcmp(nuevo.contrasena, confirmacionClave) == 0;
        if (!datoValido)
        {
            printf("[Error] Las contraseñas no coinciden. Intentemos de nuevo.\n");
        }
    } while (!datoValido);

    imprimirSeparador();
    printf("Paso 5/5 - Apertura de la cuenta\n");
    nuevo.fechaNacimiento.dia = dia;
    nuevo.fechaNacimiento.mes = mes;
    nuevo.fechaNacimiento.anio = anio;
    asignarNumeroCuentaLuhn(&nuevo.numeroCuenta, lista, *cantidad, &nuevo.fechaNacimiento);
    nuevo.saldo = 0.0;

    insertarClienteOrdenado(lista, cantidad, &nuevo);

    imprimirTitulo("CUENTA CREADA CON EXITO");
    printf("[Éxito] Bienvenido/a a Polibank, %s. Su cuenta fue registrada correctamente.\n", nuevo.nombresCompletos);
    printf("Número de cuenta: %lld\n", nuevo.numeroCuenta);
    printf("Guárdelo bien: lo necesitará junto a su usuario y clave para iniciar sesión.\n");
}

/* ==================================================================
   BUSQUEDA Y CALCULOS SOBRE LA LISTA DE CLIENTES
   ================================================================== */

/* Busqueda binaria clasica: funciona porque 'lista' se mantiene siempre
   ordenada ascendentemente por numero de cuenta (ver insertarClienteOrdenado). */
/* ======================================================================
   SECCIÓN 3: BÚSQUEDA Y RECUPERACIÓN DE DATOS
   
   Funciones para localizar clientes por número de cuenta usando
   búsqueda binaria (O(log n), muy eficiente).
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: buscarClientePorNumeroCuenta
   
   Localiza UN cliente por su número de cuenta usando BÚSQUEDA BINARIA.
   
   PRECONDICIÓN CRÍTICA:
   La lista DEBE estar ORDENADA ASCENDENTEMENTE por numeroCuenta.
   Esta se mantiene así porque insertarClienteOrdenado() lo garantiza.
   
   COMPLEJIDAD:
   - O(log n): con 1 millón de clientes, máximo 20 comparaciones
   - Vs. O(n) lineal: con 1 millón de clientes, promedio 500,000 comparaciones
   
   PARÁMETROS:
   - lista[], cantidad: lista de clientes (debe estar ordenada)
   - numeroCuenta: número a buscar
   - *posicion: salida - dónde guardar el índice si encuentra
   
   RETORNA: 1 si encontró (posicion se llena), 0 si no existe (posicion = -1)
   ====================================================================== */
int buscarClientePorNumeroCuenta(const Cliente lista[], int cantidad, long long numeroCuenta, int *posicion)
{
    int inicio = 0;
    int fin = cantidad - 1;

    if (posicion == NULL)
    {
        return 0;
    }

    while (inicio <= fin)
    {
        int medio = inicio + (fin - inicio) / 2;

        if (lista[medio].numeroCuenta == numeroCuenta)
        {
            *posicion = medio;
            return 1;
        }
        if (lista[medio].numeroCuenta < numeroCuenta)
        {
            inicio = medio + 1;
        }
        else
        {
            fin = medio - 1;
        }
    }

    *posicion = -1;
    return 0;
}

/* ======================================================================
   FUNCIÓN: calcularSaldoTotalRecursivo
   
   Suma RECURSIVAMENTE los saldos de todos los clientes.
   Útil para auditoría: "¿cuánto dinero hay en circulación?"
   
   ESTRUCTURA RECURSIVA:
   - Caso base: si cantidad <= 0, retorna 0 (fin)
   - Caso recursivo: suma último cliente + calcular para el resto
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes
   
   RETORNA: suma total de todos los saldos
   ====================================================================== */
double calcularSaldoTotalRecursivo(const Cliente lista[], int cantidad)
{
    if (cantidad <= 0)
    {
        return 0.0;
    }
    return lista[cantidad - 1].saldo + calcularSaldoTotalRecursivo(lista, cantidad - 1);
}

/* ==================================================================
   ESTADOS DE CUENTA Y DATOS DEL CLIENTE
   ================================================================== */

void generarEstadoCuenta(const Cliente *cliente, const char *tipoOperacion, double monto, double saldoResultante)
{
    char nombreArchivo[64];
    FILE *archivo;
    time_t ahora;
    struct tm *fechaHora;

    if (cliente == NULL)
    {
        return;
    }

    snprintf(nombreArchivo, sizeof(nombreArchivo), "estado_%lld.txt", cliente->numeroCuenta);
    archivo = fopen(nombreArchivo, "a");
    if (archivo == NULL)
    {
        printf("[Error] No se pudo generar el estado de cuenta.\n");
        return;
    }

    ahora = time(NULL);
    fechaHora = localtime(&ahora);

    fprintf(archivo, "====================================\n");
    fprintf(archivo, "        ESTADO DE CUENTA - POLIBANK\n");
    fprintf(archivo, "====================================\n");
    fprintf(archivo, "Titular: %s\n", cliente->nombresCompletos);
    fprintf(archivo, "Cuenta: %lld\n", cliente->numeroCuenta);
    if (fechaHora != NULL)
    {
        fprintf(archivo, "Fecha: %02d/%02d/%04d %02d:%02d\n",
                fechaHora->tm_mday, fechaHora->tm_mon + 1, fechaHora->tm_year + 1900,
                fechaHora->tm_hour, fechaHora->tm_min);
    }
    fprintf(archivo, "Operacion: %s\n", tipoOperacion);
    fprintf(archivo, "Monto: $%.2f\n", monto);
    fprintf(archivo, "Saldo resultante: $%.2f\n", saldoResultante);
    fprintf(archivo, "------------------------------------\n");
    fprintf(archivo, "Gracias por confiar en Polibank.\n");
    fprintf(archivo, "====================================\n\n");

    fclose(archivo);
}

void mostrarDatosCliente(const Cliente *cliente)
{
    imprimirTitulo("DATOS DEL CLIENTE");
    printf("Nombre: %s\n", cliente->nombresCompletos);
    printf("Cédula: %s\n", cliente->cedula);
    printf("Número de cuenta: %lld\n", cliente->numeroCuenta);
    printf("Fecha de nacimiento: %02d/%02d/%04d\n",
           cliente->fechaNacimiento.dia, cliente->fechaNacimiento.mes, cliente->fechaNacimiento.anio);
}

/* ==================================================================
   OPERACIONES DE SALDO
   ================================================================== */

void depositarSaldo(Cliente *cliente, double monto)
{
    cliente->saldo += monto;
}

int retirarSaldo(Cliente *cliente, double monto)
{
    if (monto > cliente->saldo)
    {
        return 0;
    }
    cliente->saldo -= monto;
    return 1;
}

/* ==================================================================
   MENU: CORREGIR DATOS DE CLIENTE (uso exclusivo del moderador)
   El moderador puede arreglar datos mal digitados por el cliente, pero
   NUNCA puede tocar el saldo de la cuenta: eso queda exclusivamente en
   manos del cajero, dentro del modulo de Boveda.
   ================================================================== */

void modificarCliente(Cliente lista[], int cantidad)
{
    long long numeroCuenta = 0;
    int posicion = -1;
    int opcion = 0;

    if (!leerLongLong("Número de cuenta del cliente a corregir: ", &numeroCuenta) ||
        !verificarNumeroCuentaLuhn(numeroCuenta) ||
        !buscarClientePorNumeroCuenta(lista, cantidad, numeroCuenta, &posicion))
    {
        printf("[Error] No existe un cliente con ese número de cuenta.\n");
        return;
    }

    printf("\nCliente encontrado: %s\n", lista[posicion].nombresCompletos);

    while (1)
    {
        imprimirTitulo("CORREGIR DATOS DEL CLIENTE");
        printf("1. Corregir nombre completo\n2. Corregir cédula\n3. Corregir fecha de nacimiento\n");
        printf("4. Corregir usuario\n5. Restablecer contraseña\n6. Volver\n");
        printf("(El moderador no puede modificar el saldo de la cuenta.)\n");

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida.\n");
            continue;
        }

        if (opcion == 1)
        {
            printf("Nuevo nombre completo: ");
            leerLinea(lista[posicion].nombresCompletos, sizeof(lista[posicion].nombresCompletos));
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("[Éxito] Nombre actualizado correctamente.\n");
        }
        else if (opcion == 2)
        {
            char cedulaNueva[11];
            int valida;
            do
            {
                printf("Nueva cédula (10 dígitos): ");
                leerLinea(cedulaNueva, sizeof(cedulaNueva));
                valida = validarCedula(cedulaNueva);
                if (!valida)
                {
                    printf("[Error] Cédula inválida. Intente de nuevo.\n");
                }
            } while (!valida);
            strcpy(lista[posicion].cedula, cedulaNueva);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("[Éxito] Cédula actualizada correctamente.\n");
        }
        else if (opcion == 3)
        {
            int dia = 0, mes = 0, anio = 0;
            ResultadoValidacionFecha resultado;
            do
            {
                /* Igual que en el registro: cada campo se valida apenas
                   se ingresa, no se espera a tener los 3 mal. */
                do
                {
                    if (!leerEntero("Día de nacimiento correcto (1-31): ", &dia) || dia < 1 || dia > 31)
                    {
                        printf("[Error] El día debe ser un número entre 1 y 31. Vuelva a ingresarlo.\n");
                        dia = 0;
                    }
                } while (dia == 0);

                do
                {
                    if (!leerEntero("Mes de nacimiento correcto (1-12): ", &mes) || mes < 1 || mes > 12)
                    {
                        printf("[Error] El mes debe ser un número entre 1 y 12. Vuelva a ingresarlo.\n");
                        mes = 0;
                    }
                } while (mes == 0);

                do
                {
                    if (!leerEntero("Año de nacimiento correcto (ej. 2001): ", &anio) || anio < 1900 || anio > 2100)
                    {
                        printf("[Error] Ingrese un año válido (desde 1900). Vuelva a ingresarlo.\n");
                        anio = 0;
                    }
                } while (anio == 0);

                resultado = validarFechaNacimiento(dia, mes, anio);
                if (resultado != FECHA_VALIDA)
                {
                    printf("[Error] Esa fecha no es válida (o la persona no sería mayor de edad). Vamos a corregir la fecha completa.\n");
                }
            } while (resultado != FECHA_VALIDA);
            lista[posicion].fechaNacimiento.dia = dia;
            lista[posicion].fechaNacimiento.mes = mes;
            lista[posicion].fechaNacimiento.anio = anio;
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("[Éxito] Fecha de nacimiento actualizada correctamente.\n");
        }
        else if (opcion == 4)
        {
            char usuarioNuevo[50];
            int valido;
            do
            {
                printf("Nuevo usuario: ");
                leerLinea(usuarioNuevo, sizeof(usuarioNuevo));
                valido = strlen(usuarioNuevo) > 0 &&
                         (strcmp(usuarioNuevo, lista[posicion].usuario) == 0 ||
                          !existeUsuario(lista, cantidad, usuarioNuevo));
                if (!valido)
                {
                    printf("[Error] Ese usuario ya está en uso. Intente con otro.\n");
                }
            } while (!valido);
            strcpy(lista[posicion].usuario, usuarioNuevo);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("[Éxito] Usuario actualizado correctamente.\n");
        }
        else if (opcion == 5)
        {
            char nuevaClave[50];
            printf("Nueva contraseña temporal para el cliente: ");
            leerContrasenaOculta(nuevaClave, sizeof(nuevaClave));
            strcpy(lista[posicion].contrasena, nuevaClave);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("[Éxito] Contraseña restablecida. Indique al cliente que la cambie al ingresar.\n");
        }
        else if (opcion == 6)
        {
            return;
        }
        else
        {
            printf("[Error] Opción inválida.\n");
        }
    }
}

/* ==================================================================
   MENU CLIENTE
   ================================================================== */

/* Mensaje estándar que se muestra cuando se intenta depositar, retirar o
   transferir fuera del horario de atención (ver dentroDeHorarioBancario). */
static void avisarBancoCerrado(void)
{
    printf("[Error] Polibank solo realiza movimientos de %02d:00 a %02d:00 ",
           HORA_APERTURA, HORA_CIERRE);
    printf("(jornada matutina %02d:00-%02d:00, vespertina %02d:00-%02d:00). Intente durante el horario de atención.\n",
           HORA_APERTURA, HORA_CAMBIO_JORNADA, HORA_CAMBIO_JORNADA, HORA_CIERRE);
}

void mostrarMenuCliente(Cliente *cliente, Cliente lista[], int cantidad, int posicion)
{
    int opcion = 0;

    while (1)
    {
        imprimirTitulo("MENU CLIENTE - POLIBANK");
        printf("1. Consultar saldo\n2. Depositar dinero\n3. Retirar dinero\n");
        printf("4. Transferir dinero\n5. Simular interés compuesto de mi cuenta (5%% anual)\n");
        printf("6. Cerrar sesión\n");

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida.\n");
            continue;
        }

        if (opcion == 1)
        {
            mostrarDatosCliente(cliente);
            printf("Saldo disponible: $%.2f\n", cliente->saldo);
        }
        else if (opcion == 2)
        {
            double monto = 0.0;

            if (!dentroDeHorarioBancario())
            {
                avisarBancoCerrado();
                continue;
            }
            if (!confirmarContrasenaCliente(cliente))
            {
                continue;
            }
            if (!leerDoublePositivo("Ingrese el monto a depositar: $", &monto))
            {
                printf("[Error] Monto inválido. Ingrese un número mayor a 0 (ej. 14.55).\n");
                continue;
            }
            depositarSaldo(cliente, monto);
            guardarClientes(lista, cantidad, "clientes.bin");
            generarEstadoCuenta(cliente, "DEPOSITO", monto, cliente->saldo);
            registrarEnBitacoraDiaria("DEPOSITO", cliente->numeroCuenta, monto, cliente->saldo);
            printf("[Éxito] Depósito de $%.2f realizado con éxito. Nuevo saldo: $%.2f\n", monto, cliente->saldo);
        }
        else if (opcion == 3)
        {
            double monto = 0.0;

            if (!dentroDeHorarioBancario())
            {
                avisarBancoCerrado();
                continue;
            }
            if (!confirmarContrasenaCliente(cliente))
            {
                continue;
            }
            if (!leerDoublePositivo("Ingrese el monto a retirar: $", &monto))
            {
                printf("[Error] Monto inválido. Ingrese un número mayor a 0 (ej. 14.55).\n");
                continue;
            }
            if (!retirarSaldo(cliente, monto))
            {
                printf("[Error] Saldo insuficiente. Su saldo disponible es $%.2f\n", cliente->saldo);
                continue;
            }
            guardarClientes(lista, cantidad, "clientes.bin");
            generarEstadoCuenta(cliente, "RETIRO", monto, cliente->saldo);
            registrarEnBitacoraDiaria("RETIRO", cliente->numeroCuenta, monto, cliente->saldo);
            printf("[Éxito] Retiro de $%.2f realizado con éxito. Nuevo saldo: $%.2f\n", monto, cliente->saldo);
        }
        else if (opcion == 4)
        {
            if (!dentroDeHorarioBancario())
            {
                avisarBancoCerrado();
                continue;
            }
            if (!confirmarContrasenaCliente(cliente))
            {
                continue;
            }
            transferirSaldo(lista, cantidad, posicion);
        }
        else if (opcion == 5)
        {
            int anios = 0;
            double saldoProyectado;

            if (cliente->saldo <= 0.0)
            {
                printf("[Error] Su saldo actual es $%.2f. Deposite dinero primero para poder simular el interés.\n", cliente->saldo);
                continue;
            }
            if (!leerEntero("¿Para cuántos años quiere proyectar el interés?: ", &anios) || anios <= 0)
            {
                printf("[Error] La cantidad de años debe ser un número entero mayor a 0.\n");
                continue;
            }

            /* Se calcula RECURSIVAMENTE sobre el saldo actual guardado en
               la cuenta del cliente, con la tasa fija del 5% anual. Esto
               es solo una simulación: NO modifica el saldo real. */
            saldoProyectado = calcularInteresCompuestoRecursivo(cliente->saldo, TASA_INTERES_CLIENTE, anios);

            printf("\nSaldo actual guardado: $%.2f\n", cliente->saldo);
            printf("Tasa de interés compuesto: %.0f%% anual\n", TASA_INTERES_CLIENTE * 100);
            printf("Saldo proyectado en %d año(s): $%.2f\n", anios, saldoProyectado);
            printf("(Esta es una simulación informativa; su saldo real no cambia.)\n");
        }
        else if (opcion == 6)
        {
            printf("[Éxito] Sesión cerrada. Gracias por usar Polibank, %s. ¡Hasta pronto!\n", cliente->nombresCompletos);
            return;
        }
        else
        {
            printf("[Error] Opción inválida. Elija una opción del 1 al 6.\n");
        }
    }
}

/* ==================================================================
   MENU ADMINISTRADOR
   El administrador gestiona clientes y reportes, pero NUNCA maneja
   dinero en efectivo directamente: eso es tarea del cajero (Boveda).
   ================================================================== */

void mostrarMenuAdmin(Cliente lista[], int *cantidad)
{
    int opcion = 0;

    /* NOTA IMPORTANTE: el administrador NO tiene ninguna opción para
       tocar el saldo de un cliente ni para entrar al módulo Bóveda.
       El acceso a Bóveda ahora es un punto de entrada independiente
       en el menú principal (main), con sus propias credenciales de
       cajero, para que el admin no pueda mover dinero ni siquiera
       indirectamente. */
    while (1)
    {
        imprimirTitulo("MENU ADMINISTRADOR - POLIBANK");
        printf("1. Ver clientes registrados\n2. Registrar nuevo cliente\n");
        printf("3. Reportes (ranking, CSV, saldo total)\n4. Salir\n");

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida.\n");
            continue;
        }

        if (opcion == 1)
        {
            imprimirTitulo("CLIENTES REGISTRADOS");
            if (*cantidad <= 0)
            {
                printf("No hay clientes registrados.\n");
                continue;
            }
            for (int i = 0; i < *cantidad; i++)
            {
                printf("%d. %-25s | Cédula: %-10s | Usuario: %-15s | Saldo: $%.2f\n",
                       i + 1, lista[i].nombresCompletos, lista[i].cedula, lista[i].usuario, lista[i].saldo);
            }
        }
        else if (opcion == 2)
        {
            registrarCliente(lista, cantidad);
            guardarClientes(lista, *cantidad, "clientes.bin");
        }
        else if (opcion == 3)
        {
            mostrarMenuReportes(lista, *cantidad);
        }
        else if (opcion == 4)
        {
            printf("Saliendo del menú administrador.\n");
            return;
        }
        else
        {
            printf("[Error] Opción inválida.\n");
        }
    }
}

/* ==================================================================
   MENU MODERADOR
   ================================================================== */

void mostrarMenuModerador(Cliente lista[], int cantidad)
{
    int opcion = 0;

    while (1)
    {
        imprimirTitulo("MENU MODERADOR - POLIBANK");
        printf("1. Buscar y corregir datos de un cliente\n2. Ver lista de clientes\n3. Salir\n");

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida.\n");
            continue;
        }

        if (opcion == 1)
        {
            modificarCliente(lista, cantidad);
        }
        else if (opcion == 2)
        {
            imprimirTitulo("CLIENTES REGISTRADOS");
            if (cantidad <= 0)
            {
                printf("No hay clientes registrados.\n");
            }
            else
            {
                int i;
                for (i = 0; i < cantidad; i++)
                {
                    printf("%d. %-25s | Cedula: %-10s | Usuario: %-15s | Cuenta: %lld\n",
                           i + 1, lista[i].nombresCompletos, lista[i].cedula, lista[i].usuario, lista[i].numeroCuenta);
                }
            }
        }
        else if (opcion == 3)
        {
            printf("Saliendo del menú moderador.\n");
            return;
        }
        else
        {
            printf("[Error] Opción inválida.\n");
        }
    }
}

/* ==================================================================
   INICIO DE SESION
   ================================================================== */

TipoSesion iniciarSesion(Cliente lista[], int cantidad, int *posicion)
{
    char usuarioIngresado[50];
    char contrasenaIngresada[50];
    int i;

    imprimirTitulo("INICIO DE SESION - POLIBANK");
    printf("Usuario: ");
    leerLinea(usuarioIngresado, sizeof(usuarioIngresado));
    printf("Contraseña: ");
    leerContrasenaOculta(contrasenaIngresada, sizeof(contrasenaIngresada));

    if (strcmp(usuarioIngresado, USUARIO_ADMIN) == 0 && strcmp(contrasenaIngresada, CLAVE_ADMIN) == 0)
    {
        *posicion = -1;
        printf("\n[Éxito] Acceso concedido. Bienvenido, administrador de Polibank.\n");
        return SESION_ADMINISTRADOR;
    }

    if (strcmp(usuarioIngresado, USUARIO_MODERADOR) == 0 && strcmp(contrasenaIngresada, CLAVE_MODERADOR) == 0)
    {
        *posicion = -1;
        printf("\n[Éxito] Acceso concedido. Bienvenido, moderador de Polibank.\n");
        return SESION_MODERADOR;
    }

    for (i = 0; i < cantidad; i++)
    {
        if (strcmp(lista[i].usuario, usuarioIngresado) == 0 && strcmp(lista[i].contrasena, contrasenaIngresada) == 0)
        {
            *posicion = i;
            printf("\n[Éxito] Bienvenido/a de nuevo, %s.\n", lista[i].nombresCompletos);
            printf("Cuenta: %lld | Saldo disponible: $%.2f\n", lista[i].numeroCuenta, lista[i].saldo);
            return SESION_CLIENTE;
        }
    }

    *posicion = -1;
    printf("\n[Error] Usuario o contraseña incorrectos. Verifique sus datos e intente de nuevo.\n");
    return SESION_NINGUNA;
}
