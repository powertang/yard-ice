#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define __MICROJS_I__
#include "microjs-i.h"

#include <sys/dcclog.h>



int microjs_tok_init(struct microjs_tokenizer * tkn, 
					 uint8_t * tok, unsigned int size)
{
	tkn->cnt = 0;
	tkn->offs = 0;
	tkn->err = 0;
	tkn->size = size;
	tkn->tok = tok;
	tkn->js = NULL;

	DCC_LOG2(LOG_TRACE, "tok=0x%08x size=%d", tok, size);

	return 0;
}

#define MICROJS_BRACKET_STACK_SIZE 32

int microjs_json_tokenize(struct microjs_tokenizer * tkn, 
					 const char * js, unsigned int len)
{
	uint8_t bkt_tok[MICROJS_BRACKET_STACK_SIZE];
	unsigned int bkt_sp;
	unsigned int ltok;
	unsigned int tok;
	unsigned int i;
	int err;
	int c;
	
	/* initialize token list */
	tkn->cnt = 0;
	/* set the base javascript file reference */
	tkn->js = js;

	DCC_LOG(LOG_TRACE, "parse start");
	DCC_LOG1(LOG_TRACE, "script length = %d bytes.", len);

	/* initialize bracket matching stack pointer */
	bkt_sp = 0;
	for (i = 0; i < len; ) {

		c = js[i];
		/* Remove lead blanks */
		if (isspace(c)) {
			i++;
			continue;
		}

		DCC_LOG1(LOG_MSG, "c=0x%02x", c);

		/* Quotes: copy verbatim */
		if ((c == '\'') || (c == '\"')) {
			unsigned int offs = i + 1; /* string offset in the file */
			unsigned int j = 0; /* length of the string */
			int qt = c; /* quote character */

			DCC_LOG(LOG_INFO, "string");
			for (;;) {
				if (++i == len) {
					/* parse error, unclosed quotes */
					DCC_LOG(LOG_WARNING, "unclosed quotes");
					err = MICROJS_UNCLOSED_STRING;
					goto error;
				}
				c = js[i];
				if (c == qt) {
					i++;
					break;
				}
				j++;
			}

			if (j > MICROJS_STRING_LEN_MAX) {
				DCC_LOG(LOG_WARNING, "string too long!");
				err = MICROJS_STRING_TOO_LONG;
				goto error;
			}

			if ((tkn->size - tkn->cnt) < 3) {
				DCC_LOG(LOG_WARNING, "token buffer overflow!");
				err = MICROJS_TOKEN_BUF_OVF;
				goto error;
			}

			tkn->tok[tkn->cnt++] = TOK_STRING + j;
			tkn->tok[tkn->cnt++] = offs;
			tkn->tok[tkn->cnt++] = (offs >> 8);
			continue;
		}

		/* Symbol */
		if (isalpha(c) || (c == '_')) {
			unsigned int j;
			char * s;

			if ((tkn->size - tkn->cnt) < MICROJS_SYMBOL_LEN_MAX) {
				DCC_LOG(LOG_WARNING, "token buffer overflow!");
				err = MICROJS_TOKEN_BUF_OVF;
				goto error;
			}

			j = 0;
			s = (char *)&tkn->tok[tkn->cnt + 1];
			do {
				s[j++] = c;
				if (++i == len)	
					break;
				c = js[i];
			} while (isalnum(c) || (c == '_'));
			s[j++] = '\0';

			/* look up in the kwywords table */
			if (strcmp(s, "true") == 0) {
				DCC_LOG(LOG_INFO, "true");
				tkn->tok[tkn->cnt++] = TOK_TRUE;
				continue;
			} 
			
			if (strcmp(s, "false") == 0) {
				DCC_LOG(LOG_INFO, "false");
				tkn->tok[tkn->cnt++] = TOK_FALSE;
				continue;
			}

			if (strcmp(s, "null") == 0) {
				DCC_LOG(LOG_INFO, "null");
				tkn->tok[tkn->cnt++] = TOK_FALSE;
				continue;
			}

			err = MICROJS_INVALID_SYMBOL;
			goto error;
		}

		/* Number */
		if (isdigit(c) || (c == '-'))  {
			int32_t val = 0;
			bool neg = false;
			if (c == '-') {
				if (++i == len)	{
					err = MICROJS_INVALID_SYMBOL;
					goto error;
				}	
				neg = true;
				c = js[i];
				if (!isdigit(c))  { 
					err = MICROJS_UNEXPECTED_CHAR;
					goto error;
				}
			}

			DCC_LOG(LOG_INFO, "Number");
			/* Decimal */
			do {
				val = ((val << 2) + val) << 1;
				val += c - '0';
				if (++i == len)	
					break;
				c = js[i];
			} while (isdigit(c));

			if (neg) {
				val = -val;
			}	
		
			if ((tkn->size - tkn->cnt) < 5) {
				DCC_LOG(LOG_WARNING, "token buffer overflow!");
				err = MICROJS_TOKEN_BUF_OVF;
				goto error;
			}

			if ((val & 0xffffff00) == 0) {
				tkn->tok[tkn->cnt++] = TOK_INT8;
				tkn->tok[tkn->cnt++] = val;
				continue;
			} 
			
			if ((val & 0xffff0000) == 0) {
				tkn->tok[tkn->cnt++] = TOK_INT16;
				tkn->tok[tkn->cnt++] = val;
				tkn->tok[tkn->cnt++] = val >> 8;
				continue;
			}

			if ((val & 0xffffff00) == 0) {
				tkn->tok[tkn->cnt++] = TOK_INT24;
				tkn->tok[tkn->cnt++] = val;
				tkn->tok[tkn->cnt++] = val >> 8;
				tkn->tok[tkn->cnt++] = val >> 16;
				continue;
			}

			tkn->tok[tkn->cnt++] = TOK_INT32;
			tkn->tok[tkn->cnt++] = val;
			tkn->tok[tkn->cnt++] = val >> 8;
			tkn->tok[tkn->cnt++] = val >> 16;
			tkn->tok[tkn->cnt++] = val >> 24;
			continue;
		}
	
		switch (c) {
		case ':':
			tok = TOK_COLON;
			break;
		case '[':
			tok = TOK_LEFTBRACKET;
			goto bkt_push;
		case ']':
			tok = TOK_RIGHTBRACKET;
			ltok = TOK_LEFTBRACKET;
			goto bkt_pop;
		case '{':
			tok = TOK_LEFTBRACE;
			goto bkt_push;
		case '}':
			tok = TOK_RIGHTBRACE;
			ltok = TOK_LEFTBRACE;
			goto bkt_pop;
		default:
			DCC_LOG1(LOG_WARNING, "invalid character: '%c'.", c);
			err = MICROJS_UNEXPECTED_CHAR;
			goto error;
		}

inc_push:
		/* increment the index pointer and push a token into the buffer */
		i++;
		/* push a token into the buffer */
		if ((tkn->size - tkn->cnt) < 1) {
			DCC_LOG(LOG_WARNING, "token buffer overflow!");
			err = MICROJS_TOKEN_BUF_OVF;
			goto error;
		}
		tkn->tok[tkn->cnt++] = tok;

		DCC_LOG1(LOG_INFO, "%s", microjs_tok_str[tok]);
	}

	/* the matching bracket stack must be empty at this point */
	if (bkt_sp != 0) {
		DCC_LOG(LOG_WARNING, "bracket closing mismatch!");
		err = MICROJS_BRACKET_MISMATCH;
		goto error;
	}

	DCC_LOG1(LOG_INFO, "%s", microjs_tok_str[tok]);

	DCC_LOG1(LOG_TRACE, "token stream length = %d bytes.", tkn->cnt);
	DCC_LOG(LOG_INFO, "parse done.");

	return tkn->cnt;

bkt_push:
	/* insert a brakcet into the stack */
	if (bkt_sp == MICROJS_BRACKET_STACK_SIZE) {
		DCC_LOG(LOG_WARNING, "maximum nesting level exceeded!");
		err = MICROJS_MAX_NEST_LEVEL;
		goto error;
	}
	bkt_tok[bkt_sp++] = tok;
	goto inc_push;

bkt_pop:
	/* push a brakcet from the stack and check for matching pair */
	if ((bkt_sp == 0) || (bkt_tok[--bkt_sp] != ltok)) {
		DCC_LOG(LOG_WARNING, "bracket closing mismatch!");
		err = MICROJS_BRACKET_MISMATCH;
		goto error;
	}
	goto inc_push;

error:
	tkn->offs = i;
	tkn->err = err;

	return -1;
}

