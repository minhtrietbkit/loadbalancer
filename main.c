#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>

int select_next_upstream_index(int array_size, int last_used_upstream_index) {
  //Round robin
  int next_upstream = (last_used_upstream_index + 1) % array_size;
  return next_upstream;
}
int create_socket() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_fd < 0) {
    perror("LB: Socket create failed");
    exit(EXIT_FAILURE);
  }
  return socket_fd;
}

void connect_upstream (int local_socket, int upstream_port) {
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(upstream_port);
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (connect(local_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
    perror("LB: Connect upstream failed");
    exit(EXIT_FAILURE);
  }
}

// if is_blocking != 1, blocks until receive the 1st message and returns; otherwise blocks until the otherside closes connection
unsigned int receive_message(int socket_fd, ssize_t* response_buffer, ssize_t response_buffer_size, int is_blocking) {
  ssize_t temp_buffer[BUFSIZ];
  int total_recv_bytes = 0;
  while(1){
    int recv_bytes = recv(socket_fd, temp_buffer, BUFSIZ, 0);
    if (recv_bytes < 0) {
      perror("LB: recv failed");
      exit(EXIT_FAILURE);
    }
    if (recv_bytes == 0) {
      printf("LB: No more data to receive\n");
      close(socket_fd);
      break;
    }
    total_recv_bytes += recv_bytes;
    strcpy((char* )response_buffer, (char* )temp_buffer);
    if (is_blocking != 1) {
      break;
    }
  }
  printf("LB: Received %d bytes with the following contents\n", total_recv_bytes);
  printf("%s\n", (char* )temp_buffer);
  return total_recv_bytes;
}

unsigned int forward_message_upstream(int upstream_port, ssize_t* req_buffer, unsigned int bytes_to_send, ssize_t* res_buffer, unsigned int res_buffer_size) {
  int child_socket = create_socket();
  int bytes_received = 0;
  //Connect to upstream
  connect_upstream(child_socket, upstream_port);

  //send message
  if (send(child_socket, req_buffer, bytes_to_send, 0) < 0) {
    perror("LB: Send upstream failed");
    exit(EXIT_FAILURE);
  }

  //recv Response
  bytes_received = receive_message(child_socket, res_buffer, res_buffer_size, 1);
  return bytes_received;
}

int handle_downstream_connection(int child_socket, struct sockaddr_in client_addr) {

  return 0;
}

void main(int argc, char **argv) {
    printf("PID %d\n", getpid());
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return;
    }

    int port = atoi(argv[1]); // Convert string to integer
    printf("Port: %d\n", port);
    // Open SOCK_STREAM sockets to upstream hosts
    // For each req,
    // round robin select the next host

    // ESTABLISHING CONNECTION TO UPSTREAM HOSTS
    const int UPSTREAM_COUNT = 3;
    int upstream_ports[UPSTREAM_COUNT];
    upstream_ports[0] = 3000;
    upstream_ports[1] = 3000;
    upstream_ports[2] = 3000;

    int server_fd, child_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Define server sockaddr_in
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int last_used_upstream_index = 0;
    ssize_t req_buffer[BUFSIZ], res_buffer[BUFSIZ];

    // Create server socketfd
    server_fd = create_socket();

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
      perror("LB: Bind socket failed");
      exit(EXIT_FAILURE);
    }

   if (listen(server_fd, SOMAXCONN) < 0){
     perror("LB: Socket listen failed");
     exit(EXIT_FAILURE);
   }

   while(1){
     if ((child_fd = accept(server_fd, (struct sockaddr *) &client_addr, &addr_len)) < 0) {
       perror("LB: Accept downstream connection failed");
       exit(EXIT_FAILURE);
     }
     pid_t child_pid = fork();
     if (child_pid < 0) {
       perror("LB: Fork child process to handle TCP Connection failed");
     } else if (child_pid == 0) {
       printf("Child PID %d\n", getpid());
       int bytes_received = receive_message(child_fd, req_buffer, BUFSIZ, 0);

       last_used_upstream_index = select_next_upstream_index(UPSTREAM_COUNT, last_used_upstream_index);
       // Forward message and get the response
       unsigned int bytes_received_from_upstream;
       bytes_received_from_upstream = forward_message_upstream(upstream_ports[last_used_upstream_index], req_buffer, bytes_received, res_buffer, BUFSIZ);
       //printf("LB: Received %d bytes from upstream\n", bytes_received_from_upstream);
       // Forward response back
       if (send(child_fd, res_buffer, bytes_received_from_upstream, 0) < 0){
         perror("LB: Send upstream failed");
         exit(EXIT_FAILURE);
       }
       shutdown(child_fd, SHUT_WR);
       close(child_fd);
     } else {
       printf("LB: Continue to accept other connection\n");
     }
   }
   exit(0);
}