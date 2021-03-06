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
#include <commons/string.h>
#include "blue4-lib.h"
#include "operacionesMemoria.h"
#include "memoria.h"
#include <pthread.h>
#include "configuraciones.h"
#include "estructurasAdministrativas.h"
#define int32_min -999999
#define int32_max 999999

uint32_t tamanioDeTabla() {
	return sizeof(tablaPaginas_t) * configDeMemoria.marcos;
}

uint32_t tamanioDeTablaCache() {
	return sizeof(tablaCache_t) * configDeMemoria.entradasCache;
}

int marcosDeTabla() {
	return cuantosMarcosRepresenta(tamanioDeTabla());
}

int marcosDeTablaCache() {
	return cuantosMarcosRepresenta(tamanioDeTablaCache());
}

tablaPaginas_t* obtenerTablaDePaginas() {
	tablaPaginas_t* tablaDePaginas = (tablaPaginas_t*)memoria;
	return tablaDePaginas;
}

tablaCache_t* obtenerTablaCache() {
	tablaCache_t* tabla = cache;
	return tabla;
}

int lru() {
	tablaCache_t* tabla = obtenerTablaCache();
	int marco, minV = int32_max, minM = -2;
	for (marco = 1; marco < configDeMemoria.entradasCache; marco++) {
		pthread_mutex_lock(&mutex_tablaCache);
		if (tabla[marco].pid >= 0 && tabla[marco].counter < minV) {
			minV = tabla[marco].counter;
			minM = marco;
		}
		pthread_mutex_unlock(&mutex_tablaCache);
	}
	//printf("Cache LRU: Se reemplazara el marco %i, del proceso %i, pagina %i\n",minM, tabla[minM].pid, tabla[minM].pagina);
	return minM;
}

void actualizarCounterMarco(uint32_t marco) {
	tablaCache_t* tabla = obtenerTablaCache();
	pthread_mutex_lock(&mutex_tablaCache);
	tabla[marco].counter = contador++;
	pthread_mutex_unlock(&mutex_tablaCache);
}

void escribirCache(uint32_t marco, uint32_t offset, uint32_t tamData,
		void *data) {
	pthread_mutex_lock(&escribiendoMemoriaCache);
	//printf("CACHE %p MARCO %i CONFIG TAM MARCO %i OFFSET %i\n",cache,marco,configDeMemoria.tamMarco,offset);
	//printf("DATA %p\n",data);
	char* marco_str = string_itoa(marco);
	char* offset_str=string_itoa(offset);
	char* tamMarco_str=string_itoa(configDeMemoria.tamMarco);
		char* mensaje_log = concat(2,"Se va a ESCRIBIR en CACHE el marco: ",marco_str);
		char* mensaje_log2=concat(3,mensaje_log," el cual, tiene un tamaño de: ",tamMarco_str);
		char* mensaje_log3=concat(3,mensaje_log2," con un offset de: ",offset_str);
		log_trace(logMemoria,mensaje_log3);
		free(mensaje_log);
		free(mensaje_log2);
		free(mensaje_log3);
		free(marco_str);
		free(offset_str);
		free(tamMarco_str);
	memcpy(cache + marco * configDeMemoria.tamMarco + offset, data, tamData);
	actualizarCounterMarco(marco);
	pthread_mutex_unlock(&escribiendoMemoriaCache);
}

int getMarcoCache(int pid, int pag) {
	tablaCache_t *tabla = obtenerTablaCache();
	int i;
	if (pid < -1 || pag < 0)
		return -2;
	for (i = 0; i < configDeMemoria.entradasCache; i++) {
		pthread_mutex_lock(&mutex_tablaCache);
		if (tabla[i].pid == pid) {
			if (tabla[i].pagina == pag) {
				pthread_mutex_unlock(&mutex_tablaCache);
				return i;
			}
		}
		pthread_mutex_unlock(&mutex_tablaCache);
	}
	return -2;
}

uint32_t estaEnCache(uint32_t pid, uint32_t pag) {
	return getMarcoCache(pid, pag) >= -1;
}

int marcosTabla() {
	return cuantosMarcosRepresenta(tamanioDeTabla());
}

int tratarColision(int hash) {
	if (hash < configDeMemoria.marcos)
		return hash+1;
	else
		return marcosTabla();
}

