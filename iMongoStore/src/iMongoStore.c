/*
 ============================================================================
 Name        : iMongoStore.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "iMongoStore.h"

//#define PUERTO "6667"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

#define TRIPULANTE 1
#define PATOTA 1
#define SABOTAJE 1

struct t_buffer {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
};


struct t_paquete {
    uint8_t codigo_operacion;
    t_buffer* buffer;
};

struct t_bitarray{
	char *bitarray;
	size_t size;
	bit_numbering_t mode;
};


#define PATH_CONFIG "/home/utnso/iMongoStore/iMongoStore/config/mongoStore.config"
#define PATH_CONEXION "/home/utnso/tp-2021-1c-Cebollitas-subcampeon/libCompartida/config/conexiones.config"

int main(void) {
	mongoStore_config = leer_config(PATH_CONFIG);
	conexion_config = leer_config(PATH_CONEXION);

	printf("Checkpoint 1");

	IP = config_get_string_value(conexion_config, "IP_MONGOSTORE");
	logger_path_mongostore = config_get_string_value(mongoStore_config, "ARCHIVO_LOG");

	printf("Checkpoint 2");

	logger = iniciar_logger(logger_path_mongostore);

	printf("Checkpoint 3");

	punto_montaje = config_get_string_value(mongoStore_config, "PUNTO_MONTAJE");

	//Generamos la conexion Mongo => Discordiador
	//int puerto_mongostore = config_get_int_value(conexion_config, "PUERTO_MONGOSTORE");
	//int server_FS = iniciar_servidor(IP, puerto_mongostore);

	bloques = config_get_int_value(mongoStore_config, "BLOCKS");
	tamanio_bloque = config_get_int_value(mongoStore_config, "BLOCK_SIZE");
	tiempoSincro = config_get_int_value(mongoStore_config, "TIEMPO_SINCRONIZACION");


	//Inicializamos el File System, consultamos si ya existen los archivos o no
	char* ruta_superbloque = string_new();
	char* ruta_blocks = string_new();
	string_append(&ruta_superbloque, punto_montaje);
	string_append(&ruta_blocks, punto_montaje);
	string_append(&ruta_superbloque, "/Superbloque/Superbloque.ims");
	string_append(&ruta_blocks, "/Blocks/Blocks.ims");

	//Inicializamos File System
	if(verificar_existencia(ruta_blocks) == 0 && verificar_existencia(ruta_superbloque) == 0){
		inicializar_carpetas();
		crear_superbloque();
		inicializar_bloques();
		crear_archivo_files();
	}

	agregarCaracter(3, 'o');

	printf("Checkpoint 4");



	//Para el manejo de mensajes:
//	while(1)
//	{
//		int cliente = esperar_cliente(server_FS);
//		if(cliente < 0){
//			printf("No se pudo establecer la conexion");
//			exit(1);
//		}
//		log_info(logger, "Se conecto el modulo Discordiador");
//		pthread_t hilo;
//		pthread_create(&hilo, NULL, (void* )atender_mensaje(cliente), NULL);
//	}
	return EXIT_SUCCESS;
}

void* atender_mensaje(int cliente){
//	int socket_cliente = (int) cliente;
//	t_paquete *paquete = recibir_paquete_y_desempaquetar(socket_cliente);
//	void *stream_obtenido = paquete -> buffer -> stream;
//
//	switch(paquete -> codigo_operacion){
//
//	}
//	free(paquete -> buffer -> stream);
//	free(paquete -> buffer);
//	free(paquete);
}

void inicializar_carpetas(){
	//Se inicializan las carpetas FILES y BITACORAS
	nueva_carpeta("/Files");
	log_info(logger, "Se creo la carpeta de Files");

	nueva_carpeta("/Files/Bitacoras");
	log_info(logger ,"Se creo la capeta de las bitacoras");

	nueva_carpeta("/Superbloque");
	log_info(logger, "Se creo la carpeta del Superbloque");

	nueva_carpeta("/Blocks");
	log_info(logger, "Se creo la carpeta de los bloques");
}

void nueva_carpeta(char *nueva_carpeta){
	char *ruta_general = string_new();
	string_append(&ruta_general, punto_montaje);
	string_append(&ruta_general, nueva_carpeta);
	mkdir(ruta_general, 0777);
	free(ruta_general);
	//mkdir(): Genera un nuevo directorio con ese path.
	//El primer parametro es la ruta general, el segundo es el modo en el que
	//se crea
}

void crear_superbloque(){
	char* path_superbloque = string_new();

	string_append(&path_superbloque, punto_montaje);
	string_append(&path_superbloque, "/Superbloque/Superbloque.ims");

	//Creamos superbloque
	int fd = open(path_superbloque, O_CREAT | O_RDWR, 0664);

	if (fd == -1) {
		close(fd);
		log_error(logger, "Error abriendo el Superbloque.ims");
		return;
	}

	tamanioBitmap = bloques/8; //AGREGAR ceil()
	int res = 4 + 4 + tamanioBitmap; //Tamanio a truncar el superbloque
	ftruncate(fd, res);

	//Creas el superbloque
	void* superbloque = mmap(NULL, res, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//Vas llenando la info del superbloque del superbloque
	void* tipo_variables = malloc(4); //Son los uint32 que vas metiendo en el superbloque
	tipo_variables = realloc(tipo_variables, sizeof(int));

	memcpy(tipo_variables, &tamanio_bloque, sizeof(int));
	memcpy(superbloque, tipo_variables, sizeof(int));

	memcpy(tipo_variables, &bloques, sizeof(int));
	memcpy(superbloque + sizeof(int), tipo_variables, sizeof(int));

	//CREO EL BITMAP
	bitmap = bitarray_create_with_mode((char*) superbloque+2*sizeof(int), tamanioBitmap, MSB_FIRST);
	msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);

	//Sincronizas
	msync(superbloque, res, MS_SYNC);

	//Liberas la path
	free(path_superbloque);
	log_info(logger, "Se creo el archivo superbloque!");
	return;
}

void inicializar_bloques(){
	//CADA UN BYTE, UN CARACTER!!!
	char* ruta_blocks = string_new();
	string_append(&ruta_blocks, punto_montaje);
	string_append(&ruta_blocks, "/Blocks/Blocks.ims");

	int res = tamanio_bloque * bloques;

	copiaBlock = malloc(res);

	int fd = open(ruta_blocks, O_CREAT | O_RDWR, 0664);

	if (fd == -1) {
		close(fd);
		log_error(logger, "Error abriendo el Blocks.ims");
		return;
	}

	ftruncate(fd, res);

	void* block = mmap(NULL, res, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(copiaBlock, block, res); //ACA ESTA EL PROBLEMA

//SINCRONIZACION ==> Chequear!
//	while(1){
//		sleep(tiempoSincro);
//		pthread_mutex_lock(&mutexEscrituraBloques);
//		memcpy(block,copiaBlock,res);
//		pthread_mutex_unlock(&mutexEscrituraBloques);
//
//		int resultadoSincro = msync(block, res, MS_SYNC);
//
//		if(resultadoSincro == -1){
//			log_error(logger, "Fallo en la sincronizacion con el bloque");
//		}
//		else
//		{
//			log_error(logger, "Sincronizacion exitosa con el bloque");
//		}
//	}

	close(fd);
	return;
}

void eliminarCaracter(int cantidad, char caracter){
	//SE INVOCA PARA CONSUMIR UN RECURSO
	char* rutita = string_new();
	string_append(&rutita, punto_montaje);

	switch (caracter){
		case 'O':
			string_append(&rutita, "/Files/Oxigeno.ims");
			eliminarEnBloque(cantidad, caracter, rutita);
			break;

		case 'o':
			string_append(&rutita, "/Files/Oxigeno.ims");
			eliminarEnBloque(cantidad, 'O', rutita);
			break;

		case 'B':
			string_append(&rutita, "/Files/Basura.ims");
			eliminarEnBloque(cantidad, caracter, rutita);
			break;

		case 'b':
			string_append(&rutita, "/Files/Basura.ims");
			eliminarEnBloque(cantidad, 'B', rutita);
			break;

		case 'C':
			string_append(&rutita, "/Files/Comida.ims");
			eliminarEnBloque(cantidad, caracter, rutita);
			break;

		case 'c':
			string_append(&rutita, "/Files/Comida.ims");
			eliminarEnBloque(cantidad, 'C', rutita);
			break;

		default:
			printf("No se selecciono un caracter correcto");
			break;
	}
}


void eliminarEnBloque(int cantidad, char caracter, char* rutita){
	//Misma logica para cuando escribimos en el bloque

	bloquesDelSistema = bloques;

	//Se llama config_o2 porque originalmente estaba para Oxigeno.ims, pero ahora es global (el nombre no importa)
	//Para obtener la data directamente del metadata, hacemos:
	t_config* config_o2 = leer_config(rutita);
	int cantidadDeCaracteresRestantes = config_get_int_value(config_o2, "SIZE");

	//La info sobre los bloques llenados con ese caracter la averiguas con bloquesUsados
	char** bloquesUsados = config_get_array_value(config_o2, "BLOCKS");
	int cantBloques = config_get_int_value(config_o2, "BLOCK_COUNT");

//	//El bloque donde vas a escribir
//	int bloqueAUsar;

	//La cantidad de bloques nuevos asociados al caracter
	int cantBloquesActualizacion = cantBloques;

	//Un contador con la cantidad de bloques que se desocuparon
	int contadorBloquesDesocupados = 0;

	char* bloquesNuevosPostBorrado = string_new();
	string_append(&bloquesNuevosPostBorrado, "[");

	//Si es 0, sugnifica que el caracter no tiene bloques asignados en el Blocks.ims
	//Por ende, tenes que buscar el proximo bloque libre ==> no hay bloques en uso

	if(cantBloques <= 0){
		log_error(logger, "El recurso todavia no se genero!");
		printf("No se genero el recurso!");
		return;

		//Si tiene 0 bloques ==> No esta ocupando ningun bloque ==> No tiene sentido consumir algo que no existe
	}
	else
	{
		//Lo empezas a recorrer desde el final del bitmap
		for(int i = bloquesDelSistema; i > 0; i--){
			if((bitarray_test_bit(bitmap, i) == 1 && cantidad >= 0)){ //En 1, el bloque esta ocupado
				char* bloqueAChequear = string_itoa(i);

				if(existeEnArray(bloquesUsados, *bloqueAChequear) == 1 && cantBloquesActualizacion > 0){
//					bloqueAUsar = i;

					if(cantidad >= tamanio_bloque){
						while(cantidad >= tamanio_bloque){
//							pthread_mutex_lock(&mutexEscrituraBloques);
//							memcpy(copiaBlock + (bloqueAUsar * tamanio_bloque) + (cantidadDeCaracteresRestantes % tamanio_bloque), "", sizeof(char));
//							//Escribo "W" porque el SIZE del Blocks.ims debe permanecer constante (un tamanio fijo) ==> Escribo un caracter random
//							pthread_mutex_unlock(&mutexEscrituraBloques);

							cantidad--;
							cantidadDeCaracteresRestantes--;
						}

						bitarray_set_bit(bitmap, 0);
						msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);
						cantBloquesActualizacion--;
						contadorBloquesDesocupados++;

						//Creo el nuevo array, deshaciendome del bloque


					}
					else if(cantBloquesActualizacion > 0)
					{
						//Si entra al else, significa que en el bloque no esta lleno de caracteres de ese recurso
//						bloqueAUsar = i;

						while(cantidad == 0){
//							pthread_mutex_lock(&mutexEscrituraBloques);
//							memcpy(copiaBlock + (bloqueAUsar * tamanio_bloque) + (cantidadDeCaracteresRestantes % tamanio_bloque), "", sizeof(char));
//							pthread_mutex_unlock(&mutexEscrituraBloques);

							cantidad--;
							cantidadDeCaracteresRestantes--;
						}
					}
				}
			}
		}//Fin de for

		//Generamos la lista (o el string) que va a contener los bloques todavia ocupados
		char* bloquesTodaviaOcupados = string_new();
		string_append(&bloquesTodaviaOcupados, "[");

		//Armado del array con los bloques todavia ocupados
		if(contadorBloquesDesocupados == 0){ //No se desocupo ningun bloque
			char* primeraPosicion = string_itoa(*bloquesUsados[0]);
			string_append(&bloquesNuevosPostBorrado, primeraPosicion);

			//Se manda la lista original
			for(int j = 1; j < cantBloques - 1; j++){
				string_append(&bloquesNuevosPostBorrado, ",");
				string_append(&bloquesNuevosPostBorrado, bloquesUsados[j]);
			}
			string_append(&bloquesNuevosPostBorrado, "]");
		}
		else if(contadorBloquesDesocupados == cantBloques){ //Se desocuparon todos los bloques ocupados por el recurso
			string_append(&bloquesNuevosPostBorrado, "]");
		}
		else
		{//Se desocuparon algunos bloques ==> lo concatenamos con el string de los bloques todavia ocupados

			for(int i = 0; i < cantBloques - contadorBloquesDesocupados; i++){
				//Llenamos el string bloquesTodaviaOcupados con los bloques sin desocupar
			}
			string_append(&bloquesNuevosPostBorrado, bloquesTodaviaOcupados);
			string_append(&bloquesNuevosPostBorrado, "]");
		}

	} //Fin del if

	//Actualizas el metadata
	char* actualizarCantidad = string_itoa(cantBloquesActualizacion);
	char* actualizarSize = string_itoa(cantidadDeCaracteresRestantes);

	actualizar_metadata(bloquesNuevosPostBorrado, actualizarSize, actualizarCantidad, rutita);

	config_save(config_o2);

	log_info(logger, "Ya se consumieron todos los recursos posibles");
}


int existeEnArray(char** array, char contenido){
	int existe = 0;
	for(int i = 0; i < sizeof(array); i++){
		if(array[i] == string_itoa(contenido)){
			existe = 1;
		}
	}
	return existe;
}

void escribirEnBloque(int cantidad, char caracter, char* rutita){
	bloquesDelSistema = bloques;

	//Se llama config_o2 porque originalmente estaba para Oxigeno.ims, pero ahora es global (el nombre no importa)
	//Para obtener la data directamente del metadata, hacemos:
	t_config* config_o2 = config_create(rutita);
	int cantidadDeCaracteresEscritas = config_get_int_value(config_o2, "SIZE");

	//La info sobre los bloques llenados con ese caracter la averiguas con bloquesUsados
	char** bloquesUsados = config_get_array_value(config_o2, "BLOCKS");
	int cantBloques = config_get_int_value(config_o2, "BLOCK_COUNT");

	//Cantidad de caracteres escritos
	int cantidadEscrita = 0;
	//El bloque donde vas a escribir
	int bloqueAUsar;
	//La cantidad de bloques nuevos asociados al caracter
	int cantBloquesActualizacion = cantBloques;

	//Para hacer un nuevo string con los bloques nuevos del caracter
	char* actualizar_bloques = string_new();

	//Si es 0, sugnifica que el caracter no tiene bloques asignados en el Blocks.ims
	//Por ende, tenes que buscar el proximo bloque libre ==> no hay bloques en uso
	if(cantBloques == 0){
		for(int i = 0; i < bloquesDelSistema; i++){
			if((bitarray_test_bit(bitmap, i) == 0) && cantidad > 0){ //Pregunta si el bloque esta libre (0 == libre)

				bloqueAUsar = i;
				cantBloquesActualizacion++; //Suma la cantidad de bloques ocupado por el caracter/recurso
				char* bloque_nuevo = string_itoa(i); //Para agregarlo en el metadata con los bloques que usas
				cantidadEscrita = 0;

				if(cantBloquesActualizacion==1){ //Empezas a armar el string que contiene la lista de los bloques NUEVOS!
					string_append(&actualizar_bloques, bloque_nuevo);	//Ej: [1,3,4] ==> Son los bloques
				}
				else	//Los agregas con la coma
				{
					string_append(&actualizar_bloques, ",");
					string_append(&actualizar_bloques, bloque_nuevo);
				}
				//Agregas un bloque por ciclo de for!

				bitarray_set_bit(bitmap, i); //Seteas que el bloque esta ocupado
				msync(&bitmap -> bitarray, tamanioBitmap, MS_SYNC);

				//Empezas a escribir en el bitmap hasta llenar ese bloque o hasta que no tengas mas caracteres
				//Se escribe un caracter por ciclo de while!
				while(cantidad > 0 && cantidadEscrita < tamanio_bloque){

					//ESCRIBO EN EL BLOQUE DEL BITMAP
					pthread_mutex_lock(&mutexEscrituraBloques);
					memcpy(copiaBlock + bloqueAUsar * tamanio_bloque + (cantidadDeCaracteresEscritas % tamanio_bloque), &caracter, sizeof(char));
					pthread_mutex_unlock(&mutexEscrituraBloques);

					//Sumas la cantida de caracteres escritos, disminuis la cantidad restante de caracteres
					//a escribir y aumentas el size futuro del metadata (que es cant de caracteres escritos)
					//Un byte por caracter
					cantidadEscrita++;
					cantidad--;
					cantidadDeCaracteresEscritas++;

				}
			}
		}
	}
	else
	{
		//Si entras al else, ya existe un bloque en uso, entonces terminas de llenar ese bloque y vas a otro

		bloqueAUsar = atoi(bloquesUsados[cantBloques -1]);//atoi ==> convierte un array a int ==> Agarras el ultimo bloque que llega del metadata y lo transformas a int
		//Con esto basicamente accedes al bloque directamente

		// Este for (el de int j) lo que hace es escribir la cantidad de letras hasta llenar ese bloque, si lo llena y le falta
		// caracteres, va al otro for (el de int i) a buscar el proximo bloque libre para seguir llenando
		for(int j = 0; j < cantidad; j++){

			//Con este if, lo que pregunto es que si el bloque esta lleno o no
			if(cantidadDeCaracteresEscritas % tamanio_bloque == 0){

				for(int i = 0; i < bloquesDelSistema; i++){
					if(bitarray_test_bit(bitmap, i) == 0){
						bloqueAUsar = i;
						char* bloqueNuevo = string_itoa(i);
						string_append(&actualizar_bloques, ",");
						string_append(&actualizar_bloques, bloqueNuevo); //Agrega el bloque seguido de una "," al array a
						//reemplazar el que esta en el metadata
						cantBloquesActualizacion++;
						bitarray_set_bit(bitmap, i);
						msync(&bitmap -> bitarray, tamanioBitmap, MS_SYNC);
						i = bloquesDelSistema;
					}
				}
			}
			pthread_mutex_lock(&mutexEscrituraBloques);
			memcpy(copiaBlock + (bloqueAUsar * tamanio_bloque) + (cantidadDeCaracteresEscritas % tamanio_bloque), &caracter, sizeof(char));
			pthread_mutex_unlock(&mutexEscrituraBloques);
			cantidadDeCaracteresEscritas++;

		}
	}
		//PROCEDEMOS A ACTUALIZAR EL METADATA CORRESPONDIENTE
		char* actualizarCantidad = string_itoa(cantBloquesActualizacion);
		char* actualizarBloques = string_new();
		char* actualizarSize = string_itoa(cantidadDeCaracteresEscritas);

		//Empiezo a generar el array que contiene los nuevos bloques actualizados
		string_append(&actualizarBloques, "[");

		for(int i = 0; i < cantBloques; i++){
			if(i==0){
				string_append(&actualizarBloques, bloquesUsados[i]);
			}
			else
			{
				string_append(&actualizarBloques, ",");
				string_append(&actualizarBloques, bloquesUsados[i]);
			}
		}

		string_append(&actualizarBloques, actualizar_bloques);
		string_append(&actualizarBloques, "]");

		//Actualizamos metadata o la bitacora
		actualizar_metadata(actualizarBloques, actualizarSize, actualizarCantidad, rutita);
}


void generar_bitacora(int idTripulante){

	char* id_trip = string_itoa(idTripulante);
	char* ruta_bitacora = string_new();
	string_append(&ruta_bitacora, punto_montaje);
	string_append(&ruta_bitacora, "/Files/Bitacoras/Tripulante");
	string_append(&ruta_bitacora, id_trip);
	string_append(&ruta_bitacora, ".ims");

	FILE* metadata_fd = fopen(ruta_metadata, "rb");

	if (verificar_existencia(ruta_bitacora) == 1) {
		fclose(metadata_fd);
		printf("Existe esa bitacora!");
		log_info(logger, "Bitacora encontrada");
		return;
	}

	metadata_fd = fopen(ruta_metadata, "w");
	t_config* bitacora_config = malloc(sizeof(t_config));
	bitacora_config->path = ruta_metadata;
	bitacora_config->properties = dictionary_create();

	dictionary_put(bitacora_config->properties, "SIZE", string_itoa(0));
	dictionary_put(bitacora_config->properties, "BLOCK_COUNT", string_itoa(0));
	dictionary_put(bitacora_config->properties, "BLOCKS", "[]");

	config_save(bitacora_config);
}


void escribir_en_bitacora(int idTripulante, char* texto){
	char* id_trip = string_itoa(idTripulante);
	char* ruta_bitacora = string_new();
	string_append(&ruta_bitacora, punto_montaje);
	string_append(&ruta_bitacora, "/Files/Bitacoras/Tripulante");
	string_append(&ruta_bitacora, id_trip);
	string_append(&ruta_bitacora, ".ims");

	if(verificar_existencia(ruta_bitacora) == 1){
		int longitud = strlen(texto);
		//Escribe en la bitacora!
		for (int i = 0; i < longitud; i++){
			escribirEnBloque(longitud, texto[i], ruta_bitacora);
		}
	}
	else
	{
		printf("No existe esa bitacora!");
		log_error(logger, "No existe esa bitacora!");
	}
}


void crear_archivo_files(){
	crear_metadata_basura();
	crear_metadata_comida();
	crear_metadata_oxigeno();
}

void crear_metadata_oxigeno(){
	agregar_datos_metadata("Oxigeno", "O");
}

void agregar_datos_metadata(char* archivo, char* valor){
	char* ruta_metadata = string_new();
	string_append(&ruta_metadata, punto_montaje);
	string_append(&ruta_metadata, "/Files/");
	string_append(&ruta_metadata, archivo);
	string_append(&ruta_metadata, ".ims");

	FILE* metadata_fd = fopen(ruta_metadata, "rb");

	if (verificar_existencia(ruta_metadata) == 1) {
		fclose(metadata_fd);
		printf("Esa metadata ya existe!");
		log_info(logger, "Metadata encontrada");
		return;
	}

	metadata_fd = fopen(ruta_metadata, "w");
	t_config* metadata_config = malloc(sizeof(t_config));
	metadata_config->path = ruta_metadata;
	metadata_config->properties = dictionary_create();

	dictionary_put(metadata_config->properties, "SIZE", string_itoa(0));
	dictionary_put(metadata_config->properties, "BLOCK_COUNT", string_itoa(0));
	dictionary_put(metadata_config->properties, "BLOCKS", "[]");
	dictionary_put(metadata_config->properties, "CARACTER_LLENADO", valor);
	dictionary_put(metadata_config->properties, "MD5", string_itoa(0));
	//FALTA MD5!

	config_save(metadata_config);
}

void actualizar_metadata(char* valorBlocks, char* valorSize, char* valorBlockCount, char* ruta){
	t_config* metadata_config = malloc(sizeof(t_config));
	metadata_config->path = ruta;
	metadata_config->properties = dictionary_create();

	dictionary_put(metadata_config->properties, "SIZE", valorSize);
	dictionary_put(metadata_config->properties, "BLOCK_COUNT", valorBlockCount);
	dictionary_put(metadata_config->properties, "BLOCKS", valorBlocks);

	config_save(metadata_config);
}


void crear_metadata_basura(){
	agregar_datos_metadata("Basura", "B");

}

void crear_metadata_comida(){
	agregar_datos_metadata("Comida", "C");
}

int verificar_existencia(char* nombre_archivo){
	FILE* file;
	int retorno = 0;

	if((file = fopen(nombre_archivo, "r"))){
		retorno = 1;
		fclose(file);
	}
	return retorno;
}

void agregarCaracter(int cantidad, char caracter){
	//ESTA FUNCION ES INVOCADA CUANDO GENERAMOS UN RECURSO

	char* rutita = string_new();
	string_append(&rutita, punto_montaje);

	switch (caracter){
		case 'O':
			string_append(&rutita, "/Files/Oxigeno.ims");
			escribirEnBloque(cantidad, caracter, rutita);
			break;

		case 'o':
			string_append(&rutita, "/Files/Oxigeno.ims");
			escribirEnBloque(cantidad, 'O', rutita);
			break;

		case 'B':
			string_append(&rutita, "/Files/Basura.ims");
			escribirEnBloque(cantidad, caracter, rutita);
			break;

		case 'b':
			string_append(&rutita, "/Files/Basura.ims");
			escribirEnBloque(cantidad, 'B', rutita);
			break;

		case 'C':
			string_append(&rutita, "/Files/Comida.ims");
			escribirEnBloque(cantidad, caracter, rutita);
			break;

		case 'c':
			string_append(&rutita, "/Files/Comida.ims");
			escribirEnBloque(cantidad, 'C', rutita);
			break;

		default:
			printf("No se selecciono un caracter correcto");
			break;
	}
}

t_log* iniciar_logger(char* logger_path){
	t_log *logger;
	if((logger = log_create(logger_path, "cliente", 0, LOG_LEVEL_INFO)) == NULL){
		printf("No se puede leer el logger");
		exit(1);
	}
	return logger;
}

t_config* leer_config(char* config_path){
	t_config* config;
	if((config = config_create(config_path)) == NULL){
		printf("No se pudo leer la config");
		exit(2);
	}
	return config;
}

//int crear_server(char* puerto,char* ip,int backlog){
//
//	struct addrinfo hints;
//	struct addrinfo *serverInfo;
//
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
//	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
//	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP
//
//	getaddrinfo(NULL, puerto, &hints, &serverInfo);
//
//	/*for (p=servinfo; p != NULL; p = p->ai_next)
//    {
//        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
//            continue;
//
//        int activado = 1;
//        setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEADDR,&activado,sizeof(activado));
//
//        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
//            close(socket_servidor);
//            continue;
//        }
//        break;
//    }*/
//
//	int listenningSocket;
//
//	listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
//
//	bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);
//	freeaddrinfo(serverInfo);
//
//	listen(listenningSocket, backlog);
//
//	freeaddrinfo(serverInfo);
//	return listenningSocket;
//}
//
//int esperar_cliente(int socket_server){
//		struct sockaddr_in addr;
//		socklen_t addrlen = sizeof(addr);
//
//		int socket_cliente = accept(socket_server, (struct sockaddr *) &addr, &addrlen);
//
//
//		printf("Cliente conectado. Esperando mensajes:\n");
//	/*
//		while (status != 0){
//			status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
//			if (status != 0) printf("%s", package);
//
//		}
//	*/
//		return socket_cliente;
//
//
//}
//
//void terminar_servidor(int socket_cliente){
//	close(socket_cliente);
//}
//int crear_conexion(char* ip, char* puerto)
//{
//	struct addrinfo hints;
//	struct addrinfo *server_info;
//
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_flags = AI_PASSIVE;
//
//	getaddrinfo(ip, puerto, &hints, &server_info);
//
//	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
//
//	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
//		printf("error");
//
//	freeaddrinfo(server_info);
//
//	return socket_cliente;
//}
//
//int iniciar_servidor(char* ip, int port){
//	int socket_servidor;
//	char* puerto = string_itoa(port); //Crea un string a partir de un numero
//	struct addrinfo hints, *servinfo, *p;
//
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_flags = AI_PASSIVE;
//
//	getaddrinfo(ip, puerto, &hints, &servinfo);
//
//	for (p=servinfo; p != NULL; p = p->ai_next)
//	{
//		if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
//			continue;
//
//		int activado = 1;
//		setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEADDR,&activado,sizeof(activado));
//
//		if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
//			close(socket_servidor);
//			continue;
//		}
//		break;
//	}
//	printf("Servidor corriendo correctamente\n");
//	listen(socket_servidor, SOMAXCONN);
//
//	freeaddrinfo(servinfo);
//
//	free(puerto);
//
//	//log_trace(logger, "Listo para escuchar a mi cliente");
//
//	return socket_servidor;
//}
//
//void liberar_conexion(int socket_cliente)
//{
//	close(socket_cliente);
//}
//
//void liberar_paquete(t_paquete *paquete, void* stream){
//	free(paquete -> buffer -> stream);
//	free(paquete -> buffer);
//	free(paquete);
//}
