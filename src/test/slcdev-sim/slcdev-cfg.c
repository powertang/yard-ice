#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <microjs.h>

#include <sys/dcclog.h>

#include "crc.h"
#include "slcdev.h"

uint16_t cfg_stack = FLASH_BLK_DEV_DB_BIN_SIZE;
uint16_t cfg_heap = 0;

/* Basic field (attribute) types */
enum {
	CFG_VOID,
	CFG_BIT,
	CFG_BFIELD8,
	CFG_BFIELD16,
	CFG_BFIELD32,
	CFG_UINT8,
	CFG_UINT16,
	CFG_UINT32
};

#define BFIELD_OPT(BITS, POS) ((((BITS - 1) & 0x1f) << 5) | (POS & 0x1f))


struct cfg_type {
	int (* encode)(const char * s, int opt, void * ptr); /* encode string into 
												   attribute */
	int (* decode)(char * s, int opt, void * ptr); /* serialize into string */
};

struct cfg_attr {
	char name[11];
	uint8_t type;
	uint16_t opt;
	uint16_t offs;
};

#define BFIELD32(NM, OBJ, FIELD, POS, BITS) \
	 { NM, CFG_BFIELD32, BFIELD_OPT(BITS, POS), offsetof(OBJ, FIELD) }

#define BFIELD16(NM, OBJ, FIELD, POS, BITS) \
	 { NM, CFG_BFIELD16, BFIELD_OPT(BITS, POS), offsetof(OBJ, FIELD) }

#define BFIELD8(NM, OBJ, FIELD, POS, BITS) \
	 { NM, CFG_BFIELD8, BFIELD_OPT(BITS, POS), offsetof(OBJ, FIELD) }

#define BIT(NM, OBJ, FIELD, POS) \
	 { NM, CFG_BIT, POS, offsetof(OBJ, FIELD) }

int cfg_bit_encode(const char * s, int bit, void * ptr)
{
	uint32_t * bfield = (uint32_t *)ptr;

	if ((strcmp(s, "yes") == 0) || (strcmp(s, "true") == 0) || 
		(strcmp(s, "1") == 0))
		*bfield |= 1 << bit;
	else if ((strcmp(s, "no") == 0) || (strcmp(s, "false") == 0) || 
			 (strcmp(s, "0") == 0))
		*bfield &= ~(1 << bit);
	else
		return -EINVAL;

	return 0;
} 

int cfg_bit_decode(char * s, int bit, void * ptr)
{
	uint32_t * val = (uint32_t *)ptr;

	return sprintf(s, "%s", (*val & (1 << bit)) ? "yes" : "no");
} 

int cfg_bfield8_encode(const char * s, int opt, void * ptr)
{
	uint32_t mask = 0xffffffff >> (31 - ((opt >> 5) & 0x1f));
	uint8_t * bfield = (uint8_t *)ptr;
	int shift = opt & 0x1f;
	uint32_t val;

	val = strtoul(s, NULL, 0);

	*bfield = (*bfield & ~(mask << shift)) | ((val & mask) << shift);

	return 0;
} 

int cfg_bfield8_decode(char * s, int opt, void * ptr)
{
	uint32_t mask = 0xffffffff >> (31 - ((opt >> 5) & 0x1f));
	uint8_t * bfield = (uint8_t *)ptr;
	int shift = opt & 0x1f;

	return sprintf(s, "%d", (*bfield >> shift) & mask);
} 


int cfg_bfield16_encode(const char * s, int opt, void * ptr)
{
	uint32_t mask = 0xffffffff >> (31 - ((opt >> 5) & 0x1f));
	uint16_t * bfield = (uint16_t *)ptr;
	int shift = opt & 0x1f;
	uint32_t val;

	val = strtoul(s, NULL, 0);

	*bfield = (*bfield & ~(mask << shift)) | ((val & mask) << shift);

	return 0;
} 

int cfg_bfield16_decode(char * s, int opt, void * ptr)
{
	uint32_t mask = 0xffffffff >> (31 - ((opt >> 5) & 0x1f));
	uint16_t * bfield = (uint16_t *)ptr;
	int shift = opt & 0x1f;

	return sprintf(s, "%d", (*bfield >> shift) & mask);
} 


