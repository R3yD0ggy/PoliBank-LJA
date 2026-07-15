#include "reportes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================
   MÓDULO REPORTES - REPORTES.C
   
   Proporciona funcionalidad analítica del banco: rankings, exportación de
   datos y proyección de interés compuesto sobre el saldo del cliente.
   Incluye el algoritmo de ordenamiento Quicksort, recursión (saldo total
   del banco e interés compuesto) y exportación a CSV.
   
   CARACTERÍSTICAS:
   - Ordenamiento O(n log n) sin modificar la lista original
   - Cálculo recursivo del saldo total del banco
   - Cálculo recursivo de interés compuesto (usado desde el menú cliente)
   - Exportación a CSV para Excel
   - Menú de reportes interactivo
   ====================================================================== */

/* ======================================================================
   FUNCIÓN AUXILIAR: intercambiarClientes
   
   Intercambia (swaps) dos elementos de un arreglo de clientes.
   Esto es necesario para que Quicksort reorganice los datos.
   
   POR QUÉ es función separada:
   - Evita repetir el código de intercambio en múltiples lugares
   - Mantiene el código más legible
   - Facilita debugging (punto de quiebre único)
   
   PARÁMETROS:
   - *a, *b: punteros a dos clientes que se van a intercambiar
   
   OPERACIÓN:
   1. Crea una copia temporal del cliente A
   2. Copia B a donde estaba A
   3. Copia la copia temporal (A original) a donde estaba B
   ====================================================================== */
static void intercambiarClientes(Cliente *a, Cliente *b)
{
    /* Crea un temporal para no perder datos durante el intercambio */
    Cliente temp = *a;
    *a = *b;
    *b = temp;
}

/* ======================================================================
   FUNCIÓN AUXILIAR: particionarPorSaldo
   
   Implementa la "partición" en el algoritmo Quicksort. Reorganiza el
   arreglo para que todos los saldos MAYORES que un valor pivote queden
   a la izquierda, y los menores queden a la derecha.
   
   MÉTODO USADO: Partición de Lomuto
   - El ÚLTIMO elemento se usa como pivote (referencia para comparar)
   - Se itera de izquierda a derecha moviendo valores > pivote a la izquierda
   - Al final, el pivote queda en su posición "final"
   
   INVARIANTE (se mantiene durante la ejecución):
   - Elementos desde [inicio] hasta [i] son > pivote (ya ordenados)
   - Elementos desde [i+1] hasta [j-1] son < pivote (ya ordenados)
   - Elementos desde [j] hasta [fin-1] aún no se han procesado
   
   POR QUÉ > EN VEZ DE <:
   - El objetivo es un ranking de MAYOR a MENOR saldo
   - Moviendo "mayores" a la izquierda, dejamos menores a la derecha
   
   PARÁMETROS:
   - lista[]: arreglo de clientes a particionar
   - inicio, fin: rango en el que particionar
   
   RETORNA: la posición final del pivote (elemento que ya está en su lugar)
   ====================================================================== */
static int particionarPorSaldo(Cliente lista[], int inicio, int fin)
{
    /* Elige el último elemento como pivote (criterio de comparación) */
    double pivote = lista[fin].saldo;
    int i = inicio - 1;  /* Marca el final de los elementos > pivote */
    int j;

    /* Itera desde inicio hasta fin-1 (fin es el pivote) */
    for (j = inicio; j < fin; j++)
    {
        /* Si el saldo actual es MAYOR que el pivote, muévelo a la izquierda */
        if (lista[j].saldo > pivote)
        {
            i++;
            intercambiarClientes(&lista[i], &lista[j]);
        }
        /* Si es menor, lo deja en su lugar (j sigue incrementando) */
    }
    
    /* Coloca el pivote en su posición definitiva */
    intercambiarClientes(&lista[i + 1], &lista[fin]);
    return i + 1;
}

