#ifndef __CONNECTEDDOMAIN_H
#define __CONNECTEDDOMAIN_H

#include <string.h>
#include <stdio.h>
#include "SRAM.h"
#include "malloc.h"

#define uint8_t unsigned char 

int bwLabel(const unsigned char *bw, u16 *label, int h, int w);

#endif
