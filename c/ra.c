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
#include "ra.h"

void
validate_magic (const uint64_t magic)
{
   if (magic != RA_MAGIC_NUMBER) {
        fprintf(stderr, "Invalid RA file.\n");
        exit(EX_DATAERR);
   }
}

// TODO: extend validation checks to internal consistency

void
ra_query (const char *path)
{
    ra_t a;
    int j, fd;
    uint64_t magic;
    fd = open(path, O_RDONLY);
    if (fd == -1)
        err(errno, "unable to open output file for writing");
    read(fd, &magic, sizeof(uint64_t));
    validate_magic(magic);
    read(fd, &(a.flags), sizeof(uint64_t));
    read(fd, &(a.eltype), sizeof(uint64_t));
    read(fd, &(a.elbyte), sizeof(uint64_t));
    read(fd, &(a.size), sizeof(uint64_t));
    read(fd, &(a.ndims), sizeof(uint64_t));
    printf("---\nname: %s\n", path);
    printf("endian: %s\n", a.flags  & RA_FLAG_BIG_ENDIAN ? "big" : "little");
    printf("type: %s%lld\n", RA_TYPE_NAMES[a.eltype], a.elbyte*8);
    printf("size: %lld\n", a.size);
    printf("dimension: %lld\n", a.ndims);
    a.dims = (uint64_t*)malloc(a.ndims*sizeof(uint64_t));
    read(fd, a.dims, a.ndims*sizeof(uint64_t));
    printf("shape:\n");
    for (j = 0; j < a.ndims; ++j)
        printf("  - %lld\n", a.dims[j]);
    printf("...\n");
    close(fd);
}

int
ra_read (ra_t *a, const char *path)
{
    int fd;
    uint64_t bytestoread, bytesleft, magic;
    fd = open(path, O_RDONLY);
    if (fd == -1)
        err(errno, "unable to open output file for writing");
    read(fd, &magic, sizeof(uint64_t));
    validate_magic(magic);
    read(fd, &(a->flags), sizeof(uint64_t));
    read(fd, &(a->eltype), sizeof(uint64_t));
    read(fd, &(a->elbyte), sizeof(uint64_t));
    read(fd, &(a->size), sizeof(uint64_t));
    read(fd, &(a->ndims), sizeof(uint64_t));
    a->dims = (uint64_t*)malloc(a->ndims*sizeof(uint64_t));
    read(fd, a->dims, a->ndims*sizeof(uint64_t));
    a->data = (uint8_t*)malloc(a->size);
    if (a->data == NULL)
        err(errno, "unable to allocate memory for data");
    uint8_t *data_cursor = a->data;

    bytesleft = a->size;
    while (bytesleft > 0) {
        bytestoread = bytesleft < RA_MAX_BYTES ? bytesleft : RA_MAX_BYTES;
        read(fd, data_cursor, bytestoread);
        data_cursor += bytestoread;
        bytesleft -= bytestoread;
    }
    close(fd);
    return 0;
}



int
ra_write (ra_t *a, const char *path)
{
    int fd;
    uint64_t bytesleft, bufsize;
    uint8_t *data_in_cursor;
    fd = open(path, O_WRONLY|O_TRUNC|O_CREAT,0644);
    if (fd == -1)
        err(errno, "unable to open output file for writing");
    /* write the easy stuff */
    write(fd, &RA_MAGIC_NUMBER, sizeof(uint64_t));
    write(fd, &(a->flags), sizeof(uint64_t));
    write(fd, &(a->eltype), sizeof(uint64_t));
    write(fd, &(a->elbyte), sizeof(uint64_t));
    write(fd, &(a->size), sizeof(uint64_t));
    write(fd, &(a->ndims), sizeof(uint64_t));
    write(fd, a->dims, a->ndims*sizeof(uint64_t));

    bytesleft = a->size;
    data_in_cursor = a->data;

    bufsize = bytesleft < RA_MAX_BYTES ? bytesleft : RA_MAX_BYTES;
    while (bytesleft > 0) {
        write(fd, data_in_cursor, bufsize);
        data_in_cursor += bufsize / sizeof(uint8_t);
        bytesleft -= bufsize;
    }

    close(fd);
    return 0;
}


