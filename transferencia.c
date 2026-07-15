#include "transferencia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================
   MÓDULO TRANSFERENCIA - TRANSFERENCIA.C
   
   Implementa funcionalidad de transferencias de dinero entre cuentas.
   Un cliente puede enviar dinero a otra cuenta y la aplicación:
   - Verifica que ambas existan
   - Valida que el origen tenga fondos suficientes
   - Actualiza ambos saldos
   - Persiste cambios a disco
   - Genera comprobantes para ambos clientes
   
   ALGORITMO CLAVE: BÚSQUEDA BINARIA
   Las cuentas se mantienen ordenadas por número ascendente. Esto permite
   buscar una cuenta destino en O(log n) en lugar de O(n), crítico cuando
   el banco tiene miles de clientes.
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: buscarCuentaBinaria
   
   Implementa BÚSQUEDA BINARIA clásica para localizar una cuenta por número.
   
   POR QUÉ FUNCIONA:
   - La lista SIEMPRE se mantiene ordenada ascendentemente por numeroCuenta
   - Esto sucede porque al registrar un cliente, asignarNumeroCuentaLuhn()
     genera secuencialmente y insertarClienteOrdenado() lo coloca en orden
   - Con O(log n), buscar entre 100,000 clientes toma solo ~17 iteraciones
   
   ALGORITMO:
   1. Compara el número buscado con el elemento del medio
   2. Si coincide → encontrado, retorna su índice
   3. Si es más pequeño → busca en la mitad izquierda
   4. Si es más grande → busca en la mitad derecha
   5. Si inicio > fin → no existe, retorna -1
   
   PARÁMETROS:
   - lista[]: array de clientes (DEBE estar ordenado ascendentemente)
   - tamano: cantidad de clientes en la lista
   - numeroCuentaDestino: número a buscar
   
   RETORNA:
   - índice en la lista si lo encuentra (>= 0)
   - -1 si la cuenta no existe
   
   COMPLEJIDAD: O(log n) - muy eficiente incluso con millones de clientes
   ====================================================================== */
int buscarCuentaBinaria(const Cliente lista[], int tamano, long long numeroCuentaDestino)
{
    int inicio = 0;
    int fin = tamano - 1;

    /* Itera mientras haya elementos por examinar */
    while (inicio <= fin)
    {
        /* Calcula el punto medio (evita overflow en lenguajes con int de tamaño fijo) */
        int medio = inicio + (fin - inicio) / 2;

        /* CASO 1: Encontró la cuenta en el medio */
        if (lista[medio].numeroCuenta == numeroCuentaDestino)
        {
            return medio;
        }
        /* CASO 2: El número buscado es mayor que el del medio
           → busca en la mitad derecha (valores más grandes) */
        if (lista[medio].numeroCuenta < numeroCuentaDestino)
        {
            inicio = medio + 1;
        }
        /* CASO 3: El número buscado es menor que el del medio
           → busca en la mitad izquierda (valores más pequeños) */
        else
        {
            fin = medio - 1;
        }
    }
    /* Si se sale del bucle sin encontrar, retorna -1 (no existe) */
    return -1;
}

/* ======================================================================
   FUNCIÓN: transferirSaldo
   
   Núcleo de las transferencias: toma dinero de una cuenta y lo envía a otra.
   Realiza múltiples validaciones y actualiza ambos saldos atomicamente.
   
   VALIDACIONES EN ORDEN:
   1. Verifica que posicionOrigen sea válido (dentro del rango)
   2. Lee el número de cuenta destino y lo valida con Luhn
   3. Busca la cuenta destino en la lista (búsqueda binaria)
   4. Verifica que no sea una transferencia a sí mismo
   5. Lee el monto y verifica que sea positivo
   6. Verifica que el origen tenga saldo suficiente
   
   ACTUALIZACIÓN (sin reversión si ambas pasan):
   - Resta monto del origen
   - Suma monto al destino
   - Guarda cambios en clientes.bin
   - Genera estados de cuenta para ambos clientes
   
   POR QUÉ se valida tan estrictamente:
   - Error en cualquier punto → transacción no se realiza
   - Previene dinero fantasma o transferencias inválidas
   - Es como un cajero real que rechaza movimientos sospechosos
   
   POR QUÉ se genera comprobante para AMBOS:
   - El cliente origen: ve "TRANSFERENCIA ENVIADA"
   - El cliente destino: ve "TRANSFERENCIA RECIBIDA"
   - Ambos tienen historial completo de la transacción
   
   PARÁMETROS:
   - lista[], cantidad: lista de clientes (lista[posicionOrigen] es quién envía)
   - posicionOrigen: índice del cliente que va a enviar dinero
   ====================================================================== */