int funcionDeHash( pid, pag) {
	return (pid * 10 + pag) % (maxPA) + marcosTabla();
}

int getMarco(uint32_t pid, uint32_t pagina) {
	// el hash representa el indice en la tabla que seria igual al marco
	int hash, i = 0;
	tablaPaginas_t* tablaDePaginas = obtenerTablaDePaginas();
	hash = funcionDeHash(pid, pagina);

	pthread_mutex_lock(&mutex_tablaDePaginas);
	while (tablaDePaginas[hash].pid != pid
			|| tablaDePaginas[hash].pagina != pagina) {
		hash = tratarColision(hash);
		i++;

		if (i >= maxPA) {
			pthread_mutex_unlock(&mutex_tablaDePaginas);
			return -1;
		} //NO EXISTE EL PID PAPURRI ?)

	}
	pthread_mutex_unlock(&mutex_tablaDePaginas);
	return hash;
}

int marcoVacio(int marco) {
	tablaPaginas_t *tablaDePaginas = obtenerTablaDePaginas();
	int resultado;
	pthread_mutex_lock(&mutex_tablaDePaginas);
	resultado = tablaDePaginas[marco].pid == -2;
	pthread_mutex_unlock(&mutex_tablaDePaginas);
	return resultado;
}

int nuevoMarco(uint32_t pid, uint32_t pagina) {
	// el hash representa el indice en la tabla que seria igual al marco
	int hash, i = 0;
	hash = funcionDeHash(pid, pagina);
	while (!marcoVacio(hash)) {
		hash = tratarColision(hash);
		i++;

		if (i >= configDeMemoria.marcos)
			return -1; //NO HAY MARCOS VACIOS ._.

	}
	return hash;
}

/*
 int getMarco(int pid, int pag){
 tablaPaginas_t *tdep = obtenerTablaDePaginas();
 int i;
 if(pid==-1 || pag<0)
 return -1;
 for(i=0; i<configDeMemoria.marcos; i++){
 if(tdep[i].pid == pid){
 if(tdep[i].pagina == pag){
 return i;}}
 }
 return -1;
 }
 */

void cargarCodigo(uint32_t marco, uint32_t pagina, void* data) {
	//pthread_mutex_lock(mutexMemoria);
	memcpy(memoria + marco * configDeMemoria.tamMarco,
			data + pagina * configDeMemoria.tamMarco, configDeMemoria.tamMarco);
	//pthread_mutex_unlock(mutexMemoria);
}

void cargarPaginaATabla(uint32_t pid, uint32_t pagina, unsigned marco) {
	tablaPaginas_t *tablaDePaginas = obtenerTablaDePaginas();
	pthread_mutex_lock(&mutex_tablaDePaginas);
	char* numero_str = string_itoa(marco);
		char* pid_str=string_itoa(pid);
		char* pag_str=string_itoa(pagina);
		char* mensaje_log = concat(2,"El nuevo marco a asignar segun la funcion de hash es: ",numero_str);
		char* mensaje_log2=concat(3,mensaje_log," para el proceso: ",pid_str);
		char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
		log_trace(logMemoria,mensaje_log3);
		free(mensaje_log);
		free(mensaje_log2);
		free(mensaje_log3);
		free(numero_str);
		free(pid_str);
		free(pag_str);
	tablaDePaginas[marco].pid = pid;
	tablaDePaginas[marco].pagina = pagina;
	pthread_mutex_unlock(&mutex_tablaDePaginas);
	return;
}

int guardaProcesoEn(uint32_t pid, uint32_t paginasRequeridas,int32_t pos) {
	uint32_t marco, pagina = 0;
	while (pagina < paginasRequeridas) {
		marco = nuevoMarco(pid, pagina);
		if(marco == -1){
			perror("Memoria llena");
			return -1;
		}
		cargarPaginaATabla(pid, pagina, marco);
		//uint32_t *pag=malloc(sizeof(uint32_t));
		//memcpy(pag,&pagina,sizeof(uint32_t));
		//agrego pagina a la lista
		//pthread_mutex_lock(&mutex_procesosActivos);
		//printf("  		Pag agregada: %i\n",*pag);
		//list_add(procesosActivos[pos].listaPaginas,pag);
		//pthread_mutex_unlock(&mutex_procesosActivos);

		pagina++;
	}
	return 0;
}

