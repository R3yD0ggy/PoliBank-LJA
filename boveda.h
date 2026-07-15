#ifndef BOVEDA_H
#define BOVEDA_H

#include "funkcii.h"

/* ======================================================================
   ENCABEZADO: BOVEDA.H

   Define la interfaz pública del módulo de Bóveda (boveda.c).

   REDISEÑO IMPORTANTE:
   - El acceso a Bóveda YA NO se ofrece dentro del menú del administrador:
     es un punto de entrada independiente en el menú principal (main),
     con sus propias credenciales de cajero. Así el administrador nunca
     tiene un camino, ni directo ni indirecto, para mover el dinero de
     un cliente.
   - Bóveda solo abre durante el horario de atención del banco (ver
     dentroDeHorarioBancario en funkcii.h): jornada matutina 08:00-12:00
     y vespertina 12:00-20:00.
   - El verdadero trabajo de Bóveda ahora es doble: (1) permitir que un
     cajero registre depósitos/retiros a nombre de un cliente, y (2) irle
     dejando un rastro de CADA movimiento del banco (los del cajero y
     también los que hace un cliente por su cuenta) en un archivo de
     texto por día: "bitacora_DDMMAAAA.txt". Ese registro por movimiento
     ocurre en el momento (registrarEnBitacoraDiaria, en funkcii.c), y
     cerrarBitacoraDelDia() se encarga de sumar los totales del día y
     dejar un resumen de cierre cuando el programa termina.

   DEPENDENCIAS:
   - funkcii.h: para structs Cliente, funciones de validación, I/O,
     horario bancario y bitácora diaria.
   ====================================================================== */

/* ======================================================================
   FUNCIÓN PÚBLICA: iniciarModuloBoveda

   PROPÓSITO:
   Abre el módulo de bóveda para que un cajero autenticado pueda
   registrar depósitos y retiros de clientes.

   FLUJO:
   1. Verifica que el banco esté dentro de su horario de atención;
      si está cerrado, rechaza el acceso sin pedir credenciales.
   2. Solicita autenticación del cajero (usuario/clave fijos).
   3. Si pasa, abre un menú interactivo (depósito / retiro / salir).
   4. Cada movimiento: actualiza el saldo, genera el estado de cuenta
      del cliente, y se agrega a la bitácora diaria del banco.

   PARÁMETROS:
   - Cliente lista[]: la lista completa de clientes del banco
   - int cantidad: número total de clientes registrados
   ====================================================================== */
void iniciarModuloBoveda(Cliente lista[], int cantidad);

/* ======================================================================
   FUNCIÓN PÚBLICA: cerrarBitacoraDelDia

   PROPÓSITO:
   Se llama UNA vez, cuando el programa se está cerrando (main, opción
   "Salir del programa"). Lee el archivo de bitácora del día de hoy
   (si existe), suma RECURSIVAMENTE los ingresos y egresos de todos los
   movimientos que hubo desde el primero hasta el último, y agrega al
   final del mismo archivo un bloque de cierre con esos totales.

   Si el programa se vuelve a abrir el MISMO día, el archivo de hoy ya
   existe: los nuevos movimientos se siguen agregando a ese archivo
   (no se crea uno nuevo), y al volver a cerrar el programa se agrega
   otro bloque de cierre con los totales actualizados.

   Si no hubo ningún movimiento en el día, no crea el archivo: solo
   informa en pantalla que no hubo movimientos.
   ====================================================================== */
void cerrarBitacoraDelDia(void);

#endif
