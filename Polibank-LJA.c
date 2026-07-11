#include <stdio.h>
#include "funkcii.h"

void Menu(void)
{
    printf("=== Polibank ===\n");
    printf("1. No tienes una cuenta? Registrate para obtener una.\n");
    printf("2.Ya eres cliente? Inicia sesion para acceder a tu cuenta.\n");
    printf("3.Salir del programa.\n");
    printf("Seleccione una opcion: ");
}

int main(void)
{
    Cliente lista[MAX_CLIENTES];
    int cantidad = 0;
    int opcion = 0;

    registrarCliente(lista, &cantidad);

    do
    {
        Menu();
        scanf("%d", &opcion);

        switch (opcion)
        {
        case 1:
            registrarCliente(lista, &cantidad);
            break;

        case 2:
            /*Falta implementar un inicio de sesion buscando en el archivo binario*/
            /*IniciarSesion
            Eres admin
            ver clientes modicar etc*/
            /*Si no eres admin, tu informacion personal y saldo y trasacciones*/
            break;

        case 3:


        }
    } while (opcion != 4);
    
}