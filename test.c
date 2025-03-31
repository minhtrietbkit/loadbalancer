#include <string.h>
#include <stdio.h>
#include <stdlib.h>
int get_content_length( char *req_buffer) {
  	printf("%s\n", req_buffer);
    char content_length[] = "Content-Length: ";
    char *content_length_pos;
    content_length_pos = strstr(req_buffer, content_length);

    if (!content_length_pos) {
      printf("LB: Can't find Header Content-Length\n");
      return -1;
    }

    char *content_length_value_pos = content_length_pos + sizeof(char) * strlen(content_length);
    int content_length_value_byte_counts = 0;
    while (*(content_length_value_pos + content_length_value_byte_counts) != '\r') {
      content_length_value_byte_counts++;
    }

    char content_length_value[content_length_value_byte_counts];
    memcpy(content_length_value, content_length_value_pos, content_length_value_byte_counts);

    printf("Content-Length: %u\n", atoi(content_length_value));
    return atoi(content_length_value);
}

int get_http_metadata_length(char *req_buffer) {
  char *metadata_end = strstr(req_buffer, "\r\n\r\n");
  if (!metadata_end) {
     perror("LB: Can't find the end of the HTTP metadata section\n");
     exit(EXIT_FAILURE);
  }
  return metadata_end - req_buffer;
}

int main(int argc, char **argv) {
    char *tmp = "Content-Length: 635";
    //get_content_length(tmp);
    char *tmp2 = "Header1: Value1\nHeader2: Value2\r\n\r\n";
    printf("%i",get_http_metadata_length(tmp2));
    return 0;
}