/* ======================================================================
   FUNCIÓN: ordenarPorSaldoQuicksort
   
   Implementa el algoritmo QUICKSORT de forma recursiva para ordenar
   clientes por SALDO (de mayor a menor).
   
   ALGORITMO DIVIDE Y CONQUISTA:
   1. Si [inicio >= fin] → ya ordenado, retorna (caso base)
   2. Particiona el arreglo → devuelve posición del pivote
   3. Ordena recursivamente la mitad izquierda (elementos > pivote)
   4. Ordena recursivamente la mitad derecha (elementos < pivote)
   5. No necesita mezclar (como Merge Sort) porque ya están en orden
   
   COMPLEJIDAD:
   - Mejor caso: O(n log n) cuando el pivote queda en el medio
   - Caso promedio: O(n log n)
   - Peor caso: O(n²) si el pivote siempre queda en un extremo
     (poco probable con datos reales/aleatorios)
   
   VENTAJAS:
   - Muy rápido en práctica (mejor que Merge Sort para este tamaño)
   - In-place: usa poca memoria extra (a diferencia de Merge Sort)
   - Adaptativo: mejora cuando datos ya parcialmente ordenados
   
   PARÁMETROS:
   - lista[]: arreglo de clientes a ordenar
   - inicio, fin: rango del arreglo a ordenar
   
   NOTA: Esta función MODIFICA la lista original (in-place)
         Si necesitas preservar el orden, haz una copia primero
   ====================================================================== */
void ordenarPorSaldoQuicksort(Cliente lista[], int inicio, int fin)
{
    /* Caso base: si inicio >= fin, el subarreglo tiene 0 o 1 elementos (ya está ordenado) */
    if (inicio < fin)
    {
        /* Particiona el arreglo y obtiene la posición final del pivote */
        int posicionPivote = particionarPorSaldo(lista, inicio, fin);
        
        /* Ordena recursivamente la mitad IZQUIERDA (elementos > pivote) */
        ordenarPorSaldoQuicksort(lista, inicio, posicionPivote - 1);
        
        /* Ordena recursivamente la mitad DERECHA (elementos < pivote) */
        ordenarPorSaldoQuicksort(lista, posicionPivote + 1, fin);
    }
}

/* ======================================================================
   FUNCIÓN: calcularInteresCompuestoRecursivo

   Proyecta el crecimiento del saldo de una cuenta tras N años de interés
   compuesto usando RECURSIÓN (sin bucles). Se usa desde el menú del
   cliente sobre el dinero que tiene guardado en su propia cuenta.

   FÓRMULA MATEMÁTICA:
   S_n = S_0 × (1 + i)^n

   IMPLEMENTACIÓN RECURSIVA:
   - Caso base: si anios <= 0, retorna capital sin más interés (fin)
   - Caso recursivo: aplica 1 año de interés y llama de nuevo con anios-1

   EJEMPLO CON $1000, 5% anual, 3 años:
   calcular(1000, 0.05, 3)
   → 1000 * 1.05 + calcular(1050, 0.05, 2)
   → 1050 * 1.05 + calcular(1102.5, 0.05, 1)
   → 1102.5 * 1.05 + calcular(1157.625, 0.05, 0)
   → 1157.625 + 0 = $1157.63
   ====================================================================== */
double calcularInteresCompuestoRecursivo(double capital, double tasaAnual, int anios)
{
    /* Caso base: si ya no quedan años, retorna el capital acumulado */
    if (anios <= 0)
    {
        return capital;
    }

    /* Caso recursivo: aplica un año de interés y llama de nuevo con anios-1 */
    return calcularInteresCompuestoRecursivo(capital * (1.0 + tasaAnual), tasaAnual, anios - 1);
}

/* ======================================================================
   FUNCIÓN: exportarBalanceCSV
   
   Genera un archivo CSV (Comma-Separated Values) con los saldos de todas
   las cuentas. Se puede abrir directamente en Excel, Google Sheets, etc.
   
   FORMATO DEL ARCHIVO:
   NumeroCuenta,NombreCompleto,Cedula,Saldo
   1234567890123456,Juan Pérez,1234567890,1500.00
   1234567890123457,María García,1234567891,2300.50
   ...
   
   FLUJO:
   1. Abre/crea el archivo en modo "w" (write, sobrescribe si existe)
   2. Escribe la cabecera con nombres de columnas
   3. Itera sobre todos los clientes
   4. Escribe una línea por cliente con sus datos
   5. Cierra el archivo
   
   POR QUÉ CSV:
   - Formato simple y portable (cualquier programa lo lee)
   - Se abre directamente en Excel (doble click)
   - Se puede importar a bases de datos
   - Humano-legible (puedes editarlo en Notepad)
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes del banco
   - nombreArchivo: ruta del archivo a crear (ej. "balance.csv")
   
   RETORNA: 1 si se escribió correctamente, 0 si fallo
   ====================================================================== */
