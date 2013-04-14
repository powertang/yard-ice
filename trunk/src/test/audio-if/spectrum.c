#include <stdio.h>
#include <stdlib.h>

#include "trace.h"
#include "fft.h"
#include "vt100.h"

#define FFT_LEN 256

const int16_t hann_window[FFT_LEN] = {
	0x0000, 0x0005, 0x0014, 0x002d, 0x0050, 0x007c, 0x00b3, 0x00f3,
	0x013d, 0x0191, 0x01ef, 0x0256, 0x02c7, 0x0341, 0x03c5, 0x0452,
	0x04e9, 0x0588, 0x0631, 0x06e3, 0x079d, 0x0861, 0x092d, 0x0a01,
	0x0ade, 0x0bc3, 0x0cb1, 0x0da6, 0x0ea3, 0x0fa8, 0x10b4, 0x11c7,
	0x12e2, 0x1404, 0x152d, 0x165c, 0x1792, 0x18ce, 0x1a10, 0x1b58,
	0x1ca6, 0x1df9, 0x1f51, 0x20af, 0x2211, 0x2378, 0x24e4, 0x2654,
	0x27c7, 0x293f, 0x2aba, 0x2c38, 0x2dba, 0x2f3e, 0x30c5, 0x324e,
	0x33d9, 0x3567, 0x36f6, 0x3886, 0x3a17, 0x3baa, 0x3d3d, 0x3ed0,
	0x4064, 0x41f8, 0x438b, 0x451e, 0x46af, 0x4840, 0x49d0, 0x4b5e,
	0x4cea, 0x4e75, 0x4ffd, 0x5182, 0x5305, 0x5485, 0x5602, 0x577b,
	0x58f1, 0x5a63, 0x5bd0, 0x5d3a, 0x5e9e, 0x5ffe, 0x615a, 0x62af,
	0x6400, 0x654b, 0x6690, 0x67cf, 0x6908, 0x6a3b, 0x6b67, 0x6c8c,
	0x6daa, 0x6ec1, 0x6fd1, 0x70da, 0x71db, 0x72d4, 0x73c5, 0x74ae,
	0x758f, 0x7668, 0x7738, 0x7800, 0x78bf, 0x7975, 0x7a22, 0x7ac7,
	0x7b62, 0x7bf3, 0x7c7c, 0x7cfb, 0x7d71, 0x7ddd, 0x7e3f, 0x7e98,
	0x7ee7, 0x7f2c, 0x7f68, 0x7f99, 0x7fc1, 0x7fdf, 0x7ff3, 0x7ffd,
	0x7ffd, 0x7ff3, 0x7fdf, 0x7fc1, 0x7f99, 0x7f68, 0x7f2c, 0x7ee7,
	0x7e98, 0x7e3f, 0x7ddd, 0x7d71, 0x7cfb, 0x7c7c, 0x7bf3, 0x7b62,
	0x7ac7, 0x7a22, 0x7975, 0x78bf, 0x7800, 0x7738, 0x7668, 0x758f,
	0x74ae, 0x73c5, 0x72d4, 0x71db, 0x70da, 0x6fd1, 0x6ec1, 0x6daa,
	0x6c8c, 0x6b67, 0x6a3b, 0x6908, 0x67cf, 0x6690, 0x654b, 0x6400,
	0x62af, 0x615a, 0x5fff, 0x5e9e, 0x5d3a, 0x5bd0, 0x5a63, 0x58f1,
	0x577b, 0x5602, 0x5485, 0x5305, 0x5182, 0x4ffd, 0x4e75, 0x4cea,
	0x4b5e, 0x49d0, 0x4840, 0x46af, 0x451e, 0x438b, 0x41f8, 0x4064,
	0x3ed0, 0x3d3d, 0x3baa, 0x3a17, 0x3886, 0x36f6, 0x3567, 0x33d9,
	0x324e, 0x30c5, 0x2f3e, 0x2dba, 0x2c38, 0x2aba, 0x293f, 0x27c7,
	0x2654, 0x24e4, 0x2378, 0x2211, 0x20af, 0x1f51, 0x1df9, 0x1ca6,
	0x1b58, 0x1a10, 0x18ce, 0x1792, 0x165c, 0x152d, 0x1404, 0x12e2,
	0x11c7, 0x10b4, 0x0fa8, 0x0ea3, 0x0da6, 0x0cb1, 0x0bc3, 0x0ade,
	0x0a01, 0x092d, 0x0861, 0x079d, 0x06e3, 0x0631, 0x0588, 0x04e9,
	0x0452, 0x03c5, 0x0341, 0x02c7, 0x0256, 0x01ef, 0x0191, 0x013d,
	0x00f3, 0x00b3, 0x007c, 0x0050, 0x002d, 0x0014, 0x0005, 0x0000 };

