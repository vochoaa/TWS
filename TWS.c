#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>

#define BUFFER_SIZE 32767
#define MAX_FILE_SIZE 32767
#define MAX_CLIENTS 10

// void *connection_handler(void *); para función de hilos

// Función para obtener la fecha y hora actual en formato HTTP
void get_current_date(char *date, size_t date_size)
{
    time_t now;
    time(&now);
    struct tm *tm_info = gmtime(&now);
    strftime(date, date_size, "%a, %d %b %Y %H:%M:%S GMT", tm_info);
}

char *get_content_type_by_extension(const char *file_extension)
{
    if (strcmp(file_extension, ".html") == 0)
    {
        return "text/html";
    }
    else if (strcmp(file_extension, ".txt") == 0)
    {
        return "text/plain";
    }
    else if (strcmp(file_extension, ".json") == 0)
    {
        return "application/json";
    }
    else if (strcmp(file_extension, ".jpg") == 0)
    {
        return "img/jpg";
    }
    else
    {
        // Retornar un valor por defecto o manejar otros casos
        return "application/octet-stream";
    }
}

int main(int argc, char const *argv[])
{

    int port = 8080;
    int server_sock, client_sock, *new_sock, file_fd; // Descriptores del servidor y del cliente.
    struct sockaddr_in address;                       // Estructuras para almacenar las direcciones del servidor y del cliente.
    int addrlen = sizeof(address);                    // Tamaño de la direccion del cliente.
    char buffer[BUFFER_SIZE] = {0};                   // Buffer para almacenar los datos recibidos y enviados.
    char file_content[MAX_FILE_SIZE] = {0};           // Contenido del archivo.
    char response[32767] = {0};                        // Respuesta del servidor.
    char *content_type;                               // Tipo de archivo.
    const char *server = "TWS";                       // Nombre del servidor.
    char date[128];                                   // Fecha.
    get_current_date(date, sizeof(date));             // Función para la fecha.

    // Pasar IP por medio de un archivo.
    FILE *file;
    char *content;
    long size;
    file = fopen("IP.txt", "r");
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    content = (char *)malloc(size + 1);
    fread(content, size, 1, file);
    content[size] = '\0';
    fclose(file);
    char *ip = content;

    // Crear el socket del servidor.
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("[-]Error al crear el socket.");
        exit(1);
    }
    printf("[+]TCP server socket creado.\n");

    // Configurar la direccion del servidor.
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip); // Para modificar la IP

    // Enlazar el socket con la direccion del servidor.
    if (bind(server_sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("[-]Error al asignar dirección y puerto al socket.");
        exit(1);
    }
    printf("[+]Conectar al número de puerto: %d\n", port);

    // Escuchar por conexiones entrantes.
    if (listen(server_sock, MAX_CLIENTS) == -1)
    {
        perror("Error al eesperar por conexiones entrantes");
        exit(1);
    }

    printf("Servidor iniciado. Esperando conexiones...\n");

    // Aceptar las conexiones que entran y comunicarse con el cliente.
    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&address, &addrlen);
        if (client_sock == -1)
        {
            perror("Error al aceptar la conexión del cliente");
            exit(1);
        }

        printf("Nuevo cliente conectado: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        /* // Creando un nuevo hilo para manejar la conexion.
        pthread_t thread;
        new_sock = (int *)malloc(1);
        *new_sock = client_sock;
        if (pthread_create(&thread, NULL, connection_handler, (void *)new_sock) < 0)
        {
            perror("hilo creado");
            exit(EXIT_FAILURE);
        } */

        // Leer la solicitud.
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);

        // Variables para el parseo de la petición.
        char method[10];
        char url[1000];
        char version[20];
        char headers[1000];
        char body[1000];

        // Encontrar los espacios en blanco para dividir los headers del body.
        char temp[4096];
        strcpy(temp, buffer);
        char *line = strstr(temp, "\r\n\r\n");
        if (line != NULL)
        {
            strcpy(body, line + 4);
            *line = '\0';
        }
        // Parseo de los headers.
        sscanf(buffer, "%s %s %s\n%[^\n]", method, url, version, headers);

        // Imprimir los valores parseados.
        printf("Método: %s\n", method);
        printf("Url: %s\n", url);
        printf("Versión: %s\n", version);

        // Imprimir los headers.
        char *header = strtok(temp, "\r\n");
        printf("Headers:\n");
        while (header != NULL)
        {
            if (strstr(header, method) == NULL && strstr(header, url) == NULL && strstr(header, version) == NULL)
            {
                printf("%s\n", header);
            }
            header = strtok(NULL, "\r\n");
        }

        // Ingresar al método de acuerdo a la petición.
        if (bytes_received > 0)
        {
            // Código para manejar la solicitud GET.
            if (strcmp(method, "GET") == 0)
            {
                // Encontrar el último / URL.
                char *last_slash = strrchr(url, '/');
                if (last_slash != NULL)
                {
                    char *name_file = last_slash + 1;
                    const char *file_extension = strrchr(name_file, '.');
                    content_type = get_content_type_by_extension(file_extension);
                    printf("El nombre del archivo final es: %s\n", name_file);

                    // Abrir el archivo que queremos enviar.
                    if ((file_fd = open(name_file, O_RDONLY)) < 0)
                    {
                        // Error 404.
                        perror("Falló al abrir el archivo.");
                        sprintf(response, "HTTP/1.1 404 Not Found\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, 0);
                        send(client_sock, response, strlen(response), 0);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    printf("La url no contiene un archivo valido. \n");
                }

                // Construir la respuesta HTTP.
                int file_size = read(file_fd, file_content, MAX_FILE_SIZE);
                sprintf(response, "HTTP/1.1 200 OK\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, file_size);
                send(client_sock, response, strlen(response), 0);
                send(client_sock, file_content, file_size, 0);
                printf("Contenido del archivo enviado.\n");
            }

            else if (strcmp(method, "HEAD") == 0)
            {
                // Encontrar el último / URL.
                char *last_slash = strrchr(url, '/');
                if (last_slash != NULL)
                {
                    char *name_file = last_slash + 1;
                    const char *file_extension = strrchr(name_file, '.');
                    content_type = get_content_type_by_extension(file_extension);
                    printf("El nombre del archivo final es: %s\n", name_file);

                    // Abrir el archivo que queremos enviar.
                    if ((file_fd = open(name_file, O_RDONLY)) < 0)
                    {
                        // Error 404.
                        perror("Falló al abrir el archivo.");
                        sprintf(response, "HTTP/1.1 404 Not Found\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, 0);
                        send(client_sock, response, strlen(response), 0);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    printf("La url no contiene un archivo valido.\n");
                }

                // Construir la respuesta HTTP.
                int file_size = read(file_fd, file_content, MAX_FILE_SIZE);
                sprintf(response, "HTTP/1.1 200 OK\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, file_size);
                send(client_sock, response, strlen(response), 0);
                printf("Confirmado.\n");
            }

            // Código para manejar la solicitud POST.
            else if (strcmp(method, "POST") == 0)
            {
                // Encontrar el último / URL.
                char *last_slash = strrchr(url, '/');
                if (last_slash != NULL)
                {
                    char *name_file = last_slash + 1;
                    const char *file_extension = strrchr(name_file, '.');
                    content_type = get_content_type_by_extension(file_extension);
                    printf("El nombre del archivo final es: %s\n", name_file);

                    // Abrir el archivo que queremos enviar
                    if ((file_fd = open(name_file, O_RDONLY)) < 0)
                    {
                        // Error 404.
                        perror("Falló al abrir el archivo.");
                        sprintf(response, "HTTP/1.1 404 Not Found\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, 0);
                        send(client_sock, response, strlen(response), 0);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    printf("La url no contiene un archivo valido. \n");
                }

                // Construir la respuesta HTTP.
                int file_size = read(file_fd, file_content, MAX_FILE_SIZE);
                sprintf(response, "HTTP/1.1 200 OK\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, file_size);
                send(client_sock, response, strlen(response), 0);
                send(client_sock, file_content, file_size, 0);
                printf("Recibido del cliente: \n");
                printf("%s\n", body);

                // Escribir el body en un archivo de texto.
                FILE *file_body = fopen("example.txt", "w");
                if (file_body != NULL)
                {
                    fwrite(body, strlen(body), 1, file_body);
                    fclose(file_body);
                    printf("El body se ha guardado en example.txt\n");
                }
                else
                {
                    printf("No se pudo abrir el archivo para escribir\n");
                }
            }
            else
            {
                // Error 400.
                sprintf(response, "HTTP/1.1 400 Bad Request\nDate: %s\nServer: %s\nContent-Type: %s\nContent-Length: %d\nConnection: close\n\n", date, server, content_type, 0);
                send(client_sock, response, strlen(response), 0);
                exit(0);
            }
        }
    }
    close(client_sock);
    return 0;
}

/* // Función de hilos para manejar conexiones entrantes
void *connection_handler(void *socket_desc)
{
    // Obtiene el descriptor del socket
    int sock = *(intptr_t *)socket_desc;
    char buffer[1024] = {0};
    int bytes_read;
    // Recibe datos del cliente
    while ((bytes_read = read(sock, buffer, 1024)) > 0)
    {
        printf("Recibido: %s\n", buffer);

        // Datos devueltos al cliente
        if (write(sock, buffer, strlen(buffer)) < 0)
        {
            perror("Escribir");
            exit(EXIT_FAILURE);
        }
    }
    // Descriptor memory Cerrar el socket y libera la memoria del descriptor de socket
    close(sock);
    free(socket_desc);
    return NULL;
} */