int cfg_bfield32_encode(const char * s, int opt, void * ptr)
{
	uint32_t mask = 0xffffffff >> (31 - ((opt >> 5) & 0x1f));
	uint32_t * bfield = (uint32_t *)ptr;
	int shift = opt & 0x1f;
	uint32_t val;

	val = strtoul(s, NULL, 0);

	*bfield = (*bfield & ~(mask << shift)) | ((val & mask) << shift);

	return 0;
} 

int cfg_bfield32_decode(char * s, int opt, void * ptr)
{
	uint32_t mask = 0xffffffff >> (31 - ((opt >> 5) & 0x1f));
	uint32_t * bfield = (uint32_t *)ptr;
	int shift = opt & 0x1f;

	return sprintf(s, "%d", (*bfield >> shift) & mask);
} 



int cfg_uint8_encode(const char * s, int opt, void * ptr)
{
	uint8_t * val = (uint8_t *)ptr;

	*val = strtoul(s, NULL, 0);

	return 0;
} 

int cfg_uint8_decode(char * s, int hex, void * ptr)
{
	uint8_t * val = (uint8_t *)ptr;

	if (hex)
		return sprintf(s, "0x%02x", *val);

	return sprintf(s, "%d", *val);
} 

int cfg_uint16_encode(const char * s, int opt, void * ptr)
{
	uint16_t * val = (uint16_t *)ptr;

	*val = strtoul(s, NULL, 0);

	return 0;
} 

int cfg_uint16_decode(char * s, int hex, void * ptr)
{
	uint16_t * val = (uint16_t *)ptr;

	if (hex)
		return sprintf(s, "0x%04x", *val);

	return sprintf(s, "%d", *val);
} 

int cfg_uint32_encode(const char * s, int opt, void * ptr)
{
	uint16_t * val = (uint16_t *)ptr;

	*val = strtoul(s, NULL, 0);

	return 0;
} 

int cfg_uint32_decode(char * s, int hex, void * ptr)
{
	uint16_t * val = (uint16_t *)ptr;

	if (hex)
		return sprintf(s, "0x%04x", *val);

	return sprintf(s, "%d", *val);
} 


const struct cfg_type cfg_type_lut[] = {
	{ NULL, NULL }, /* VOID */
	{ cfg_bit_encode, cfg_bit_decode },      /* BIT */
	{ cfg_bfield8_encode, cfg_bfield8_decode }, /* BFIELD8 */
	{ cfg_bfield16_encode, cfg_bfield16_decode }, /* BFIELD16 */
	{ cfg_bfield32_encode, cfg_bfield32_decode }, /* BFIELD32 */
	{ cfg_uint8_encode, cfg_uint8_decode },  /* UINT8 */
	{ cfg_uint16_encode, cfg_uint16_decode }, /* UINT16 */
	{ cfg_uint32_encode, cfg_uint32_decode } /* UINT32 */
};

const struct cfg_attr dev_attr_lut[] = {
	BIT(  "enabled", struct ss_device, opt,  0),
	BFIELD32("addr", struct ss_device, opt,  1, 9),
	BIT(       "ap", struct ss_device, opt, 10),
	BFIELD32("type", struct ss_device, opt, 11, 6),
	{ "poll",    CFG_BIT,    17, offsetof(struct ss_device, opt) },
	{ "model",   CFG_UINT8,  0, offsetof(struct ss_device, model) },
	{ "tbias",   CFG_UINT8,  0, offsetof(struct ss_device, tbias) },
	{ "icfg",    CFG_UINT8,  1, offsetof(struct ss_device, icfg) },
	{ "ipre",    CFG_UINT8,  0, offsetof(struct ss_device, ipre) },
	{ "ilat",    CFG_UINT8,  0, offsetof(struct ss_device, ilat) },
	{ "pw1",     CFG_UINT16, 0, offsetof(struct ss_device, pw1) },
	{ "pw2",     CFG_UINT16, 0, offsetof(struct ss_device, pw2) },
	{ "pw3",     CFG_UINT16, 0, offsetof(struct ss_device, pw3) },
	{ "pw4",     CFG_UINT16, 0, offsetof(struct ss_device, pw4) },
	{ "pw5",     CFG_UINT16, 0, offsetof(struct ss_device, pw5) },
	{ "", CFG_VOID, 0}
};


