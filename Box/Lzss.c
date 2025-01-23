/*
 * LZSS compression routines.
 *
 * Modified from the original to work out of a buffer and use dynamically
 * allocated storage.
 */


/**************************************************************
    LZSS.C -- A Data Compression Program
    (tab = 4 spaces)
***************************************************************
    4/6/1989 Haruhiko Okumura
    Use, distribute, and modify this program freely.
    Please send me your improved versions.
         PC-VAN  SCIENCE
         NIFTY-Serve    PAF01022
         CompuServe     74050,1022
**************************************************************/

#include "WTVTypes.h"
#include "ErrorNumbers.h"
#include "boxansi.h"
#include "Debug.h"

#include "Compress.h"

/*
 * LZSS parameters.
 */
#define N          4096     /* size of ring buffer */
#define F            18     /* upper limit for match_length */
#define THRESHOLD     2     /* encode string into position and length
                               if match_length is greater than this */
#define NIL           N     /* index for root of binary search trees */



#ifdef FUCK_YOU_JOE

static int LZInitTree(void);
static void LZFreeTrees(void);
static void LZInsertNode(int r);
static void LZDeleteNode(int p);

static ulong
    textsize = 0,       /* text size counter */
    codesize = 0;       /* code size counter */

/*printcount = 0;*/     /* counter for reporting progress every 1K bytes */

static uchar *text_buf = nil;
static int match_position, match_length,
            *lson = nil, *rson = nil, *dad = nil;


static int
LZInitTree(void)  /* initialize trees */
{
    int  i;

    text_buf = (char *)malloc(N + F - 1);
    lson = (int *)malloc((N + 1) * sizeof(int));
    rson = (int *)malloc((N + 257) * sizeof(int));
    dad = (int *)malloc((N + 1) * sizeof(int));
    if (text_buf == nil || lson == nil || rson == nil || dad == nil) {
        LZFreeTrees();
        return (-1);
    }

    /* For i = 0 to N - 1, rson[i] and lson[i] will be the right and
       left children of node i.  These nodes need not be initialized.
       Also, dad[i] is the parent of node i.  These are initialized to
       NIL (= N), which stands for 'not used.'
       For i = 0 to 255, rson[N + i + 1] is the root of the tree
       for strings that begin with character i.  These are initialized
       to NIL.  Note there are 256 trees. */

    for (i = N + 1; i <= N + 256; i++)
        rson[i] = NIL;
    for (i = 0; i < N; i++)
        dad[i] = NIL;
    return (0);
}

static void
LZFreeTrees(void)    /* free data if necessary */
{
    if (text_buf != nil) {
        free(text_buf);
        text_buf = nil;
    }
    if (lson != nil) {
        free(lson);
        lson = nil;
    }
    if (rson != nil) {
        free(rson);
        rson = nil;
    }
    if (dad != nil) {
        free(dad);
        dad = nil;
    }
}


static void
LZInsertNode(int r)
{
    /* Inserts string of length F, text_buf[r..r+F-1], into one of the
       trees (text_buf[r]'th tree) and returns the longest-match position
       and length via the global variables match_position and match_length.
       If match_length = F, then removes the old node in favor of the new
       one, because the old one will be deleted sooner.
       Note r plays double role, as tree node and position in buffer. */
    register int  i, p, cmp;
    register unsigned char  *key;

    cmp = 1;  
    key = &text_buf[r];  
    p = N + 1 + key[0];
    rson[r] = lson[r] = NIL;  
    match_length = 0;
    for ( ; ; ) {
        if (cmp >= 0) {
            if (rson[p] != NIL) p = rson[p];
            else {  
                rson[p] = r;  
                dad[r] = p;  
                return;  
            }
        } else {
            if (lson[p] != NIL) p = lson[p];
            else {  
                lson[p] = r;  
                dad[r] = p;  
                return;  
            }
        }
        for (i = 1; i < F; i++)
            if ((cmp = key[i] - text_buf[p + i]) != 0)  break;
        if (i > match_length) {
            match_position = p;
            if ((match_length = i) >= F)  break;
        }
    }
    dad[r] = dad[p];  
    lson[r] = lson[p];  
    rson[r] = rson[p];
    dad[lson[p]] = r;  
    dad[rson[p]] = r;
    if (rson[dad[p]] == p) rson[dad[p]] = r;
    else lson[dad[p]] = r;
    dad[p] = NIL;  /* remove p */
}

static void
LZDeleteNode(int p)  /* deletes node p from tree */
{
    register int  q;

    if (dad[p] == NIL) return;  /* not in tree */
    if (rson[p] == NIL) q = lson[p];
    else if (lson[p] == NIL) q = rson[p];
    else {
        q = lson[p];
        if (rson[q] != NIL) {
            do {  
                q = rson[q];  
            } while (rson[q] != NIL);
            rson[dad[q]] = lson[q];  
            dad[lson[q]] = dad[q];
            lson[q] = lson[p];  
            dad[lson[p]] = q;
        }
        rson[q] = rson[p];  
        dad[rson[p]] = q;
    }
    dad[q] = dad[p];
    if (rson[dad[p]] == p) rson[dad[p]] = q;  
    else lson[dad[p]] = q;
    dad[p] = NIL;
}


/*
 * Compressed from in_buf to out_buf.
 *
 * Returns the compressed length.
 */
