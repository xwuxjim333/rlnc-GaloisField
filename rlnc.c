#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "encoder.h"
#include "decoder.h"

#include <sys/stat.h>

static double diff_in_second(struct timespec t1, struct timespec t2)
{
	struct timespec diff;
	if (t2.tv_nsec-t1.tv_nsec < 0) {
		diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
		diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
	} else {
		diff.tv_sec  = t2.tv_sec - t1.tv_sec;
		diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
	}
	return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}


int main(int argc, char *argv[])
{
	struct timespec start, end;
	double cpu_time = 0.0;
	if (argc < 3) {
		fprintf(stdout, "Usage: %s [file_name] [decode file]\n", argv[0]);
	}

	FILE *fp;
	fp = fopen(argv[1], "r");

	FILE *fb;
	fb = fopen(argv[2], "w");

	uint32_t symbol_size = 128;//1 : a packet has 8-bits
	struct encoder *encoder = encoder_create(symbol_size);

	uint8_t *payload = calloc(encoder->symbol_size + encoder->symbols, sizeof(uint8_t));

	clock_gettime(CLOCK_REALTIME, &start);
	while (!feof(fp)) {
		int number = fread(encoder->block, sizeof(char), encoder->block_size, fp);

		struct decoder *decoder = decoder_create(symbol_size);

		int cout = 0;
		while (decoder->rank < decoder->symbols) {
			encoder_write_payload(encoder, payload, cout);
			decoder_read_payload(decoder, payload);
			printf("decoder rank = %u\n", decoder->rank);
			decoder_print(decoder);
			cout++;
		}

		printf("\ndecoding block...\n\n");
		decoder_decode_block(decoder);
		decoder_print(decoder);

		printf("\ndecode finished\n\n");

		if (memcmp(decoder->block, encoder->block, decoder->block_size) == 0)
			printf("decode success\n");
		else
			printf("decode fail\n");

		fwrite(decoder->block, sizeof(char), number, fb);
		memset(payload, 0, encoder->symbol_size + encoder->symbols);
		//decoder_flush(decoder);
		//decoder_destroy(&decoder);
		free(decoder);
	}
	clock_gettime(CLOCK_REALTIME, &end);
	cpu_time = diff_in_second(start, end);
	printf("execution time of transmission : %lf sec\n", cpu_time);

	fclose(fb);
	free(payload);
	encoder_destroy(&encoder);
	return 0;
}