int agregarNuevoProceso(uint32_t pid, uint32_t paginasRequeridas,int32_t pos) {
	int i = guardaProcesoEn(pid, paginasRequeridas, pos);
	actualizarMarcosDisponibles(paginasRequeridas);
	return i;
}

int tieneMarcosSuficientes(int paginasRequeridas) {
	pthread_mutex_lock(&mutex_marcos);
	int condicion= paginasRequeridas <= configDeMemoria.marcosDisponibles;
	pthread_mutex_unlock(&mutex_marcos);
	return condicion;
}

int inicializarPrograma(uint32_t pid, uint32_t paginasRequeridas) {
	//pthread_mutex_lock(mutexMemoria);
	int32_t pos;
	pos=agregar_DataDeProcesoActivo(pid, paginasRequeridas);
	return agregarNuevoProceso(pid, paginasRequeridas,pos);
	//pthread_mutex_unlock(mutexMemoria);
}


int eliminarPaginaDeUnProceso(uint32_t pid, uint32_t paginaAEliminar) {
	tablaPaginas_t* tablaDePaginas;
	tablaDePaginas = obtenerTablaDePaginas();
	int marco = getMarco(pid, paginaAEliminar);
	if (marco < 0)
		return -1;
	pthread_mutex_lock(&mutex_tablaDePaginas);
	char* numero_str = string_itoa(marco);
		char* pid_str=string_itoa(pid);
		char* pag_str=string_itoa(paginaAEliminar);
		char* mensaje_log = concat(2,"Se eliminara el marco: ",numero_str);
		char* mensaje_log2=concat(3,mensaje_log," del proceso: ",pid_str);
		char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
		log_trace(logMemoria,mensaje_log3);
		free(mensaje_log);
		free(mensaje_log2);
		free(mensaje_log3);
		free(numero_str);
		free(pid_str);
		free(pag_str);
	tablaDePaginas[marco].pid = -2;
	tablaDePaginas[marco].pagina = -2;
	aumentarMarcosDisponibles(1);
	pthread_mutex_unlock(&mutex_tablaDePaginas);
	disminuir_PaginasActualesDeProcesoActivo(pid, paginaAEliminar);
	return 0;
}
/*
int paginasMax(uint32_t pid){
	int res;
	uint32_t posicion_PidBuscado=obtener_PosicionProcesoActivo(pid);
	pthread_mutex_lock(&mutex_procesosActivos);
	res=list_size(procesosActivos[posicion_PidBuscado].listaPaginas);
	pthread_mutex_unlock(&mutex_procesosActivos);
	return res;
}
*//*
int primerPagina(uint32_t pid){
	int *res;
	uint32_t posicion_PidBuscado=obtener_PosicionProcesoActivo(pid);
	pthread_mutex_lock(&mutex_procesosActivos);
	res=list_get(procesosActivos[posicion_PidBuscado].listaPaginas,0);
	//printf("                     OBTENIDO: %i\n",*res);
	pthread_mutex_unlock(&mutex_procesosActivos);
	return *res;
}
*/
uint32_t finalizarPrograma(uint32_t pid) {
	tablaPaginas_t* tablaDePaginas = obtenerTablaDePaginas();
	int pagina=0,paginasTotales=0, paginasMaximas, i = 0;

	//pagina = primerPagina(pid);
	//int marco/* = getMarco(pid, pagina)*/;
	/*if (marco < 0)
		return -1;*/
	//paginasMaximas = paginasMax(pid);
/*
	pagina = obtener_PaginaDeInicioDeProcesoActivo(pid);
	int marco = getMarco(pid, pagina);
	if (marco < 0)
		return -1;
		*/
	paginasMaximas = obtener_PaginasMaximas(pid);
	if(paginasMaximas==-1)return -1;

	int marco=getMarco(pid,pagina);
	while (i < paginasMaximas) {
		//printf("I:%i,paginasMax:%i\n",i,paginasMaximas);
		//pagina = primerPagina(pid);
		//marco = getMarco(pid, pagina);
		if (marco >= 0) {
			pthread_mutex_lock(&mutex_tablaDePaginas);
			tablaDePaginas[marco].pid = -2;
			tablaDePaginas[marco].pagina = marco;
			paginasTotales++;
			pthread_mutex_unlock(&mutex_tablaDePaginas);
			//disminuir_PaginasActualesDeProcesoActivo(pid, pagina);
		}
		pagina++;

		i++;
		marco = getMarco(pid, pagina);
	}
	//obtener_PosicionProcesoActivo(pid);
	tablaCache_t* tablaCache = obtenerTablaCache();
	for(i=0; i< configDeMemoria.entradasCache; i++){
		pthread_mutex_lock(&mutex_tablaCache);
		if(tablaCache[i].pid == pid) tablaCache[i].pid = -2;
		pthread_mutex_unlock(&mutex_tablaCache);
	}

	//textoAmarillo("LAS PAGINAS TOTALES SON: ");
	paginasTotales=obtener_paginasActualesDeProcesoActivo(pid);
	//char* string_num = string_itoa(paginasTotales);
	//textoAmarillo(string_num);
	//free(string_num);
	aumentarMarcosDisponibles(paginasTotales);
	eliminar_DataDeProcesoActivo(pid);
	return 0;
}


