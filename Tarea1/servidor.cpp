#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>
#include <cstdlib>
#include <ctime>

using namespace std;

const int FILAS = 6;
const int COLUMNAS = 7;

struct Tablero {
    vector<vector<char>> grid;

    Tablero() : grid(FILAS, vector<char>(COLUMNAS, '.')) {}

    void display() {
        for (auto& fila : grid) {
            for (char c : fila) {
                cout << c << " ";
            }
            cout << "\n";
        }
        cout << "1 2 3 4 5 6 7\n\n";
    }

    string toString() {
        string tableroStr;
        for (auto& fila : grid) {
            for (char c : fila) {
                tableroStr += c;
                tableroStr += " ";
            }
            tableroStr += "\n";
        }
        tableroStr += "1 2 3 4 5 6 7\n";
        return tableroStr;
    }

    bool hacerJugada(int col, char ficha) {
        for (int i = FILAS - 1; i >= 0; --i) {
            if (grid[i][col] == '.') {
                grid[i][col] = ficha;
                return true;
            }
        }
        return false;
    }

    bool chequearVictoria(char ficha) {
        for (int i = 0; i < FILAS; ++i) {
            for (int j = 0; j < COLUMNAS; ++j) {
                if (j + 3 < COLUMNAS &&
                    ficha == grid[i][j] && ficha == grid[i][j + 1] && ficha == grid[i][j + 2] && ficha == grid[i][j + 3])
                    return true;
                if (i + 3 < FILAS &&
                    ficha == grid[i][j] && ficha == grid[i + 1][j] && ficha == grid[i + 2][j] && ficha == grid[i + 3][j])
                    return true;
                if (i + 3 < FILAS && j + 3 < COLUMNAS &&
                    ficha == grid[i][j] && ficha == grid[i + 1][j + 1] && ficha == grid[i + 2][j + 2] && ficha == grid[i + 3][j + 3])
                    return true;
                if (i + 3 < FILAS && j - 3 >= 0 &&
                    ficha == grid[i][j] && ficha == grid[i + 1][j - 1] && ficha == grid[i + 2][j - 2] && ficha == grid[i + 3][j - 3])
                    return true;
            }
        }
        return false;
    }

    bool estaLleno() {
        for (int j = 0; j < COLUMNAS; ++j) {
            if (grid[0][j] == '.') return false;
        }
        return true;
    }
};

void jugar(int socket_cliente, struct sockaddr_in direccionCliente) {
    Tablero tablero;
    char buffer[1024];
    memset(buffer, '\0', sizeof(char)*1024);
    int n_bytes = 0;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(direccionCliente.sin_addr), ip, INET_ADDRSTRLEN);
    cout << "Juego nuevo[" << ip << ":" << ntohs(direccionCliente.sin_port) << "]\n";

    char fichaCliente = 'C'; 
    char fichaServidor = 'S';

    bool turnoCliente = rand() % 2 == 0;
    string mensajeInicio = turnoCliente ? "Cliente comienza\n" : "Servidor comienza\n";
    cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: " << mensajeInicio;
    send(socket_cliente, mensajeInicio.c_str(), mensajeInicio.size(), 0);

    while (true) {
        if (turnoCliente) {
            n_bytes = recv(socket_cliente, buffer, 1024, 0);
            buffer[n_bytes] = '\0';
            if (buffer[0] == 'Q') break;

            int col = atoi(&buffer[1]) - 1;
            if (col < 0 || col >= COLUMNAS || !tablero.hacerJugada(col, fichaCliente)) {
                send(socket_cliente, "Movimiento invalido\n", 20, 0);
            } else {
                cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: cliente juega columna " << col + 1 << ".\n";
                string tableroStr = tablero.toString();
                send(socket_cliente, tableroStr.c_str(), tableroStr.size(), 0);
                if (tablero.chequearVictoria(fichaCliente)) {
                    send(socket_cliente, "Ganaste\n", 8, 0);
                    cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: gana cliente.\n";
                    break;
                }
                if (tablero.estaLleno()) {
                    send(socket_cliente, "Empate\n", 7, 0);
                    cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: empate.\n";
                    break;
                }
                turnoCliente = false;
            }
        } else {
            int col = rand() % COLUMNAS;
            while (!tablero.hacerJugada(col, fichaServidor)) {
                col = rand() % COLUMNAS;
            }
            cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: servidor juega columna " << col + 1 << ".\n";
            string tableroStr = tablero.toString();
            send(socket_cliente, tableroStr.c_str(), tableroStr.size(), 0);
            if (tablero.chequearVictoria(fichaServidor)) {
                send(socket_cliente, "Gana el servidor\n", 17, 0);
                cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: gana servidor.\n";
                break;
            }
            if (tablero.estaLleno()) {
                send(socket_cliente, "Empate\n", 7, 0);
                cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: empate.\n";
                break;
            }
            turnoCliente = true;
        }
    }
    cout << "Juego [" << ip << ":" << ntohs(direccionCliente.sin_port) << "]: fin del juego.\n";
    close(socket_cliente);
}

void handle_connection(int client_socket, struct sockaddr_in client_addr) {
    jugar(client_socket, client_addr);
}

int main(int argc, char **argv) {
    srand(time(0));

    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <puerto>\n";
        return 1;
    }
    int port = atoi(argv[1]);
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear el socket de escucha\n";
        return 1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error al llamar a bind()\n";
        return 1;
    }
    if (listen(server_socket, 10) < 0) {
        cerr << "Error al llamar a listen()\n";
        return 1;
    }
    cout << "Esperando conexiones ...\n";
    socklen_t addr_size = sizeof(struct sockaddr_in);

    while (true) {
        int client_socket;
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) < 0) {
            cerr << "Error al llamar a accept()\n";
            continue;
        }
        thread client_thread(handle_connection, client_socket, client_addr);
        client_thread.detach();
    }
    return 0;
}
