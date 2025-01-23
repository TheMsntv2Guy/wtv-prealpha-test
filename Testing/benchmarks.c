/* stanford baby benchmark suite from john hennessy --

There are copies all around; the MIPS performance brief has up-to-date
numbers for a bunch of machines. BUT, these wouldn't be my first
choice. SPEC, the industry benchmark collection group, will probably
specify a new set of benchmarks shortly and release then right after
that. I imagine they will include espresso, gcc, and other large, but
public domain programs, for real things.

here's bench.c
	John Hennessy
*/


/*  This is a suite of benchmarks that are relatively short, both in program
    size and execution time.  It requires no input, and prints the execution
    time for each program, using the system- dependent routine Getclock,
    below, to find out the current CPU time.  It does a rudimentary check to
    make sure each program gets the right output.  These programs were
    gathered by John Hennessy and modified by Peter Nye. 

4.2 VERSION   */

#include "boxansi.h"
#include "BoxAbsoluteGlobals.h"
#include "MemoryManager.h"
#include "WTVTypes.h"
#include "BoxUtils.h"

    /* Towers */
#define maxcells 	 18

    /* Intmm, Mm */
#define rowsize 	 40

    /* Puzzle */
#define size	 	 511
#define classmax 	 3
#define typemax 	 12
#define d 		 8

    /* Bubble, Quick */
#define sortelements 	 5000
#define srtelements 	 500

    /* fft */
#define fftsize 	 256 
#define fftsize2 	 129  

    /* Perm */
#define    permrange 10

   /* tree */
struct node {
	struct node *left,*right;
	int val;
    };

    /* Towers */ /* discsizrange = 1..maxcells; */
#define    stackrange	3
/*    cellcursor = 0..maxcells; */
struct    element {
	    int discsize;
	    int next;
	};
/*    emsgtype = packed array[1..15] of char;
*/
    /* Intmm, Mm */ 
/*
    index = 1 .. rowsize;
    intmatrix = array [index,index] of integer;
    realmatrix = array [index,index] of real;
*/
    /* Puzzle */ 
/*
    piececlass = 0..classmax;
    piecetype = 0..typemax;
    position = 0..size;
*/
    /* Bubble, Quick */ 
/*
    listsize = 0..sortelements;
    sortarray = array [listsize] of integer;
*/


    /* global */
int    timer;
int    xtimes[11];
int    seed;

    /* Perm */
int    permarray[permrange+1];
int    pctr;

    /* tree */
struct node *tree;

    /* Towers */
int    stack[stackrange+1];
struct element    cellspace[maxcells+1];
int    freelist,
       movesdone;

    /* Intmm, Mm */
int ima[rowsize+1][rowsize+1], imb[rowsize+1][rowsize+1], imr[rowsize+1][rowsize+1];

    /* Puzzle */
int	piececount[classmax+1],
	gClass[typemax+1],
	piecemax[typemax+1],
	puzzl[size+1],
	p[typemax+1][size+1],
	n,
	kount;

    /* Bubble, Quick */
int    sortlist[sortelements+1],
    biggest, littlest,
    top;

/* global procedures */

int Getclock ()
    {
		return FetchCounter();
    }

void Initrand (void)
    {
    seed = 74755;
    }

int Rand (void)
    {
    seed = (seed * 1309 + 13849) & 65535;
    return( seed );
    }



    /* Permutation program, heavily recursive, written by Denny Brown. */

void   Swap (int *a, int *b)
	{
	int t;
	t = *a;  *a = *b;  *b = t;
	}

void    Initialize (void)
	{
	int i;
	for ( i = 1; i <= 7; i++ ) {
	    permarray[i]=i-1;
	    };
	}

void    Permute (int n)
	{   /* permute */
	int k;
	pctr = pctr + 1;
	if ( n!=1 )  {
	    Permute(n-1);
	    for ( k = n-1; k >= 1; k-- ) {
		Swap(&permarray[n],&permarray[k]);
		Permute(n-1);
		Swap(&permarray[n],&permarray[k]);
		};
	    };
	}     /* permute */

void Perm (void )    {   /* Perm */
    int i;
    pctr = 0;
    for ( i = 1; i <= 5; i++ ) {
	Initialize();
	Permute(7);
	};
    if ( pctr != 43300 )
	printf(" Error in Perm.\n");
    }     /* Perm */



    /*  Program to Solve the Towers of Hanoi */

void    Error (char *emsg)
	{
	printf(" Error in Towers: %s\n",emsg);
	}

