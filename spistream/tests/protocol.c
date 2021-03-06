/*
 * =====================================================================================
 *
 *       Filename:  protocol.c
 *
 *         Author:  Evan Friis, evan.friis@cern.ch
 *        Company:  UW Madison
 *
 * =====================================================================================
 */


#include <stdlib.h>
#include <string.h>

#include "minunit.h"
#include "circular_buffer.h"
#include "adler32.h"

#include "protocol.h"


static char* test_verify_packet(void) {
  uint32_t my_data[10] = {0xBEEF, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  add_checksum(my_data, 10);
  mu_assert_eq("not clobbered", my_data[8], 9);
  mu_assert("chksum", my_data[9] != 10);
  int cksum_error = -1;
  uint32_t packet_id = spi_stream_verify_packet(my_data, 10, &cksum_error);
  mu_assert_eq("pkt_id", packet_id, 0xBEEF);
  mu_assert_eq("error", cksum_error, 0);
  // simulate error
  my_data[5] = 1;
  packet_id = spi_stream_verify_packet(my_data, 10, &cksum_error);
  mu_assert_eq("pkt_id", packet_id, 0xBEEF);
  mu_assert_eq("error", cksum_error, 1);
  return 0;
}

static char* test_spi_stream_construct_tx_packet(void) {
  
  uint32_t my_data[5] = {1, 2, 3, 4, 5};
  CircularBuffer* mybuf = cbuffer_new();
  cbuffer_append(mybuf, my_data, 5);

  uint32_t my_pkt_expected[10] = {0xBEEF, 5, 1, 2, 3, 4, 5, 0xDEADBEEF, 0xDEADBEEF, -1};
  add_checksum(my_pkt_expected, 10);
  int checksum_err = -1;
  spi_stream_verify_packet(my_pkt_expected, 10, &checksum_err);
  mu_assert_eq("no cksum err", checksum_err, 0);

  uint32_t my_pkt[10];
  spi_stream_construct_tx_packet(0xBEEF, my_pkt, 10, mybuf);

//  for (int i = 0; i < 10; ++i) {
//    printf("%i %lx %lx\n", i, my_pkt_expected[i], my_pkt[i]);
//  }

  mu_assert_eq("packet", memcmp(my_pkt_expected, my_pkt, 10 * sizeof(uint32_t)), 0);
  mu_assert_eq("consumed", cbuffer_size(mybuf), 0);

  // put more in the buffer than we can consume at once
  cbuffer_append(mybuf, my_data, 5);
  cbuffer_append(mybuf, my_data, 5);
  cbuffer_append(mybuf, my_data, 5);

  uint32_t my_pkt_expected_2[10] = {0xBEEF, 7, 1, 2, 3, 4, 5, 1, 2, -1};
  add_checksum(my_pkt_expected_2, 10);
  spi_stream_verify_packet(my_pkt_expected_2, 10, &checksum_err);
  mu_assert_eq("no cksum err 2", checksum_err, 0);

  spi_stream_construct_tx_packet(0xBEEF, my_pkt, 10, mybuf);
  mu_assert_eq("packet", memcmp(my_pkt_expected_2, my_pkt, 10 * sizeof(uint32_t)), 0);
  mu_assert_eq("consumed", cbuffer_size(mybuf), 8);

  spi_stream_verify_packet(my_pkt, 10, &checksum_err);
  mu_assert_eq("no cksum actual", checksum_err, 0);

  cbuffer_free(mybuf);
  return 0;
}

static char* test_spi_stream_construct_empty_tx_packet(void) {
  
  CircularBuffer* mybuf = cbuffer_new();

  uint32_t my_pkt_expected[10] = {0xBEEF, 0, 
    0xDEADBEEF, 
    0xDEADBEEF, 
    0xDEADBEEF, 
    0xDEADBEEF, 
    0xDEADBEEF, 
    0xDEADBEEF, 
    0xDEADBEEF, 
    -1};
  add_checksum(my_pkt_expected, 10);
  int checksum_err = -1;
  spi_stream_verify_packet(my_pkt_expected, 10, &checksum_err);
  mu_assert_eq("no cksum err", checksum_err, 0);

  uint32_t my_pkt[10];
  spi_stream_construct_tx_packet(0xBEEF, my_pkt, 10, mybuf);

//  for (int i = 0; i < 10; ++i) {
//    printf("%i %lx %lx\n", i, my_pkt_expected[i], my_pkt[i]);
//  }

  mu_assert_eq("packet", memcmp(my_pkt_expected, my_pkt, 10 * sizeof(uint32_t)), 0);
  spi_stream_verify_packet(my_pkt, 10, &checksum_err);
  mu_assert_eq("no cksum actual", checksum_err, 0);

  cbuffer_free(mybuf);
  return 0;
}

static char* test_spi_stream_read_rx_packet(void) {
  
  CircularBuffer* mybuf = cbuffer_new();

  uint32_t my_pkt_expected[10] = {0xBEEF, 5, 1, 2, 3, 4, 5, 0xDEADBEEF, 0xDEADBEEF, -1};
  uint32_t my_pkt_expected_2[10] = {0xBEEF, 7, 1, 2, 3, 4, 5, 1, 2, -1};
  add_checksum(my_pkt_expected, 10);
  add_checksum(my_pkt_expected_2, 10);

  uint32_t my_data[7] = {1, 2, 3, 4, 5, 1, 2};

  // read with non-full buffer
  int ret = spi_stream_read_rx_packet(my_pkt_expected, mybuf);
  mu_assert_eq("okay1", ret, 1);
  mu_assert_eq("data1", memcmp(my_data, mybuf->data, 5 * sizeof(uint32_t)), 0);
  mybuf->tail = 0;

  // read with full buffer
  ret = spi_stream_read_rx_packet(my_pkt_expected_2, mybuf);
  mu_assert_eq("okay2", ret, 1);
  mu_assert_eq("data2", memcmp(my_data, mybuf->data, 7 * sizeof(uint32_t)), 0);

  // overflowing local buffer
  mybuf->tail = IO_BUFFER_SIZE - 5;
  mu_assert_eq("its too small", cbuffer_freespace(mybuf), 4);
  ret = spi_stream_read_rx_packet(my_pkt_expected_2, mybuf);
  mu_assert_eq("overflow err", ret, 0);
  mu_assert_eq("unmodified", cbuffer_freespace(mybuf), 4);

  cbuffer_free(mybuf);
  return 0;
}

int tests_run;

char * all_tests(void) {
  printf("\n\n=== protocol tests ===\n");
  mu_run_test(test_verify_packet);
  mu_run_test(test_spi_stream_construct_tx_packet);
  mu_run_test(test_spi_stream_construct_empty_tx_packet);
  mu_run_test(test_spi_stream_read_rx_packet);
  return 0;
}