int object_dump(void * obj, const struct cfg_attr attr[], FILE * f)
{
	int i;

	if (obj == NULL)
		return -1;

	for (i = 0; (attr[i].type != CFG_VOID); ++i) {
		const struct cfg_attr * a = &attr[i];
		char s[16];

		cfg_type_lut[a->type].decode(s, a->opt, obj + a->offs);
		fprintf(f, "%s = %s\n", a->name, s);
	}

	return 0;
}

static const struct cfg_attr * attr_lookup(const struct cfg_attr attr[], 
										   const char * name)
{
	int i;

	if (name == NULL)
		return NULL;

	for (i = 0; (attr[i].type != CFG_VOID); ++i) {
		const struct cfg_attr * a = &attr[i];
		if (strcmp(a->name, name) == 0)
			return a;
	}

	return NULL;
}

int object_attr_set(void * obj, const struct cfg_attr attr[], 
					const char * name, const char * val) 
{
	const struct cfg_attr * a;

	if ((obj == NULL) | (val == NULL))
		return -EINVAL;

	if ((a = attr_lookup(attr, name)) == NULL)
		return -EINVAL;

	return cfg_type_lut[a->type].encode(val, a->opt, obj + a->offs);
}

int object_attr_get(void * obj, const struct cfg_attr attr[], 
					const char * name, char * val) 
{
	const struct cfg_attr * a;

	if ((obj == NULL) | (val == NULL))
		return -EINVAL;

	if ((a = attr_lookup(attr, name)) == NULL)
		return -EINVAL;

	return cfg_type_lut[a->type].decode(val, a->opt, obj + a->offs);
}


int device_dump(FILE * f, unsigned int addr)
{
	return object_dump(&ss_dev_tab[addr], dev_attr_lut, f);
}

int device_attr_set(int addr, const char * name, const char * val)
{
	return object_attr_set(&ss_dev_tab[addr], dev_attr_lut, name, val);
}

int device_attr_print(FILE * f, unsigned int addr, const char * name)
{
	char val[16];
	int ret;

	if ((ret = object_attr_get(&ss_dev_tab[addr], dev_attr_lut, 
							  name, val)) > 0) {
		fprintf(f, "dev[%d].%s = %s\n", addr, name, val);
	}

	return ret;
}

int device_pw1_lookup(struct obj_device * obj, unsigned int sel,
					  unsigned int bias)
{
	uint32_t avg;
	uint32_t min;
	uint32_t max;
	uint32_t pw;

	if (sel >= obj->pw1->cnt)
		sel = obj->pw1->cnt - 1;

	max = obj->pw1->pw[sel].max;
	min = obj->pw1->pw[sel].min;
	avg = (max + min) / 2;
	pw = (avg * bias) / 128;
	DCC_LOG4(LOG_TRACE, "min=%d max=%d avg=%d pw=%d", min, max, avg, pw);

	return pw;
}

int device_pw2_lookup(struct obj_device * obj, unsigned int sel,
					  unsigned int bias)
{
	uint32_t avg;
	uint32_t min;
	uint32_t max;
	uint32_t pw;

	if (sel >= obj->pw2->cnt)
		sel = obj->pw2->cnt - 1;

	max = obj->pw2->pw[sel].max;
	min = obj->pw2->pw[sel].min;
	avg = (max + min) / 2;
	pw = (avg * bias) / 128;
	DCC_LOG4(LOG_TRACE, "min=%d max=%d avg=%d pw=%d", min, max, avg, pw);

	return pw;
}

int device_pw3_lookup(struct obj_device * obj, unsigned int sel,
					  unsigned int bias)
{
	uint32_t avg;
	uint32_t min;
	uint32_t max;
	uint32_t pw;

	if (sel >= obj->pw3->cnt)
		sel = obj->pw3->cnt - 1;

	max = obj->pw3->pw[sel].max;
	min = obj->pw3->pw[sel].min;
	avg = (max + min) / 2;
	pw = (avg * bias) / 128;
	DCC_LOG4(LOG_TRACE, "min=%d max=%d avg=%d pw=%d", min, max, avg, pw);

	return pw;
}

