#include "funkcii.h"
#include "boveda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ======================================================================
   GENERADOR DE USUARIOS SEMILLA (solo para pruebas / demo)

   Este programa NO es parte de Polibank: es una herramienta aparte que
   usa las funciones REALES del proyecto (validarCedula, asignarNumeroCuentaLuhn,
   guardarClientes) para crear un archivo clientes.bin con 12 clientes ya
   cargados, listos para usar en la presentación, sin tener que registrarlos
   uno por uno a mano.
   ====================================================================== */

/* Genera una cédula ecuatoriana válida usando la MISMA función validarCedula
   del proyecto, probando dígitos verificadores hasta encontrar uno válido. */
static void generarCedulaValida(char *destino, int provincia, int tipoPersona, int semilla)
{
    char base[10];
    char candidato[11];
    int digito;

    snprintf(base, sizeof(base), "%02d%d%06d", provincia, tipoPersona, semilla % 1000000);

    for (digito = 0; digito <= 9; digito++)
    {
        snprintf(candidato, sizeof(candidato), "%s%d", base, digito);
        if (validarCedula(candidato))
        {
            strcpy(destino, candidato);
            return;
        }
    }
    /* No debería pasar (siempre existe 1 dígito verificador válido),
       pero por seguridad dejamos una cédula de respaldo. */
    strcpy(destino, "1710034065");
}

int main(void)
{
    Cliente lista[MAX_CLIENTES];
    int cantidad = 0;
    int i;

    /* Datos base de los 12 clientes semilla: nombre, provincia cédula,
       tipo de persona, día/mes/año nacimiento, usuario, clave y saldo inicial. */
    struct
    {
        const char *nombre;
        int provincia;
        int tipoPersona;
        int dia, mes, anio;
        const char *usuario;
        const char *clave;
        double saldo;
    } datos[12] = {
        {"Maria Fernanda Lopez Cevallos",      17, 0, 12, 3, 1998, "mlopez",    "Clave123",  1250.75},
        {"Juan Carlos Perez Andrade",          9,  1, 25, 7, 1990, "jperez",    "Clave123",  3400.00},
        {"Ana Gabriela Torres Vega",           1,  2, 5,  11, 2000, "atorres",  "Clave123",   500.50},
        {"Luis Alberto Ramirez Chavez",        17, 0, 30, 1, 1985, "lramirez",  "Clave123",  8900.20},
        {"Sofia Valentina Morales Rios",       4,  1, 18, 9, 1999, "smorales",  "Clave123",   150.00},
        {"Carlos Eduardo Sanchez Vinueza",     13, 2, 22, 5, 1995, "csanchez",  "Clave123",  2200.00},
        {"Daniela Alejandra Castro Ponce",     6,  0, 8,  12, 2003, "dcastro",  "Clave123",    75.25},
        {"Diego Fernando Ortiz Salazar",       17, 1, 14, 4, 1988, "dortiz",    "Clave123", 15000.00},
        {"Camila Nicole Vasquez Herrera",      10, 2, 27, 6, 1997, "cvasquez",  "Clave123",   980.40},
        {"Andres Sebastian Guaman Cusco",      1,  0, 3,  10, 1992, "aguaman",  "Clave123",  4650.60},
        {"Valentina Isabel Jaramillo Nunez",   17, 1, 20, 2, 2001, "vjaramillo","Clave123",   320.10},
        {"Pedro Pablo Cordova Villacis",       9,  0, 16, 8, 1980, "pcordova",  "Clave123", 12750.90}
    };

    srand(2026); /* semilla fija: mismos números de cuenta cada vez que se corre */

    for (i = 0; i < 12; i++)
    {
        Cliente nuevo;
        memset(&nuevo, 0, sizeof(nuevo));

        strncpy(nuevo.nombresCompletos, datos[i].nombre, sizeof(nuevo.nombresCompletos) - 1);
        generarCedulaValida(nuevo.cedula, datos[i].provincia, datos[i].tipoPersona, 1000 + i * 137);
        strncpy(nuevo.usuario, datos[i].usuario, sizeof(nuevo.usuario) - 1);
        strncpy(nuevo.contrasena, datos[i].clave, sizeof(nuevo.contrasena) - 1);
        nuevo.fechaNacimiento.dia = datos[i].dia;
        nuevo.fechaNacimiento.mes = datos[i].mes;
        nuevo.fechaNacimiento.anio = datos[i].anio;
        nuevo.saldo = datos[i].saldo;

        asignarNumeroCuentaLuhn(&nuevo.numeroCuenta, lista, cantidad, &nuevo.fechaNacimiento);

        /* Inserta manteniendo la lista ordenada por numeroCuenta ascendente
           (mismo invariante que usa insertarClienteOrdenado en funkcii.c),
           para que la búsqueda binaria del programa real funcione bien. */
        {
            int pos = cantidad;
            while (pos > 0 && lista[pos - 1].numeroCuenta > nuevo.numeroCuenta)
            {
                lista[pos] = lista[pos - 1];
                pos--;
            }
            lista[pos] = nuevo;
            cantidad++;
        }
    }

    guardarClientes(lista, cantidad, "clientes.bin");

    /* Imprime una tabla de referencia para copiar en el documento de pruebas */
    printf("cuenta,usuario,clave,cedula,nombre,saldo\n");
    for (i = 0; i < cantidad; i++)
    {
        printf("%lld,%s,%s,%s,%s,%.2f\n",
               lista[i].numeroCuenta, lista[i].usuario, lista[i].contrasena,
               lista[i].cedula, lista[i].nombresCompletos, lista[i].saldo);
    }

    return 0;
}
