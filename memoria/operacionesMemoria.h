/*
 * operacionesMemoria.h
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESMEMORIA_H_
#define OPERACIONESMEMORIA_H_

#include "memoria.h"

pthread_mutex_t escribiendoMemoria, escribiendoMemoriaCache;

uint32_t tamanioDeTabla();

uint32_t tamanioDeTablaCache();

void inicializarPrograma(uint32_t ,uint32_t , void* );


/**
 * LeerMemoria(pid, pag, offset, tamañoLectura)
 * @return void* memoriaBuscada en caso de exito || NULL en caso de error
 */
void *leerMemoria(uint32_t, uint32_t, uint32_t, uint32_t);

/**
 * escribirMemoria(pid, pag, offset, tamañoEscritura, datosAEscribir)
 * Escribe la memoria
 * @return 0 en caso de exito || -1 en caso de error
 */
uint32_t escribirMemoria(uint32_t, uint32_t, uint32_t, uint32_t, void*);

void *leerCache(uint32_t, uint32_t, uint32_t, uint32_t);

void escribirCache(uint32_t, uint32_t, uint32_t, uint32_t, void*);

tablaPaginas_t* obtenerTablaDePaginas();

/**
 *
 * @param (int marcos) cantidad de marcosa evaluar
 * @return bool
 */
int tieneMarcosSuficientes(int );


/**
 * funcion que sirve para que el kernel finalize un proceso
 * setea la tabla de paginas en -2 correspondiente al lugar donde estaba ese pid.
 *
 * @param uint32_t pid (identificador unico de un proceso)
 * @return 0 en casod exito || -1 en caso de error
 */
uint32_t finalizarPrograma(uint32_t );


/**
 * Agranda un proceso dandole mas paginas
 * @param uint32_t pid (identificador unico de un proceso)
 * @param uint32_t paginas requeridas
 * @return 0 en caso de exito || -1 en caso de error
 */
int asignarPaginasAUnProceso(uint32_t ,uint32_t );

/**
 * Elimina la pagina dada por parametro de un proceso
 * @param uint32_t pid
 * @param uint32_t pagina a eliminar
 * @return 0 en caso de exito || -1 en caso de error
 */
int eliminarPaginaDeUnProceso(uint32_t ,uint32_t );

#endif /* OPERACIONESMEMORIA_H_ */