int exportarBalanceCSV(const Cliente lista[], int cantidad, const char *nombreArchivo)
{
    FILE *archivo = fopen(nombreArchivo, "w");
    int i;

    if (archivo == NULL)
    {
        printf("[Error] No se pudo crear el archivo CSV.\n");
        return 0;
    }

    /* Encabezados de las columnas */
    fprintf(archivo, "NumeroCuenta,NombreCompleto,Cedula,Saldo\n");
    
    /* Una línea por cada cliente con sus datos básicos */
    for (i = 0; i < cantidad; i++)
    {
        fprintf(archivo, "%lld,%s,%s,%.2f\n",
                lista[i].numeroCuenta, lista[i].nombresCompletos, lista[i].cedula, lista[i].saldo);
    }
    fclose(archivo);
    return 1;
}

/* ======================================================================
   FUNCIÓN: exportarUsuariosCSV
   
   Genera un archivo CSV MÁS COMPLETO con TODOS los datos de los clientes
   EXCEPTO la contraseña (por seguridad).
   
   FORMATO DEL ARCHIVO:
   NumeroCuenta,NombreCompleto,Cedula,Usuario,FechaNacimiento,Saldo
   1234567890123456,Juan Pérez,1234567890,juanperez,12/05/1990,1500.00
   ...
   
   DATOS INCLUIDOS:
   - Número de cuenta
   - Nombres completos
   - Cédula
   - Usuario (login)
   - Fecha de nacimiento (DD/MM/AAAA)
   - Saldo
   
   DATO EXCLUIDO (SEGURIDAD):
   - Contraseña: NUNCA se exporta en reportes
     (un banco real nunca enviaría contraseñas en un email/reporte)
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes
   - nombreArchivo: ruta del archivo CSV a crear
   
   RETORNA: 1 si éxito, 0 si error
   ====================================================================== */
int exportarUsuariosCSV(const Cliente lista[], int cantidad, const char *nombreArchivo)
{
    FILE *archivo = fopen(nombreArchivo, "w");
    int i;

    if (archivo == NULL)
    {
        printf("[Error] No se pudo crear el archivo de usuarios.\n");
        return 0;
    }

    /* Encabezados con todos los campos EXCEPTO contraseña */
    fprintf(archivo, "NumeroCuenta,NombreCompleto,Cedula,Usuario,FechaNacimiento,Saldo\n");
    
    /* Una línea completa por cliente */
    for (i = 0; i < cantidad; i++)
    {
        fprintf(archivo, "%lld,%s,%s,%s,%02d/%02d/%04d,%.2f\n",
                lista[i].numeroCuenta, lista[i].nombresCompletos, lista[i].cedula, lista[i].usuario,
                lista[i].fechaNacimiento.dia, lista[i].fechaNacimiento.mes, lista[i].fechaNacimiento.anio,
                lista[i].saldo);
    }
    fclose(archivo);
    return 1;
}

/* ======================================================================
   FUNCIÓN AUXILIAR: mostrarRankingSaldos
   
   Crea UN RANKING de clientes ordenados por saldo (mayor a menor).
   Importante: NO modifica la lista original (necesita que búsqueda binaria
   siga funcionando).
   
   ESTRATEGIA:
   1. Crea una COPIA de la lista en memoria (memcpy)
   2. Ordena la copia usando Quicksort
   3. Imprime la copia ordenada (1º lugar = saldo mayor)
   4. Descarta la copia (la original sigue intacta)
   
   POR QUÉ usar copia:
   - Si ordena la lista original, búsqueda binaria se rompe (espera orden ascendente)
   - En un banco real, hay múltiples procesos leyendo/escribiendo simultáneamente
   - Hacer una copia es el patrón correcto: "separar vista de datos"
   
   PARÁMETROS:
   - lista[], cantidad: lista original de clientes
   ====================================================================== */
