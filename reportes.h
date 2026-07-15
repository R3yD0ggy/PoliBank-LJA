#ifndef REPORTES_H
#define REPORTES_H

#include "funkcii.h"

/* ======================================================================
   ENCABEZADO: REPORTES.H
   
   Define las funciones públicas del módulo de Reportes.
   Proporciona herramientas analíticas: rankings, exportación CSV,
   proyecciones financieras usando algoritmos como Quicksort y recursión.
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: ordenarPorSaldoQuicksort
   
   PROPÓSITO:
   Ordena recursivamente un arreglo de clientes por saldo de MAYOR a MENOR.
   
   ALGORITMO: Quicksort (Divide y Conquista)
   - Particiona el arreglo alrededor de un pivote
   - Ordena recursivamente las mitades
   - Complejidad O(n log n) en promedio
   
   MÉTODO DE PARTICIÓN: Lomuto
   - Los elementos > pivote quedan a la izquierda (mayores saldos)
   - Los elementos < pivote quedan a la derecha (menores saldos)
   - Resultado: ranking de mayor a menor
   
   PARÁMETROS:
   - Cliente lista[]: arreglo de clientes a ordenar
   - int inicio, fin: rango [inicio, fin] a ordenar
   
   NOTA IMPORTANTE: Esta función MODIFICA la lista original
   Si necesitas preservar el orden, haz una copia primero (ver mostrarRankingSaldos)
   
   COMPLEJIDAD:
   - Mejor caso: O(n log n)
   - Caso promedio: O(n log n)
   - Peor caso: O(n²) [poco probable con datos reales]
   ====================================================================== */
void ordenarPorSaldoQuicksort(Cliente lista[], int inicio, int fin);

/* ======================================================================
   FUNCIÓN: calcularInteresCompuestoRecursivo

   PROPÓSITO:
   Proyecta el crecimiento del saldo GUARDADO en una cuenta bajo interés
   compuesto durante N años, usando recursión (tal como lo pide el
   documento del proyecto). Se usa desde el menú del CLIENTE: cada cliente
   puede simular el interés compuesto de su propia cuenta, con una tasa
   fija del 5% anual (ver TASA_INTERES_CLIENTE en funkcii.c).

   FÓRMULA:
   S_n = S_0 × (1 + i)^n
   donde:
   - S_0 = capital inicial (el saldo actual guardado en la cuenta)
   - i = tasa anual (0.05 = 5%)
   - n = años
   - S_n = saldo final proyectado

   IMPLEMENTACIÓN RECURSIVA:
   - Caso base: si anios <= 0, retorna el capital tal cual (fin)
   - Caso recursivo: aplica 1 año de interés y llama de nuevo con anios-1

   PARÁMETROS:
   - double capital: saldo actual guardado en la cuenta ($)
   - double tasaAnual: tasa en decimal (0.05 = 5%)
   - int anios: cuántos años proyectar

   RETORNA: saldo proyectado después de anios años (NO modifica el saldo
   real de la cuenta; es solo una simulación informativa)

   COMPLEJIDAD: O(anios) [lineal en el número de años]
   ====================================================================== */
double calcularInteresCompuestoRecursivo(double capital, double tasaAnual, int anios);

/* ======================================================================
   FUNCIÓN: exportarBalanceCSV
   
   PROPÓSITO:
   Genera un archivo CSV simple con los saldos de todas las cuentas.
   
   FORMATO:
   NumeroCuenta,NombreCompleto,Cedula,Saldo
   12345...,Juan Pérez,123456...,1500.00
   
   VENTAJAS:
   - Se abre en Excel con doble click
   - Portable: cualquier programa lo lee
   - Importable a bases de datos
   - Humano-legible
   
   PARÁMETROS:
   - const Cliente lista[]: todos los clientes
   - int cantidad: cantidad de clientes
   - const char *nombreArchivo: ruta del archivo (ej. "balance.csv")
   
   RETORNA: 1 si se escribió, 0 si hubo error
   ====================================================================== */
int exportarBalanceCSV(const Cliente lista[], int cantidad, const char *nombreArchivo);

/* ======================================================================
   FUNCIÓN: exportarUsuariosCSV
   
   PROPÓSITO:
   Exporta el listado COMPLETO de usuarios (todos los datos EXCEPTO contraseña)
   en formato CSV para análisis en Excel.
   
   CAMPOS INCLUIDOS:
   - Número de cuenta
   - Nombres completos
   - Cédula
   - Usuario (login)
   - Fecha de nacimiento
   - Saldo
   
   CAMPO EXCLUIDO (SEGURIDAD):
   - Contraseña: NUNCA se exporta
   - Principio: un banco nunca envía contraseñas en reportes
   
   PARÁMETROS:
   - const Cliente lista[]: todos los clientes
   - int cantidad: cantidad de clientes
   - const char *nombreArchivo: ruta (ej. "usuarios.csv")
   
   RETORNA: 1 si éxito, 0 si error
   ====================================================================== */
int exportarUsuariosCSV(const Cliente lista[], int cantidad, const char *nombreArchivo);

/* ======================================================================
   FUNCIÓN: mostrarMenuReportes
   
   PROPÓSITO:
   Abre el menú de reportes y análisis para el administrador.
   
   OPCIONES DISPONIBLES:
   1. Ranking de saldos
   2. Exportar balance a CSV
   3. Exportar usuarios a CSV
   4. Ver saldo total del banco
   5. Volver al menú anterior
   
   FLUJO:
   - Muestra menú
   - Pide opción al usuario
   - Ejecuta la acción correspondiente
   - Repite hasta que elige "Volver"
   
   PARÁMETROS:
   - const Cliente lista[]: todos los clientes
   - int cantidad: cantidad de clientes
   ====================================================================== */
void mostrarMenuReportes(const Cliente lista[], int cantidad);

#endif
