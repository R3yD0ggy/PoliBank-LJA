#include <stdio.h>
#include "funkcii.h"

void Menu(void)
{
    printf("=== Polibank ===\n");
    printf("1. No tienes una cuenta? Registrate para obtener una.\n");
    printf("2. Ya eres cliente? Inicia sesion para acceder a tu cuenta.\n");
    printf("3. Salir del programa.\n");
    printf("Seleccione una opcion: ");
}

int main(void)
{
    Cliente lista[MAX_CLIENTES];
    int cantidad = 0;
    int opcion = 0;

    if (!cargarClientes(lista, &cantidad, "clientes.bin"))
    {
        cantidad = 0;
    }

    do
    {
        Menu();
        scanf("%d", &opcion);
        while (getchar() != '\n') {}

        switch (opcion)
        {
        case 1:
            registrarCliente(lista, &cantidad);
            guardarClientes(lista, cantidad, "clientes.bin");
            break;

        case 2:
            printf("Inicio de sesion aun no implementado.\n");
            break;

        case 3:
            printf("Saliendo del programa.\n");
            break;

        default:
            printf("Opcion invalida.\n");
            break;
        }
    } while (opcion != 3);
}