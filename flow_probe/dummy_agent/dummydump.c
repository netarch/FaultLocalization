#include <arpa/inet.h>
#include <pthread.h> 
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ALARM_SLEEP             1
#define FLOW_PER_EXPORT         100
#define MAX_FLOW_PER_EXPORT     25
#define SIZE_FLOW_DATA_RECORD   52
// #define DEBUG_

char* collector_addr = NULL;
int collector_port = 0;

u_int32_t export_seq_no = 0;
int export_socket = 0;

/* ******************************** */

void create_export_socket() {
  // Create export socket first.
  if (collector_addr != NULL && collector_port > 0) {
    export_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (export_socket < 0) {
      fprintf(stderr, "Export Socket creation error\n");
    } else {
      struct sockaddr_in collector_sock_addr;
      collector_sock_addr.sin_family = AF_INET;
      collector_sock_addr.sin_port = htons(collector_port);
      // Convert Collector IPv4 addresse from text to binary form.
      if (inet_pton(AF_INET, collector_addr, &collector_sock_addr.sin_addr) <=
          0) {
        fprintf(stderr, "Invalid collector address or address not supported\n");
        close(export_socket);
        export_socket = 0;
        return;
      } else {
        if (connect(export_socket, (struct sockaddr *)&collector_sock_addr,
                    sizeof(collector_sock_addr)) < 0) {
          fprintf(stderr, "Fail to connect to collector at %s:%d\n",
              collector_addr, collector_port);
          close(export_socket);
          export_socket = 0;
          return;
        } else {
          fprintf(stderr, "Connected to collector at %s:%d\n",
              collector_addr, collector_port);
        }
      }
    }
  }
}

/* ******************************** */
u_int32_t get_int_ip_from_octets(u_int32_t a, u_int32_t b, u_int32_t c, u_int32_t d){
  return (a * 16777216 + b * 65536 + c * 256 + d);
  //return (a<<24 + b<<16 + c<<8 + d);
}


void encapsulate_flow_record(char* buffer) {
  u_int32_t src_ip = get_int_ip_from_octets(192,168,100,110);
  u_int32_t dst_ip = get_int_ip_from_octets(192,168,100,116);
  u_int16_t src_port = 12345;
  u_int16_t dst_port = 5001;
  u_int32_t out_bytes = 150000;
  u_int16_t out_packets = 120;
  u_int32_t retrans_bytes = 0;
  u_int16_t retrans_packets = 0;
  u_int32_t min_rtt_usec = 3000;
  u_int32_t max_rtt_usec = 3000;
  memcpy(buffer, &src_ip, sizeof(u_int32_t));
  memcpy(buffer + 4, &dst_ip, sizeof(u_int32_t));
  memcpy(buffer + 8, &src_port, sizeof(u_int16_t));
  memcpy(buffer + 10, &dst_port, sizeof(u_int16_t));
  memcpy(buffer + 12, &out_bytes, sizeof(u_int32_t));
  memcpy(buffer + 16, &out_packets, sizeof(u_int32_t));
  memcpy(buffer + 20, &retrans_bytes, sizeof(u_int32_t));
  memcpy(buffer + 24, &retrans_packets, sizeof(u_int32_t));
  memcpy(buffer + 28, &min_rtt_usec, sizeof(u_int32_t));
  memcpy(buffer + 32, &max_rtt_usec, sizeof(u_int32_t));

  u_int32_t ts = 0;
  memcpy(buffer + 36, &ts, sizeof(u_int32_t));
  memcpy(buffer + 40, &ts, sizeof(u_int32_t));
  memcpy(buffer + 44, &ts, sizeof(u_int32_t));
  memcpy(buffer + 48, &ts, sizeof(u_int32_t));
}

/* ******************************** */

void export_flow_record(char* ipfix_message, int message_length) {
  int sent_bytes = 0;
  while (sent_bytes < message_length) {
    int tmp = send(export_socket, ipfix_message + sent_bytes,
        message_length - sent_bytes, 0);
    if (tmp < 0) {
      fprintf(stderr, "Error when exporting to collector.\n");
      break;
    }
    sent_bytes += tmp;
  }
  return;
}

/* ******************************** */