void
ra_free (ra_t *a)
{
    free(a->dims);
    free(a->data);
}

void
validate_conversion (const ra_t* r, const uint64_t neweltype, const uint64_t newelbyte)
{
    if (neweltype == RA_TYPE_USER) {
        printf("USER-defined type must be handled by the USER. :-)\n");
        exit(EX_USAGE);
    } else if (r->eltype == RA_TYPE_COMPLEX && neweltype != RA_TYPE_COMPLEX) {
        printf("Warning: converting complex to non-complex types may discard information.\n");
    } else if (newelbyte == r->elbyte && neweltype == r->eltype) {
        printf("Specified type is already the type of the source. Nothing to be done.\n");
        exit(EX_OK);
    } else if (r->flags & RA_FLAG_COMPRESSED) {
        printf("Conversion of compressed types is not implemented yet.\n");
        exit(EX_USAGE);
    } else if (newelbyte < r->elbyte)
        printf("Warning: reducing type size may cause loss of precision.\n");
}


#undef CASE
#define CASE(TYPE1,BYTE1,TYPE2,BYTE2) \
    (r->eltype == RA_TYPE_##TYPE1 && r->elbyte == BYTE1 && \
     eltype == RA_TYPE_##TYPE2 && elbyte == BYTE2)

#define CONVERT(TYPE1,TYPE2) { \
    TYPE1 *tmp_src; tmp_src = (TYPE1 *)r->data; \
    TYPE2 *tmp_dst; tmp_dst = (TYPE2 *)tmp_data; \
    for (size_t i = 0; i < nelem; ++i) tmp_dst[i] = tmp_src[i]; }


uint16_t
float32_to_float16 (const float x)
{
    uint32_t fltInt32 = (uint32_t)x;
    uint16_t fltInt16;
    fltInt16 = (fltInt32 >> 31) << 5;
    uint16_t tmp = (fltInt32 >> 23) & 0xff;
    tmp = (tmp - 0x70) & ((uint32_t)((int32_t)(0x70 - tmp) >> 4) >> 27);
    fltInt16 = (fltInt16 | tmp) << 10;
    fltInt16 |= (fltInt32 >> 13) & 0x3ff;
    return fltInt16;
}