void spectrum_run(struct spectrum * sa)
{
	cplx16_t fft_in[FFT_LEN];
	cplx16_t fft_out[FFT_LEN];
	int16_t * frm;
	int16_t v;
	int pos;
	int i;
	int j;

	sa->locked = true;

	pos = sa->frm_cnt;

	for (j = 0; j < (FFT_LEN / SNDBUF_LEN); ++j) {
		frm = (int16_t *)sa->buf[pos++ & ((FFT_LEN / SNDBUF_LEN) - 1)];
		for (i = 0; i < SNDBUF_LEN; ++i) {
			v = Q15_MUL(frm[i], hann_window[j * SNDBUF_LEN + i]);
			fft_in[j * SNDBUF_LEN + i].re = v;
			fft_in[j * SNDBUF_LEN + i].im = 0;
		}
		sndbuf_free(frm);
	}

	/* FFT calculation */
	fftR4(fft_in, fft_out, FFT_LEN);

	for (i = 0; i < (FFT_LEN / 2); ++i) {
		/* Accumulate the absolute values */
		sa->mag[i] += cplx16_abs(fft_out[i]);
	}

	sa->run_cnt++;
	sa->locked = false;
}

void spectrum_normalize(struct spectrum * sa, uint16_t * out)
{
	int i;

	for (i = 0; i < (FFT_LEN / 2); ++i) {
		out[i] = sa->mag[i] / sa->run_cnt;
		sa->mag[i]  = 0;
	}

	sa->run_cnt = 0;
}

void spectrum_reset(struct spectrum * sa)
{
	int i;

	for (i = 0; i < 256; i++)
		sa->mag[i] = 0;

	sa->run_cnt = 0;
}

void spectrum_init(struct spectrum * sa, unsigned int rate)
{
	int i;

	for (i = 0; i < 256; i++)
		sa->mag[i] = 0;

	sa->run_cnt = 0;
	sa->frm_cnt = 0;
	sa->locked = false;
	sa->rate = rate;
}

void spectrum_rec(struct spectrum * sa, sndbuf_t buf)
{
	int pos;

	/* spectrum analizer is locked */
	if (sa->locked)
		return;

	/* increment the buffer reference counter */
	if ((buf = sndbuf_use(buf)) == NULL)
		return;

	/* insert into the queue */
	pos = sa->frm_cnt++ & ((FFT_LEN / SNDBUF_LEN) - 1);

	if (sa->buf[pos] != NULL)
		sndbuf_free(sa->buf[pos]);

	sa->buf[pos] = buf;
}

void spectrum_print(struct spectrum * sa, int16_t mag[])
{
	int max = 0;
	int i;
	int f;
	int j;
	int n;

	for (i = 0; i < FFT_LEN / 2; ++i) {
		if (mag[i] > max)
			max = mag[i];
	}

	for (i = 0; i < FFT_LEN / 2; ++i) {

		f = (i * sa->rate + (FFT_LEN / 2)) / FFT_LEN;
		printf("%4d %5d ", f, mag[i]);
		n = (mag[i] * 64) / max;
		for (j = 0; j < n; j++)
			printf("#");
		printf("\n");
	}
}

void spectrum_show(uint16_t mag[])
{
	uint16_t pwr[80];
	char ln[128];
	char * cp;
	int i;
	int x;
	int y;
	char mark;	

	printf(VT100_CURSOR_HIDE); 

	for (i = 0; i < 80; ++i) {
		pwr[i] = Q15_UMUL(mag[i], mag[i]);
	}

	for (y = 4; y < 41; ++y) {
		cp = ln;
		cp += sprintf(cp, VT100_GOTO, y - 2, 1); 
		mark = (y % 10) ? ' ' : '-';

		for (x = 0; x < 80; ++x) {
			cp[x] = (pwr[x] >= q15_db2pwr_ltu[y]) ? '|' : mark;
		}
		cp[x] = '\0';
		printf(ln); 
	}
}