void    Makenull (int s)
	{
	stack[s]=0;
	}

    int Getelement (void)
	{
	int temp;
	if ( freelist>0 )
	    {
	    temp = freelist;
	    freelist = cellspace[freelist].next;
	    }
	else
	    Error("out of space   ");
	return (temp);
	}

void    Push(int i,int s)
	{
        int errorfound, localel;
	errorfound=false;
	if ( stack[s] > 0 )
	    if ( cellspace[stack[s]].discsize<=i )
		{
		errorfound=true;
		Error("disc size error");
		}
	if ( ! errorfound )
	    {
	    localel=Getelement();
	    cellspace[localel].next=stack[s];
	    stack[s]=localel;
	    cellspace[localel].discsize=i;
	    }
	}

void    Init (int s,int n)
	{
	int discctr;
	Makenull(s);
	for ( discctr = n; discctr >= 1; discctr-- )
	    Push(discctr,s);
	}

    int Pop (int s)
	{
	 int temp, temp1;
	if ( stack[s] > 0 )
	    {
	    temp1 = cellspace[stack[s]].discsize;
	    temp = cellspace[stack[s]].next;
	    cellspace[stack[s]].next=freelist;
	    freelist=stack[s];
	    stack[s]=temp;
	    return (temp1);
	    }
	else
	    Error("nothing to pop ");
	}

void    MoveIt (int s1,int s2)
	{
	Push(Pop(s1),s2);
	movesdone=movesdone+1;
	}

void    tower(int i,int j,int k)
	{
	int other;
	if ( k==1 )
	    MoveIt(i,j);
	else
	    {
	    other=6-i-j;
	    tower(i,other,k-1);
	    MoveIt(i,j);
	    tower(other,j,k-1);
	    }
	}


void Towers (void)    { /* Towers */
    int i;
    for ( i=1; i <= maxcells; i++ )
	cellspace[i].next=i-1;
    freelist=maxcells;
    Init(1,14);
    Makenull(2);
    Makenull(3);
    movesdone=0;
    tower(1,2,14);
    if ( movesdone != 16383 )
	printf (" Error in Towers.\n");
    } /* Towers */


    /* The eight queens problem, solved 50 times. */
/*
	type    
	    doubleboard =   2..16;
	    doublenorm  =   -7..7;
	    boardrange  =   1..8;
	    aarray      =   array [boardrange] of boolean;
	    barray      =   array [doubleboard] of boolean;
	    carray      =   array [doublenorm] of boolean;
	    xarray      =   array [boardrange] of boardrange;
*/

void	Try(int i,int  *q,int  a[],int  b[],int  c[],int  x[])
	    {
	    int     j;
	    j = 0;
	    *q = false;
	    while ( (! *q) && (j != 8) )
		{ j = j + 1;
		*q = false;
		if ( b[j] && a[i+j] && c[i-j+7] )
		    { x[i] = j;
		    b[j] = false;
		    a[i+j] = false;
		    c[i-j+7] = false;
		    if ( i < 8 )
			{ Try(i+1,q,a,b,c,x);
			if ( ! *q )
			    { b[j] = true;
			    a[i+j] = true;
			    c[i-j+7] = true;
			    }
			}
		    else *q = true;
		    }
		}
	    }
	
void    Doit (void)
	{
	int i,q;
	int a[9], b[17], c[15], x[9];
	i = 0 - 7;
	while ( i <= 16 )
	    { if ( (i >= 1) && (i <= 8) ) a[i] = true;
	    if ( i >= 2 ) b[i] = true;
	    if ( i <= 7 ) c[i+7] = true;
	    i = i + 1;
	    }

	Try(1, &q, b, a, c, x);
	if ( ! q )
	    printf (" Error in Queens.\n");
	}

void Queens (void)
    {
    int i;
    for ( i = 1; i <= 50; i++ ) Doit();
    }

    /* Multiplies two integer matrices. */

void    Initmatrix ( int m[rowsize+1][rowsize+1] )
	{
	int temp, i, j;
	for ( i = 1; i <= rowsize; i++ )
	    for ( j = 1; j <= rowsize; j++ ) {
		temp = Rand();
		m[i][j] = temp - (temp/120)*120 - 60;
	    }
	}

void    Innerproduct( int *result,int a[rowsize+1][rowsize+1],int b[rowsize+1][rowsize+1],int row,int column)
	/* computes the inner product of A[row,*] and B[*,column] */
	{
	int i;
	*result = 0;
	for(i = 1; i <= rowsize; i++ )*result = *result+a[row][i]*b[i][column];
	}