int proximaPaginaAAsignar(uint32_t pid,uint32_t paginasMaximas)
{
	int i=0;
	int marco=getMarco(pid,i);
	while(i<paginasMaximas)
	{
		// Esta seria la proxima pagina a asignar!!!
		if(marco==-1)return i;
		i++;
		marco=getMarco(pid,i);
	}
	// en este caso no hay paginas intermedias que asignar!!!
	return paginasMaximas;
}
int asignarPaginasAUnProceso(uint32_t pid, uint32_t paginasRequeridas) {
	int pagina, marco, i = 0;
	tablaPaginas_t* tablaDePaginas;
	int resultado = tieneMarcosSuficientes(paginasRequeridas);
	if (resultado == 0)
		return -1;
	tablaDePaginas = obtenerTablaDePaginas();
	actualizarMarcosDisponibles(paginasRequeridas);

	int paginasMaximas=obtener_PaginasMaximas(pid);

	while (i < paginasRequeridas) {
		pagina = proximaPaginaAAsignar(pid,paginasMaximas);
		/*
		char* prox_pagina_string= string_itoa(pagina);
		textoAmarillo("LA PROXIMA PAGINA A ASIGNAR ES : ");
		textoAmarillo(prox_pagina_string);
		free(prox_pagina_string);
		*/
		marco = nuevoMarco(pid, pagina);
		pthread_mutex_lock(&mutex_tablaDePaginas);
		char* numero_str = string_itoa(marco);
			char* pid_str=string_itoa(pid);
			char* pag_str=string_itoa(pagina);
			char* mensaje_log = concat(2,"el nuevo  marco a asignar segun la funcion de hash es: ",numero_str);
			char* mensaje_log2=concat(3,mensaje_log," al proceso: ",pid_str);
			char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
			log_trace(logMemoria,mensaje_log3);
			free(mensaje_log);
			free(mensaje_log2);
			free(mensaje_log3);
			free(numero_str);
			free(pid_str);
			free(pag_str);
		tablaDePaginas[marco].pid = pid;
		tablaDePaginas[marco].pagina = pagina;
		pthread_mutex_unlock(&mutex_tablaDePaginas);
		paginasMaximas++;
		pagina++;
		i++;
	}
	/*
	while (i < paginasRequeridas) {
		pagina = obtener_ProximaPaginaAAsignar(pid); //aca tmb agrego a la lista la pos nueva
		marco = nuevoMarco(pid, pagina);
		if(marco == -1){
			perror("Memoria Llena");
			return -1;
		}
		pthread_mutex_lock(&mutex_tablaDePaginas);
		tablaDePaginas[marco].pid = pid;
		tablaDePaginas[marco].pagina = pagina;
		pthread_mutex_unlock(&mutex_tablaDePaginas);
		i++;
	}
	*/
	aumentar_PaginasActualesDeProcesoActivo(pid, paginasRequeridas);
	return 0;
}

