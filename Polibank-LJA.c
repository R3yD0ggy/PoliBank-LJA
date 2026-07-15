#include <stdio.h>
#include "funkcii.h"
#include "boveda.h"
#include <windows.h>
#include <locale.h>
#include <time.h>
#include <stdlib.h>

/* ======================================================================
   PROGRAMA PRINCIPAL: POLIBANK-LJA.C (main)

   PROPÓSITO:
   Punto de entrada de la aplicación bancaria Polibank. Implementa:
   - Menú principal antes de iniciar sesión (registrar, login, Bóveda, salir)
   - Carga/guardado de datos persistentes (clientes.bin)
   - Configuración de locale y consola (Windows UTF-8)
   - Inicialización de variables globales
   - Cierre de la bitácora diaria de movimientos al salir del programa

   FLUJO GENERAL:
   1. Configura Windows para mostrar tildes/ñ correctamente
   2. Carga clientes guardados en clientes.bin
   3. Muestra menú principal en bucle
   4. Según opción: registro, login, acceso a Bóveda (cajero), o salida
   5. Si login exitoso, dirige a menú de su rol (admin, moderador, cliente)
   6. Al salir: guarda cambios y cierra la bitácora del día

   REDISEÑO IMPORTANTE:
   El acceso a Bóveda (Cajero) ya NO está dentro del menú de administrador:
   ahora es una opción independiente aquí en el menú principal, con sus
   propias credenciales de cajero, para que el administrador nunca tenga
   forma de mover el dinero de un cliente (ni directa ni indirectamente).

   DEPENDENCIAS:
   - funkcii.h: structs, autenticación, persistencia, horario bancario
   - boveda.h: módulo de Bóveda y cierre de bitácora diaria
   - reportes.h, transferencia.h: módulos especializados (usados a través
     de funkcii.c)
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: mostrarMenuPrincipal

   Imprime el primer menú que ve el usuario (antes de autenticarse).

   OPCIONES:
   1. Registrarse como cliente nuevo
   2. Iniciar sesión (cliente/admin/moderador)
   3. Acceso a Bóveda (Cajero)
   4. Salir del programa
   ====================================================================== */
static void mostrarMenuPrincipal(void)
{
    imprimirTitulo("POLIBANK - TU BANCO DE CONFIANZA");
    printf("1. ¿No tienes una cuenta? Regístrate para obtener una.\n");
    printf("2. ¿Ya eres cliente? Inicia sesión para acceder a tu cuenta.\n");
    printf("3. Acceso a Bóveda (Cajero).\n");
    printf("4. Salir del programa.\n");
}

/* ======================================================================
   FUNCIÓN: ofrecerRegistroTrasLoginFallido

   Ofrece al usuario que falló el login la opción de registrarse como
   cliente nuevo.

   PARÁMETROS:
   - lista[], *cantidad: lista de clientes (se modifica si registra)
   ====================================================================== */
static void ofrecerRegistroTrasLoginFallido(Cliente lista[], int *cantidad)
{
    char respuesta[8];

    printf("\n¿Desea registrarse como un nuevo cliente en Polibank? (s/n): ");
    leerLinea(respuesta, sizeof(respuesta));

    if (respuesta[0] == 's' || respuesta[0] == 'S')
    {
        printf("\n--- Redireccionando al registro ---\n");
        registrarCliente(lista, cantidad);
        /* Guarda inmediatamente al disco (previene pérdida de datos) */
        guardarClientes(lista, *cantidad, "clientes.bin");
    }
}

/* ======================================================================
   FUNCIÓN: procesarInicioSesion

   PROPÓSITO:
   Ejecuta el flujo completo de login y enrutamiento según el tipo de usuario.

   PARÁMETROS:
   - lista[], *cantidad: lista de clientes
   ====================================================================== */
static void procesarInicioSesion(Cliente lista[], int *cantidad)
{
    int posicionLogueada = -1;
    /* Intenta autenticar: devuelve el tipo de sesión + posición si es cliente */
    TipoSesion sesion = iniciarSesion(lista, *cantidad, &posicionLogueada);

    /* Enruta según el tipo de usuario autenticado */
    switch (sesion)
    {
    case SESION_ADMINISTRADOR:
        /* Admin: ver clientes, registrar clientes, reportes (nunca dinero) */
        mostrarMenuAdmin(lista, cantidad);
        break;
    case SESION_MODERADOR:
        /* Moderador: vista de clientes y reportes (sin modificaciones) */
        mostrarMenuModerador(lista, *cantidad);
        break;
    case SESION_CLIENTE:
        /* Cliente: su propio menú de operaciones (transferencias, consultas) */
        mostrarMenuCliente(&lista[posicionLogueada], lista, *cantidad, posicionLogueada);
        break;
    case SESION_NINGUNA:
    default:
        /* Credenciales equivocadas: ofrece registro */
        ofrecerRegistroTrasLoginFallido(lista, cantidad);
        break;
    }
}

/* ======================================================================
   FUNCIÓN: main

   PROPÓSITO:
   Punto de entrada del programa. Inicializa todo y ejecuta el loop principal.

   RETORNA: 0 al salir exitosamente
   ====================================================================== */
int main(void)
{
    Cliente lista[MAX_CLIENTES];
    int cantidad = 0;
    int opcion = 0;

    /* ================================================================
       CONFIGURACIÓN DE CONSOLA Y LOCALE (WINDOWS UTF-8)
       ================================================================ */
    setlocale(LC_ALL, ".UTF8");
    setlocale(LC_NUMERIC, "C");
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    /* ================================================================
       INICIALIZACIÓN DE DATOS
       ================================================================ */
    srand((unsigned int)time(NULL));
    cargarClientes(lista, &cantidad, "clientes.bin");

    /* ================================================================
       MENÚ PRINCIPAL EN BUCLE
       ================================================================ */
    imprimirTitulo("BIENVENIDO A POLIBANK");
    printf("Su banco digital de confianza. Gracias por elegirnos.\n");

    do
    {
        mostrarMenuPrincipal();

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida. Escriba un número entre 1 y 4.\n");
            continue;
        }

        switch (opcion)
        {
        case 1:
            /* Opción 1: Registrar cliente nuevo */
            registrarCliente(lista, &cantidad);
            guardarClientes(lista, cantidad, "clientes.bin");
            break;
        case 2:
            /* Opción 2: Iniciar sesión (cliente, admin o moderador) */
            procesarInicioSesion(lista, &cantidad);
            break;
        case 3:
            /* Opción 3: Acceso a Bóveda (Cajero), independiente del admin */
            iniciarModuloBoveda(lista, cantidad);
            break;
        case 4:
            /* Opción 4: Salir del programa */
            imprimirTitulo("GRACIAS POR CONFIAR EN POLIBANK");
            printf("[Éxito] Su sesión se ha cerrado de forma segura. ¡Que tenga un excelente día!\n");
            break;
        default:
            printf("[Error] Opción inválida. Escriba un número entre 1 y 4.\n");
            break;
        }
    } while (opcion != 4);

    /* Antes de terminar, se cierra la bitácora diaria: si hubo movimientos
       hoy (del cajero o de los propios clientes), se calculan sus totales
       (recursivamente) y se deja un resumen de cierre en el archivo del día. */
    cerrarBitacoraDelDia();

    return 0;
}
