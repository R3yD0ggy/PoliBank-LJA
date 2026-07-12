#include "transferencia.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int leerDoublePositivoTransferencia(const char *mensaje, double *valor)
{
    char entrada[64];
    char *fin = NULL;

    printf("%s", mensaje);
    if (!fgets(entrada, sizeof(entrada), stdin))
    {
        return 0;
    }

    errno = 0;
    *valor = strtod(entrada, &fin);

    if (errno != 0 || fin == entrada || (*fin != '\n' && *fin != '\r' && *fin != '\0') || *valor <= 0)
    {
        return 0;
    }

    return 1;
}

int buscarCuentaBinaria(const Cliente lista[], int tamano, long long numeroCuentaDestino)
{
    for (int i = 0; i < tamano; i++)
    {
        if (lista[i].numeroCuenta == numeroCuentaDestino)
        {
            return i;
        }
    }

    return -1;
}

void transferirSaldo(Cliente lista[], int cantidad, int posicionOrigen)
{
    char entrada[32];
    char *fin = NULL;
    long long numeroCuentaDestino = 0;
    double monto = 0.0;
    int posicionDestino = -1;

    if (posicionOrigen < 0 || posicionOrigen >= cantidad)
    {
        printf("No se puede realizar la transferencia en este momento.\n");
        return;
    }

    printf("Ingrese el número de cuenta del destinatario: ");
    if (!fgets(entrada, sizeof(entrada), stdin))
    {
        printf("No se pudo leer el número de cuenta.\n");
        return;
    }
    entrada[strcspn(entrada, "\r\n")] = '\0';

    errno = 0;
    numeroCuentaDestino = strtoll(entrada, &fin, 10);
    if (errno != 0 || fin == entrada || (*fin != '\0'))
    {
        printf("Número de cuenta inválido.\n");
        return;
    }

    if (!verificarNumeroCuentaLuhn(numeroCuentaDestino))
    {
        printf("No existe un cliente con ese número de cuenta.\n");
        return;
    }

    posicionDestino = buscarCuentaBinaria(lista, cantidad, numeroCuentaDestino);
    if (posicionDestino < 0)
    {
        printf("No existe un cliente con ese número de cuenta.\n");
        return;
    }

    if (posicionDestino == posicionOrigen)
    {
        printf("No puedes transferirte dinero a ti mismo.\n");
        return;
    }

    if (!leerDoublePositivoTransferencia("Ingrese el monto a transferir: ", &monto))
    {
        printf("Monto inválido.\n");
        return;
    }

    if (monto > lista[posicionOrigen].saldo)
    {
        printf("Saldo insuficiente para realizar la transferencia.\n");
        return;
    }

    lista[posicionOrigen].saldo -= monto;
    lista[posicionDestino].saldo += monto;
    guardarClientes(lista, cantidad, "clientes.bin");
    printf("Transferencia realizada con éxito.\n");
}

int realizarTransferencia(Cliente *cuentaOrigen, Cliente lista[], int tamano, const char *archivoClientes)
{
    int posicionOrigen = -1;

    if (cuentaOrigen == NULL || archivoClientes == NULL)
    {
        return 0;
    }

    for (int i = 0; i < tamano; i++)
    {
        if (&lista[i] == cuentaOrigen)
        {
            posicionOrigen = i;
            break;
        }
    }

    if (posicionOrigen < 0)
    {
        return 0;
    }

    transferirSaldo(lista, tamano, posicionOrigen);
    guardarClientes(lista, tamano, archivoClientes);
    return 1;
}