long
CompressLzss(const uchar *in_buf, uchar *out_buf, ulong uncr_length)
{
    register int i, len;
    int  c, r, s, last_match_length, code_buf_ptr;
    unsigned char  code_buf[17], mask;
    const unsigned char *in_start = in_buf;
    unsigned char *out_start = out_buf;

    codesize = 0;

    if (LZInitTree() != 0)  /* initialize trees */
        return (0);
    code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
         code_buf[0] works as eight flags, "1" representing that the unit
         is an unencoded letter (1 byte), "0" a position-and-length pair
             (2 bytes).  Thus, eight units require at most 16 bytes of code. */
    code_buf_ptr = mask = 1;
    s = 0;  
    r = N - F;
    for (i = s; i < r; i++) text_buf[i] = ' ';  /* Clear the buffer with
             any character that will appear often. */
    for (len = 0; len < F && (in_buf - in_start < uncr_length); len++) {
        c = *in_buf++;
        text_buf[r + len] = c;  /* Read F bytes into the last F bytes of
                  the buffer */
    }
    if ((textsize = len) == 0) {
        LZFreeTrees();
        return (0);  /* text of size zero */
    }
#if 0   /* NOTE: we don't have a circular buffer, so don't do this */
    for (i = 1; i <= F; i++) LZInsertNode(r - i);  / * Insert the F strings,
        each of which begins with one or more 'space' characters.  Note
        the order in which these strings are inserted.  This way,
        degenerate trees will be less likely to occur. * /
#endif
    LZInsertNode(r);  /* Finally, insert the whole string just read.  The
             global variables match_length and match_position are set. */
    do {
        if (match_length > len) match_length = len;  /* match_length
                      may be spuriously long near the end of text. */
        if (match_length <= THRESHOLD) {
            match_length = 1;  /* Not long enough match.  Send one byte. */
            code_buf[0] |= mask;  /* 'send one byte' flag */
            code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
        } else {
            /* store distance back rather than offset into circ buf
             * an offset of zero means one char back
             */
            int match_back = (r - match_position) -1;
            if (match_back < 0) match_back += N;

            /*printf("match_position = %d, r/s = %d/%d, match_back = %d\n",
             *    match_position, r, s, match_back);
             */

            code_buf[code_buf_ptr++] = (unsigned char) match_back
                                                    /*match_position*/;
            code_buf[code_buf_ptr++] = (unsigned char)
                (((match_back /*match_position*/ >> 4) & 0xf0)
                | (match_length - (THRESHOLD + 1)));  /* Send position and
                                length pair. Note match_length > THRESHOLD. */
        }
        if ((mask <<= 1) == 0) {  /* Shift mask left one bit. */
            for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
                *out_buf++ = code_buf[i];     /* code together */
            codesize += code_buf_ptr;
            code_buf[0] = 0;  
            code_buf_ptr = mask = 1;
        }
        last_match_length = match_length;
        for (i = 0; i < last_match_length &&
            (in_buf - in_start < uncr_length); i++) {
            c = *in_buf++;
            LZDeleteNode(s);    /* Delete old strings and */
            text_buf[s] = c;    /* read new bytes */
            if (s < F - 1)
                text_buf[s + N] = c;
                /* If the position is
                   near the end of buffer, extend the buffer to make
                   string comparison easier. */
            s = (s + 1) & (N - 1);  
            r = (r + 1) & (N - 1);
            /* Since this is a ring buffer, increment the position
                      modulo N. */
            LZInsertNode(r); /* Register the string in text_buf[r..r+F-1] */
        }
#if 0
        if ((textsize += i) > printcount) {
            printf("%12ld\r", textsize);  
            printcount += 1024;
            /* Reports progress each time the textsize exceeds
                    multiples of 1024. */
        }
#endif
        while (i++ < last_match_length) {  /* After the end of text, */
            LZDeleteNode(s);                 /* no need to read, but */
            s = (s + 1) & (N - 1);  
            r = (r + 1) & (N - 1);
            if (--len)
                LZInsertNode(r);          /* buffer may not be empty. */
        }
    } while (len > 0);  /* until length of string to be processed is zero */

    if (code_buf_ptr > 1) {       /* Send remaining code. */
        for (i = 0; i < code_buf_ptr; i++)
            *out_buf++ = code_buf[i];
        codesize += code_buf_ptr;
    }

    LZFreeTrees();
    return (out_buf - out_start);
}

#endif  /*FUCK_YOU_JOE*/


/*
 * Decodes from in_buf to out_buf.
 *
 * Returns the uncompressed size.
 */
long
ExpandLzss(const uchar *in_buf, uchar *out_buf, ulong length,
    ulong uncr_length)
{
    register int  i, j, k;
    unsigned int  lzflags;
    unsigned char *out_start = out_buf;

	Message(("decompressing from %x",in_buf));
	Message(("decompressing to %x",out_buf));

    lzflags = 0;
    while (out_buf - out_start < uncr_length) {
        if (((lzflags >>= 1) & 256) == 0) {
            lzflags = (*in_buf++) | 0xff00;      /* uses higher byte cleverly */
        }                                 		 /* to count eight */
        if (lzflags & 1) {
            /* single character */
            *out_buf++ = *in_buf++;
        } else {
            /* matched a full string */
            i = *in_buf++;
            j = *in_buf++;
            i |= ((j & 0xf0) << 4);  
            j = (j & 0x0f) + THRESHOLD;
            for (k = 0; k <= j; k++) {
                *out_buf = *(out_buf - (i+1));
                out_buf++;
            }
        }
    }

    if (out_buf - out_start != uncr_length) {
        Message(("ERROR, expected %ld, got %ld\n", uncr_length,
            (long)(out_buf - out_start)));
    }

    return (out_buf - out_start);
}
