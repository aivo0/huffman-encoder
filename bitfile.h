#ifndef _BITFILE_
#define _BITFILE_

struct BITFILE
{
    unsigned buffer;  /* intermediate store for bits */
    unsigned counter; /*  how many bits are waiting */
    FILE *f;          /* byte file */
};                    /* bit input/output buffer */

struct BITFILE *bitOpen(FILE *f);                                          /*initialise bit input or output*/
int getbit(struct BITFILE *bf);                                            /*get one bit from bf->buffer */
unsigned int getbits(struct BITFILE *bf, unsigned int length);             /* read n bits */
int writebits(struct BITFILE *bf, unsigned int word, unsigned int length); /* put nbits lower bits of word */
void closeoutput(struct BITFILE *bf);                                      /*finish writing. Flush the last bits to bf->file*/

#endif
