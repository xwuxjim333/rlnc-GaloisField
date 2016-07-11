#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#include <stdio.h>

#include "gf.h"
#include "encoder.h"

static inline void exclusive_or(uint8_t *dst, uint8_t *src, size_t n)
{
    int i;
#ifdef _XMMINTRIN_H_INCLUDED
	/* Implement AVX/SIMD : xor 128-bits in one turn*/
	for (i=0; i<n ; i=i+16) {
		__m128i xmm1 = _mm_loadu_si128((__m128i *)(dst+i));
		__m128i xmm2 = _mm_loadu_si128((__m128i *)(src+i));
		xmm1 = _mm_xor_si128(xmm1, xmm2);
		_mm_store_si128((__m128i *)(dst+i), xmm1);
	}
#else
    for (i = 0; i < n; i++)
        *dst++ ^= *src++;
#endif
}

struct encoder *encoder_create(uint32_t symbol_size)
{
	struct encoder *encoder;
	encoder = (struct encoder *) calloc(1, sizeof(struct encoder));

	/* TODO: implement more symbols support */
	encoder->symbols = 8;//2
	encoder->symbol_size = symbol_size;//1 = 8-bits
	encoder->block_size  = symbol_size * encoder->symbols;//8*2=16
	encoder->block = (uint8_t *) calloc(encoder->block_size, sizeof(uint8_t));
	encoder->symbol = (uint8_t **) calloc(encoder->block_size/*encoder->symbols*/, sizeof(uint8_t *));

	int32_t i;
	uint8_t *mem = encoder->block;
	for (i = 0; i < encoder->block_size; i++, mem += 1/*symbol_size*/) {
		encoder->symbol[i] = mem;
	}
	encoder->counter = 0;
	return encoder;
}

void encoder_destroy(struct encoder **encoder_t)
{
	if (*encoder_t) {
		if ((*encoder_t)->symbol) {
			free((*encoder_t)->symbol);
			(*encoder_t)->symbol = NULL;
		}
		if ((*encoder_t)->block) {
			free((*encoder_t)->block);
			(*encoder_t)->block = NULL;
		}
		free(*encoder_t);
		*encoder_t = NULL;
	}
}

void encoder_write_payload(struct encoder *encoder, uint8_t *payload_out, int cout)
{	/* rlnc with gf(2^8) */

	int32_t i, j;
	for (i = 0; i < encoder->symbols; i++) {
		if (i == cout)
			encoder->vector[i] = 1;
		else if (i >= cout)
			encoder->vector[i] = (uint8_t) rand();
		else
			encoder->vector[i] = 0;
	}

	/* clear memory, and the 2 is for 2*8-bit vector */
	memset(payload_out, 0, encoder->symbol_size + encoder->symbols);
	
	for (i = 0; i < encoder->symbols; i++) {
		/* attach vector for decoder */
		payload_out[encoder->symbol_size + i] = encoder->vector[i];
		for (j = 0; j < encoder->symbol_size; j++) {
			/* calculate finite field parameter */
			encoder->calculate[i*encoder->symbol_size + j] = gmul(*(encoder->symbol[i*encoder->symbol_size +j]), encoder->vector[i]);
		}
	}

	for (j = 0; j < encoder->symbol_size; j++) {
		/* calculate coded packet */
		encoder->coded[j] = 0;
		for (i = 0; i < encoder->symbols; i++) {
			encoder->coded[j] = gadd(encoder->coded[j], encoder->calculate[i*encoder->symbol_size + j]);
		}
		payload_out[j] = encoder->coded[j];
	}
}

