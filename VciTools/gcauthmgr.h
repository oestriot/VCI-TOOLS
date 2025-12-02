#ifndef GCAUTHMGR_H
#define GCAUTHMGR_H 1
#include "vci.h"

void get_cart_hash(GcCmd56Keys* keys, uint8_t* packet20_hash);
void get_cart_secret(GcCmd56Keys* keys, uint8_t* cart_secret);

#endif