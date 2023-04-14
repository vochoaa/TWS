#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include <sys/types.h>
#include <pthread.h>

void *connection_handler(void *);

int main(int argc,char const *argv[]){

  WSADATA wsa;
  int resultado = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (resultado != 0) {
     printf("Error al inicializar Winsock: %d\n", resultado);
     return 1;
    }

  char *ip = "127.0.0.1";
  int port = 8080;
  #define BUFFER_SIZE 4096
  int server_sock, client_sock, *new_sock; //Descriptores del servidor y del cliente
  struct sockaddr_in address; //Estructuras para almacenar las direcciones del servidor y del cliente
  int opt = 1;
  int addrlen = sizeof(address); //Tamaño de la direccion del cliente
  char buffer[BUFFER_SIZE] = {0}; //Buffer para almacenar los datos recibidos y enviados
  char *response = NULL;

  //Crear el socket del servidor
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0){
    perror("[-]Error al crear el socket");
    exit(1);
  }
  printf("[+]TCP server socket creado.\n");

  //Configurar la direccion del servidor
  address.sin_family = AF_INET;
  address.sin_port = port;
  address.sin_addr.s_addr = inet_addr(ip); //Para modificar la IP

  //Enlazar el socket con la direccion del servidor
  if (bind(server_sock, (struct sockaddr*)&address, sizeof(address))<0){
    perror("[-]Error al asignar dirección y puerto al socket");    
    exit(1);
  }
  printf("[+]Conectar al número de puerto: %d\n", port);

      //Escuchar las conexiones entrantes
      if (listen(server_sock, 3) < 0){
        perror("escuchar");
        exit(EXIT_FAILURE);
    }
  printf("Conectando...\n");

   //Aceptar las conexiones que entran y comunicarse con el cliente
   while ((client_sock = accept(server_sock, (struct sockaddr*)&address,&addrlen))){
        printf("Nuevo cliente conectado: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // Creando un nuevo hilo para manejar la conexion
        pthread_t thread;
        new_sock = (int*)malloc(1);
        *new_sock = client_sock;
        if (pthread_create(&thread, NULL, connection_handler, (void*)new_sock) < 0) {
            perror("hilo creado");
            exit(EXIT_FAILURE);
        }
    }

    //Leer la solicitud
    int  bytesrecibidos = recv(client_sock, buffer, BUFFER_SIZE,0);
    char *method = strtok(buffer, " ");
    char *url = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");
    char *header_line;
    sscanf(buffer,version,method,url,header_line); 

    // Leer los encabezados HTTP
    char headers[10];
    int i = 0;
    while ((header_line = strtok(NULL, "\r\n"))) {
        if (header_line == "\0") break;
            headers[i] = *header_line;
            i++;
        }

    // Procesar la cadena de consulta
    char *query_string = strchr(url, '?');
    if (query_string != NULL) {
            printf("Parámetros de consulta: %s\n", query_string+1);
    }

    if(bytesrecibidos>0){
    // Código para manejar la solicitud GET
        if (strcmp(method, "GET") == 0){
        
        // Imprimir los resultados
        printf("Método HTTP: %s\n", method);
        printf("URL solicitada: %s\n", url);
        printf("Versión HTTP: %s\n", version);
        printf("Encabezados HTTP:\n");
        for (int j=0; j<i; j++) {
            printf("%s\n", headers[j]);
        }

        // Construir la respuesta HTTP
        sprintf(response, "HTTP/1.1 200 OK\r\n"
                            "Content-Length: %ld\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\n"
                            "%s", strlen(url), url);

        // Enviar la respuesta HTTP
        send(client_sock, response, strlen(response), 0);
        printf("Respuesta enviada\n");
        }

        // Código para manejar la solicitud HEAD
        else if (strcmp(method, "HEAD") == 0){
        
        // Imprimir los resultados
        printf("Método HTTP: %s\n", method);
        printf("URL solicitada: %s\n", url);
        printf("Versión HTTP: %s\n", version);
        printf("Encabezados HTTP:\n");
        for (int j=0; j<i; j++) {
            printf("%s\n", headers[j]);
        }

        // Construir la respuesta HTTP
        sprintf(response, "HTTP/1.1 200 OK");

        // Enviar la respuesta HTTP
        send(client_sock, response, strlen(response), 0);
        printf("Respuesta enviada\n");
        }

        // Código para manejar la solicitud POST
        else if (strcmp(method, "POST") == 0){
        // Leer los datos enviados por el cliente
        char *body = strtok(NULL, "");
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                    "<html><body><h1>Los datos han sido recibidos correctamente.</h1></body></html>";
        send(client_sock, response, strlen(response), 0);
        }

        // Procesar la respuesta del servidor
        else if (strncmp(buffer, "HTTP/1.1 200 OK", 15) == 0){
            printf("Response 200: OK\n");
        }

        // Código para manejar error 400
        else if(strncmp(buffer, "HTTP/1.1 400 Bad Request", 23) == 0){
            printf("Error 400: Bad Request\n");
            exit(EXIT_FAILURE);
        }

        // Código para manejar error 404
        else if(strncmp(buffer, "HTTP/1.1 404 Not Found", 22) == 0){
        printf("Error 404: Not Found\n");
        exit(EXIT_FAILURE);
        }
    }
    return 0;
    WSACleanup();
}

// Función de hilos para manejar conexiones entrantes
void *connection_handler(void *socket_desc){
    // Obtiene el descriptor del socket
    int sock = *(intptr_t*) socket_desc;
    char buffer[1024] = {0};
    int bytes_read;

    // Recibe datos del cliente
    while ((bytes_read = read(sock, buffer, 1024)) > 0){
        printf("Recibido: %s\n", buffer);

        // Datos devueltos al cliente
        if (write(sock, buffer, strlen(buffer)) < 0){
            perror("Escribir");
            exit(EXIT_FAILURE);
        }
    }

    // Descriptor memory Cerrar el socket y libera la memoria del descriptor de socket
 
  close(sock);
    free(socket_desc);

    return NULL;
}