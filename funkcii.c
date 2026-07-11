#include "funkcii.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

static void configurarSalidaTexto(void)
{
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

}

static void limpiarBufferEntrada(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

int guardarClientes(const Cliente lista[], int cantidad, const char *nombreArchivo)
{
    configurarSalidaTexto();

    FILE *archivo = fopen(nombreArchivo, "wb");
    if(archivo == NULL)
    {
        printf("No se pudo abrir el archivo para guardar los datos.\n");
        return 0;
    }

    size_t escritos = fwrite(lista, sizeof(Cliente), cantidad, archivo);
    fclose(archivo);

    /* Devolver un booleano para saber si falló o no; 1 = todo salió bien, 0 = algo falló. */
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

/* Crear la función para registrar clientes; se necesita:
Asignar un número de cuenta por algoritmo Luhn
Verificar si el número de cédula es válido por algoritmo
Verificar si tiene una fecha de nacimiento válida y apta para abrir una cuenta. */

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
    SetConsoleOutputCP(CP_UTF8);
    char texto[32];
    char base[32];
    int longitud;

    snprintf(texto, sizeof(texto), "%lld", (long long)numeroCuenta);
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
    int dia = 0;
    int mes = 0;
    int etos = 0;
    char usuario[50];
    char contrasena[50];

    if (*cantidad >= MAX_CLIENTES)
    {
        printf("No se pueden registrar más clientes.\n");
        return;
    }

    printf("Ingrese nombre completo: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\r\n")] = '\0';

    while (1)
    {
        printf("Ingrese cédula: ");
        if (!fgets(cedula, sizeof(cedula), stdin))
        {
            cedula[0] = '\0';
        }
        cedula[strcspn(cedula, "\r\n")] = '\0';

        if (validarCedula(cedula))
        {
            break;
        }

        printf("Cédula inválida. Ingrese una cédula válida de 10 dígitos.\n");
    }

    while (1)
    {
        printf("Ingrese día de nacimiento: ");
        if (scanf("%d", &dia) != 1)
        {
            limpiarBufferEntrada();
            printf("Día inválido. Ingrese un número entero.\n");
            continue;
        }

        printf("Ingrese mes de nacimiento: ");
        if (scanf("%d", &mes) != 1)
        {
            limpiarBufferEntrada();
            printf("Mes inválido. Ingrese un número entero.\n");
            continue;
        }

        printf("Ingrese año de nacimiento: ");
        if (scanf("%d", &etos) != 1)
        {
            limpiarBufferEntrada();
            printf("Año inválido. Ingrese un número entero.\n");
            continue;
        }
        limpiarBufferEntrada();

        time_t tiempoActual = time(NULL);
        struct tm *fechaActual = localtime(&tiempoActual);

        if (fechaActual == NULL)
        {
            printf("No se pudo obtener la fecha actual. Intente nuevamente.\n");
            continue;
        }

        int etosActual = fechaActual->tm_year + 1900;
        int mesActual = fechaActual->tm_mon + 1;
        int diaActual = fechaActual->tm_mday;

        if (mes < 1 || mes > 12)
        {
            printf("Mes inválido. Ingrese un mes entre 1 y 12.\n");
            continue;
        }

        int diasMes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int maxDias = diasMes[mes - 1];

        if (mes == 2)
        {
            maxDias = ((etos % 400 == 0) || (etos % 4 == 0 && etos % 100 != 0)) ? 29 : 28;
        }

        if (dia < 1 || dia > maxDias)
        {
            printf("Día inválido para ese mes. Ingrese un día válido.\n");
            continue;
        }

        if (etos < 1900 || etos > etosActual)
        {
            printf("Año inválido. Ingrese un año entre 1900 y %d.\n", etosActual);
            continue;
        }

        if (etos > etosActual || (etos == etosActual && (mes > mesActual || (mes == mesActual && dia > diaActual))))
        {
            printf("La fecha no puede ser futura. Ingrese una fecha válida.\n");
            continue;
        }

        if ((etosActual - etos) < 18 || ((etosActual - etos) == 18 && (mes > mesActual || (mes == mesActual && dia > diaActual))))
        {
            printf("Debes ser mayor de 18 años para registrarte.\n");
            continue;
        }

        break;
    }

    printf("Ingrese usuario: ");
    fgets(usuario, sizeof(usuario), stdin);
    usuario[strcspn(usuario, "\r\n")] = '\0';

    printf("Ingrese contraseña: ");
    fgets(contrasena, sizeof(contrasena), stdin);
    contrasena[strcspn(contrasena, "\r\n")] = '\0';

    snprintf(lista[*cantidad].nombresCompletos, sizeof(lista[*cantidad].nombresCompletos), "%s", nombre);
    snprintf(lista[*cantidad].cedula, sizeof(lista[*cantidad].cedula), "%s", cedula);
    snprintf(lista[*cantidad].usuario, sizeof(lista[*cantidad].usuario), "%s", usuario);
    snprintf(lista[*cantidad].contrasena, sizeof(lista[*cantidad].contrasena), "%s", contrasena);
    lista[*cantidad].fechaNacimiento.dia = dia;
    lista[*cantidad].fechaNacimiento.mes = mes;
    lista[*cantidad].fechaNacimiento.etos = etos;
    asignarNumeroCuentaLuhn(&lista[*cantidad].numeroCuenta, *cantidad);
    lista[*cantidad].saldo = 0.0;
    (*cantidad)++;

    printf("Cliente registrado con éxito.\n");
    printf("Número de cuenta asignado: %lld\n", lista[*cantidad - 1].numeroCuenta);
}

