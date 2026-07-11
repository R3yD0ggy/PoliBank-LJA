#include "funkcii.h"

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

static void configurarSalidaTexto(void)
{
    setlocale(LC_ALL, "");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

int guardarClientes(const Cliente lista[], int cantidad, const char *nombreArchivo)
{
    configurarSalidaTexto();

    FILE *archivo = fopen(nombreArchivo, "wb");
    if(archivo == NULL)
    {
        printf("No se pudo abrir el archivo para hacer el guardado\n");
        return 0;
    }

    size_t escritos = fwrite(lista, sizeof(Cliente), cantidad, archivo);
    fclose(archivo);

    /*Devolver un booleano para saber si fallo o no, 1 todo salio bien, 0 algo fallo*/
    return (int)escritos == cantidad;
}

int cargarClientes(Cliente lista[], int *cantidad, const char *nombreArchivo)
{
    configurarSalidaTexto();

    FILE *archivo = fopen(nombreArchivo, "rb");
    if (archivo == NULL)
    {
        *cantidad = 0;
        return 0;
    }

    size_t leidos = fread(lista, sizeof(Cliente), MAX_CLIENTES, archivo);
    fclose(archivo);

    *cantidad = (int)leidos;
    return 1;
}

/*Crear la funcion para registrar clientes, se necesita:
Asignar un numero de cuenta por algoritmo Luhn
Verificar si el numero de cedula es valido por algoritmo
verificar si tiene una fecha de nacimiento valida y apta para abrir una cuenta*/

int validarCedula(const char *cedula)
{
    int suma = 0;
    int coeficientes[9] = {2, 1, 2, 1, 2, 1, 2, 1, 2};

    if (cedula == NULL || strlen(cedula) != 10)
    {
        return 0;
    }

    for (int i = 0; i < 10; i++)
    {
        if (!isdigit((unsigned char)cedula[i]))
        {
            return 0;
        }
    }

    int provincia = (cedula[0] - '0') * 10 + (cedula[1] - '0');
    if ((provincia < 1 || provincia > 24) && provincia != 30)
    {
        return 0;
    }

    if (cedula[2] - '0' >= 6)
    {
        return 0;
    }

    for (int i = 0; i < 9; i++)
    {
        int valor = (cedula[i] - '0') * coeficientes[i];
        if (valor > 9)
        {
            valor -= 9;
        }
        suma += valor;
    }

    int digitoVerificador = cedula[9] - '0';
    int decenaSuperior = ((suma + 9) / 10) * 10;
    int resultado = decenaSuperior - suma;

    if (resultado == 10)
    {
        resultado = 0;
    }

    return resultado == digitoVerificador;
}

static int verificarFechas(int dia, int mes, int etos)
{
    // etos = año en griego
    time_t tiempoActual = time(NULL);
    struct tm *fechaActual = localtime(&tiempoActual);

    if (fechaActual == NULL)
    {
        return 0;
    }

    int etosActual = fechaActual->tm_year + 1900;
    int mesActual = fechaActual->tm_mon + 1;
    int diaActual = fechaActual->tm_mday;

    if (etos < 1900 || mes < 1 || mes > 12)
    {
        return 0;
    }

    int diasMes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDias = diasMes[mes - 1];

    if (mes == 2)
    {
        maxDias = ((etos % 400 == 0) || (etos % 4 == 0 && etos % 100 != 0)) ? 29 : 28;
    }

    if (dia < 1 || dia > maxDias)
    {
        return 0;
    }

    if (etos > etosActual)
    {
        return 0;
    }

    if (etos == etosActual)
    {
        if (mes > mesActual)
        {
            return 0;
        }
        if (mes == mesActual && dia > diaActual)
        {
            return 0;
        }
    }

    return (etosActual - etos) > 18 || ((etosActual - etos) == 18 && (mes < mesActual || (mes == mesActual && dia <= diaActual)));
}

/* Algoritmo de números Luhn para los números de cuenta */
static int calcularDigitoVerificadorLuhn(const char *numero)
{
    int suma = 0;
    int longitud = (int)strlen(numero);
    int alternar = 0;

    for (int i = longitud - 1; i >= 0; i--)
    {
        int digito = numero[i] - '0';
        if (alternar % 2 == 1)
        {
            digito *= 2;
            if (digito > 9)
            {
                digito -= 9;
            }
        }

        suma += digito;
        alternar++;
    }

    return (10 - (suma % 10)) % 10;
}

void asignarNumeroCuentaLuhn(long long *numeroCuenta, int cantidad)
{
    char base[32];
    char numeroCompleto[32];
    int digito;
    int semilla = 100000000 + cantidad + 1;

    snprintf(base, sizeof(base), "%d", semilla);
    digito = calcularDigitoVerificadorLuhn(base);
    snprintf(numeroCompleto, sizeof(numeroCompleto), "%s%d", base, digito);
    *numeroCuenta = atoll(numeroCompleto);
}

int verificarNumeroCuentaLuhn(long long numeroCuenta)
{
    char texto[32];
    char base[32];
    int longitud;

    snprintf(texto, sizeof(texto), "%lld", numeroCuenta);
    longitud = (int)strlen(texto);

    if (longitud < 2)
    {
        return 0;
    }

    snprintf(base, sizeof(base), "%.*s", longitud - 1, texto);
    int digitoCalculado = calcularDigitoVerificadorLuhn(base);
    int digitoReal = texto[longitud - 1] - '0';

    return digitoCalculado == digitoReal;
}

void registrarCliente(Cliente lista[], int *cantidad)
{
    char nombre[100];
    char cedula[11];
    int dia;
    int mes;
    int etos;
    char usuario[50];
    char contraseña[50];

    if (*cantidad >= MAX_CLIENTES)
    {
        printf("No se pueden registrar mas clientes.\n");
        return;
    }

    printf("Ingrese nombre completo: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\r\n")] = '\0';

    printf("Ingrese cedula: ");
    fgets(cedula, sizeof(cedula), stdin);
    cedula[strcspn(cedula, "\r\n")] = '\0';

    printf("Ingrese dia de nacimiento: ");
    scanf("%d", &dia);
    printf("Ingrese mes de nacimiento: ");
    scanf("%d", &mes);
    printf("Ingrese anio de nacimiento: ");
    scanf("%d", &etos);

    printf("Ingrese usuario: ");
    fgets(usuario, sizeof(usuario), stdin);
    usuario[strcspn(usuario, "\r\n")] = '\0';

    printf("Ingrese contraseña: ");
    fgets(contraseña, sizeof(contraseña), stdin);
    contraseña[strcspn(contraseña, "\r\n")] = '\0';

    if (!validarCedula(cedula) || !verificarFechas(dia, mes, etos))
    {
        printf("Datos invalidos. No se registro el cliente.\n");
        return;
    }

    snprintf(lista[*cantidad].nombresCompletos, sizeof(lista[*cantidad].nombresCompletos), "%s", nombre);
    snprintf(lista[*cantidad].cedula, sizeof(lista[*cantidad].cedula), "%s", cedula);
    snprintf(lista[*cantidad].usuario, sizeof(lista[*cantidad].usuario), "%s", usuario);
    snprintf(lista[*cantidad].contraseña, sizeof(lista[*cantidad].contraseña), "%s", contraseña);
    lista[*cantidad].fechaNacimiento.dia = dia;
    lista[*cantidad].fechaNacimiento.mes = mes;
    lista[*cantidad].fechaNacimiento.anio = etos;
    asignarNumeroCuentaLuhn(&lista[*cantidad].numeroCuenta, *cantidad);
    lista[*cantidad].saldo = 0.0;
    (*cantidad)++;

    printf("Cliente registrado con exito.\n");
    printf("Numero de cuenta asignado: %lld\n", lista[*cantidad - 1].numeroCuenta);
}