void* leerMemoria(uint32_t pid, uint32_t pag, uint32_t offset, uint32_t tam) { //REQUIERE FREE
	int marco = getMarco(pid, pag);
	if (marco < 0)
		return NULL;
	void* datos = malloc(tam);
	usleep(configDeMemoria.retardo);
	pthread_mutex_lock(&escribiendoMemoria);
	char* numero_str = string_itoa(marco);
		char* pid_str=string_itoa(pid);
		char* pag_str=string_itoa(pag);
		char* mensaje_log = concat(2,"Se lee de memoria el marco: ",numero_str);
		char* mensaje_log2=concat(3,mensaje_log," del proceso: ",pid_str);
		char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
		log_trace(logMemoria,mensaje_log3);
		free(mensaje_log);
		free(mensaje_log2);
		free(mensaje_log3);
		free(numero_str);
		free(pid_str);
		free(pag_str);
	memcpy(datos, memoria + marco * configDeMemoria.tamMarco + offset, tam);
	pthread_mutex_unlock(&escribiendoMemoria);
	return datos;
}

uint32_t escribirMemoria(uint32_t pid, uint32_t pag, uint32_t offset,
		uint32_t tamData, void *data) {
	int marco = getMarco(pid, pag);
	if (marco < 0)
		return -1;
	usleep(configDeMemoria.retardo);
	pthread_mutex_lock(&escribiendoMemoria);
	char* numero_str = string_itoa(marco);
		char* pid_str=string_itoa(pid);
		char* pag_str=string_itoa(pag);
		char* mensaje_log = concat(2,"Se escribe en memoria el marco: ",numero_str);
		char* mensaje_log2=concat(3,mensaje_log," del proceso: ",pid_str);
		char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
		log_trace(logMemoria,mensaje_log3);
		free(mensaje_log);
		free(mensaje_log2);
		free(mensaje_log3);
		free(numero_str);
		free(pid_str);
		free(pag_str);
	memcpy(memoria + marco * configDeMemoria.tamMarco + offset, data, tamData);
	pthread_mutex_unlock(&escribiendoMemoria);
	return 0;
}

void* leerCache(uint32_t pid, uint32_t pag, uint32_t offset, uint32_t tam) { //NECESITA UN FREE :D
	uint32_t marco = getMarcoCache(pid, pag);
	void* datos = malloc(tam);
	pthread_mutex_lock(&escribiendoMemoriaCache);
	char* numero_str = string_itoa(marco);
		char* pid_str=string_itoa(pid);
		char* pag_str=string_itoa(pag);
		char* mensaje_log = concat(2,"Se lee de CACHE el marco: ",numero_str);
		char* mensaje_log2=concat(3,mensaje_log," del proceso: ",pid_str);
		char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
		log_trace(logMemoria,mensaje_log3);
		free(mensaje_log);
		free(mensaje_log2);
		free(mensaje_log3);
		free(numero_str);
		free(pid_str);
		free(pag_str);
	memcpy(datos, cache + marco * configDeMemoria.tamMarco + offset, tam);
	pthread_mutex_unlock(&escribiendoMemoriaCache);
	actualizarCounterMarco(marco);
	return datos;
}

int nuevoMarcoDelMismoProceso(int pid) {
	tablaCache_t* tabla = obtenerTablaCache();
	int marco, paginas = 0, counter = 0, marcoNuevo = -1;
	for (marco = 0; marco < configDeMemoria.entradasCache; marco++) {
		pthread_mutex_lock(&mutex_tablaCache);
		if (tabla[marco].pid == pid) {
			paginas++;
			if (counter == 0 || tabla[marco].counter < counter) {
				marcoNuevo = marco;
				counter = tabla[marco].counter;
			}
		}
		pthread_mutex_unlock(&mutex_tablaCache);
	}
	if (paginas < configDeMemoria.cachePorProceso)
		return -1;
	return marcoNuevo;
}

int buscarMarcoVacioCache() {
	tablaCache_t* tabla = obtenerTablaCache();
	int marco = 0;
	while (marco < configDeMemoria.entradasCache) {
		pthread_mutex_lock(&mutex_tablaCache);
		if (tabla[marco].pid == -2){
			pthread_mutex_unlock(&mutex_tablaCache);
			return marco;
		}
		pthread_mutex_unlock(&mutex_tablaCache);
		marco++;
	}
	return -2;
}

