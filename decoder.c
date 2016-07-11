#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xmmintrin.h>

#include "gf.h"
#include "decoder.h"

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

struct decoder *decoder_create(uint32_t symbol_size)
{
	struct decoder *decoder;
	decoder = (struct decoder *) calloc(1, sizeof(struct decoder));
	
	/* TODO: implement more symbols support */
	decoder->symbols = 8;
	decoder->symbol_size = symbol_size;//1 = 8-bits
	decoder->state = (uint8_t *) calloc(decoder->symbols * decoder->symbols, sizeof(uint8_t));
	decoder->block_size = decoder->symbols * decoder->symbol_size;
	decoder->block = (uint8_t *) calloc(decoder->block_size, sizeof(uint8_t));
	decoder->data = (uint8_t **) calloc(decoder->block_size/*decoder->symbols*/, sizeof(uint8_t *));
	decoder->rank = 0;
	return decoder;
}

void decoder_destroy(struct decoder **decoder_t)
{
	int32_t i;
	if (*decoder_t) {
		if ((*decoder_t)->data) {
			for (i = 0; i < (*decoder_t)->symbols; i++) {
				if ((*decoder_t)->data[i]) {
					free((*decoder_t)->data[i]);
					(*decoder_t)->data[i] = NULL;
				}
			}
			free((*decoder_t)->data);
			(*decoder_t)->data = NULL;
		}
		if ((*decoder_t)->state) {
			free((*decoder_t)->state);
			(*decoder_t)->state = NULL;
		}
		if ((*decoder_t)->block) {
			free((*decoder_t)->block);
			(*decoder_t)->block = NULL;
		}
		free(*decoder_t);
		*decoder_t = NULL;
	}
}

void decoder_read_payload(struct decoder *decoder, uint8_t *payload_in)
{
	uint8_t *payload = (uint8_t *) calloc(decoder->symbol_size, sizeof(uint8_t));

	memcpy(payload, payload_in, decoder->symbol_size);

	int32_t i;
	for (i = 0; i < decoder->symbols; i++) {
		/* find the coded vector, then fill in the state */
		decoder->vector[i] = payload_in[decoder->symbol_size + i];
		decoder->state[decoder->rank *decoder->symbols + i] = decoder->vector[i];
	}

	uint8_t *mem = payload;
	for (i = 0; i < decoder->symbol_size; i++, mem += 1) {
		decoder->data[decoder->rank*decoder->symbol_size + i] = mem;
	}

	decoder->rank++;
	decoder->count++;
}

void decoder_decode_block(struct decoder *decoder)
{
	int32_t i, j, k;
	uint8_t state, data_next, calculate, data_now, original;

	for (i = decoder->symbols-1; i >= 0; i--) {
		for (j = decoder->symbol_size-1; j >= 0; j--) {
			
			if (i == decoder->symbols-1) {
				decoder->data[i*decoder->symbol_size+j] = decoder->data[i*decoder->symbol_size+j];
			}
			else {
				for (k = decoder->symbols-1; k > i; k--) {
					state = decoder->state[i*decoder->symbols +k];
					data_next = *(decoder->data[k*decoder->symbol_size + j]);
					calculate = gmul(data_next, state);
					data_now = *(decoder->data[i*decoder->symbol_size + j]);
					original = gsub(data_now, calculate);
					*(decoder->data[i*decoder->symbol_size + j]) = original;
				}
			}
		}
	}

	for (i = decoder->symbols-1; i >= 0; i--) {
		for (k = decoder->symbols-1; k > i; k--) {
			decoder->state[i*decoder->symbols +k] = 0;
		}
	}

	uint8_t *mem = decoder->block;
	for (i = 0; i < decoder->block_size; i++) {
		memcpy(mem, decoder->data[i], 1/*decoder->symbol_size*/);
		mem += 1/*decoder->symbol_size*/;
	}
}

void decoder_flush(struct decoder *decoder)
{
	int32_t i;
	for (i = 0; i < decoder->symbols; i++) {
		free(decoder->data[i]);
		decoder->data[i] = NULL;
	}
	memset(decoder->block, 0, decoder->block_size);
}

void decoder_print(struct decoder *decoder)
{
	printf("decoder state :\n");
	int32_t i, j;
	for (i = 0; i < decoder->symbols; i++) {
		printf("%03d: ", i);
		for (j = 0; j < decoder->symbols; j++) {
			printf("%u	", decoder->state[i*decoder->symbols + j]);
		}
		printf("\n");
	}
	printf("\n");
}