void export_dummy_flow(int sig) {
  // Create export socket.
  create_export_socket();
  if (export_socket <= 0) {
    alarm(ALARM_SLEEP);
    signal(SIGALRM, export_dummy_flow);
    return;
  }
#ifdef DEBUG_
  printf("Export\n");
#endif

  int num_record = FLOW_PER_EXPORT;
  int num_full_size_export = num_record / MAX_FLOW_PER_EXPORT;
  int residual_record = num_record % MAX_FLOW_PER_EXPORT;

  u_int16_t i16_tmp = 0;
  u_int32_t i32_tmp = 0;
  int full_size_message_length =
      16 + 4 + SIZE_FLOW_DATA_RECORD * MAX_FLOW_PER_EXPORT;
  char* ipfix_message =
      (char *)malloc(full_size_message_length * sizeof(char));
  // Version number (increased by 1 based on NetFlow version 9).
  i16_tmp = 10;
  memcpy(ipfix_message, &i16_tmp, sizeof(u_int16_t));
  // Observation Domain ID (may use threadId in multi-thread mode).
  i32_tmp = 0;
  memcpy(ipfix_message + 12, &i32_tmp, sizeof(u_int32_t));
  // Set ID (use 256 by default).
  i16_tmp = 256;
  memcpy(ipfix_message + 16, &i16_tmp, sizeof(u_int16_t));

  // Export full-sized message first.
  // Message length (in unit of bytes).
  i16_tmp = (u_int16_t)full_size_message_length;
  memcpy(ipfix_message + 2, &i16_tmp, sizeof(u_int16_t));
  // Data set length.
  i16_tmp = (u_int16_t)(full_size_message_length - 16);
  memcpy(ipfix_message + 18, &i16_tmp, sizeof(u_int16_t));
  // Full-sized flow records.
  int i, j;
  for (i = 0; i < num_full_size_export; ++i) {
    // Unix seconds as export timestamp.
    i32_tmp = (u_int32_t)time(NULL);
    memcpy(ipfix_message + 4, &i32_tmp, sizeof(u_int32_t));
    // Per-export message sequence number.
    memcpy(ipfix_message + 8, &export_seq_no, sizeof(u_int32_t));
    export_seq_no++;

    for (j = 0; j < MAX_FLOW_PER_EXPORT; ++j) {
      encapsulate_flow_record(ipfix_message + 20 + j * SIZE_FLOW_DATA_RECORD);
    }
    export_flow_record(ipfix_message, full_size_message_length);
  }

  // Export remaining flows (if there is any) in one message.
  if (residual_record > 0) {
    int residual_message_length =
        16 + 4 + SIZE_FLOW_DATA_RECORD * residual_record;
    // Message length (in unit of bytes).
    i16_tmp = (u_int16_t)residual_message_length;
    memcpy(ipfix_message + 2, &i16_tmp, sizeof(u_int16_t));
    // Data set length.
    i16_tmp = (u_int16_t)(residual_message_length - 16);
    memcpy(ipfix_message + 18, &i16_tmp, sizeof(u_int16_t));
    // Unix seconds as export timestamp.
    i32_tmp = (u_int32_t)time(NULL);
    memcpy(ipfix_message + 4, &i32_tmp, sizeof(u_int32_t));
    // Per-export message sequence number.
    memcpy(ipfix_message + 8, &export_seq_no, sizeof(u_int32_t));
    export_seq_no++;

    for (j = 0; j < residual_record; ++j) {
      encapsulate_flow_record(ipfix_message + 20 + j * SIZE_FLOW_DATA_RECORD);
    }
    export_flow_record(ipfix_message, residual_message_length);
  }

  free(ipfix_message);

  close(export_socket);
  alarm(ALARM_SLEEP);
  signal(SIGALRM, export_dummy_flow);
}

/* ******************************** */

void* dummy_export_thread(void* arg) {
  while (1) {
    sleep(1);
  }
}

/* ******************************** */

int main(int argc, char* argv[]) {
  char c;
  while((c = getopt(argc,argv,"z:x:")) != '?') {
    if((c == 255) || (c == -1)) break;

    switch(c) {
    case 'z':
      collector_addr = (char*)malloc(strlen(optarg) * sizeof(char) + 1);
      memcpy(collector_addr, optarg, strlen(optarg));
      collector_addr[strlen(optarg)] = '\0';
      break;
    case 'x':
      collector_port = atoi(optarg);
      break;
    }
  }

#ifdef DEBUG_
  if (collector_addr != NULL && collector_port > 0) {
    printf("Dummy Export to %s:%d\n", collector_addr, collector_port);
  }
#endif

  signal(SIGALRM, export_dummy_flow);
  alarm(ALARM_SLEEP);

  pthread_t dummy_thread;
  pthread_create(&dummy_thread, NULL, dummy_export_thread, NULL);
  pthread_join(dummy_thread, NULL);

  return 0;
}