int microjs_token_get(struct microjs_tokenizer * tkn, 
					  struct microjs_val * val)
{
	unsigned int offs;
	int idx = 0;
	uint32_t x;
	int tok;
	int len;
	(void)len;

//	idx = tkn->idx;
	tok = tkn->tok[idx++];
	if (tok >= TOK_STRING) {
		len = tok - TOK_STRING;
		offs = tkn->tok[idx++];
		offs |= tkn->tok[idx++] << 8;
		val->str.dat = (char *)tkn->js + offs;
		val->str.len = len;
		tok = TOK_STRING;
	} else if (tok >= TOK_SYMBOL) {
		len = tok - TOK_SYMBOL + 1;
		val->str.dat = (char *)&tkn->tok[idx];
		val->str.len = len;
		idx += len;
		tok = TOK_SYMBOL;
	} else if (tok >= TOK_INT8) {
		x = tkn->tok[idx++];
		if (tok >= TOK_INT16) {
			x |= tkn->tok[idx++] << 8;
			if (tok == TOK_INT24) {
				x |= tkn->tok[idx++] << 16;
				if (tok >= TOK_INT32)
					x |= tkn->tok[idx++] << 24;
			}
		} 
		val->u32 = x;
		tok = TOK_INT32;
	} 

//	tkn->idx = idx;

