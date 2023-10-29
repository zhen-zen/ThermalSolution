//  SPDX-License-Identifier: GPL-2.0-only
//
//  thd_lzma_dec.h
//  intel/thermal_daemon
//
//  Created by Hao Song on 2021/12/31.
//  Copyright © 2021 Intel. All rights reserved.
//

//#include <errno.h>
//#include <inttypes.h>
//#include <linux/input.h>
#include <sys/types.h>

int  lzma_decompress(
        unsigned char *dest,
        size_t *destLen,
        const unsigned char *src,
        size_t srcLen
);
