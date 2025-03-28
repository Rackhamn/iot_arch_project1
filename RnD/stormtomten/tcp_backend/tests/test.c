#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define TEST_STRING "FE 5F F6 6F\r\n" // String to send to server
#define DEFAULT_CONNECTIONS 50
#define DEFAULT_PER_THREAD 2
#define DEFAULT_PORT 8080
#define DEFAULT_IP "127.0.0.1"

int counter = 0;

// Structure to hold thread data
typedef struct {
  int thread_id;
  int num_requests;
  char *server_ip;
  int server_port;
} ThreadData;

// Function to get time difference in seconds
double get_time_diff(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) +
         (end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

// Function to handle each client connection
void *client_thread(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  int sock;
  struct sockaddr_in server_addr;
  char buffer[BUFFER_SIZE];

  // Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    printf("Thread %d: Socket creation failed\n", data->thread_id);
    pthread_exit(NULL);
  }

  // Configure server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(data->server_port);
  server_addr.sin_addr.s_addr = inet_addr(data->server_ip);

  // Connect to server
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("Thread %d: Connection failed to %s:%d\n", data->thread_id,
           data->server_ip, data->server_port);
    close(sock);
    pthread_exit(NULL);
  }

  /*
  printf("Thread %d: Connected successfully to %s:%d\n", data->thread_id,
         data->server_ip, data->server_port);
         */

  // Send requests
  for (int i = 0; i < data->num_requests; i++) {
    // Send test string
    if (send(sock, TEST_STRING, strlen(TEST_STRING), 0) < 0) {
      printf("Thread %d: Send failed\n", data->thread_id);
      break;
    }

    // Receive response (optional, comment out if not needed)
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
      printf("Thread %d: Receive failed\n", data->thread_id);
      break;
    }

    buffer[bytes_received] = '\0';
    // printf("%s : %ld", buffer, strlen(buffer));
    if (strcmp(buffer, "En uppstoppad katt\r\n")) {
      /*
    printf("Thread %d: Request %d response received\n", data->thread_id,
           i + 1);
           */
      counter++;
    } else {
      printf("Thread %d: Request %d failed\n", data->thread_id, i + 1);
    }

    // Small delay between requests (in microseconds)
    usleep(800);
  }

  // Cleanup
  close(sock);
  free(data);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  // Default values
  char *server_ip = DEFAULT_IP;
  int server_port = DEFAULT_PORT;
  int num_threads = DEFAULT_CONNECTIONS;
  int requests_per_thread = DEFAULT_PER_THREAD;
  struct timespec start_time, end_time;

  // Check and parse command line arguments
  if (argc < 4) {
    printf("Usage: %s <server_ip> <port> <num_connections> "
           "[requests_per_thread]\n",
           argv[0]);
    printf("Using defaults: %s %d %d %d\n", server_ip, server_port, num_threads,
           requests_per_thread);
  } else {
    server_ip = argv[1];
    server_port = atoi(argv[2]);
    num_threads = atoi(argv[3]);

    if (argc > 4) {
      requests_per_thread = atoi(argv[4]);
    }
  }

  // Validate inputs
  if (server_port <= 0 || server_port > 65535) {
    printf("Error: Invalid port number. Using default: %d\n", DEFAULT_PORT);
    server_port = DEFAULT_PORT;
  }
  if (num_threads <= 0) {
    printf("Error: Invalid number of connections. Using default: %d\n",
           DEFAULT_CONNECTIONS);
    num_threads = DEFAULT_CONNECTIONS;
  }
  if (requests_per_thread <= 0) {
    printf("Error: Invalid number of requests. Using default: 100\n");
    requests_per_thread = 100;
  }

  printf("Starting stress test:\n");
  printf("Server: %s:%d\n", server_ip, server_port);
  printf("Connections: %d\n", num_threads);
  printf("Requests per connection: %d\n", requests_per_thread);

  pthread_t threads[num_threads];
  srand(time(NULL));

  // Start timer
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  // Create threads
  for (int i = 0; i < num_threads; i++) {
    ThreadData *data = malloc(sizeof(ThreadData));
    data->thread_id = i;
    data->num_requests = requests_per_thread;
    data->server_ip = server_ip;
    data->server_port = server_port;

    if (pthread_create(&threads[i], NULL, client_thread, (void *)data) != 0) {
      printf("Failed to create thread %d\n", i);
      free(data);
    }
    // Small delay between thread creation
    usleep(800);
  }

  // Wait for all threads to complete
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  // Stop timer
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  double execution_time = get_time_diff(start_time, end_time);
  double requests_per_second =
      (num_threads * requests_per_thread) / execution_time;

  printf("Stress test completed\n");

  printf("Total execution time: %.3f seconds\n", execution_time);
  printf("Total requests sent: %d\n", num_threads * requests_per_thread);
  printf("Requests per second: %.2f\n", requests_per_second);

  printf("%d Succesful replies\n", counter);
  return 0;
}