static void mostrarRankingSaldos(const Cliente lista[], int cantidad)
{
    Cliente copia[MAX_CLIENTES];
    int i;

    if (cantidad <= 0)
    {
        printf("No hay clientes registrados.\n");
        return;
    }

    /* Copia TODO el arreglo de clientes a 'copia' */
    memcpy(copia, lista, (size_t)cantidad * sizeof(Cliente));
    
    /* Ordena la COPIA (no toca la lista original) */
    ordenarPorSaldoQuicksort(copia, 0, cantidad - 1);

    /* Imprime el ranking basado en la copia ordenada */
    imprimirTitulo("RANKING DE SALDOS");
    for (i = 0; i < cantidad; i++)
    {
        printf("%d. %-25s | Cuenta: %lld | Saldo: $%.2f\n",
               i + 1, copia[i].nombresCompletos, copia[i].numeroCuenta, copia[i].saldo);
    }
}

/* ======================================================================
   FUNCIÓN AUXILIAR: mostrarSaldoTotalBanco
   
   Calcula y muestra el saldo total acumulado de TODOS los clientes.
   Usa recursión (no bucles) para sumar.
   
   PROPÓSITO:
   - Auditoría: verificar que el total de pasivos = total de activos
   - Análisis: saber cuánto dinero hay "en circulación" en el banco
   - Educativo: ejemplo de cálculo recursivo
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes
   ====================================================================== */
static void mostrarSaldoTotalBanco(const Cliente lista[], int cantidad)
{
    printf("\nSaldo total acumulado en el banco (calculo recursivo): $%.2f\n",
           calcularSaldoTotalRecursivo(lista, cantidad));
}

/* ======================================================================
   FUNCIÓN: mostrarMenuReportes
   
   Menú interactivo de análisis y reportes para el ADMINISTRADOR.
   Permite generar rankings de saldos y exportar datos a CSV.
   
   OPCIONES:
   1. Ranking de saldos
   2. Exportar balance a CSV
   3. Exportar listado completo de usuarios
   4. Ver saldo total del banco
   5. Volver al menú anterior
   
   PROPÓSITO:
   - Administración: ver quiénes tienen más dinero
   - Auditoría: exportar datos para análisis externo
   - Operaciones: sorting y manejo de archivos
   
   PARÁMETROS:
   - lista[], cantidad: todos los clientes
   ====================================================================== */
void mostrarMenuReportes(const Cliente lista[], int cantidad)
{
    int opcion = 0;

    /* Menú permanente hasta que el admin elige "Volver" */
    do
    {
        imprimirTitulo("MENU REPORTES - POLIBANK");
        printf("1. Ver ranking de saldos\n");
        printf("2. Exportar balance a CSV\n");
        printf("3. Exportar listado completo de usuarios\n");
        printf("4. Ver saldo total del banco\n");
        printf("5. Volver\n");

        if (!leerEntero("Seleccione una opción: ", &opcion))
        {
            printf("[Error] Opción inválida.\n");
            continue;
        }

        switch (opcion)
        {
        case 1:
            /* Ordena y muestra ranking (hace copia para no romper búsqueda binaria) */
            mostrarRankingSaldos(lista, cantidad);
            break;
        case 2:
            /* Exporta balance simple a CSV */
            if (exportarBalanceCSV(lista, cantidad, "balance_cuentas.csv"))
            {
                printf("[Éxito] Balance exportado a balance_cuentas.csv\n");
            }
            break;
        case 3:
            /* Exporta datos completos (sin contraseña) a CSV */
            if (exportarUsuariosCSV(lista, cantidad, "usuarios_polibank.csv"))
            {
                printf("[Éxito] Listado completo de usuarios exportado a usuarios_polibank.csv\n");
                printf("(Se puede abrir directamente con Microsoft Excel o Google Sheets.)\n");
            }
            break;
        case 4:
            /* Calcula total recursivamente */
            mostrarSaldoTotalBanco(lista, cantidad);
            break;
        case 5:
            printf("Volviendo al menu administrador.\n");
            break;
        default:
            printf("[Error] Opción inválida.\n");
            break;
        }
    } while (opcion != 5);
}
