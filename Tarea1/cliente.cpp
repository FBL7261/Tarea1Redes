#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

void clienteJuega(int socket_fd) {
    char buffer[1024];
    string mensaje;

    int n_bytes = recv(socket_fd, buffer, 1024, 0);
    buffer[n_bytes] = '\0';
    cout << buffer << endl;

    while (true) {
        cout << "Ingresa tu comando (C columna o Q para salir): ";
        getline(cin, mensaje);

        send(socket_fd, mensaje.c_str(), mensaje.size(), 0);

        if (mensaje[0] == 'Q') {
            break;
        }

        n_bytes = recv(socket_fd, buffer, 1024, 0);
        buffer[n_bytes] = '\0';
        cout << "Estado del tablero:\n" << buffer << endl;

        if (strstr(buffer, "Ganaste") || strstr(buffer, "Gana el servidor") || strstr(buffer, "Empate")) {
            break;
        }

        n_bytes = recv(socket_fd, buffer, 1024, 0);
        buffer[n_bytes] = '\0';
        cout << "Estado del tablero:\n" << buffer << endl;

        if (strstr(buffer, "Ganaste") || strstr(buffer, "Gana el servidor") || strstr(buffer, "Empate")) {
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <dirección IP> <puerto>\n";
        return 1;
    }

    const char *ip = argv[1];
    int puerto = atoi(argv[2]);

    int socket_fd;
    struct sockaddr_in direccionServidor;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear el socket\n";
        return 1;
    }

    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = htons(puerto);

    if (inet_pton(AF_INET, ip, &direccionServidor.sin_addr) <= 0) {
        cerr << "Dirección IP inválida\n";
        return 1;
    }

    if (connect(socket_fd, (struct sockaddr *)&direccionServidor, sizeof(direccionServidor)) < 0) {
        cerr << "Error al conectar\n";
        return 1;
    }

    clienteJuega(socket_fd);

    close(socket_fd);
    return 0;
}
