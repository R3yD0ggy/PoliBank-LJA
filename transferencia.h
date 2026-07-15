#ifndef TRANSFERENCIA_H
#define TRANSFERENCIA_H

#include "funkcii.h"

/* ======================================================================
   ENCABEZADO: TRANSFERENCIA.H
   
   Define las funciones públicas para transferencias de dinero entre cuentas.
   
   FLUJO TÍPICO:
   1. Cliente A inicia sesión
   2. Elige "transferir" desde su menú
   3. Se llama a transferirSaldo() pasando su índice
   4. El cliente ingresa la cuenta destino (número 16 dígitos con Luhn)
   5. Se validan ambas cuentas y el saldo
   6. Se actualizan ambos saldos
   7. Se generan comprobantes para ambos
   
   CARACTERÍSTICAS:
   - Búsqueda binaria: O(log n) para encontrar cuenta destino
   - Validación con Luhn: previene números inventados
   - Validaciones múltiples: saldo, existencia, autotransferencia
   - Persistencia inmediata: cambios se guardan en clientes.bin
   - Comprobantes: ambos clientes reciben confirmación
   ====================================================================== */

/* ======================================================================
   FUNCIÓN: buscarCuentaBinaria
   
   PROPÓSITO:
   Localiza rapidísimamente una cuenta por su número usando búsqueda binaria.
   
   EFICIENCIA:
   - Con 100 clientes: ~7 comparaciones
   - Con 10,000 clientes: ~14 comparaciones
   - Con 1 millón de clientes: ~20 comparaciones
   
   PARÁMETROS:
   - const Cliente lista[]: lista ordenada ascendentemente por numeroCuenta
   - int tamano: cantidad total de clientes
   - long long numeroCuentaDestino: número a buscar
   
   RETORNA:
   - índice (>= 0) si encontró
   - -1 si no existe
   
   COMPLEJIDAD: O(log n)
   ====================================================================== */
int buscarCuentaBinaria(const Cliente lista[], int tamano, long long numeroCuentaDestino);

/* ======================================================================
   FUNCIÓN: transferirSaldo
   
   PROPÓSITO:
   Ejecuta UNA transferencia de dinero de un cliente a otro.
   
   VALIDACIONES (en este orden):
   1. Verifica índice origen válido
   2. Lee cuenta destino y valida con Luhn
   3. Busca cuenta destino (búsqueda binaria)
   4. Verifica que no sea autotransferencia
   5. Lee monto y verifica que sea positivo
   6. Verifica saldo suficiente
   
   ACTUALIZACIÓN:
   - Resta monto de la cuenta origen
   - Suma monto a la cuenta destino
   - Guarda inmediatamente en clientes.bin
   - Genera comprobantes para ambas cuentas
   
   PARÁMETROS:
   - Cliente lista[]: lista de todos los clientes
   - int cantidad: cantidad total de clientes
   - int posicionOrigen: índice de quién ENVÍA el dinero
   
   RETORNA: void (pero modifica saldos y archivos)
   
   PRECONDICIÓN: posicionOrigen debe ser un índice válido (>= 0 && < cantidad)
   ====================================================================== */
void transferirSaldo(Cliente lista[], int cantidad, int posicionOrigen);

/* ======================================================================
   FUNCIÓN: realizarTransferencia
   
   PROPÓSITO:
   Variante que trabaja con PUNTEROS en lugar de índices.
   Sirve cuando solo tienes &cliente sin saber su índice en la lista.
   
   PROCESO INTERNO:
   1. Valida que cuentaOrigen y archivoClientes no sean NULL
   2. Busca el índice de cuentaOrigen comparando direcciones de memoria
   3. Si lo encuentra, llama a transferirSaldo()
   4. Guarda cambios en archivoClientes
   
   PARÁMETROS:
   - Cliente *cuentaOrigen: puntero al cliente que envía dinero
   - Cliente lista[]: lista completa (debe contener a cuentaOrigen)
   - int tamano: cantidad de clientes en la lista
   - const char *archivoClientes: ruta del archivo donde guardar (ej. "clientes.bin")
   
   RETORNA:
   - 1 si se realizó la transferencia exitosamente
   - 0 si hubo error (NULL, índice inválido, etc)
   ====================================================================== */
int realizarTransferencia(Cliente *cuentaOrigen, Cliente lista[], int tamano, const char *archivoClientes);

#endif
