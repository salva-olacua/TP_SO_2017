/*
 * operacionesMemoria.c
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "blue4-lib.h"
#include "operacionesMemoria.h"
#include "memoria.h"
#include <pthread.h>

/*
 ===============================================================================================
 Name        : PaginacionInvertida.c
 Author      : Tom
 Version     : 1.0
 Copyright   : Your copyright notice
 Description : La paginacion invertida!
 ===============================================================================================
 */


void* memoria;

void asignarTamanioAMemoria(){
	memoria = calloc(marcos,tamMarco);
}

void inicializarTablaDePaginas(tablaPaginas_t* tablaDePaginas){
	uint32_t marco;
	for(marco = 0; marco < marcos ; marco ++){
		tablaDePaginas[marco].pid = -2;
		tablaDePaginas[marco].pagina = -1;
	}
	return;
}
tablaPaginas_t* crearTablaDePaginas(){
	tablaPaginas_t *tablaDePaginas = calloc(marcos,sizeof(tablaPaginas_t));
	inicializarTablaDePaginas(tablaDePaginas);
	return tablaDePaginas;
}
int cuantosMarcosRepresenta(int unTamanio){
	if(unTamanio % tamMarco)
		return unTamanio/tamMarco + 1;
	else
		return unTamanio/tamMarco;
}

void reservarEstructurasEnTablaDePaginas(tablaPaginas_t* tablaDePaginas){
	unsigned cantidadMarcos = cuantosMarcosRepresenta(sizeof(tablaDePaginas));
	unsigned marco;
	for(marco = 0; marco < cantidadMarcos ; marco++){
		tablaDePaginas[marco].pid = -1;
    tablaDePaginas[marco].pagina = marco; // Como son los primeros marcos coinciden con el numero de pagina, solo pasa aca!
	}
}
void cargarTablaDePaginasAMemoria(){
	tablaPaginas_t* tablaDePaginas = crearTablaDePaginas();
	reservarEstructurasEnTablaDePaginas(tablaDePaginas);
	memcpy(memoria,tablaDePaginas,marcos * sizeof(tablaPaginas_t));
  free(tablaDePaginas);
}

void inicializarMemoria(){
	asignarTamanioAMemoria();
	cargarTablaDePaginasAMemoria();
}

bloque_t* cargarBloque(bloque_t* bloque,uint32_t tamanio, void* datos){
	bloque->header.tam = tamanio;
	bloque->header.disp = '1';
	bloque->data = datos;
	return bloque;
}

uint32_t cargarAMemoria(uint32_t tamanio ,void* datos, uint32_t marco){ //Todavia es pequeña, hay que agregarle hash y varias cosas mas

	bloque_t *bloque = malloc(tamanio+5);
	bloque = cargarBloque(bloque,tamanio, datos);
	if(marco > marcos || marco < 0){
		printf("Error al cargar, marco inexistente");
		return -1;
	}
	if(tamanio > (tamMarco - sizeof(headerB_t))){
		printf("Error al cargar, tamaño > bloque");
		return -1;
	}
	memcpy(memoria+(marco*tamMarco), bloque, sizeof(headerB_t) + sizeof(datos));
	free(bloque);
	return 0;
}

void mostrarTabla(){
	int i;
	tablaPaginas_t *tablaDePaginas = calloc(marcos,sizeof(tablaPaginas_t));
	memcpy(tablaDePaginas,memoria,marcos * sizeof(tablaPaginas_t));
	for(i=0; i<20; i++){
		printf("PID: %d , Pagina: %d \n", tablaDePaginas[i].pid, tablaDePaginas[i].pagina);
	}
	free(tablaDePaginas);
}

void mostrarDeMemoria(uint32_t marco){
	//Tiene aritmetica de punteros hasta la muerte! hay que trabajar todo memoria asi? :/
	uint32_t carry = 0;
	uint32_t* leer;
	void* mostrar;
	while((carry) < tamMarco){
		memcpy(leer, (memoria+(marco*tamMarco)+carry+2), sizeof(uint32_t) );
		printf("Leer es: %d", *leer);
		if((memoria+(marco*tamMarco)+carry + 1) == '1'){
			memcpy(mostrar, (memoria+(marco*tamMarco)+carry+5), leer);
			printf("con el carry %d se ve leer%s\n", carry ,mostrar);
		}else {
			printf("con el carry %d paso x un bloque vacio\n", carry);
		}
		carry += *leer+5;
	}
}


