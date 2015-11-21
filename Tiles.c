#include "jasmine.h" 


int hash_UNH(int ints[], int num_ints, long m, int increment)
{
    static UINT32 rndseq[2048];
    static UINT8 first_call =  1;
    UINT16 i,k;
    int index;
    int sum = 0;

    // if first call to hashing, initialize table of random numbers 
    if (first_call)
    {
        for (k = 0; k < 2048; k++)
        {
            rndseq[k] = 0;
            for (i=0; i < (int)sizeof(int); ++i)
                rndseq[k] = (rndseq[k] << 8) | (rand() & 0xff);
	    //uart_printf ( "rndseq[%d]: %d", k, rndseq[k] ) ;
        }
        first_call = 0;
    }

    //uart_printf ( "\nNum_ints: %d", num_ints ) ;
    //for (i = 0; i < num_ints; i++) {
    //uart_printf ( "\nints[%d]: %d", i, ints[i] ) ;
    //}


    for (i = 0; i < num_ints; i++)
    {
      // add random table offset for this dimension and wrap around 
      index = ints[i];
      //uart_printf ( "\n1.0 Index: %d, i: %d", index, i ) ; 
      index += (increment * i);
      //uart_printf ( "\n2.0 Index: %d i: %d", index, i ) ; 
      index %= 2048;
      //uart_printf ( "\n3.0 Index: %d i: %d", index, i ) ; 
      while (index < 0)
	index += 2048;
      //uart_printf ( "\n4.0 Index: %d i: %d", index, i ) ; 

      // add selected random number to sum 
      sum += (long)rndseq[(int)index];
      //uart_printf ( "\n5.0 Sum: %d i: %d", sum, i ) ; 
    }
    //uart_printf ( "\n6.0 Index: %d, sum: %d\tm: %d", index, sum, m ) ; 
    index = (int)(sum % m);
    //uart_printf ( "\n7.0 Index: %d, sum: %d\tm: %d", index, sum, m ) ; 
    while (index < 0)
        index += m;

    //uart_printf ( "\n8.0 Index: %d, sum: %d\tm: %d", index, sum, m ) ; 

    //uart_printf ( "\nINDEX: %d", index ) ;
    return(index);
}


#define MAX_NUM_VARS 128        // Maximum number of variables in a grid-tiling
int mod(int n, int k) {return (n >= 0) ? n%k : k-1-((-n-1)%k);}

void GetTiles(
    UINT16 tiles[],               // provided array contains returned tiles (tile indices)
    UINT8 num_tilings,           // number of tile indices to be returned in tiles
    UINT16 memory_size,           // total number of possible tiles
    float floats[],            // array of floating point variables
    UINT8 num_floats,            // number of floating point variables
    UINT16 ints[],       // array of integer variables
    UINT8 num_ints)              // number of integer variables
{
    UINT8 i,j;
    int qstate[MAX_NUM_VARS];
    int base[MAX_NUM_VARS];
    int coordinates[MAX_NUM_VARS * 2 + 1];   /* one interval number per relevant dimension */
    int num_coordinates = num_floats + num_ints + 1;

    for ( i=0; i<num_ints; i++)
        coordinates[num_floats+1+i] = ints[i];

    /* quantize state to integers (henceforth, tile widths == num_tilings) */
    for (i = 0; i < num_floats; i++)
    {
        qstate[i] = (int) floor(floats[i] * num_tilings);
        base[i] = 0;
    }

    /*compute the tile numbers */
    for (j = 0; j < num_tilings; j++)
    {
        /* loop over each relevant dimension */
        for (i = 0; i < num_floats; i++)
        {
            /* find coordinates of activated tile in tiling space */
            coordinates[i] = qstate[i] - mod(qstate[i]-base[i],num_tilings);

            /* compute displacement of next tiling in quantized space */
            base[i] += 1 + (2 * i);
        }
        /* add additional indices for tiling and hashing_set so they hash differently */
        coordinates[i] = j;

        tiles[j] = hash_UNH(coordinates, num_coordinates, memory_size, 449);
    }
    return;
}



