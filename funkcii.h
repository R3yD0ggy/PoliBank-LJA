#ifndef FUNKCII_H
#define FUNKCII_H

#include <stddef.h>
#include <stdio.h>

#define MAX_CLIENTES 100

typedef struct FechaNacimiento
{
    int dia;
    int mes;
    int anio;
} FechaNacimiento;

typedef struct Cliente
{
    char nombresCompletos[100];
    char cedula[11];
    char usuario[50];
    char contraseña[50];
    FechaNacimiento fechaNacimiento;
    long long numeroCuenta;
    double saldo;
} Cliente;

int guardarClientes(const Cliente lista[], int cantidad, const char *nombreArchivo);
int cargarClientes(Cliente lista[], int *cantidad, const char *nombreArchivo);
int validarCedula(const char *cedula);
void asignarNumeroCuentaLuhn(long long *numeroCuenta, int cantidad);
int verificarNumeroCuentaLuhn(long long numeroCuenta);

#endif
