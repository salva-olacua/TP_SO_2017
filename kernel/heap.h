/*
 * heap.h
 *
 *  Created on: 4/7/2017
 *      Author: utnso
 */

#ifndef HEAP_H_
#define HEAP_H_

#include "main.h"

typedef struct heapMetadata{
	uint32_t size;
	uint8_t isFree;
}heapMetadata;

typedef struct pagina{
	void*contenido;
	uint32_t numero;
	t_list* bloques;
}paginaHeap;

typedef struct bloque {
	uint32_t posicionInicioBloque;
	uint32_t tamanioData;
	heapMetadata * metadata;
}bloque;

//void iniciarBloqueHeap(paginaHeap* unaPagina);
//bloque *guardarDataHeap(paginaHeap* unaPagina, void* data,int32_t tamData);
//int memoriaLibre(paginaHeap* unaPagina);
//int memoriaLibreContigua(paginaHeap* unaPagina);
//int compactar(paginaHeap* unaPagina);


#endif /* HEAP_H_ */