static int leerDoublePositivo(const char *mensaje, double *valor)
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

int buscarClientePorNumeroCuenta(const Cliente lista[], int cantidad, long long numeroCuenta, int *posicion)
{
    if (posicion == NULL)
    {
        return 0;
    }

    for (int i = 0; i < cantidad; i++)
    {
        if (lista[i].numeroCuenta == numeroCuenta)
        {
            *posicion = i;
            return 1;
        }
    }

    *posicion = -1;
    return 0;
}

void mostrarDatosCliente(const Cliente *cliente)
{
    printf("\n=== DATOS DEL CLIENTE ===\n");
    printf("Nombre: %s\n", cliente->nombresCompletos);
    printf("Cédula: %s\n", cliente->cedula);
    printf("Fecha de nacimiento: %d/%d/%d\n", cliente->fechaNacimiento.dia, cliente->fechaNacimiento.mes, cliente->fechaNacimiento.etos);
}

void depositarSaldo(Cliente *cliente, double monto)
{
    cliente->saldo += monto;
}

void retirarSaldo(Cliente *cliente, double monto)
{
    if (monto > cliente->saldo)
    {
        printf("Saldo insuficiente para realizar el retiro.\n");
        return;
    }

    cliente->saldo -= monto;
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

    if (!buscarClientePorNumeroCuenta(lista, cantidad, numeroCuentaDestino, &posicionDestino))
    {
        printf("No existe un cliente con ese número de cuenta.\n");
        return;
    }

    if (posicionDestino == posicionOrigen)
    {
        printf("No puedes transferirte dinero a ti mismo.\n");
        return;
    }

    if (!leerDoublePositivo("Ingrese el monto a transferir: ", &monto))
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

void modificarCliente(Cliente lista[], int cantidad)
{
    char entrada[32];
    char *fin = NULL;
    long long numeroCuenta = 0;
    int posicion = -1;
    int opcion = 0;

    printf("Ingrese el número de cuenta del cliente a modificar: ");
    if (!fgets(entrada, sizeof(entrada), stdin))
    {
        printf("No se pudo leer el número de cuenta.\n");
        return;
    }
    entrada[strcspn(entrada, "\r\n")] = '\0';

    errno = 0;
    numeroCuenta = strtoll(entrada, &fin, 10);
    if (errno != 0 || fin == entrada || (*fin != '\0'))
    {
        printf("Número de cuenta inválido.\n");
        return;
    }

    if (!verificarNumeroCuentaLuhn(numeroCuenta) || !buscarClientePorNumeroCuenta(lista, cantidad, numeroCuenta, &posicion))
    {
        printf("No existe un cliente con ese número de cuenta.\n");
        return;
    }

    printf("\nCliente encontrado: %s\n", lista[posicion].nombresCompletos);

    while (1)
    {
        printf("\n=== MODIFICAR CLIENTE ===\n");
        printf("1. Cambiar usuario\n");
        printf("2. Cambiar contraseña\n");
        printf("3. Aumentar saldo\n");
        printf("4. Volver\n");
        printf("Seleccione una opción: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBufferEntrada();
            printf("Opción inválida.\n");
            continue;
        }
        limpiarBufferEntrada();

        switch (opcion)
        {
        case 1:
        {
            char nuevoUsuario[50];
            printf("Ingrese el nuevo usuario: ");
            if (!fgets(nuevoUsuario, sizeof(nuevoUsuario), stdin))
            {
                printf("No se pudo leer el usuario.\n");
                break;
            }
            nuevoUsuario[strcspn(nuevoUsuario, "\r\n")] = '\0';
            snprintf(lista[posicion].usuario, sizeof(lista[posicion].usuario), "%s", nuevoUsuario);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("Usuario actualizado.\n");
            break;
        }
        case 2:
        {
            char nuevaContrasena[50];
            printf("Ingrese la nueva contraseña: ");
            if (!fgets(nuevaContrasena, sizeof(nuevaContrasena), stdin))
            {
                printf("No se pudo leer la contraseña.\n");
                break;
            }
            nuevaContrasena[strcspn(nuevaContrasena, "\r\n")] = '\0';
            snprintf(lista[posicion].contrasena, sizeof(lista[posicion].contrasena), "%s", nuevaContrasena);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("Contraseña actualizada.\n");
            break;
        }
        case 3:
        {
            double monto = 0.0;
            if (!leerDoublePositivo("Ingrese el monto a agregar al saldo: ", &monto))
            {
                printf("Monto inválido.\n");
                break;
            }
            lista[posicion].saldo += monto;
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("Saldo actualizado.\n");
            break;
        }
        case 4:
            return;
        default:
            printf("Opción inválida.\n");
            break;
        }
    }
}

void mostrarMenuCliente(Cliente *cliente, Cliente lista[], int cantidad, int posicion)
{
    (void)posicion;
    int opcion = 0;

    while (1)
    {
        printf("\n=== MENÚ CLIENTE ===\n");
        printf("1. Consultar saldo\n");
        printf("2. Depositar dinero\n");
        printf("3. Retirar dinero\n");
        printf("4. Transferir dinero\n");
        printf("5. Cerrar sesión\n");
        printf("Seleccione una opción: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBufferEntrada();
            printf("Opción inválida.\n");
            continue;
        }
        limpiarBufferEntrada();

        switch (opcion)
        {
        case 1:
            mostrarDatosCliente(cliente);
            printf("Saldo disponible: $%.2f\n", cliente->saldo);
            break;
        case 2:
        {
            double monto = 0.0;
            if (!leerDoublePositivo("Ingrese el monto a depositar: ", &monto))
            {
                printf("Monto inválido.\n");
                break;
            }
            depositarSaldo(cliente, monto);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("Depósito realizado con éxito.\n");
            break;
        }
        case 3:
        {
            double monto = 0.0;
            if (!leerDoublePositivo("Ingrese el monto a retirar: ", &monto))
            {
                printf("Monto inválido.\n");
                break;
            }
            retirarSaldo(cliente, monto);
            guardarClientes(lista, cantidad, "clientes.bin");
            printf("Retiro realizado con éxito.\n");
            break;
        }
        case 4:
            transferirSaldo(lista, cantidad, posicion);
            break;
        case 5:
            printf("Sesión cerrada.\n");
            return;
        default:
            printf("Opción inválida.\n");
            break;
        }
    }
}

void mostrarMenuAdmin(Cliente lista[], int *cantidad)
{
    int opcion = 0;

    while (1)
    {
        printf("\n=== MENÚ ADMINISTRADOR ===\n");
        printf("1. Revisar clientes\n");
        printf("2. Registrar otro cliente\n");
        printf("3. Modificar cliente\n");
        printf("4. Salir\n");
        printf("Seleccione una opción: ");

        if (scanf("%d", &opcion) != 1)
        {
            limpiarBufferEntrada();
            printf("Opción inválida.\n");
            continue;
        }
        limpiarBufferEntrada();

        switch (opcion)
        {
        case 1:
            printf("\n=== CLIENTES REGISTRADOS ===\n");
            if (*cantidad <= 0)
            {
                printf("No hay clientes registrados.\n");
                break;
            }
            for (int i = 0; i < *cantidad; i++)
            {
                printf("%d. %s | Cédula: %s | Usuario: %s | Saldo: $%.2f\n",
                       i + 1,
                       lista[i].nombresCompletos,
                       lista[i].cedula,
                       lista[i].usuario,
                       lista[i].saldo);
            }
            break;
        case 2:
            registrarCliente(lista, cantidad);
            guardarClientes(lista, *cantidad, "clientes.bin");
            break;
        case 3:
            modificarCliente(lista, *cantidad);
            break;
        case 4:
            printf("Saliendo del menú administrador.\n");
            return;
        default:
            printf("Opción inválida.\n");
            break;
        }
    }
}

int iniciarSesion(Cliente lista[], int cantidad, int *posicion, int *esAdmin)
{
    configurarSalidaTexto();

    char usuarioIngresado[50];
    char contrasenaIngresada[50];

    printf("\n=== INICIO DE SESIÓN ===\n");
    printf("Ingrese su usuario: ");
    fgets(usuarioIngresado, sizeof(usuarioIngresado), stdin);
    usuarioIngresado[strcspn(usuarioIngresado, "\r\n")] = '\0';

    printf("Ingrese su contraseña: ");
    fgets(contrasenaIngresada, sizeof(contrasenaIngresada), stdin);
    contrasenaIngresada[strcspn(contrasenaIngresada, "\r\n")] = '\0';

    if (strcmp(usuarioIngresado, "R3yD0ggy") == 0 && strcmp(contrasenaIngresada, "admin123") == 0)
    {
        *posicion = -1;
        *esAdmin = 1;
        printf("\n[ÉXITO] Bienvenido administrador.\n");
        return 1;
    }

    for (int i = 0; i < cantidad; i++)
    {
        if (strcmp(lista[i].usuario, usuarioIngresado) == 0 && strcmp(lista[i].contrasena, contrasenaIngresada) == 0)
        {
            *posicion = i;
            *esAdmin = 0;
            printf("\n[ÉXITO] Bienvenido/a, %s!\n", lista[i].nombresCompletos);
            printf("Número de cuenta: %lld | Saldo disponible: $%.2f\n", lista[i].numeroCuenta, lista[i].saldo);
            return 1;
        }
    }

    *posicion = -1;
    *esAdmin = 0;
    printf("\n[ERROR] Usuario o contraseña incorrectos.\n");
    return 0;
}