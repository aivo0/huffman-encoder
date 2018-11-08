#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include "bitfile.h"

#define END_OF_FILE 256
#define MAX_CHAR 257

int handle_error(char *message)
{
    perror(message);
    exit(-1);
}

struct tree
{
    /* if left==0, it is a leaf. Otherwise it has two branches */
    struct tree *left;
    struct tree *right; /* not  in a leaf */
    int ch;             /* only in a leaf */
    int freq;           /* occurence rate of the tree */
};

typedef unsigned char *ratetable;

struct char_code
{
    unsigned int b_code;
    unsigned int char_len;
};

ratetable CollectRate(FILE *f)
{
    ratetable freq;
    //unsigned char c;
    if (!(freq = (ratetable)malloc(sizeof(int) * MAX_CHAR)))
        handle_error("Error allocating memory for tree");
    int i;
    while (!feof(f))
    {
        i = fgetc(f);
        //c = (unsigned char)i;
        freq[i]++;
    }
    return freq;
}

int find_min_tree(struct tree *forest[])
{
    int i;
    int index = INT_MAX;
    int min_freq = INT_MAX;

    for (i = 0; i < MAX_CHAR; i++)
        if (forest[i] != NULL)
            if (forest[i]->freq < min_freq)
            {
                min_freq = forest[i]->freq;
                index = i;
            }

    return index;
}

struct tree *MakeTree(ratetable freq)
{
    struct tree *forest[MAX_CHAR];
    struct tree *t, *t1, *t2;
    int i, min;
    int forestSize = 0;

    for (i = 0; i < MAX_CHAR; i++)
    {
        forest[i] = NULL;
    }
    // for each tree c with freq>0, add a new tree {0,0,c,freq[c]} to forest
    for (i = 0; i < MAX_CHAR; i++)
        if (freq[i] > 0)
        {
            if (!(t = (struct tree *)malloc(sizeof(struct tree))))
                handle_error("Error allocating memory for tree");
            t->left = NULL;
            t->right = NULL;
            t->ch = i;
            t->freq = freq[i];
            //printf("t->ch: %c %d\n", t->ch, t->ch);
            //printf("t->freq: %d\n", t->freq);
            forest[i] = t;
            forestSize++;
        }

    while (forestSize > 1)
    {
        min = find_min_tree(forest);
        t1 = forest[min];
        forest[min] = NULL;
        min = find_min_tree(forest);
        t2 = forest[min];
        forest[min] = NULL;
        //printf("t1->ch: %c %d\n", t1->ch, t1->ch);
        //printf("t2->ch: %c %d\n", t2->ch, t1->ch);
        if (!(t = (struct tree *)malloc(sizeof(struct tree))))
            handle_error("Error allocating memory for tree");
        t->left = t1;
        t->right = t2;
        t->ch = -1; // int == -1 will be a branch to avoid conflicts
        //printf("t->ch: %c %d\n", 0, 0);
        t->freq = t1->freq + t2->freq;
        forest[min] = t;
        forestSize--;
    }

    // Find and return the forest
    for (i = 0; i < MAX_CHAR; i++)
        if (forest[i] != NULL)
            return forest[i];
}

int get_char_codes(struct tree *tr, struct char_code *chars, unsigned int cur_char_code, int depth)
{
    if (tr->ch != -1)
    {
        chars[tr->ch].b_code = cur_char_code;
        chars[tr->ch].char_len = (unsigned int)depth;
        //printf("charcode %d %c len %d\n", cur_char_code, tr->ch, (unsigned int)depth);
    }
    else
    {
        cur_char_code <<= 1;
        depth++;
        get_char_codes(tr->right, chars, cur_char_code | 1, depth);
        get_char_codes(tr->left, chars, cur_char_code, depth);
    }
    return (0);
}

// write a representation of the tree with characters
int write_legend(struct BITFILE *bf, struct tree *tr)
{
    if (tr->ch == -1)
    {
        // -1 will represent branching
        writebits(bf, 1, 1);
        //printf("@");
        write_legend(bf, tr->left);
        write_legend(bf, tr->right);
    }
    else
    {
        // 0 will represent beginning of character
        writebits(bf, 0, 1);
        writebits(bf, tr->ch, 9);
        //printf("0%c", tr->ch);
    }
    return (0);
}

// Restore a tree from the legend
struct tree *tree_from_legend(struct BITFILE *bf)
{
    struct tree *ptr;

