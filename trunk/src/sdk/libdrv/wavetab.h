#ifndef __WAVETAB_H__
#define __WAVETAB_H__

#include <stdint.h>

#define SAMPLE_RATE 8000

static const uint8_t a4[] = /* 880 */
{
	128,
	210,
	253,
	238,
	171,
	 85,
	 18,
	  3,
	 46,
	128,
	210,
	253,
	238,
	171,
	 85,
	 18,
	  3,
	 46,
	128,
	210,
	253,
	238,
	171,
	 85,
	 18,
	  3,
	 46,
	128,
	210,
	253,
	238,
	171,
	 85,
	 18,
	  3,
	 46,
};

static const uint8_t a4s[] = /* 932 */
{
	128,
	214,
	254,
	229,
	151,
	 61,
	  6,
	 14,
	 82,
	174,
	242,
	250,
	195,
	105,
	 27,
	  2,
	 42,
	128,
	214,
	254,
	229,
	151,
	 61,
	  6,
	 14,
	 82,
	174,
	242,
	250,
	195,
	105,
	 27,
	  2,
	 42,
};

static const uint8_t b4[] = /* 988 */
{
	128,
	218,
	255,
	218,
	128,
	 38,
	  1,
	 38,
	128,
	218,
	255,
	218,
	128,
	 38,
	  1,
	 38,
	128,
	218,
	255,
	218,
	128,
	 38,
	  1,
	 38,
	128,
	218,
	255,
	218,
	128,
	 38,
	  1,
	 38,
};

static const uint8_t c4[] = /* 1046 */
{
	128,
	220,
	255,
	211,
	115,
	 28,
	  2,
	 55,
	154,
	236,
	251,
	190,
	 90,
	 14,
	  9,
	 78,
	178,
	247,
	242,
	166,
	 66,
	  5,
	 20,
	102,
	201,
	254,
	228,
	141,
	 45,
	  1,
	 36,
};

static const uint8_t c4s[] = /* 1108 */
{
	128,
	225,
	253,
	193,
	 87,
	 10,
	 16,
	101,
	205,
	255,
	215,
	114,
	 23,
	  6,
	 75,
	181,
	250,
	233,
	142,
	 41,
	  1,
	 51,
	155,
	240,
	246,
	169,
	 63,
	  3,
	 31,
};

static const uint8_t d4[] = /* 1175 */
{
	128,
	230,
	250,
	171,
	 58,
	  1,
	 46,
	157,
	245,
	238,
	143,
	 36,
	  3,
	 71,
	185,
	253,
	220,
	113,
	 18,
	 11,
	 99,
	210,
	255,
	198,
	 85,
	  6,
	 26,
};

static const uint8_t d4s[] = /* 1244 */
{
	128,
	233,
	247,
	158,
	 44,
	  2,
	 69,
	187,
	254,
	212,
	 98,
	  9,
	 23,
	128,
	233,
	247,
	158,
	 44,
	  2,
	 69,
	187,
	254,
	212,
	 98,
	  9,
	 23,
};

static const uint8_t e4[] = /* 1318 */
{
	128,
	238,
	238,
	128,
	 18,
	 18,
	128,
	238,
	238,
	128,
	 18,
	 18,
	128,
	238,
	238,
	128,
	 18,
	 18,
	128,
	238,
	238,
	128,
	 18,
	 18,
};

static const uint8_t f4[] = /* 1397 */
{
	128,
	241,
	232,
	111,
	  8,
	 35,
	162,
	252,
	208,
	 77,
	  1,
	 62,
	194,
	255,
	179,
	 48,
	  4,
	 94,
	221,
	248,
	145,
	 24,
	 15,
};

static const uint8_t f4s[] = /* 1480 */
{
	128,
	244,
	224,
	 92,
	  2,
	 59,
	197,
	254,
	164,
	 32,
	 12,
	128,
	244,
	224,
	 92,
	  2,
	 59,
	197,
	254,
	164,
	 32,
	 12,
};

static const uint8_t g4[] = /* 1568 */
{
	128,
	249,
	203,
	 53,
	  7,
	128,
	249,
	203,
	 53,
	  7,
	128,
	249,
	203,
	 53,
	  7,
	128,
	249,
	203,
	 53,
	  7,
};

static const uint8_t g4s[] = /* 1661 */
{
	128,
	251,
	188,
	 35,
	 22,
	169,
	255,
	149,
	 12,
	 50,
	206,
	244,
	107,
	  1,
	 87,
	234,
	221,
	 68,
	  5,
};

static const uint8_t sin1khz[] = /* 1000 */
{
	128,
	218,
	255,
	218,
	128,
	 38,
	  1,
	 38,
};


static const struct {
	const uint8_t * buf;
	uint32_t len;
} tone_lut[] = {
	{ .buf = a4, .len = sizeof(a4) },
	{ .buf = a4s, .len = sizeof(a4s) },
	{ .buf = b4, .len = sizeof(b4) },
	{ .buf = c4, .len = sizeof(c4) },
	{ .buf = c4s, .len = sizeof(c4s) },
	{ .buf = d4, .len = sizeof(d4) },
	{ .buf = d4s, .len = sizeof(d4s) },
	{ .buf = e4, .len = sizeof(e4) },
	{ .buf = f4, .len = sizeof(f4) },
	{ .buf = f4s, .len = sizeof(f4s) },
	{ .buf = g4, .len = sizeof(g4) },
	{ .buf = g4s, .len = sizeof(g4s) },
	{ .buf = sin1khz, .len = sizeof(sin1khz) },
};

#define TONE_A4 0
#define TONE_A4S 1
#define TONE_B4 2
#define TONE_C4 3
#define TONE_C4S 4
#define TONE_D4 5
#define TONE_D4S 6
#define TONE_E4 7
#define TONE_F4 8
#define TONE_F4S 9
#define TONE_G4 10
#define TONE_G4S 11
#define TONE_SIN1KHZ 12


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
#endif /* __WAVETAB_H__ */

