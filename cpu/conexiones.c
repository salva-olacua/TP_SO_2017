/*
 * conexiones.c
 *
 *  Created on: 5/4/2017
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/types.h>
#include <netdb.h>

struct addrinfo *crear_addrinfo_socket(char* ip,char* puerto)
{
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;

	//Llenar info

	memset(&hints,0,sizeof hints); //Lleno hints de 0
	hints.ai_family=AF_INET; //ip4v
	hints.ai_socktype=SOCK_STREAM;

	status=getaddrinfo(ip,puerto,&hints,&servinfo);

	if(status!=0)
	{
		printf("\a");
		system("tput setaf 1");
		fprintf(stderr,"Error de getaddrinfo: %s\n",gai_strerror(status));
		system("tput setaf 9");
		exit(1);
	}

	return servinfo;
}

int crear_socket(struct addrinfo *servinfo)
{
	int sockfd;

	sockfd=socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol); //El protocolo esta en 0 por el memset

	if (sockfd==-1)
		perror("socekt");

	return sockfd;
}
int conectar(char *puerto, char *ip)
{
	struct addrinfo *servinfo=crear_addrinfo_socket(ip,puerto);

	int socketCliente = crear_socket(servinfo);

	printf("Socket creado: %i\n",socketCliente);
	if (connect(socketCliente, servinfo->ai_addr, servinfo->ai_addrlen) != 0)
	{
		perror("No se pudo conectar");
		return 1;
	}
	send(socketCliente,"3",1,0);
	return 0;
}