void Intmm (void)
    {
    int i, j;
    Initrand();
    Initmatrix (ima);
    Initmatrix (imb);
    for ( i = 1; i <= rowsize; i++ )
	for ( j = 1; j <= rowsize; j++ ) Innerproduct(&imr[i][j],ima,imb,i,j);
    }




    /* A compute-bound program from Forest Baskett. */

int Fit (int i,int j)
{
int k;
for ( k = 0; k <= piecemax[i]; k++ )
    if ( p[i][k] ) if ( puzzl[j+k] ) return (false);
return (true);
}

int Place (int i, int j)
{
int k;
for ( k = 0; k <= piecemax[i]; k++ )
    if ( p[i][k] ) puzzl[j+k] = true;
piececount[gClass[i]] = piececount[gClass[i]] - 1;
for ( k = j; k <= size; k++ )
    if ( ! puzzl[k] ) {
	return (k);
	}
return (0);
}

void RemoveIt (int i,int  j)
{
int k;
for ( k = 0; k <= piecemax[i]; k++ )
    if ( p[i][k] ) puzzl[j+k] = false;
piececount[gClass[i]] = piececount[gClass[i]] + 1;
}

int Trial (int j)
{
int i, k;
kount = kount + 1;
for ( i = 0; i <= typemax; i++ )
    if ( piececount[gClass[i]] != 0 )
	if ( Fit (i, j) ) {
	    k = Place (i, j);
	    if ( Trial(k) || (k == 0) ) {
		return (true);
		}
	    else RemoveIt (i, j);
	    }
return (false);
}

void Puzzle (void)
    {
    int i, j, k, m;
    for ( m = 0; m <= size; m++ ) puzzl[m] = true;
    for( i = 1; i <= 5; i++ )for( j = 1; j <= 5; j++ )for( k = 1; k <= 5; k++ )
	puzzl[i+d*(j+d*k)] = false;
    for( i = 0; i <= typemax; i++ )for( m = 0; m<= size; m++ ) p[i][m] = false;
    for( i = 0; i <= 3; i++ )for( j = 0; j <= 1; j++ )for( k = 0; k <= 0; k++ )
	p[0][i+d*(j+d*k)] = true;
    gClass[0] = 0;
    piecemax[0] = 3+d*1+d*d*0;
    for( i = 0; i <= 1; i++ )for( j = 0; j <= 0; j++ )for( k = 0; k <= 3; k++ )
	p[1][i+d*(j+d*k)] = true;
    gClass[1] = 0;
    piecemax[1] = 1+d*0+d*d*3;
    for( i = 0; i <= 0; i++ )for( j = 0; j <= 3; j++ )for( k = 0; k <= 1; k++ )
	p[2][i+d*(j+d*k)] = true;
    gClass[2] = 0;
    piecemax[2] = 0+d*3+d*d*1;
    for( i = 0; i <= 1; i++ )for( j = 0; j <= 3; j++ )for( k = 0; k <= 0; k++ )
	p[3][i+d*(j+d*k)] = true;
    gClass[3] = 0;
    piecemax[3] = 1+d*3+d*d*0;
    for( i = 0; i <= 3; i++ )for( j = 0; j <= 0; j++ )for( k = 0; k <= 1; k++ )
	p[4][i+d*(j+d*k)] = true;
    gClass[4] = 0;
    piecemax[4] = 3+d*0+d*d*1;
    for( i = 0; i <= 0; i++ )for( j = 0; j <= 1; j++ )for( k = 0; k <= 3; k++ )
	p[5][i+d*(j+d*k)] = true;
    gClass[5] = 0;
    piecemax[5] = 0+d*1+d*d*3;
    for( i = 0; i <= 2; i++ )for( j = 0; j <= 0; j++ )for( k = 0; k <= 0; k++ )
	p[6][i+d*(j+d*k)] = true;
    gClass[6] = 1;
    piecemax[6] = 2+d*0+d*d*0;
    for( i = 0; i <= 0; i++ )for( j = 0; j <= 2; j++ )for( k = 0; k <= 0; k++ )
	p[7][i+d*(j+d*k)] = true;
    gClass[7] = 1;
    piecemax[7] = 0+d*2+d*d*0;
    for( i = 0; i <= 0; i++ )for( j = 0; j <= 0; j++ )for( k = 0; k <= 2; k++ )
	p[8][i+d*(j+d*k)] = true;
    gClass[8] = 1;
    piecemax[8] = 0+d*0+d*d*2;
    for( i = 0; i <= 1; i++ )for( j = 0; j <= 1; j++ )for( k = 0; k <= 0; k++ )
	p[9][i+d*(j+d*k)] = true;
    gClass[9] = 2;
    piecemax[9] = 1+d*1+d*d*0;
    for( i = 0; i <= 1; i++ )for( j = 0; j <= 0; j++ )for( k = 0; k <= 1; k++ )
	p[10][i+d*(j+d*k)] = true;
    gClass[10] = 2;
    piecemax[10] = 1+d*0+d*d*1;
    for( i = 0; i <= 0; i++ )for( j = 0; j <= 1; j++ )for( k = 0; k <= 1; k++ )
	p[11][i+d*(j+d*k)] = true;
    gClass[11] = 2;
    piecemax[11] = 0+d*1+d*d*1;
    for( i = 0; i <= 1; i++ )for( j = 0; j <= 1; j++ )for( k = 0; k <= 1; k++ )
	p[12][i+d*(j+d*k)] = true;
    gClass[12] = 3;
    piecemax[12] = 1+d*1+d*d*1;
    piececount[0] = 13;
    piececount[1] = 3;
    piececount[2] = 1;
    piececount[3] = 1;
    m = 1+d*(1+d*1);
    kount = 0;
    if ( Fit(0, m) ) n = Place(0, m);
    else printf("Error1 in Puzzle\n");
    if ( ! Trial(n) ) printf ("Error2 in Puzzle.\n");
    else if ( kount != 2005 ) printf ( "Error3 in Puzzle.\n");
    }



    /* Sorts an array using quicksort */

