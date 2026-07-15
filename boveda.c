#include "boveda.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ======================================================================
   MÓDULO BÓVEDA - BOVEDA.C

   Este módulo implementa la funcionalidad de la bóveda del banco:
   1. Permite a un cajero autenticado (usuario "cajero1", clave
      "boveda123") registrar depósitos y retiros de efectivo a nombre
      de cualquier cliente, dentro del horario de atención del banco.
   2. Al cerrar el programa, lee la bitácora diaria de TODOS los
      movimientos del banco (los del cajero y los que hace cada cliente
      por su cuenta) y calcula RECURSIVAMENTE los totales de ingresos y
      egresos del día, dejando un resumen de cierre en el mismo archivo.

   El acceso a este módulo es independiente del administrador: se entra
   desde el menú principal (main), no desde el menú de administrador,
   para que el admin nunca tenga forma de mover el dinero de un cliente.
   ====================================================================== */

#define CAJERO_USUARIO "cajero1"   /* Credencial fija de cajero (educativo) */
#define CAJERO_CLAVE "boveda123"   /* Clave fija de cajero (educativo) */
#define MAX_MOVIMIENTOS_DIA 500    /* Máximo de líneas que se leen de la bitácora al cerrar */

/* ======================================================================
   ESTRUCTURA: MovimientoLeido

   Representa UNA línea ya leída e interpretada de la bitácora diaria
   (bitacora_DDMMAAAA.txt). Solo guarda lo necesario para sumar totales:
   el tipo de operación (para saber si es ingreso o egreso) y el monto.
   ====================================================================== */
typedef struct MovimientoLeido
{
    char tipo[32];
    double monto;
} MovimientoLeido;

/* ======================================================================
   FUNCIÓN: esTipoDeIngreso

   Clasifica un tipo de movimiento como INGRESO (entra dinero al banco)
   o EGRESO (sale dinero del banco), para poder sumar cada bolsa por
   separado en el cierre del día.

   INGRESOS: DEPOSITO, DEPOSITO_BOVEDA, TRANSFERENCIA_RECIBIDA
   EGRESOS:  RETIRO, RETIRO_BOVEDA, TRANSFERENCIA_ENVIADA
   ====================================================================== */
static int esTipoDeIngreso(const char *tipo)
{
    return strcmp(tipo, "DEPOSITO") == 0 ||
           strcmp(tipo, "DEPOSITO_BOVEDA") == 0 ||
           strcmp(tipo, "TRANSFERENCIA_RECIBIDA") == 0;
}

/* ======================================================================
   FUNCIÓN: sumarMontosRecursivo

   Suma RECURSIVAMENTE los montos de todos los movimientos leídos de la
   bitácora que sean del "lado" pedido (ingreso o egreso).

   ESTRATEGIA RECURSIVA (igual que en la versión anterior del proyecto):
   - Caso base: si cantidad <= 0, devuelve 0 (fin de la recursión)
   - Caso recursivo: suma el movimiento actual (si coincide con el lado
     pedido) + el resultado de llamar a la función con cantidad-1

   PARÁMETROS:
   - movimientos[]: arreglo de movimientos ya leídos de la bitácora
   - cantidad: cuántos movimientos hay que procesar
   - buscarIngresos: 1 para sumar solo ingresos, 0 para sumar solo egresos

   RETORNA: la suma total de los montos que coinciden con el lado pedido
   ====================================================================== */
static double sumarMontosRecursivo(const MovimientoLeido movimientos[], int cantidad, int buscarIngresos)
{
    double actual;

    if (cantidad <= 0)
    {
        return 0.0;
    }

    actual = (esTipoDeIngreso(movimientos[cantidad - 1].tipo) == buscarIngresos)
                 ? movimientos[cantidad - 1].monto
                 : 0.0;

    return actual + sumarMontosRecursivo(movimientos, cantidad - 1, buscarIngresos);
}

/* ======================================================================
   FUNCIÓN: autenticarCajero

   Implementa el control de acceso al módulo de bóveda. Solo un cajero
   con credenciales fijas puede acceder a registrar depósitos/retiros.

   RETORNA: 1 si las credenciales son correctas, 0 si son incorrectas
   ====================================================================== */