	return tok;
}




int microjs_json_init(struct microjs_json_parser * jsn, 
					  struct microjs_tokenizer * tkn)
{
	jsn->tkn = tkn;
	jsn->idx = 0;

	return 0;
}

int microjs_json_get_val(struct microjs_json_parser * jsn,
						   struct microjs_val * val)
{
	unsigned int offs;
	uint32_t x;
	int idx;
	int tok;
	int len;
	int ret;

	idx = jsn->idx;
	tok = jsn->tkn->tok[idx++];
	if (tok >= TOK_STRING) {
		len = tok - TOK_STRING;
		offs = jsn->tkn->tok[idx++];
		offs |= jsn->tkn->tok[idx++] << 8;
		if (val != NULL) {
			val->str.dat = (char *)jsn->tkn->js + offs;
			val->str.len = len;
		}
		if (jsn->tkn->tok[idx] == TOK_COLON) {
			jsn->idx = ++idx;
			DCC_LOG1(LOG_INFO, "<%d>:",idx);
			return MICROJS_JSON_LABEL;
		} else {
			ret = MICROJS_JSON_STRING;
			DCC_LOG(LOG_INFO, "STRING");
		}
	} else if (tok >= TOK_SYMBOL) {
		DCC_LOG(LOG_WARNING, "INVALID");
		return MICROJS_JSON_INVALID;
	} else if (tok >= TOK_INT8) {
		x = jsn->tkn->tok[idx++];
		if (tok >= TOK_INT16) {
			x |= jsn->tkn->tok[idx++] << 8;
			if (tok == TOK_INT24) {
				x |= jsn->tkn->tok[idx++] << 16;
				if (tok >= TOK_INT32)
					x |= jsn->tkn->tok[idx++] << 24;
			}
		} 
		if (val != NULL)
			val->u32 = x;
		ret = MICROJS_JSON_INTEGER;
		DCC_LOG(LOG_INFO, "int");
	} else if (tok == TOK_LEFTBRACE) {
		DCC_LOG1(LOG_INFO, "<%d> {",idx);
		jsn->idx = idx;
		return MICROJS_JSON_OBJECT;
	} else if (tok == TOK_LEFTBRACKET) {
		DCC_LOG(LOG_INFO, "[");
		jsn->idx = idx;
		return MICROJS_JSON_ARRAY;
	} else if (tok >= TOK_TRUE) {
		if (val != NULL)
			val->logic = true;
		ret = MICROJS_JSON_BOOLEAN;
		DCC_LOG(LOG_INFO, "true");
	} else if (tok >= TOK_FALSE) {
		if (val != NULL)
			val->logic = false;
		ret = MICROJS_JSON_BOOLEAN;
		DCC_LOG(LOG_INFO, "false");
	} else if (tok == TOK_RIGHTBRACE) {
		ret = MICROJS_JSON_END_OBJECT;
		DCC_LOG(LOG_INFO, "}");
	} else if (tok == TOK_RIGHTBRACKET) {
		ret = MICROJS_JSON_END_ARRAY;
		DCC_LOG(LOG_INFO, "]");
	} else {
		DCC_LOG(LOG_INFO, "INVALID");
		return MICROJS_JSON_INVALID;
	}

	tok = jsn->tkn->tok[idx];

	if (tok == TOK_COMMA) {
		idx++;
	} else if ((tok != TOK_RIGHTBRACE) && 
			   (tok != TOK_RIGHTBRACKET) && 
			   (tok != TOK_EOF) ) {
		DCC_LOG(LOG_INFO, "INVALID");
		return MICROJS_JSON_INVALID;
	} 

	jsn->idx = idx;

	return ret;
}

const struct microjs_attr_desc null_desc[] = {
	{ "", 0, 0, 0, NULL},
};

int microjs_json_parse_array(struct microjs_json_parser * jsn, void * ptr)
{
	struct microjs_val val;
	int typ;

	while ((typ = microjs_json_get_val(jsn, &val)) != MICROJS_JSON_END_ARRAY) {
		if (typ == MICROJS_JSON_ARRAY) {
			microjs_json_parse_array(jsn, NULL);
		} else if (typ == MICROJS_JSON_OBJECT) {
			microjs_json_parse_obj(jsn, null_desc, NULL);
		}
	}

	return 0;
}

