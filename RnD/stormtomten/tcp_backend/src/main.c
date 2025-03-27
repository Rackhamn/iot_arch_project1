#include <arpa/inet.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

volatile int run = 1;

void catch_ctrl_c(int sig) { run = 0; }

// Server setup
int setup_server(int port) {
  int server_socket;
  struct sockaddr_in server_addr;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("Socket creation failed");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Bind failed");
    exit(1);
  }

  if (listen(server_socket, MAX_CLIENTS) < 0) {
    perror("Listen failed");
    exit(1);
  }

  printf("Server listening on port %d...\n", port);
  return server_socket;
}

// Accepting new connections
int accept_new_client(int server_socket, fd_set *read_fds, int *max_fd) {
  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);

  int client_socket =
      accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);

  if (client_socket < 0) {
    perror("Accept failed");
    return -1;
  }

  printf("New client connected: %d\n", client_socket);
  FD_SET(client_socket, read_fds);
  if (client_socket > *max_fd)
    *max_fd = client_socket;

  return client_socket;
}

// Process SQL-query
char *process_client_query(sqlite3 *db, const char *tag_id) {
  sqlite3_stmt *stmt;
  char *result = malloc(BUFFER_SIZE);
  const char *query = "SELECT description FROM tag WHERE tag_id = ?;";

  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    snprintf(result, BUFFER_SIZE, "SQL Error: %s", sqlite3_errmsg(db));
    return result;
  }

  /* // Debug block
  fprintf(stderr, "Bud: %s\n", tag_id);
  fprintf(stderr, "Bud: %ld\n", strlen(tag_id));
  i*/

  if (sqlite3_bind_text(stmt, 1, tag_id, -1, SQLITE_STATIC) != SQLITE_OK) {
    snprintf(result, BUFFER_SIZE, "SQL Error: %s", sqlite3_errmsg(db));
    return result;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *result_value = (const char *)sqlite3_column_text(stmt, 0);
    snprintf(result, BUFFER_SIZE, "%s\n", result_value);
  } else {
    snprintf(result, BUFFER_SIZE, "No results found.\n");
  }

  sqlite3_finalize(stmt);

  return result;
}

// Handle client communication
int handle_client(int client_socket, sqlite3 *db) {
  char buffer[BUFFER_SIZE] = {0};
  int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

  if (bytes_received == 0) {
    printf("Client disconnected: %d\n", client_socket);
    return 1; // Signal client disconnect
  } else if (bytes_received < 0) {
    perror("recv() error");
    return 1;
  }

  // Ensure string termination (handling newline from telnet).
  if (bytes_received > 2) {
    buffer[bytes_received - 2] = '\0';
  }

  printf("Recieved query: %s from client: %d\n", buffer, client_socket);
  char *result = process_client_query(db, buffer);

  if (send(client_socket, result, strlen(result), 0) < 0) {
    perror("Send failed");
    free(result);
    return 2; // Signal Send failure
  }

  free(result); // Free memory from process_client_query

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <port> <database_path>\n", argv[0]);
    return 1;
  }

  signal(SIGINT, catch_ctrl_c);
  int port = atoi(argv[1]);
  const char *db_path = argv[2];

  int server_socket = setup_server(port);
  sqlite3 *db;

  // Open database in readonly
  if (sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
    return 1;
  }

  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(server_socket, &read_fds);
  int max_fd = server_socket;

  while (run) {
    fd_set temp_fds = read_fds;
    if (select(max_fd + 1, &temp_fds, NULL, NULL, NULL) < 0) {
      perror("select() error");
      break;
    }

    if (FD_ISSET(server_socket, &temp_fds)) {
      accept_new_client(server_socket, &read_fds, &max_fd);
    }

    for (int fd = 0; fd <= max_fd; fd++) {
      if (fd != server_socket && FD_ISSET(fd, &temp_fds)) {
        handle_client(fd, db);
        close(fd);
        printf("Closing client connection: %d\n", fd);
        FD_CLR(fd, &read_fds);
      }
    }
  }
  printf("Shutting down\n");
  // Shutting all connections
  for (int fd = 0; fd <= max_fd; fd++) {
    if (FD_ISSET(fd, &read_fds)) { // Checking if fd is active
      if (fd != server_socket) {   // Close if not server
        printf("Closing client connection: %d\n", fd);
        close(fd);
        FD_CLR(fd, &read_fds);
      }
    }
  }

  close(server_socket);
  printf("Server socket closed\n");

  sqlite3_close(db);
  printf("Database closed\n");

  return 0;
}