void    Initarr(void)
	{
	int i, temp;
	Initrand();
	biggest = 0; littlest = 0;
	for ( i = 1; i <= sortelements; i++ )
	    {
	    temp = Rand();
	    sortlist[i] = temp - (temp/100000)*100000 - 50000;
	    if ( sortlist[i] > biggest ) biggest = sortlist[i];
	    else if ( sortlist[i] < littlest ) littlest = sortlist[i];
	    }
	}

void    Quicksort(int a[],int l,int r)
	/* quicksort the array A from start to finish */
	{
	int i,j,x,w;

	i=l; j=r;
	x=a[(l+r) / 2];
	do {
	    while ( a[i]<x ) i = i+1;
	    while ( x<a[j] ) j = j-1;
	    if ( i<=j ) {
		w = a[i];
		a[i] = a[j];
		a[j] = w;
		i = i+1;    j= j-1;
		}
	} while ( i<=j );
	if ( l <j ) Quicksort(a,l,j);
	if ( i<r ) Quicksort(a,i,r);
	}


void Quick (void)
    {
    Initarr();
    Quicksort(sortlist,1,sortelements);
    if ( (sortlist[1] != littlest) || (sortlist[sortelements] != biggest) )
	printf ( " Error in Quick.\n");
    }


    /* Sorts an array using treesort */

void    tInitarr(void)
	{
	int i, temp;
	Initrand();
	biggest = 0; littlest = 0;
	for ( i = 1; i <= sortelements; i++ )
	    {
	    temp = Rand();
	    sortlist[i] = temp - (temp/100000)*100000 - 50000;
	    if ( sortlist[i] > biggest ) biggest = sortlist[i];
	    else if ( sortlist[i] < littlest ) littlest = sortlist[i];
	    }
	}

void	CreateNode (struct node **t, int n)
	{
		*t = (struct node *)AllocateMemory(sizeof(struct node), false); 
		(*t)->left = nil; (*t)->right = nil;
		(*t)->val = n;
	}

void    Insert( int n, struct node *t)
	/* insert n into tree */
    {
	   if ( n > t->val ) 
		if ( t->left == nil ) CreateNode(&t->left,n);
		else Insert(n,t->left);
    	   else if ( n < t->val )
		if ( t->right == nil ) CreateNode(&t->right,n);
		else Insert(n,t->right);
    }

int Checktree(struct node *p) 
    /* check by inorder traversal */
    {
    int result;
        result = true;
		if ( p->left != nil ) 
		   if ( p->left->val <= p->val ) result=false;
		   else result = Checktree(p->left) && result;
		if ( p->right != nil )
		   if ( p->right->val >= p->val ) result = false;
		   else result = Checktree(p->right) && result;
	return( result);
    } /* checktree */