void
ra_convert (ra_t *r, const uint64_t eltype, const uint64_t elbyte)
{
    uint64_t j, nelem;
    uint8_t *tmp_data;

    // make sure this conversion will work
    validate_conversion(r, eltype, elbyte);

    // set new properties
    nelem = 1;
    for (j = 0; j < r->ndims; ++j)
        nelem *= r->dims[j];

    // convert the data type
    uint64_t newsize = elbyte * r->size / r->elbyte;
    tmp_data = (uint8_t*)malloc(newsize);

    if CASE(INT,1,INT,2)             // INT -> INT
        CONVERT(int8_t,int16_t)
    else if CASE(INT,1,INT,4)
        CONVERT(int8_t,int32_t)
    else if CASE(INT,1,INT,8)
        CONVERT(int8_t,int64_t)

    else if CASE(INT,2,INT,1)
        CONVERT(int16_t,int8_t)
    else if CASE(INT,2,INT,4)
        CONVERT(int16_t,int32_t)
    else if CASE(INT,2,INT,8)
        CONVERT(int16_t,int64_t)

    else if CASE(INT,4,INT,1)
        CONVERT(int32_t,int8_t)
    else if CASE(INT,4,INT,2)
        CONVERT(int32_t,int16_t)
    else if CASE(INT,4,INT,8)
        CONVERT(int32_t,int64_t)

    else if CASE(UINT,8,INT,1)
        CONVERT(uint64_t,int8_t)
    else if CASE(UINT,8,INT,2)
        CONVERT(uint64_t,int16_t)
    else if CASE(UINT,8,INT,4)
        CONVERT(uint64_t,int32_t)

    else if CASE(UINT,1,INT,2)        // UINT -> INT
        CONVERT(uint8_t,int16_t)
    else if CASE(UINT,1,INT,4)
        CONVERT(uint8_t,int32_t)
    else if CASE(UINT,1,INT,8)
        CONVERT(uint8_t,int64_t)

    else if CASE(UINT,2,INT,1)
        CONVERT(uint16_t,int8_t)
    else if CASE(UINT,2,INT,4)
        CONVERT(uint16_t,int32_t)
    else if CASE(UINT,2,INT,8)
        CONVERT(uint16_t,int64_t)

    else if CASE(UINT,4,INT,1)
        CONVERT(uint32_t,int8_t)
    else if CASE(UINT,4,INT,2)
        CONVERT(uint32_t,int16_t)
    else if CASE(UINT,4,INT,8)
        CONVERT(uint32_t,int64_t)

    else if CASE(UINT,8,INT,1)
        CONVERT(uint64_t,int8_t)
    else if CASE(UINT,8,INT,2)
        CONVERT(uint64_t,int16_t)
    else if CASE(UINT,8,INT,4)
        CONVERT(uint64_t,int32_t)

    else if CASE(UINT,1,UINT,2)        // UINT -> UINT
        CONVERT(uint8_t,uint16_t)
    else if CASE(UINT,1,UINT,4)
        CONVERT(uint8_t,uint32_t)
    else if CASE(UINT,1,UINT,8)
        CONVERT(uint8_t,uint64_t)

    else if CASE(UINT,2,UINT,1)
        CONVERT(uint16_t,uint8_t)
    else if CASE(UINT,2,UINT,4)
        CONVERT(uint16_t,uint32_t)
    else if CASE(UINT,2,UINT,8)
        CONVERT(uint16_t,uint64_t)

    else if CASE(UINT,4,UINT,1)
        CONVERT(uint32_t,uint8_t)
    else if CASE(UINT,4,UINT,2)
        CONVERT(uint32_t,uint16_t)
    else if CASE(UINT,4,UINT,8)
        CONVERT(uint32_t,uint64_t)

    else if CASE(UINT,8,UINT,1)
        CONVERT(uint64_t,uint8_t)
    else if CASE(UINT,8,UINT,2)
        CONVERT(uint64_t,uint16_t)
    else if CASE(UINT,8,UINT,4)
        CONVERT(uint64_t,uint32_t)

    else if CASE(FLOAT,4,FLOAT,8)     // FLOATS AND COMPLEX
        CONVERT(float,double)
    else if CASE(FLOAT,8,FLOAT,4)
        CONVERT(double,float)
    else if CASE(COMPLEX,16,COMPLEX,8) {
        nelem *= 2;
        CONVERT(double,float)
    } else if CASE(COMPLEX,8,COMPLEX,16) {
        nelem *= 2;
        CONVERT(float,double)
    } else if CASE(FLOAT,4,COMPLEX,8) {
        float *tmp_src = (float *)r->data;
        float *tmp_dst = (float *)tmp_data;
        for (size_t i = 0; i < nelem; ++i) {
            tmp_dst[2*i] = tmp_src[i];
            tmp_dst[2*i+1] = 0.f;
        }
    } else if CASE(FLOAT,8,COMPLEX,16) {
        double *tmp_src = (double *)r->data;
        double *tmp_dst = (double *)tmp_data;
        for (size_t i = 0; i < nelem; ++i) {
            tmp_dst[2*i] = tmp_src[i];
            tmp_dst[2*i+1] = 0.;
        }
    } else if CASE(COMPLEX,8,FLOAT,4) {       // complex -> float using real part
        float *tmp_src = (float *)r->data;
        float *tmp_dst = (float *)tmp_data;
        for (size_t i = 0; i < nelem; ++i) {
            tmp_dst[i] = tmp_src[2*i];
        }
    } else if CASE(COMPLEX,16,FLOAT,8) {
        double *tmp_src = (double *)r->data;
        double *tmp_dst = (double *)tmp_data;
        for (size_t i = 0; i < nelem; ++i) {
            tmp_dst[i] = tmp_src[2*i];
        }
    } else {
        printf("Specified type and size did not conform to any supported combinations.\n");
        exit(EX_USAGE);
    }

    r->eltype = eltype;
    r->elbyte = elbyte;
    r->size = newsize;
    free(r->data);
    r->data = (uint8_t*)tmp_data;
}