    if (!(ptr = malloc(sizeof(struct tree))))
        handle_error("Error allocating memory for tree");
    if (getbit(bf))
    {
        // -1 represents a branch
        ptr->ch = -1;
        //printf("t->ch: %c %d\n", 0, 0);
        ptr->left = tree_from_legend(bf);
        ptr->right = tree_from_legend(bf);
    }
    else
    {
        ptr->right = NULL;
        ptr->left = NULL;
        ptr->ch = getbits(bf, 9);
        //printf("t->ch: %c %d\n", ptr->ch, ptr->ch);
    }
    return (ptr);
}

int encode(struct char_code *chars, FILE *input_file, struct BITFILE *bf, struct tree *tr)
{
    int ch;

    write_legend(bf, tr);
    //printf("\n");
    while ((ch = getc(input_file)) != EOF)
    {
        writebits(bf, chars[ch].b_code, chars[ch].char_len);
    }
    writebits(bf, chars[END_OF_FILE].b_code, chars[END_OF_FILE].char_len);
    closeoutput(bf);
    return (0);
}

int compress(FILE *in, FILE *out)
{
    int ch;
    struct BITFILE *bf;
    ratetable rates;
    struct tree *t;
    struct char_code *chars;

    rates = CollectRate(in);
    rates[END_OF_FILE] = 1;
    t = MakeTree(rates);
    //printf("%d\n", t->freq);
    if ((chars = (struct char_code *)malloc(MAX_CHAR * sizeof(struct char_code))) == NULL)
        handle_error("Error in allocating memory for char code mapping array");

    get_char_codes(t, chars, 0, 0);

    bf = bitOpen(out);

    rewind(in);
    encode(chars, in, bf, t);

    fclose(in);
    fclose(out);
    return (0);
}

struct tree *free_memory(struct tree *tr)
{
    if (tr->left != NULL && tr->right != NULL)
    {
        free_memory(tr->left);
        free_memory(tr->right);
    }
    free(tr);
    return (NULL);
}

int decompress(FILE *in, FILE *out)
{
    struct tree *tr, *ptr;
    struct BITFILE *bf;

    bf = bitOpen(in);
    /*
    tr = tree_from_legend(bf);
    printf("esimene char %c %d\n", tr->ch, tr->ch);
    printf("vasak char %c %d\n", tr->left->ch, tr->left->ch);
    printf("parem char %c %d\n", tr->right->ch, tr->right->ch);
    */
    if ((tr = tree_from_legend(bf)))
    {
        while (1)
        {

            ptr = tr;
            //printf("ch: %c %d", ptr->ch, ptr->ch);
            while (ptr->ch == -1)
            {
                if (getbit(bf))
                    ptr = ptr->right;
                else
                    ptr = ptr->left;
            };
            if (ptr->ch == END_OF_FILE)
                break;
            //printf("char is %d %c\n", ptr->ch, ptr->ch);

            fputc(ptr->ch, out);
        }
    }

    free_memory(tr);
    fclose(in);
    fclose(out);

    return (0);
}

int main(int argc, char *argv[])
{
    FILE *in, *out;

    //in = fopen("test.txt", "r");
    //out = fopen("test.out", "w");
    if (argc == 4 && !strncmp(argv[1], "-d", strlen("-d")))
    {
        //printf("-d\n");
        if ((in = fopen(argv[2], "rb")) == NULL)
            handle_error("Problem opening the input file");
        if ((out = fopen(argv[3], "wb")) == NULL)
            handle_error("Problem opening the output file");
        decompress(in, out);
    }

    else if (argc == 2 && !strncmp(argv[1], "-d", strlen("-d")))
    {
        decompress(stdin, stdout);
    }
    /*
    else if (argc == 3)
    {
        if ((in = fopen(argv[1], "rb")) == NULL)
            handle_error("Problem opening the input file");
        if ((out = fopen(argv[2], "wb")) == NULL)
            handle_error("Problem opening the output file");
        compress(in, out);
    }*/
    /*
    else
    {
        //FILE *const in = fdopen(dup(fileno(stdin)), "rb");
        //FILE *const out = fdopen(dup(fileno(stdout)), "wb");
        //compress(in, out);
    }*/

    else if (argc == 3)
    {
        if ((in = fopen(argv[1], "rb")) == NULL)
            handle_error("Problem opening the input file");
        if ((out = fopen(argv[2], "wb")) == NULL)
            handle_error("Problem opening the output file");
        compress(in, out);
    }

    else if (argc == 1)
    {
        //FILE *const in = fdopen(dup(fileno(stdin)), "rb");
        //FILE *const out = fdopen(dup(fileno(stdout)), "wb");
        //compress(in, out);
        compress(stdin, stdout);
    }

    else
        printf("Use -d for decoding and -e for encoding.");

    return 0;
}