int device_pw4_lookup(struct obj_device * obj, unsigned int sel,
					  unsigned int bias)
{
	uint32_t avg;
	uint32_t min;
	uint32_t max;
	uint32_t pw;

	if (sel >= obj->pw4->cnt)
		sel = obj->pw4->cnt - 1;

	max = obj->pw4->pw[sel].max;
	min = obj->pw4->pw[sel].min;
	avg = (max + min) / 2;
	pw = (avg * bias) / 128;
	DCC_LOG4(LOG_TRACE, "min=%d max=%d avg=%d pw=%d", min, max, avg, pw);

	return pw;
}

int device_pw5_lookup(struct obj_device * obj, unsigned int sel,
					  unsigned int bias)
{
	uint32_t avg;
	uint32_t min;
	uint32_t max;
	uint32_t pw;

	if (sel >= obj->pw5->cnt)
		sel = obj->pw5->cnt - 1;

	max = obj->pw5->pw[sel].max;
	min = obj->pw5->pw[sel].min;
	avg = (max + min) / 2;
	pw = (avg * bias) / 128;
	DCC_LOG4(LOG_TRACE, "min=%d max=%d avg=%d pw=%d", min, max, avg, pw);

	return pw;
}

int config_dump(FILE * f)
{
	DCC_LOG(LOG_TRACE, "...");

	return 0;
}

int config_erase(void)
{
	DCC_LOG1(LOG_TRACE, "configuration size = %d.", sizeof(struct devsim_cfg));
#if 0
	uint32_t blk_offs = FLASH_BLK_DEV_DB_BIN_OFFS;
	uint32_t blk_size = FLASH_BLK_DEV_DB_BIN_SIZE;
	int ret;

	if ((ret = stm32_flash_erase(blk_offs, blk_size)) < 0) {
		DCC_LOG(LOG_ERROR, "stm32_flash_erase() failed!");
		return ret;
	};

	DCC_LOG(LOG_TRACE, "configuration erased!");

	return ret;
#endif
	return 0;
}

/* This is an auxiliarly structure for device configuration */
struct cfg_device {
	union {
		struct {
			uint32_t enabled: 1;
			uint32_t module: 1;
		}; 
		uint32_t bf_opt;	
	};

	uint8_t model;   /* reference to a device model */
	uint8_t tbias;
};


/* Encode the model id of the device. This is an index into 
   the devices database. */
int cfg_device_model_enc(struct microjs_json_parser * jsn, 
						  struct microjs_val * val, 
						  unsigned int bit, void * ptr)
{
	DCC_LOG(LOG_TRACE, "...");

	return 0;
}

/* Encode the array of address. This effectivelly write the configuration 
   into the device objects. */
int cfg_device_addr_enc(struct microjs_json_parser * jsn, 
						struct microjs_val * val, 
						unsigned int bit, void * ptr)
{
	struct cfg_device * cdev = (struct cfg_device *)ptr;
	struct ss_device * dev;
	struct obj_device * obj;
	int bias = cdev->tbias;
	int cnt = 0;
	int typ;
	int addr;

	DCC_LOG(LOG_TRACE, "...");

	if ((obj = device_db_model_lookup(cdev->model)) == NULL) {
		DCC_LOG1(LOG_WARNING, "invalid model: %d", cdev->model);
		return -1;
	}

	while ((typ = microjs_json_get_val(jsn, val)) == MICROJS_JSON_INTEGER) {
		addr = val->u32;
		DCC_LOG1(LOG_TRACE, "addr=%d", addr);

		if ((addr < 1) || (addr > 159)) {
			DCC_LOG1(LOG_WARNING, "invalid address addr=%d", addr);
			return -1;
		}

		if (cdev->module)
			addr += 160;
	
		dev = &ss_dev_tab[addr];
		/* disable the device prior to configuration */
		dev->enabled = 0;

		dev->pw1 = device_pw1_lookup(obj, 0, bias);
		dev->pw2 = device_pw2_lookup(obj, 0, bias);
		dev->pw3 = device_pw3_lookup(obj, 0, bias);
		dev->pw4 = device_pw4_lookup(obj, 0, bias);
		dev->pw5 = device_pw5_lookup(obj, 0, bias);

		/* enable the device per configuration */
		dev->enabled = cdev->enabled;
		cnt++;
	}

	if (typ != MICROJS_JSON_END_ARRAY) {
		return -1;
	}

	return cnt;
}

