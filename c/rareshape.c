/*
  This file is part of the RA package (http://github.com/davidssmith/ra).

  The MIT License (MIT)

  Copyright (c) 2015-2017 David Smith

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <math.h>
#include <sysexits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ra.h"


int
main (int argc, char *argv[])
{
    ra_t r;
    uint64_t *newdims;
    uint64_t ndimsnew;
    if (argc > 2) {
        ra_read(&r, argv[1]);
        ndimsnew = argc - 2;
        printf("# of new dims: %u\n", ndimsnew);
        newdims = (uint64_t*) malloc(ndimsnew*sizeof(uint64_t));
        for (uint64_t k = 0; k < ndimsnew; ++k) {
            newdims[k] = atol(argv[k+2]);
            printf("newdims[%d] = %u\n", k, newdims[k]);
        }
        if (ra_reshape(&r, newdims, ndimsnew) == 0)
            ra_write(&r, argv[1]);
    } else {
        printf("Reshape ra file.\n");
        printf("Usage: %s file.ra n1 n2 ...\n", argv[0]);
        exit(EX_USAGE);
    }
    ra_free(&r);
    free(newdims);
    return 0;
}