void transferirSaldo(Cliente lista[], int cantidad, int posicionOrigen)
{
    long long numeroCuentaDestino = 0;
    double monto = 0.0;
    int posicionDestino;

    /* VALIDACIÓN 1: El índice de origen está dentro de los límites */
    if (posicionOrigen < 0 || posicionOrigen >= cantidad)
    {
        printf("[Error] No se puede realizar la transferencia en este momento.\n");
        return;
    }

    /* VALIDACIÓN 2 & 3: Lee la cuenta destino y verifica su validez con Luhn */
    if (!leerLongLong("Ingrese el número de cuenta del destinatario: ", &numeroCuentaDestino) ||
        !verificarNumeroCuentaLuhn(numeroCuentaDestino))
    {
        printf("[Error] Ese número de cuenta no es válido.\n");
        return;
    }

    /* VALIDACIÓN 4: Busca la cuenta destino en la lista (búsqueda binaria) */
    posicionDestino = buscarCuentaBinaria(lista, cantidad, numeroCuentaDestino);
    if (posicionDestino < 0)
    {
        printf("[Error] No existe un cliente con ese número de cuenta.\n");
        return;
    }
    
    /* VALIDACIÓN 5: Previene transferencias a la propia cuenta */
    if (posicionDestino == posicionOrigen)
    {
        printf("[Error] No puedes transferirte dinero a ti mismo.\n");
        return;
    }

    /* VALIDACIÓN 6: Lee el monto y verifica que sea positivo */
    if (!leerDoublePositivo("Ingrese el monto a transferir: $", &monto))
    {
        printf("[Error] Monto inválido. Ingrese un número mayor a 0 (ej. 14.55).\n");
        return;
    }
    
    /* VALIDACIÓN 7: Verifica que el origen tenga saldo disponible */
    if (monto > lista[posicionOrigen].saldo)
    {
        printf("[Error] Saldo insuficiente. Su saldo disponible es $%.2f\n", lista[posicionOrigen].saldo);
        return;
    }

    /* ACTUALIZACIÓN: Ambas operaciones se hacen aquí (punto crítico) */
    lista[posicionOrigen].saldo -= monto;       /* Reduce saldo del remitente */
    lista[posicionDestino].saldo += monto;      /* Aumenta saldo del receptor */
    
    /* Persiste cambios INMEDIATAMENTE (nunca dejar en inconsistencia) */
    guardarClientes(lista, cantidad, "clientes.bin");

    /* Genera comprobantes: el remitente ve "ENVIADA", el receptor ve "RECIBIDA" */
    generarEstadoCuenta(&lista[posicionOrigen], "TRANSFERENCIA ENVIADA", monto, lista[posicionOrigen].saldo);
    generarEstadoCuenta(&lista[posicionDestino], "TRANSFERENCIA RECIBIDA", monto, lista[posicionDestino].saldo);

    /* Deja constancia de AMBOS lados del movimiento en la bitácora diaria
       del banco (afecta la auditoría del día completo, no solo a Bóveda) */
    registrarEnBitacoraDiaria("TRANSFERENCIA_ENVIADA", lista[posicionOrigen].numeroCuenta, monto, lista[posicionOrigen].saldo);
    registrarEnBitacoraDiaria("TRANSFERENCIA_RECIBIDA", lista[posicionDestino].numeroCuenta, monto, lista[posicionDestino].saldo);

    /* Confirma la transferencia en pantalla */
    printf("[Éxito] Transferencia de $%.2f realizada con éxito. Su nuevo saldo es $%.2f\n",
           monto, lista[posicionOrigen].saldo);
}

/* ======================================================================
   FUNCIÓN: realizarTransferencia
   
   VARIANTE ALTERNATIVA de transferencia que trabaja con un PUNTERO a Cliente
   en lugar de un índice. Útil cuando solo se conoce la referencia en memoria
   del cliente, no su posición en el arreglo.
   
   PROCESO:
   1. Valida que los parámetros no sean NULL
   2. Busca el índice del cliente referenciado (iteración lineal)
   3. Llama a transferirSaldo() con ese índice
   4. Guarda cambios en el archivo especificado
   
   POR QUÉ EXISTE:
   - Abstracción: el llamador solo tiene el puntero, no el índice
   - En algunos casos, es más conveniente que buscar el índice primero
   - Mantiene encapsulación: la búsqueda está aquí, no en el código que llama
   
   POR QUÉ GUARDAR AL FINAL:
   - transferirSaldo() ya guarda, pero esta función lo hace de nuevo
   - Es redundante pero garantiza persistencia incluso si algo cambia
   - En un sistema real, sería transaccional (guardar una sola vez)
   
   PARÁMETROS:
   - *cuentaOrigen: puntero al Cliente que envía dinero
   - lista[], tamano: la lista completa de clientes
   - *archivoClientes: nombre del archivo donde guardar cambios
   
   RETORNA: 1 si se realizó la transferencia, 0 si hubo error en los parámetros
   ====================================================================== */
int realizarTransferencia(Cliente *cuentaOrigen, Cliente lista[], int tamano, const char *archivoClientes)
{
    int posicionOrigen = -1;
    int i;

    /* Valida que los parámetros no sean NULL (importante para no causar seg fault) */
    if (cuentaOrigen == NULL || archivoClientes == NULL)
    {
        return 0;
    }

    /* Busca el índice del cliente: compara direcciones de memoria
       (si &lista[i] == cuentaOrigen, encontró la posición) */
    for (i = 0; i < tamano; i++)
    {
        if (&lista[i] == cuentaOrigen)
        {
            posicionOrigen = i;
            break;
        }
    }
    
    /* Si no encontró la posición (puntero inválido), retorna error */
    if (posicionOrigen < 0)
    {
        return 0;
    }

    /* Realiza la transferencia normal con el índice encontrado */
    transferirSaldo(lista, tamano, posicionOrigen);
    
    /* Guarda cambios al archivo (redundante pero safe) */
    guardarClientes(lista, tamano, archivoClientes);
    return 1;
}
