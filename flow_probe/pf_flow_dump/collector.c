#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_COLLECTOR_PORT 12355
#define MAX_MESSAGE_SIZE 1500
#define SIZE_FLOW_DATA_RECORD 52

/* ****************************************************** */

char *_intoa(unsigned int addr, char* buf, u_short bufLen) {
  char *cp, *retStr;
  u_int byte;
  int n;

  cp = &buf[bufLen];
  *--cp = '\0';

  n = 4;
  do {
    byte = addr & 0xff;
    *--cp = byte % 10 + '0';
    byte /= 10;
    if (byte > 0) {
      *--cp = byte % 10 + '0';
      byte /= 10;
      if (byte > 0) {
	      *--cp = byte + '0';
	    }
    }
    *--cp = '.';
    addr >>= 8;
  } while (--n > 0);

  retStr = (char*)(cp+1);

  return (retStr);
}

/* *************************************** */

static char *intoa(unsigned int addr) {
  static char buf[sizeof "ff:ff:ff:ff:ff:ff:255.255.255.255"];
  return(_intoa(addr, buf, sizeof(buf)));
}

/* ****************************************************** */

void decapsulate_flow_record(char* ipfix_message) {
  u_int16_t message_length;
  memcpy(&message_length, ipfix_message + 2, sizeof(u_int16_t));
  int num_record = (message_length - 16 - 4) / SIZE_FLOW_DATA_RECORD;
  int i, offset;
  u_int16_t i16_tmp;
  u_int32_t i32_tmp;
  for (i = 0; i < num_record; ++i) {
    offset = 20 + i * SIZE_FLOW_DATA_RECORD;
    // IPV4_SRC_ADDR and L4_SRC_PORT.
    memcpy(&i32_tmp, ipfix_message + offset, sizeof(u_int32_t));
    memcpy(&i16_tmp, ipfix_message + offset + 8, sizeof(u_int16_t));
    printf("%s:%d ", intoa(i32_tmp), (int)i16_tmp);
    // IPV4_DST_ADDR and L4_DST_PORT.
    memcpy(&i32_tmp, ipfix_message + offset + 4, sizeof(u_int32_t));
    memcpy(&i16_tmp, ipfix_message + offset + 10, sizeof(u_int16_t));
    printf("%s:%d ", intoa(i32_tmp), i16_tmp);
    // FIRST_SWITCHED and FIRST_SWITCHED_USEC.
    memcpy(&i32_tmp, ipfix_message + offset + 36, sizeof(u_int32_t));
    printf("[%u.", i32_tmp);
    memcpy(&i32_tmp, ipfix_message + offset + 40, sizeof(u_int32_t));
    printf("%u -> ", i32_tmp);
    // LAST_SWITCHED and LAST_SWITCHED_USEC.
    memcpy(&i32_tmp, ipfix_message + offset + 44, sizeof(u_int32_t));
    printf("%u.", i32_tmp);
    memcpy(&i32_tmp, ipfix_message + offset + 48, sizeof(u_int32_t));
    printf("%u] ", i32_tmp);
    // OUT_BYTES and OUT_PKTS.
    memcpy(&i32_tmp, ipfix_message + offset + 12, sizeof(u_int32_t));
    printf("%u/", i32_tmp);
    memcpy(&i32_tmp, ipfix_message + offset + 16, sizeof(u_int32_t));
    printf("%u ", i32_tmp);
    // RETRANSMITTED_OUT_BYTES and RETRANSMITTED_OUT_PKTS.
    memcpy(&i32_tmp, ipfix_message + offset + 20, sizeof(u_int32_t));
    printf("(%u/", i32_tmp);
    memcpy(&i32_tmp, ipfix_message + offset + 24, sizeof(u_int32_t));
    printf("%u) ", i32_tmp);
    // MIN_DELAY_US and MAX_DELAY_US.
    memcpy(&i32_tmp, ipfix_message + offset + 28, sizeof(u_int32_t));
    printf("(%u/", i32_tmp);
    memcpy(&i32_tmp, ipfix_message + offset + 32, sizeof(u_int32_t));
    printf("%u)\n", i32_tmp);
  }
}

/* ****************************************************** */

int main(int argc, char const *argv[]) {
  int port;
  int server_fd, exporter_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[MAX_MESSAGE_SIZE] = {0};

  port = DEFAULT_COLLECTOR_PORT;
  if (argc > 1) {
    port = atoi(argv[1]);
  }

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  printf("Preparing to accept incoming connnection\n");
  exporter_socket =
      accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
  if (exporter_socket < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  while(1) {
   valread = read(exporter_socket , buffer, MAX_MESSAGE_SIZE);
   if (valread <= 0) {
    printf("No export message to collected!!!!!!!!\n");
    break;
   }
   decapsulate_flow_record(buffer);
  }

  return 0;
}