static int autenticarCajero(void)
{
    char usuario[50];
    char clave[50];

    imprimirTitulo("ACCESO A BÓVEDA (CAJERO)");
    printf("Usuario cajero: ");
    leerLinea(usuario, sizeof(usuario));
    printf("Clave cajero: ");
    leerLinea(clave, sizeof(clave));

    return strcmp(usuario, CAJERO_USUARIO) == 0 && strcmp(clave, CAJERO_CLAVE) == 0;
}

/* ======================================================================
   FUNCIÓN: registrarMovimiento

   Maneja el registro de UN depósito o retiro hecho por el cajero a
   nombre de un cliente. Valida la cuenta (Luhn + búsqueda binaria),
   valida el monto, actualiza el saldo, persiste el cambio, genera el
   comprobante del cliente y deja constancia en la bitácora diaria.

   PARÁMETROS:
   - lista[], cantidad: lista de clientes del banco
   - esDeposito: 1 para depositar, 0 para retirar
   RETORNA: 1 si el movimiento se registró exitosamente, 0 si algo falló
   ====================================================================== */
static int registrarMovimiento(Cliente lista[], int cantidad, int esDeposito)
{
    long long numeroCuenta = 0;
    int posicion = -1;
    double monto = 0.0;

    if (!leerLongLong("Número de cuenta del cliente: ", &numeroCuenta) ||
        !verificarNumeroCuentaLuhn(numeroCuenta) ||
        !buscarClientePorNumeroCuenta(lista, cantidad, numeroCuenta, &posicion))
    {
        printf("[Error] No existe un cliente con ese número de cuenta. Verifíquelo e intente de nuevo.\n");
        return 0;
    }

    if (!leerDoublePositivo("Monto: $", &monto))
    {
        printf("[Error] Monto inválido. Ingrese un número mayor a 0 (ej. 14.55).\n");
        return 0;
    }

    if (esDeposito)
    {
        depositarSaldo(&lista[posicion], monto);
    }
    else
    {
        if (monto > lista[posicion].saldo)
        {
            printf("[Error] Saldo insuficiente. El cliente dispone de $%.2f\n", lista[posicion].saldo);
            return 0;
        }
        retirarSaldo(&lista[posicion], monto);
    }

    /* Persiste el cambio de saldo a disco (CRÍTICO: no perder datos) */
    guardarClientes(lista, cantidad, "clientes.bin");

    /* Comprobante individual del cliente + línea en la bitácora del día */
    generarEstadoCuenta(&lista[posicion], esDeposito ? "DEPÓSITO (BÓVEDA)" : "RETIRO (BÓVEDA)", monto, lista[posicion].saldo);
    registrarEnBitacoraDiaria(esDeposito ? "DEPOSITO_BOVEDA" : "RETIRO_BOVEDA", numeroCuenta, monto, lista[posicion].saldo);

    printf("[Éxito] Movimiento registrado correctamente. Nuevo saldo del cliente: $%.2f\n", lista[posicion].saldo);
    return 1;
}

void iniciarModuloBoveda(Cliente lista[], int cantidad)
{
    int opcion = 0;

    /* El módulo Bóveda solo abre dentro del horario de atención. */
    if (!dentroDeHorarioBancario())
    {
        printf("[Error] Bóveda está cerrada fuera del horario de atención (08:00 a 20:00).\n");
        return;
    }

    if (!autenticarCajero())
    {
        printf("[Error] Acceso denegado. Usuario o clave de cajero incorrectos.\n");
        return;
    }
    printf("\n[Éxito] Acceso concedido. Bienvenido/a, cajero de Polibank.\n");

    do
    {
        imprimirTitulo("MÓDULO BÓVEDA");
        printf("1. Registrar depósito a cliente\n2. Registrar retiro de cliente\n3. Salir de Bóveda\n");

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida.\n");
            continue;
        }

        if (opcion == 1 || opcion == 2)
        {
            /* Cada movimiento revalida el horario: si el cajero se queda
               en el módulo hasta después del cierre, no debe poder
               seguir registrando movimientos. */
            if (!dentroDeHorarioBancario())
            {
                printf("[Error] El horario de atención terminó. No se pueden registrar más movimientos hoy.\n");
                continue;
            }
            registrarMovimiento(lista, cantidad, opcion == 1);
        }
        else if (opcion != 3)
        {
            printf("[Error] Opción inválida.\n");
        }
    } while (opcion != 3);

    printf("Saliendo de Bóveda. Gracias por su trabajo, cajero de Polibank.\n");
}

