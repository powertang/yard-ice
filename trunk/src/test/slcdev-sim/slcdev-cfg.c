#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <microjs.h>

#include <sys/dcclog.h>

#include "crc.h"
#include "slcdev.h"
#include "isink.h"

/* Basic field (attribute) types */
enum {
	CFG_VOID,
	CFG_BIT,
	CFG_BFIELD8,
	CFG_BFIELD16,
	CFG_BFIELD32,
	CFG_UINT8,
	CFG_UINT16,
	CFG_UINT32,
	CFG_ARRAY_U8,
	CFG_ARRAY_U16,
	CFG_ARRAY_U32,
};

#define BFIELD_OPT(BITS, POS) (((((BITS) - 1) & 0x1f) << 5) | ((POS) & 0x1f))
#define ARRAY_OPT(LEN, HEX) ((((HEX) ? 0 : 1) << 15) | ((LEN) & 0x7fff))

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
		return sprintf(s, "0x%08x", *val);

	return sprintf(s, "%d", *val);
} 

int cfg_array_u8_encode(const char * s, int opt, void * ptr)
{
	uint8_t * val = (uint8_t *)ptr;
	int cnt = 0;
	char * cp;
	char * ep;
	int len;

	len = opt & 0xffff;
	ep = (char *)s;

	do {
		cp = ep;
		val[cnt++] = strtoul(s, &ep, 0);
	} while ((cnt < len) && (cp != ep)); 

	return 0;
} 

int cfg_array_u8_decode(char * s, int opt, void * ptr)
{
	uint8_t * val = (uint8_t *)ptr;
	int len = opt & 0x7fff;
	char * cp = s;
	int i;
	int n;

	DCC_LOG1(LOG_INFO, "len=%d", len);

	for (i = 0; i < len; ++i) {
		DCC_LOG2(LOG_INFO, "val[%d]=%d", i, val[i]);
		if (i > 0)
			*cp++ = ' ';
		if (opt & 0x8000)
			n = sprintf(cp, "0x%02x", val[i]);
		n = sprintf(cp, "%d", val[i]);
		cp += n;
	}

	return cp - s;
} 


const struct cfg_type cfg_type_lut[] = {
	{ NULL,                NULL }, /* VOID */
	{ cfg_bit_encode,      cfg_bit_decode },      /* BIT */
	{ cfg_bfield8_encode,  cfg_bfield8_decode }, /* BFIELD8 */
	{ cfg_bfield16_encode, cfg_bfield16_decode }, /* BFIELD16 */
	{ cfg_bfield32_encode, cfg_bfield32_decode }, /* BFIELD32 */
	{ cfg_uint8_encode,    cfg_uint8_decode },  /* UINT8 */
	{ cfg_uint16_encode,   cfg_uint16_decode }, /* UINT16 */
	{ cfg_uint32_encode,   cfg_uint32_decode }, /* UINT32 */
	{ cfg_array_u8_encode, cfg_array_u8_decode } /* UINT8 Array */
};