int agregarProcesoACache(int pid, int pag) {
	int marco = nuevoMarcoDelMismoProceso(pid);
	if (marco < 0) {
		pthread_mutex_lock(&semCacheDisp);
		if (configDeMemoria.cacheDisponible > 0) {
			configDeMemoria.cacheDisponible--;
			pthread_mutex_unlock(&semCacheDisp);
			marco = buscarMarcoVacioCache();
		} else
			pthread_mutex_unlock(&semCacheDisp);
			marco = lru();
	}
	//printf("Cache: se le asignara el marco %i al proceso %i, pagina %i\n", marco, pid, pag);
	tablaCache_t* tabla = obtenerTablaCache();
	pthread_mutex_lock(&mutex_tablaCache);
	char* numero_str = string_itoa(marco);
	char* pid_str=string_itoa(pid);
	char* pag_str=string_itoa(pag);
	char* mensaje_log = concat(2,"Cache: se le asignara el marco: ",numero_str);
	char* mensaje_log2=concat(3,mensaje_log," al proceso: ",pid_str);
	char* mensaje_log3=concat(3,mensaje_log2," pagina: ",pag_str);
	log_trace(logMemoria,mensaje_log3);
	free(mensaje_log);
	free(mensaje_log2);
	free(mensaje_log3);
	free(numero_str);
	free(pid_str);
	free(pag_str);
	tabla[marco].pid = pid;
	tabla[marco].pagina = pag;
	tabla[marco].counter = contador++;
	pthread_mutex_unlock(&mutex_tablaCache);
	return marco;
}

void copiarMemoriaACache(int pid, int pag) {
	int marco = getMarcoCache(pid, pag);
	void* data = leerMemoria(pid, pag, 0, configDeMemoria.tamMarco);
	if(data != NULL) escribirCache(marco, 0, configDeMemoria.tamMarco, data);
	free(data);
}

void* leer(uint32_t pid, uint32_t pag, uint32_t offset, uint32_t tam) { // SIGUE NECESITANDO UN FREE
	if(configDeMemoria.cachePorProceso==0)
		return leerMemoria(pid,pag,offset,tam);
	if (!estaEnCache(pid, pag)){
		agregarProcesoACache(pid, pag);
		copiarMemoriaACache(pid, pag);
	}
	else
		log_trace(logMemoria,"el proceso se encuentra en cache, se accede rapidamente");
	return leerCache(pid,pag,offset, tam);
}

int escribir(uint32_t pid, uint32_t pag, uint32_t offset, uint32_t tamData,
	void *data) {
	if(tamData==0 || data == NULL) return 0;
	if(configDeMemoria.cachePorProceso==0)
		{
			int resultado=escribirMemoria(pid,pag,offset,tamData,data);
			return resultado;
		}
	escribirMemoria(pid,pag,offset,tamData,data);
	if (estaEnCache(pid, pag))
		escribirCache(getMarcoCache(pid, pag), offset, tamData, data);
	return 0;
}

int validarPIDPAG(int pid, int pag){
	int resultado = getMarco(pid,pag);
	return (resultado >= 0);
}

/*
void* leer(uint32_t pid, uint32_t pag, uint32_t offset, uint32_t tam) { // SIGUE NECESITANDO UN FREE
	if (estaEnCache(pid, pag))
		return leerCache(pid, pag, offset, tam);
	else
		return leerMemoria(pid, pag, offset, tam);
}




int escribir(uint32_t pid, uint32_t pag, uint32_t offset, uint32_t tamData,
	void *data) {
	if(tamData==0 || data == NULL) return 0;
	if(configDeMemoria.cachePorProceso==0)
		{
			int resultado=escribirMemoria(pid,pag,offset,tamData,data);
			return resultado;
		}
	if (estaEnCache(pid, pag))
		escribirCache(getMarcoCache(pid, pag), offset, tamData, data);
	else {
		int pidViejo, pagVieja;
		escribirMemoria(pid, pag, offset, tamData, data);
		agregarProcesoACache(pid, pag, &pidViejo, &pagVieja);
		escribirMemoria(pidViejo, pagVieja, 0, configDeMemoria.tamMarco, leerCache(pid, pag,0, configDeMemoria.tamMarco));
		copiarMemoriaACache(pid, pag);
	}
	return 0;
}
*/