void cerrarBitacoraDelDia(void)
{
    char nombreArchivo[64];
    FILE *archivo;
    char linea[256];
    MovimientoLeido movimientos[MAX_MOVIMIENTOS_DIA];
    int totalMovimientos = 0;
    double totalIngresos, totalEgresos;
    time_t ahora;
    struct tm *fechaHora;

    obtenerNombreBitacoraHoy(nombreArchivo, sizeof(nombreArchivo));

    archivo = fopen(nombreArchivo, "r");
    if (archivo == NULL)
    {
        printf("No se registraron movimientos bancarios el día de hoy.\n");
        return;
    }

    /* Lee cada línea de la bitácora. El formato escrito por
       registrarEnBitacoraDiaria es: HH:MM|JORNADA|TIPO|CUENTA|MONTO|SALDO
       Las líneas de un cierre anterior (si el programa ya se cerró y se
       reabrió hoy) no calzan con este formato y sscanf simplemente no
       las cuenta, así que no se duplican en la suma. */
    while (fgets(linea, sizeof(linea), archivo) != NULL && totalMovimientos < MAX_MOVIMIENTOS_DIA)
    {
        int hh, mm;
        char jornada[16];
        char tipo[32];
        long long cuenta;
        double monto, saldoResultante;

        int leidos = sscanf(linea, "%d:%d|%15[^|]|%31[^|]|%lld|%lf|%lf",
                             &hh, &mm, jornada, tipo, &cuenta, &monto, &saldoResultante);
        if (leidos == 7)
        {
            strncpy(movimientos[totalMovimientos].tipo, tipo, sizeof(movimientos[totalMovimientos].tipo) - 1);
            movimientos[totalMovimientos].tipo[sizeof(movimientos[totalMovimientos].tipo) - 1] = '\0';
            movimientos[totalMovimientos].monto = monto;
            totalMovimientos++;
        }
    }
    fclose(archivo);

    if (totalMovimientos == 0)
    {
        printf("No se registraron movimientos bancarios el día de hoy.\n");
        return;
    }

    /* Totales calculados con RECURSIÓN sobre todo lo leído del día. */
    totalIngresos = sumarMontosRecursivo(movimientos, totalMovimientos, 1);
    totalEgresos = sumarMontosRecursivo(movimientos, totalMovimientos, 0);

    archivo = fopen(nombreArchivo, "a");
    if (archivo == NULL)
    {
        printf("[Error] No se pudo escribir el resumen de cierre en la bitácora.\n");
        return;
    }

    ahora = time(NULL);
    fechaHora = localtime(&ahora);

    fprintf(archivo, "======= CIERRE DEL DÍA =======\n");
    if (fechaHora != NULL)
    {
        fprintf(archivo, "Fecha y hora de cierre: %02d/%02d/%04d %02d:%02d\n",
                fechaHora->tm_mday, fechaHora->tm_mon + 1, fechaHora->tm_year + 1900,
                fechaHora->tm_hour, fechaHora->tm_min);
    }
    fprintf(archivo, "Movimientos del día (desde el primero hasta el último): %d\n", totalMovimientos);
    fprintf(archivo, "Total ingresos: $%.2f\n", totalIngresos);
    fprintf(archivo, "Total egresos: $%.2f\n", totalEgresos);
    fprintf(archivo, "Balance neto del día: $%.2f\n", totalIngresos - totalEgresos);
    fprintf(archivo, "===============================\n\n");
    fclose(archivo);

    printf("\n[Éxito] Bitácora del día cerrada en %s\n", nombreArchivo);
    printf("Movimientos: %d | Ingresos: $%.2f | Egresos: $%.2f | Neto: $%.2f\n",
           totalMovimientos, totalIngresos, totalEgresos, totalIngresos - totalEgresos);
}
