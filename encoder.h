#pragma once

#include <stdint.h>

struct encoder {
	uint8_t *block;
	uint32_t block_size;
	uint8_t **symbol;

	uint8_t vector[8];//2 : for two packet coded in the same time
	uint8_t coded[128];//1 : coded packet has 8-bit //symbol_size
	uint8_t calculate[1024];//1*2 : a packet has 8-bits(1), and code two(2) packet for one symbols*symbol_size


	uint32_t symbols;
	uint32_t symbol_size;

	uint32_t counter;
};

struct encoder *encoder_create(uint32_t symbol_size);
void encoder_destroy(struct encoder **encoder_t);
void encoder_write_payload(struct encoder *encoder, uint8_t *payload_out, int cout);