static const struct microjs_attr_desc sensor_desc[] = {
	{ "model", MICROJS_JSON_STRING, 0, offsetof(struct cfg_device, model),
		microjs_const_str_enc },
	{ "enabled", MICROJS_JSON_BOOLEAN, 0, offsetof(struct cfg_device, bf_opt),
		microjs_bit_enc },
	{ "addr", MICROJS_JSON_ARRAY, 0, 0, cfg_device_addr_enc },
	{ "", 0, 0, 0, NULL},
};

/* Encode a sensor list */
int cfg_sensor_enc(struct microjs_json_parser * jsn, 
				   struct microjs_val * val, 
				   unsigned int bit, void * ptr)
{
	struct cfg_device cdev;
	int ret;

	DCC_LOG(LOG_TRACE, "...");
	cdev.enabled = 0;
	cdev.model = 0;
	cdev.module = 0;
	cdev.tbias = 128;

	if ((ret = microjs_json_parse_obj(jsn, sensor_desc, &cdev)) < 0) {
		DCC_LOG(LOG_ERROR, "microjs_json_parse_obj() failed!");
		return ret;
	}

	return 0;
}

/* Encode a module list */
int cfg_module_enc(struct microjs_json_parser * jsn, 
				   struct microjs_val * val, 
				   unsigned int bit, void * ptr)
{
	struct cfg_device cdev;
	int ret;

	DCC_LOG(LOG_TRACE, "...");
	cdev.enabled = 0;
	cdev.model = 0;
	cdev.module = 1;
	cdev.tbias = 128;

	if ((ret = microjs_json_parse_obj(jsn, sensor_desc, &cdev)) < 0) {
		DCC_LOG(LOG_ERROR, "microjs_json_parse_obj() failed!");
		return ret;
	}

	return 0;
}

static const struct microjs_attr_desc cfg_desc[] = {
	{ "sensor", MICROJS_JSON_OBJECT, 0, 0, cfg_sensor_enc },
	{ "module", MICROJS_JSON_OBJECT, 0, 0, cfg_module_enc },
	{ "", 0, 0, 0, NULL},
};

#define JS_TOK_BUF_MAX 4096

int config_compile(void)
{
	struct microjs_json_parser jsn;
	struct microjs_tokenizer tkn;
	uint8_t tok_buf[JS_TOK_BUF_MAX];
	unsigned int json_crc;
	int json_len;
	char * js;
	int ret;

	DCC_LOG1(LOG_TRACE, "sp=0x%08x ..........................", cm3_sp_get());

	js = (char *)(STM32_MEM_FLASH + FLASH_BLK_SIM_CFG_JSON_OFFS);

	json_len = microjs_json_root_len(js);
	json_crc = crc16ccitt(0, js, json_len);
	(void)json_crc;

	DCC_LOG3(LOG_TRACE, "js=0x%08x len=%d crc=0x%04x", 
			 js, json_len, json_crc);

	microjs_tok_init(&tkn, tok_buf, JS_TOK_BUF_MAX);

	/* parse the JASON file with the microjs tokenizer */
	if ((ret = microjs_tokenize(&tkn, js, json_len)) < 0) {
		DCC_LOG(LOG_ERROR, "microjs_parse() failed!");
		return ret;
	}

	//microjs_tok_dump(stdout, &tkn);

	microjs_json_init(&jsn, &tkn);

	if (microjs_json_get_val(&jsn, NULL) != MICROJS_JSON_OBJECT) {
		DCC_LOG(LOG_ERROR, "root must be an object!");
		return -1;
	}

	/* decode the token stream */
	if ((ret = microjs_json_parse_obj(&jsn, cfg_desc, NULL)) < 0) {
		DCC_LOG(LOG_ERROR, "microjs_json_parse_obj() failed!");
		return ret;
	}

	return ret;
}