const struct cfg_attr dev_attr_lut[] = {
	BFIELD32( "addr", struct ss_device, opt,  0, 8), /* RO */
	BFIELD32("model", struct ss_device, opt,  8, 6), /* RO */
	BIT(        "ap", struct ss_device, opt, 14),    /* RO */
	BIT(    "module", struct ss_device, opt, 15),    /* RO */
	BIT(   "enabled", struct ss_device, opt, 16),
	BIT(       "cfg", struct ss_device, opt, 17),    /* RO */
	BIT(       "led", struct ss_device, opt, 18),    
	BIT(       "tst", struct ss_device, opt, 19),
	BIT(      "out1", struct ss_device, opt, 20),
	BIT(      "out2", struct ss_device, opt, 21),
	BIT(      "out3", struct ss_device, opt, 22),
	BIT(      "out5", struct ss_device, opt, 23),
	BFIELD32(  "alm", struct ss_device, opt, 24, 4), /* RW */
	BFIELD32(  "tbl", struct ss_device, opt, 28, 4), /* RW */
	{ "tbias",   CFG_UINT8,  0, offsetof(struct ss_device, tbias) },
	{ "icfg",    CFG_UINT8,  1, offsetof(struct ss_device, icfg) },
	{ "ipre",    CFG_UINT8,  0, offsetof(struct ss_device, ipre) },
	{ "ilat",    CFG_UINT8,  0, offsetof(struct ss_device, ilat) },
	{ "pw1",     CFG_UINT16, 0, offsetof(struct ss_device, pw1) },
	{ "pw2",     CFG_UINT16, 0, offsetof(struct ss_device, pw2) },
	{ "pw3",     CFG_UINT16, 0, offsetof(struct ss_device, pw3) },
	{ "pw4",     CFG_UINT16, 0, offsetof(struct ss_device, pw4) },
	{ "pw5",     CFG_UINT16, 0, offsetof(struct ss_device, pw5) },
	{ "grp",     CFG_ARRAY_U8, ARRAY_OPT(4, 0), 
		offsetof(struct ss_device, grp) },
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


int device_dump(FILE * f, bool mod, unsigned int addr)
{
	struct db_dev_model * db_model;
	struct ss_device * dev;

	if (addr > 160) 
		return -1;

	dev = &ss_dev_tab[addr + (mod ? 160 : 0)];

	db_model = db_dev_model_by_index(db_info_get(), dev->model);
	printf("\"%s\" \"%s\":\n", const_str(db_model->model), 
		   const_str(db_model->desc));

	return object_dump(dev, dev_attr_lut, f);
}

int device_attr_set(bool mod, unsigned int addr, 
					const char * name, const char * val)
{
	if (addr > 160) 
		return -1;

	return object_attr_set(&ss_dev_tab[addr + (mod ? 160 : 0)], 
						   dev_attr_lut, name, val);
}

int device_attr_print(FILE * f, bool mod, unsigned int addr, 
					  const char * name)
{
	char val[16];
	int ret;

	if (addr > 160) 
		return -1;
	
	if ((ret = object_attr_get(&ss_dev_tab[addr + (mod ? 160 : 0)], 
							   dev_attr_lut, name, val)) > 0) {
		fprintf(f, "%s[%d].%s = %s\n", mod ? "mod" : "sens", 
				addr, name, val);
	}

	return ret;
}

int config_dump(FILE * f)
{
	DCC_LOG(LOG_INFO, "...");

	return 0;
}
/*
#define CFG_MAGIC 0x90f93e9a

struct cfg_info {
	uint32_t magic;
	uint16_t json_crc;
	uint16_t json_len;
	uint16_t dev_offs;
	uint16_t dev_crc;
};
*/

#if 0
{
	DCC_LOG1(LOG_INFO, "configuration size = %d.", sizeof(struct devsim_cfg));
	uint32_t blk_offs = FLASH_BLK_DEV_DB_BIN_OFFS;
	uint32_t blk_size = FLASH_BLK_DEV_DB_BIN_SIZE;
	int ret;

	if ((ret = stm32_flash_erase(blk_offs, blk_size)) < 0) {
		DCC_LOG(LOG_ERROR, "stm32_flash_erase() failed!");
		return ret;
	};

	DCC_LOG(LOG_INFO, "configuration erased!");

	return ret;
	return 0;
}
#endif

int config_erase(void)
{
	uint32_t blk_offs = FLASH_BLK_CFG_BIN_OFFS;
	uint32_t blk_size = FLASH_BLK_CFG_BIN_SIZE;
	int ret;

	if ((ret = stm32_flash_erase(blk_offs, blk_size)) < 0) {
		DCC_LOG(LOG_ERROR, "stm32_flash_erase() failed!");
		return ret;
	};

	DCC_LOG(LOG_INFO, "configuration erased!");

	return ret;
}

/* Encode the array of address. This effectivelly write the configuration 
   into the device objects. */
int cfg_device_addr_enc(struct microjs_json_parser * jsn, 
						struct microjs_val * val, 
						unsigned int opt, void * ptr)
{
	uint32_t * addr_bmp = (uint32_t *)ptr;
	int typ;

	while ((typ = microjs_json_get_val(jsn, val)) == MICROJS_JSON_INTEGER) {
		unsigned int addr = val->u32;
		if ((addr < 1) || (addr > 159)) {
			DCC_LOG1(LOG_WARNING, "invalid address addr=%d", addr);
			printf("Invalid address: %d\n", addr);
			return -1;
		}
		DCC_LOG1(LOG_INFO, "addr=%d", addr);
		addr_bmp[addr / 32] |= (1 << (addr % 32));
	}

	return (typ == MICROJS_JSON_END_ARRAY) ? 0 : -1;
}

/* This is an auxiliarly structure for parsing the device 
   configuration JSON file */
struct cfg_device {
	union {
		struct {
			uint32_t enabled: 1;
			uint32_t module: 1;
		}; 
		uint32_t opt;	
	};

	uint8_t ilat;
	uint8_t ipre;
	uint8_t imode;
	uint8_t irate;
	uint8_t model;   /* reference to a database device model */
	uint8_t tag; 
	uint8_t tbias;
	uint8_t grp[4]; /* list of groups */	
	uint32_t addr_bmp[160 / 32]; /* list of addresses */
};

static const struct microjs_attr_desc sensor_desc[] = {
	{ "model", MICROJS_JSON_STRING, 0, offsetof(struct cfg_device, model),
		microjs_const_str_enc },
	{ "tag", MICROJS_JSON_STRING, 0, offsetof(struct cfg_device, tag),
		microjs_const_str_enc },
	{ "enabled", MICROJS_JSON_BOOLEAN, 0, offsetof(struct cfg_device, opt),
		microjs_bit_enc },
	{ "ilat", MICROJS_JSON_INTEGER, 0, offsetof(struct cfg_device, ilat),
		microjs_u8_enc },
	{ "ipre", MICROJS_JSON_INTEGER, 0, offsetof(struct cfg_device, ipre),
		microjs_u8_enc },
	{ "irate", MICROJS_JSON_INTEGER, 0, offsetof(struct cfg_device, irate),
		microjs_u8_enc },
	{ "imode", MICROJS_JSON_INTEGER, 0, offsetof(struct cfg_device, imode),
		microjs_u8_enc },
	{ "tbias", MICROJS_JSON_INTEGER, 0, offsetof(struct cfg_device, tbias),
		microjs_u8_enc },
	{ "group", MICROJS_JSON_ARRAY, 4, offsetof(struct cfg_device, grp), 
		microjs_array_u8_enc },
	{ "addr", MICROJS_JSON_ARRAY, 0, offsetof(struct cfg_device, addr_bmp), 
		cfg_device_addr_enc },
	{ "", 0, 0, 0, NULL},
};

static int cfg_device_list_add(struct cfg_device * cdev)
{
	struct ss_device * dev;
	struct db_dev_model * mod;
	unsigned int addr;
	int mod_idx;
	int tbias;
	int icfg;

	if ((mod_idx = db_dev_model_index_by_name(db_info_get(), 
											  cdev->model)) < 0) {
		DCC_LOG1(LOG_WARNING, "invalid model: %d", cdev->model);
		printf("Invalid model: \"%s\"\n", const_str(cdev->model));
		return mod_idx;
	}

	DCC_LOG2(LOG_INFO, "model=%d idx=%d", cdev->model, mod_idx);

	mod = db_dev_model_by_index(db_info_get(), mod_idx);
	printf("%c \"%s\" \"%s\":", cdev->enabled ? '+' : '-', 
		   const_str(mod->model), const_str(mod->desc));

	if (mod->module != cdev->module) {
		DCC_LOG(LOG_WARNING, "mod->module != cdev->module");
		printf(" not a %s!\n", cdev->module ? "module" : "sensor");
		return -1;
	}

	tbias = (cdev->tbias * 128) / 100;
	icfg = (cdev->irate << 5) + (cdev->imode & 0x1f);

	DCC_LOG4(LOG_INFO, "grp={%d %d %d %d}", 
			 cdev->grp[0], cdev->grp[1], cdev->grp[2], cdev->grp[3]);

	for (addr = 0; addr < 160; ++addr) {
		if (!(cdev->addr_bmp[addr / 32] & (1 << (addr % 32))))
			continue;

		DCC_LOG1(LOG_INFO, "addr=%d", addr);
		printf(" %d", addr);

		dev = &ss_dev_tab[addr + (cdev->module ? 160 : 0)];

		if (dev->addr != addr) {
			DCC_LOG2(LOG_WARNING, "addr(%d) != dev->addr", 
					 addr, dev->addr);
			printf("Internal error!\n");
			return -1;
		};

		if (dev->module != cdev->module) {
			DCC_LOG2(LOG_WARNING, "module(%d) != dev->module", 
					 addr, dev->addr);
			printf("Internal error!\n");
			return -1;
		};

		dev->model = mod_idx;
		dev->pw1 = device_db_pw_lookup(mod->pw1, 0);
		dev->pw2 = device_db_pw_lookup(mod->pw2, 0);
		dev->pw3 = device_db_pw_lookup(mod->pw3, 0);
		dev->pw4 = device_db_pw_lookup(mod->pw4, 0);
		dev->pw5 = device_db_pw_lookup(mod->pw5, 0);

		dev->ilat = cdev->ilat;
		dev->ipre = cdev->ipre;
		dev->icfg = icfg;
		dev->tbias = tbias;
		dev->grp[0] = cdev->grp[0];
		dev->grp[1] = cdev->grp[1];
		dev->grp[2] = cdev->grp[2];
		dev->grp[3] = cdev->grp[3];
		/* mark the device as configured */
		dev->cfg = 1;
		/* enable the device per configuration */
		dev->enabled = cdev->enabled ? 1 : 0;

		DCC_LOG3(LOG_INFO, "%s %d: enabled=%d", 
				 dev->module ? "module" : "sensor", dev->addr, dev->enabled);
	}

	printf("\n");

	return 0;
}


/* Encode a module list */
int cfg_device_enc(struct microjs_json_parser * jsn, 
				   struct microjs_val * val, 
				   unsigned int opt, void * ptr)
{
	struct cfg_device cdev;
	int ret;

	memset(&cdev, 0, sizeof(struct cfg_device));
	cdev.module = opt;
	cdev.ilat = ILAT_DEFAULT;
	cdev.ipre = IPRE_DEFAULT;
	cdev.irate = ISINK_RATE_NORMAL >> 5;
	cdev.imode = ISINK_CURRENT_NOM;
	cdev.tbias = 100; /* 100 % */

	if ((ret = microjs_json_parse_obj(jsn, sensor_desc, &cdev)) < 0) {
		DCC_LOG(LOG_ERROR, "microjs_json_parse_obj() failed!");
		return ret;
	}

	return cfg_device_list_add(&cdev);
}

static const struct microjs_attr_desc cfg_desc[] = {
	{ "sensor", MICROJS_JSON_OBJECT, 0, 0, cfg_device_enc },
	{ "module", MICROJS_JSON_OBJECT, 1, 0, cfg_device_enc },
	{ "", 0, 0, 0, NULL},
};

#define CFG_MAGIC 0xc01fe9da

struct cfg_info {
	uint32_t magic;
	const char * json_txt;
	uint16_t json_crc;
	uint16_t json_len;
	const void * tab_ptr;
	uint16_t tab_size;
	uint16_t tab_crc;
};

int dev_tab_save(struct cfg_info * inf)
{
	const void * ptr;
	int offs;
	int size;
	int ret;

	/* wrtie device table */
	size = (sizeof(ss_dev_tab) + 3) & ~3;
	offs = FLASH_BLK_CFG_BIN_OFFS + FLASH_BLK_CFG_BIN_SIZE - size;

	if ((ret = stm32_flash_write(offs, ss_dev_tab, size)) < 0) {
		DCC_LOG(LOG_WARNING, "stm32_flash_write() failed!");
		return -1;
	}

	ptr = (const void *)(STM32_MEM_FLASH + offs);
	inf->tab_crc = crc16ccitt(0, ptr, size);
	inf->tab_size = size;
	inf->tab_ptr = ptr;

	return ret;
}

#define JSON_TOK_BUF_MAX 384

/* lables recognized by the JSON scanner */
const char * const cfg_labels[] = {
	"sensor",
	"module",
	"model",
	"group",
	"enabled",
	"addr",
	"pw1",
	"pw2",
	"pw3",
	"pw4",
	"pw5",
	"irate",
	"imode",
	"ilat",
	"ipre",
	"tbias",
	"tag",
	"sw1",
	"sw2",
	"on",
	"off",
	"up",
	"down",
	"script",
	NULL	
};

int config_compile(struct fs_file * json)
{
	struct microjs_json_parser jsn;
	uint8_t tok_buf[JSON_TOK_BUF_MAX];
	int ret;

	microjs_json_init(&jsn, tok_buf, JSON_TOK_BUF_MAX, cfg_labels);

	if ((ret = microjs_json_open(&jsn, (char *)json->data, json->size)) < 0) {
		DCC_LOG(LOG_ERROR, "microjs_json_open() failed!");
		return ret;
	}

	/* uncofigure all devices */
	dev_sim_uncofigure_all();

	/* skip to the object oppening to allow object by object parsing */
	microjs_json_flush(&jsn);

	/* parse the JASON file with the microjs tokenizer */
	while ((ret = microjs_json_scan(&jsn)) == MICROJS_OK) {
		/* decode the token stream */
		if ((ret = microjs_json_parse_obj(&jsn, cfg_desc, NULL)) < 0) {
			DCC_LOG(LOG_ERROR, "microjs_json_parse_obj() failed!");
			return ret;
		}
		microjs_json_flush(&jsn);
	}

	if (ret != MICROJS_EMPTY_STACK) {
		DCC_LOG(LOG_ERROR, "microjs_json_scan() failed!");
		return ret;
	}

	return 0;
}

int config_load(void)
{
	struct cfg_info * inf;
	unsigned int offs;
	unsigned int size;
	unsigned int crc;
	const void * ptr;

	/* check configuration integrity */
	inf = (struct cfg_info *)(STM32_MEM_FLASH + FLASH_BLK_CFG_BIN_OFFS);
	/* wrtie device table */
	size = (sizeof(ss_dev_tab) + 3) & ~3;
	offs = FLASH_BLK_CFG_BIN_OFFS + FLASH_BLK_CFG_BIN_SIZE - size;
	ptr = (const void *)(STM32_MEM_FLASH + offs);
	crc = crc16ccitt(0, ptr, size);

	if ((inf->magic == CFG_MAGIC) && (inf->tab_ptr == ptr) && 
		(inf->tab_crc == crc) && (inf->tab_size == size)) {
		memcpy(ss_dev_tab, ptr, sizeof(ss_dev_tab));
		return 0;
	}

	return -1;
}

int config_save(struct fs_file * json)
{
	struct cfg_info inf;
	int offs;
	int size;
	int ret;

	DCC_LOG(LOG_INFO, "...");

	inf.magic = CFG_MAGIC;
	if (json == NULL) {
		inf.json_txt = NULL;
		inf.json_crc = 0;
		inf.json_len = 0;
	} else {
		inf.json_txt = (char *)json->data;
		inf.json_crc = json->crc;
		inf.json_len = json->size;
	}
	if ((ret = dev_tab_save(&inf)) < 0)
		return ret;

	size = sizeof(struct cfg_info);
	offs = FLASH_BLK_CFG_BIN_OFFS;

	if ((ret = stm32_flash_write(offs, &inf, size)) < 0) {
		DCC_LOG(LOG_WARNING, "stm32_flash_write() failed!");
	}

	return ret;
}

bool config_is_sane(void)
{
	struct cfg_info * inf;
	unsigned int offs;
	unsigned int size;
	unsigned int crc;
	const void * ptr;

	/* check configuration integrity */
	inf = (struct cfg_info *)(STM32_MEM_FLASH + FLASH_BLK_CFG_BIN_OFFS);
	/* wrtie device table */
	size = (sizeof(ss_dev_tab) + 3) & ~3;
	offs = FLASH_BLK_CFG_BIN_OFFS + FLASH_BLK_CFG_BIN_SIZE - size;
	ptr = (const void *)(STM32_MEM_FLASH + offs);
	crc = crc16ccitt(0, ptr, size);

	if ((inf->magic == CFG_MAGIC) && (inf->tab_ptr == ptr) && 
		(inf->tab_crc == crc) && (inf->tab_size == size)) {
		return true;
	}

	return false;
}

bool config_need_update(struct fs_file * json)
{
	struct cfg_info * inf;

	/* check configuration integrity */
	inf = (struct cfg_info *)(STM32_MEM_FLASH + FLASH_BLK_CFG_BIN_OFFS);

	if (inf->magic != CFG_MAGIC || inf->json_txt != (char *)json->data ||
		inf->json_crc != json->crc || inf->json_len != json->size)
		return true;

	return false;
}

int config_show_info(FILE * f)
{
	struct cfg_info * inf;
	unsigned int offs;
	unsigned int size;
	unsigned int crc;
	const void * ptr;

	/* check configuration integrity */
	inf = (struct cfg_info *)(STM32_MEM_FLASH + FLASH_BLK_CFG_BIN_OFFS);
	/* wrtie device table */
	size = (sizeof(ss_dev_tab) + 3) & ~3;
	offs = FLASH_BLK_CFG_BIN_OFFS + FLASH_BLK_CFG_BIN_SIZE - size;
	ptr = (const void *)(STM32_MEM_FLASH + offs);
	crc = crc16ccitt(0, ptr, size);

	fprintf(f, " Info: magic=0x%08x size=%d crc=0x%04x\n", inf->magic,
			inf->tab_size, inf->tab_crc);
	fprintf(f, " JSON: txt=0x%08x len=%d crc=0x%04x\n", 
			(uint32_t)inf->json_txt, inf->json_len, inf->json_crc);

	if ((inf->magic == CFG_MAGIC) && (inf->tab_ptr == ptr) && 
		(inf->tab_crc == crc) && (inf->tab_size == size)) {
		fprintf(f, "Ok.\n");
		return 0;
	}

	fprintf(f, "Invalid configuration!\n");
	fprintf(f, "Table: magic=0x%08x size=%d crc=0x%04x\n", 
			(uint32_t)CFG_MAGIC, size, crc);

	return -1;
}
