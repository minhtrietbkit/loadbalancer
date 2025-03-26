#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv) {
    printf("PID id %d\n", getpid());
    printf("Port is %s\n", argv[1]);

    int server_fd, child_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    ssize_t buffer[BUFSIZ];

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
      perror("Upstream: Socket create failed");
      exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
      perror("Upstream: Socket bind failed");
      exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
      perror("Upstream: Socket listen failed");
      exit(EXIT_FAILURE);
    }
    while(1) {
      if ((child_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len)) < 0) {
        perror("Upstream: Socket accept failed");
        exit(EXIT_FAILURE);
      };
      int recv_return;
      recv_return = recv(child_fd, buffer, BUFSIZ, 0) ;
      if (recv_return < 0) {
        perror("Upstream: Socket receive failed");
        exit(EXIT_FAILURE);
      }
      if (recv_return == 0) {
        printf("Upstream: LB closes connection\n");
      }
      printf("%s\n", (char *) buffer);

      char response[BUFSIZ];
      snprintf(response, sizeof(response), "[Server Port: %d] - echo downstream: %s", ntohs(client_addr.sin_port), (char *)buffer);
      if (send(child_fd, response, strlen(response), 0) < 0) {
        perror("Upstream: Send Failed");
        exit(EXIT_FAILURE);
      }
      printf("Upstream: send to LB successfully\n");
      close(child_fd);
    }
    exit(0);
}