
#include "crypto.h"
#include "envelope.h"
#include "rc4.h"

extern void RC4_Test();
extern void DES_Test();
extern void SHA1_Test();
extern void RSA_Test();


typedef struct challenge_struct {
	uchar 	vector[8];
	uchar	c_text[40];
	uchar	key1[16];
	uchar	key2[16];
	uchar	new_exchange_key[8];
	uchar	hash[16];
} Challenge_t;

	
typedef struct response_struct {
	uchar 	vector[8];
	uchar	hash[16];
	uchar	c_text[40];
} Response_t;

