#ifndef FUNKCII_H
#define FUNKCII_H

#include <stddef.h>
#include <stdio.h>

#define MAX_CLIENTES 100

typedef struct FechaNacimiento
{
    int dia;
    int mes;
    int etos;
} FechaNacimiento;

typedef struct Cliente
{
    char nombresCompletos[100];
    char cedula[11];
    char usuario[50];
    char contrasena[50];
    FechaNacimiento fechaNacimiento;
    long long numeroCuenta;
    double saldo;
} Cliente;

int guardarClientes(const Cliente lista[], int cantidad, const char *nombreArchivo);
int cargarClientes(Cliente lista[], int *cantidad, const char *nombreArchivo);
void registrarCliente(Cliente lista[], int *cantidad);
int validarCedula(const char *cedula);
void asignarNumeroCuentaLuhn(long long *numeroCuenta, int cantidad);
int verificarNumeroCuentaLuhn(long long numeroCuenta);
int iniciarSesion(Cliente lista[], int cantidad, int *posicion, int *esAdmin);
void mostrarMenuCliente(Cliente *cliente, Cliente lista[], int cantidad, int posicion);
void mostrarMenuAdmin(Cliente lista[], int *cantidad);
void depositarSaldo(Cliente *cliente, double monto);
void retirarSaldo(Cliente *cliente, double monto);
void modificarCliente(Cliente lista[], int cantidad);
int buscarClientePorNumeroCuenta(const Cliente lista[], int cantidad, long long numeroCuenta, int *posicion);
void mostrarDatosCliente(const Cliente *cliente);

#endif