int microjs_json_parse_obj(struct microjs_json_parser * jsn,
						   const struct microjs_attr_desc desc[],
						   void * ptr)
{
	struct microjs_val val;
	uint8_t * p;
	int cnt = 0;
	int typ;

	DCC_LOG(LOG_INFO, "...");

	while ((typ = microjs_json_get_val(jsn, &val)) == MICROJS_JSON_LABEL) {

		int i;

		for (i = 0; desc[i].parse != NULL; ++i) {
			/* look for a decoder that matches the label */ 
			if (strncmp(desc[i].key, val.str.dat, val.str.len) == 0) {
				DCC_LOG1(LOG_INFO, "%s:", desc[i].key);
				typ = microjs_json_get_val(jsn, &val);
				if (typ != desc[i].type) {
					/* the attribute type do not matches the decoder */
					DCC_LOG(LOG_WARNING, "attribute type mismatch");
					return -1;
				}
				p = (uint8_t *)ptr + desc[i].offs;

				DCC_LOG1(LOG_INFO, "parse(p=0x%08x)", p);

				if (desc[i].parse(jsn, &val, desc[i].opt, p) < 0) {
					DCC_LOG(LOG_WARNING, "attribute parse error");
					return -1;
				}
		
				break;
			}
		}
	
		if (desc[i].parse == NULL) {
			if (val.str.len == 1) {
				DCC_LOG1(LOG_WARNING, "unsupported attribute: %c", 
						 val.str.dat[0]);
			} else if (val.str.len == 2) {
				DCC_LOG2(LOG_WARNING, "unsupported attribute: %c%c", 
					 	val.str.dat[0], val.str.dat[1]);
			} else if (val.str.len == 3) {
				DCC_LOG3(LOG_WARNING, "unsupported attribute: %c%c%c", 
					 	val.str.dat[0], val.str.dat[1], val.str.dat[2]);
			} else {
				DCC_LOG4(LOG_WARNING, "unsupported attribute: %c%c%c%c", 
						 val.str.dat[0], val.str.dat[1],
						 val.str.dat[2], val.str.dat[3]);
			}
			/* skip unsupported attribute */
			typ = microjs_json_get_val(jsn, &val);
			if (typ == MICROJS_JSON_ARRAY) {
				microjs_json_parse_array(jsn, NULL);
			} else if (typ == MICROJS_JSON_OBJECT) {
				microjs_json_parse_obj(jsn, null_desc, NULL);
			}
		}

		cnt++;
	
		DCC_LOG1(LOG_INFO, "3. cnt=%d", cnt);
	}


	if (typ == MICROJS_JSON_END_OBJECT) {
		DCC_LOG(LOG_INFO, "}");
		return cnt;
	}

	DCC_LOG(LOG_WARNING, "invalid type!");
	return -1;
}

/* Encode a 16bit integral value */
int microjs_u16_enc(struct microjs_json_parser * jsn, 
					struct microjs_val * val, 
					unsigned int opt, void * ptr)
{
	uint16_t * pval = (uint16_t *)ptr;
	*pval = val->u32;

	DCC_LOG2(LOG_INFO, "val=%d ptr=%08x", *pval, pval);
	return 0;
}

/* Encode a boolean as a single bit */
int microjs_bit_enc(struct microjs_json_parser * jsn, 
					struct microjs_val * val, 
					unsigned int bit, void * ptr)
{
	uint32_t * bfield = (uint32_t *)ptr;

	*bfield &= ~(1 << bit);
	*bfield |= (val->logic ? 1 : 0) << bit;

	DCC_LOG1(LOG_INFO, "val=%d", val->logic ? 1 : 0);
	return 0;
}

/* Encode a string as a index to the constant string pool */
int microjs_const_str_enc(struct microjs_json_parser * jsn, 
					struct microjs_val * val, 
					unsigned int opt, void * ptr)
{
	uint8_t * sip = (uint8_t *)ptr;
	int ret;

	if ((ret = const_str_write(val->str.dat, val->str.len)) < 0)
		return ret;

	*sip = ret;

	DCC_LOG1(LOG_INFO, "<%d>", ret);

	return 0;
}

int microjs_json_root_len(const char * js)
{
	char * cp;
	char * ep;
	int c;

	for (cp = ep = (char *)js; (c = *cp) != '\0'; ++cp) {
		if (c == '}')
			ep = cp;
	}

	return (ep - js) + 1; 
}

