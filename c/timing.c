/*
  This file is part of the RA package (http://github.com/davidssmith/ra).

  The MIT License (MIT)

  Copyright (c) 2015-2019 David Smith

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
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ra.h"

static clock_t begin, end;
size_t total_bytes;

void
rasmalltest (size_t n, size_t nfiles)
{
	char filename[32];
	ra_t r;
	r.magic = RA_MAGIC_NUMBER;
	r.ndims = 1;
	r.flags = 0;
	r.eltype = 3;
	r.eltype = 4;
	r.dims = (uint64_t *) malloc(1*sizeof(uint64_t));
	r.dims[0] = n;
	r.data = (uint8_t *) calloc(n,sizeof(float));
	r.size = n*sizeof(float);
	total_bytes = r.size * nfiles;
	begin = clock();
	for (size_t i = 0; i < nfiles; ++i) {
		sprintf(filename, "tmp/%ld.ra", i);
		ra_write(&r, filename);
	}
	ra_free(&r);
	for (size_t i = 0; i < nfiles; ++i) {
		sprintf(filename, "tmp/%ld.ra", i);
		ra_read(&r, filename);
		ra_free(&r);
	}
	end = clock();
	//printf("r.data[0] = %f\n", testval);
	for (size_t i = 0; i < nfiles; ++i) {
		sprintf(filename, "tmp/%ld.ra", i);
		unlink(filename);
	}
	float t = (double)(end - begin) / (double)CLOCKS_PER_SEC;
	float mb = total_bytes * 1e-6;
	printf("RawArray %ld %ldx1 files:       %6.1f ms, %6.1f MBps\n", nfiles, n, 1000*t, mb/t);
	ra_free(&r);
}

void
rabigtest (size_t n, size_t nfiles)
{
	ra_t r;
	r.magic = RA_MAGIC_NUMBER;
	r.flags = 0;
	r.eltype = 3;
	r.eltype = 4;
	r.ndims = 2;
	r.dims = (uint64_t *) malloc(r.ndims*sizeof(uint64_t));
	r.dims[0] = n;
	r.dims[1] = nfiles;
	r.data = (uint8_t *) calloc(r.dims[0]*r.dims[1],sizeof(float));
	r.size = r.dims[0]*r.dims[1]*sizeof(float);
	total_bytes = r.size;
	begin = clock();
	ra_write(&r, "tmp/big.ra");
	ra_free(&r);
	ra_read(&r, "tmp/big.ra");
	end = clock();
	unlink("tmp/big.ra");
	float t = (double)(end - begin) / (double)CLOCKS_PER_SEC;
	float mb = total_bytes * 1e-6;
	printf("RawArray 1 %ldx%ld file:        %6.1f ms, %6.1f MBps\n", n, nfiles, t*1000, mb/t);
	ra_free(&r);
}


int
main (int argc, char *argv[])
{
	size_t nfiles = 10000;
	size_t n = 100;

	rasmalltest(n, nfiles);
	rabigtest(n, nfiles);

    return 0;
}