void Trees(void)
    {
    int i;
    tInitarr();
    tree = (struct node *)AllocateMemory(sizeof(struct node), false); 
    tree->left = nil; tree->right=nil; tree->val=sortlist[1];
    for ( i = 2; i <= sortelements; i++ ) Insert(sortlist[i],tree);
    if ( ! Checktree(tree) ) printf ( " Error in Tree.\n");
    }


    /* Sorts an array using bubblesort */

void    bInitarr(void)
	{
	int i, temp;
	Initrand();
	biggest = 0; littlest = 0;
	for ( i = 1; i <= srtelements; i++ )
	    {
	    temp = Rand();
	    sortlist[i] = temp - (temp/100000)*100000 - 50000;
	    if ( sortlist[i] > biggest ) biggest = sortlist[i];
	    else if ( sortlist[i] < littlest ) littlest = sortlist[i];
	    }
	}

void Bubble(void)
    {
    int i, j;
    bInitarr();
    top=srtelements;

    while ( top>1 ) {

	i=1;
	while ( i<top ) {

	    if ( sortlist[i] > sortlist[i+1] ) {
		j = sortlist[i];
		sortlist[i] = sortlist[i+1];
		sortlist[i+1] = j;
		}
	    i=i+1;
	    }

	top=top-1;
	}
    if ( (sortlist[1] != littlest) || (sortlist[srtelements] != biggest) )
	printf ( "Error3 in Bubble.\n");
    }


int Min0(int  arg1,int  arg2)
    {
    if ( arg1 < arg2 )
	return (arg1);
    else
	return (arg2);
    }

#define	CONVERT_TO_USEC(x) ((x)*2/(READ_AG(agCPUSpeed)/1000000))

uchar* buf1;
uchar* buf2;

#define BUF_SIZE 32

void DoMemTests(void)
{
	ulong stop;
	int i;
	
	buf1 = (uchar*)AllocateMemory(BUF_SIZE*1024, false);
	if(buf1 == nil)
	{
		printf("MemTest : couldn't allocate %d bytes for buffer 1\n",BUF_SIZE*1024);
		return;
	}
	buf2 = (uchar*)AllocateMemory(BUF_SIZE*1024, false);
	if(buf2 == nil)
	{
		printf("MemTest : couldn't allocate %d bytes for buffer 1\n",BUF_SIZE*1024);
		return;
	}
	printf("memcpy test from RAM to RAM...\n");
	for(i=4;i <= BUF_SIZE; i+=4)
	{
		timer = Getclock(); 
		memcpy(buf1,buf2,i*1024);  
		stop = Getclock()-timer; 
		printf("%2dk memcpy : %d\n", i,CONVERT_TO_USEC(stop));
	}
	printf("memcpy test from ROM to RAM...\n");
	for(i=4;i <= BUF_SIZE; i+=4)
	{
		timer = Getclock(); 
		memcpy(buf1,(void*)0x9fc00000,i*1024);  
		stop = Getclock()-timer; 
		printf("%2dk memcpy : %d\n", i,CONVERT_TO_USEC(stop));
	}
	FreeMemory(buf1);
	FreeMemory(buf2);
}

void Stanford(void)
{
	printf("Doing Stanford benchmark suite...\n");

	printf("    Perm : "); timer = Getclock(); Perm();   xtimes[0] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[0]));
	
	printf("  Towers : "); timer = Getclock(); Towers(); xtimes[1] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[1]));
	
	printf("  Queens : "); timer = Getclock(); Queens(); xtimes[2] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[2]));
	
	printf("   Intmm : "); timer = Getclock(); Intmm();  xtimes[3] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[3]));
	
	printf("  Puzzle : "); timer = Getclock(); Puzzle(); xtimes[4] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[4]));
	
	printf("   Quick : "); timer = Getclock(); Quick();  xtimes[5] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[5]));
	
	printf("  Bubble : "); timer = Getclock(); Bubble(); xtimes[6] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[6]));
#if 0
	/* this bastard leaks memory */
	printf("    Tree : "); timer = Getclock(); Trees();  xtimes[7] = Getclock()-timer; 
	printf("%d\n", CONVERT_TO_USEC(xtimes[7]));
#endif	

}

void benchmarks(void)
{
	
	printf("All times in microseconds\n");

	Stanford();
	DoMemTests();
	
	printf("Done\n");
}
