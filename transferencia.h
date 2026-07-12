#ifndef TRANSFERENCIA_H
#define TRANSFERENCIA_H

#include "funkcii.h"

int buscarCuentaBinaria(const Cliente lista[], int tamano, long long numeroCuentaDestino);
void transferirSaldo(Cliente lista[], int cantidad, int posicionOrigen);
int realizarTransferencia(Cliente *cuentaOrigen, Cliente lista[], int tamano, const char *archivoClientes);

#endif