uint64_t
calc_min_elbyte_int (const int64_t max, const int64_t min)
{
    printf("min: %ll, max: %ll\n", min, max);
    int minbits_reqd = log2(max - min);
    printf("minbits_reqd: %d\n", minbits_reqd);
    uint64_t m = 1;
    while (m*8 < minbits_reqd)
        m *= 2;
    return m;
}

uint64_t
calc_min_elbyte_float (const double max, const double min)
{
    printf("min: %g, max: %g\n", min, max);
    int minbits = log2(fabs(max/min));
    printf("minbits: %d\n", minbits);
    if (minbits < 16)
        return 2;
    else if (minbits < 32)
        return 4;
    else
        return 8;
}


#define MINMAX_AND_RESCALE_INT(NAME,TYPE) \
    { TYPE* rdr; rdr = (TYPE*) r->data; \
      int64_t min = rdr[0]; int64_t max = rdr[0]; \
      for (size_t i = 1; i < nelem; ++i) { \
          if (rdr[i] < min) min = rdr[i]; \
          if (rdr[i] > max) max = rdr[i]; }\
      min_elbyte = calc_min_elbyte_int(max, min); \
      if (min_elbyte < r->elbyte)\
          for (size_t i = 0; i < nelem; ++i)  rdr[i] -= min; \
     }

 #define MINMAX_AND_RESCALE_FLOAT(NAME,TYPE) \
     { TYPE* rdr; rdr = (TYPE*) r->data; \
       double min = rdr[0]; double max = rdr[0]; \
       for (size_t i = 1; i < nelem; ++i) { \
           if (rdr[i] < min) min = rdr[i]; \
           if (rdr[i] > max) max = rdr[i]; }\
       min_elbyte = calc_min_elbyte_float(max, min); \
       if (min_elbyte < r->elbyte)\
           for (size_t i = 0; i < nelem; ++i)  rdr[i] -= min; \
      }

#undef CASE
#define CASE(TYPE1,BYTE1) \
    (r->eltype == RA_TYPE_##TYPE1 && r->elbyte == BYTE1)




int
ra_compress (ra_t *r)
{
    uint64_t nelem = 1, min_elbyte, orig_elbyte;
    for (uint64_t j = 0; j < r->ndims; ++j)
        nelem *= r->dims[j];

        printf("assuming NELEM: %u\n", nelem);

    if CASE(INT,2)
        MINMAX_AND_RESCALE_INT(r,int16_t)
    else if CASE(INT,4)
        MINMAX_AND_RESCALE_INT(r,int32_t)
    else if CASE(INT,8)
        MINMAX_AND_RESCALE_INT(r,int64_t)
    else if CASE(UINT,2)
        MINMAX_AND_RESCALE_INT(r,uint16_t)
    else if CASE(UINT,4)
        MINMAX_AND_RESCALE_INT(r,uint32_t)
    else if CASE(UINT,8)
        MINMAX_AND_RESCALE_INT(r,uint64_t)
    else if CASE(FLOAT,4)
        MINMAX_AND_RESCALE_FLOAT(r,float)   // TODO: implement float16
    else if CASE(FLOAT,8)
        MINMAX_AND_RESCALE_FLOAT(r,double)
    else if CASE(COMPLEX,8) {
        nelem *= 2;
        MINMAX_AND_RESCALE_FLOAT(r,float)
    } else if CASE(COMPLEX,16) {
        nelem *= 2;
        MINMAX_AND_RESCALE_FLOAT(r,double)
    }

    if (min_elbyte < r->elbyte) {
        printf("Can compress to %u bytes.\n", min_elbyte);
        orig_elbyte = r->elbyte;
        ra_convert(r, r->eltype, min_elbyte);
        r->flags |= RA_FLAG_COMPRESSED;
    }

    return min_elbyte != orig_elbyte;
}
