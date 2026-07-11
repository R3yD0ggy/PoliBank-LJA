#include <stdio.h>
#include "funkcii.h"
#include <windows.h>
#include <locale.h>

void Menu(void)
{
    printf("=== Polibank ===\n");
    printf("1. ¿No tienes una cuenta? Regístrate para obtener una.\n");
    printf("2. ¿Ya eres cliente? Inicia sesión para acceder a tu cuenta.\n");
    printf("3. Salir del programa.\n");
    printf("Seleccione una opción: ");
}

int main(void)
{
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

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
        {
            int posicionLogueada = iniciarSesion(lista, cantidad);

            if (posicionLogueada == -1)
            {
                char respuesta;
                printf("\n¿Desea registrarse como un nuevo cliente en Polibank? (s/n): ");
                scanf(" %c", &respuesta); 
                while (getchar() != '\n') {} 
                
                if (respuesta == 's' || respuesta == 'S')
                {
                    printf("\n--- Redireccionando al registro ---\n");
                    registrarCliente(lista, &cantidad);
                    guardarClientes(lista, cantidad, "clientes.bin");
                }
            }
            break;
        }
        case 3:
            printf("Saliendo del sistema Polibank... ¡Hasta luego!\n");
            break;
        default:
            printf("Opción inválida. Intente nuevamente.\n");
            break;
        }
    } while (opcion != 3);
    
}