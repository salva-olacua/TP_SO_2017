/*
 * primitivas.c
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include "pcb.h"
#include "conexiones.h"

t_puntero posAPuntero(t_pos pos,uint32_t tamPag){
	return pos.pag*tamPag+pos.off;
}

t_pos punteroAPos(t_puntero puntero, uint32_t tamPag){
	t_pos pos;
	pos.pag = puntero/tamPag;
	pos.off = puntero - tamPag * pos.pag;
	pos.size = 0;
	return pos;
}

t_pos proxPos(t_pos posActual,uint32_t tamPag){
	if(posActual.off==tamPag-4){
		posActual.pag++;
		posActual.off=0;
	}
	else{
		posActual.off+=4;
	}

	return posActual;
}

t_puntero dummy_definirVariable(t_nombre_variable variable){
	t_stack *stack;
	t_list *args,*vars;
	t_pos pos={0,0,0},lastPos;
	t_var *var;

	printf("Llamada a DEFINIR VARIABLE\nNombre variable: %c",variable);

	if(pcb_global.sp==0){
		args=list_create();
		vars=list_create();
		list_add(pcb_global.indiceStack,(void*)stack_create(args,vars,0,pos));
		pcb_global.sp=1;
	}else{
		stack=list_get(pcb_global.indiceStack,pcb_global.sp);
		args=stack->args;
		vars=stack->vars;
	}

	if(list_size(vars)!=0){
		var=list_get(vars,list_size(vars));
		lastPos=var->pos;
		pos=proxPos(lastPos,tamPag_global);
	}

	//todo verificar args

	if(pos.pag>maxStack_global){
		printf("ERROR: stack overflow\n");
		setExitCode(&pcb_global,"stack overflow",10);
		return STACK_OVERFLOW;
	}

	list_add(vars,(void*)var_create(variable,pos));

	return posAPuntero(pos,tamPag_global);
}

t_puntero dummy_obtenerPosicionVariable(t_nombre_variable variable) {
	//t_stack *stack;
	//t_list *vars,*args;
	//t_list *stack_l=pcb_global.indiceStack;
	t_var *var=NULL;
	//int i;
	printf("Obtener posicion de %c\n", variable);

	int _is_the_one(t_var *v){
		return v->id==variable;
	}

	int _buscar_variable(t_stack *s){
		return(list_find(s->args,(void*)_is_the_one)||list_find(s->vars,(void*)_is_the_one));
	}

	var=list_find(pcb_global.indiceStack,(void*)_buscar_variable);

	if(var){
		return posAPuntero(var->pos,tamPag_global);
	}else{
		setExitCode(&pcb_global,"no existe variable",11);
		return NO_EXISTE_VARIABLE;
	}

	/*
	for(i=1;i<=list_size(stack_l);i++){
		stack=list_get(stack_l,i);
		vars=stack->vars;
		args=stack->args;
		var=list_find(args,(void*)_is_the_one);
		if(var!=NULL)
			break;
		var=list_find(vars,(void*)_is_the_one);
		if(var!=NULL)
			break;
	}
*/
}

void dummy_finalizar(void){
	printf("Finalizar\n");
}

t_valor_variable dummy_dereferenciar(t_puntero puntero) {
	t_valor_variable res;
	t_pos pos;

	pos=punteroAPos(puntero,tamPag_global);

	printf("Dereferenciar Pag %i Off %i/n",pos.pag,pos.off);

	res=pedirAMemoria(pos);

	if(res==-1){
		printf("ERROR: fallo al leer en memoria");
		setExitCode(pcb_global,"fallo al leer en memoria",11);
		return NO_EXISTE_VARIABLE;
	}

	return res;
}

void dummy_asignar(t_puntero puntero, t_valor_variable variable) {
	t_pos pos;
	int res;

	pos=punteroAPos(puntero,tamPag_global);

	printf("Asignando en Pag %i Off %i el valor %d\n", pos.pag,pos.off, variable);

	res=asignarAMemoria(pos,variable);

	if(res==-1){
		printf("ERROR: fallo al escribir en memoria\n");
		setExitCode(pcb_global,"fallo al escribir en memoria",12);
	}
}
