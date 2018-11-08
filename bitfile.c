#include <stdio.h>
#include <stdlib.h>
#include "bitfile.h"

#define MAX_BUFFER 32

struct BITFILE *bitOpen(FILE *f)
{
  struct BITFILE *pbf;
  pbf = (struct BITFILE *)malloc(sizeof(struct BITFILE));
  pbf->buffer = 0;
  pbf->counter = 0;
  pbf->f = f;
  return (pbf);
}

int getbit(struct BITFILE *bf)
{
  // if buffer is full or empty
  if (bf->counter == MAX_BUFFER || bf->counter == 0)
  {
    fread(&(bf->buffer), sizeof(unsigned int), 1, bf->f);
    bf->counter = 0;
  }
  // if next bit is 1
  if ((bf->buffer >> bf->counter) & 1)
  {
    bf->counter++;
    return (1);
  }
  // if next bit is 0
  else
  {
    bf->counter++;
    return (0);
  }
}

unsigned int getbits(struct BITFILE *bf, unsigned int length)
{
  unsigned int b1, ret;
  ret = 0;
  while (length)
  {
    b1 = getbit(bf);
    b1 <<= (length - 1);
    // copies a bit if it exists in either operand
    ret |= b1;
    length--;
  }
  return (ret);
}

int writebits(struct BITFILE *bf, unsigned int word, unsigned int length)
{
  unsigned int bit;
  while (length)
  {
    if (((word >> (length - 1)) & 1) == 0)
      bit = 0;
    else
      bit = 1;
    bit <<= bf->counter;
    bf->buffer |= bit;
    bf->counter++;
    if (bf->counter == MAX_BUFFER)
    {
      fwrite(&(bf->buffer), sizeof(unsigned int), 1, bf->f);
      bf->buffer = 0;
      bf->counter = 0;
    }
    length--;
  }
  return (0);
}

void closeoutput(struct BITFILE *bf)
{
  fwrite(&(bf->buffer), sizeof(unsigned int), 1, bf->f);
  free(bf);
}