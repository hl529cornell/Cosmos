#include "ftl.h"
#include "pagemap.h"
#include <stdint.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>
#define debug_cmac 1
//CMAC constructor
void initializeCMAC( unsigned char _numFloatInputs, unsigned char _numIntInputs, uint16_t _memorySize,
	       float _alpha, unsigned char _numTilings, unsigned char _tableDimensionality) {

  unsigned char i, j ;

  qLearning.cmac.alpha = _alpha ;
  qLearning.cmac.numFloatInputs = _numFloatInputs ;
  qLearning.cmac.numIntInputs = _numIntInputs ;
  qLearning.cmac.memorySize = _memorySize ;
  qLearning.cmac.numTilings = _numTilings ;
  qLearning.cmac.tableDimensionality = _tableDimensionality ;
    
  //Initialize input, tile, and CMAC arrays
  for(i=0; i< _numFloatInputs; i++){
    qLearning.cmac.floatInputs[i] = 0;
  }
  for(i=0; i< _numIntInputs; i++){
    qLearning.cmac.intInputs[i] = 0;
  }
  
  for(i=0; i< _numTilings; i++){
    qLearning.cmac.tiles[i] = 0;
  }

  for ( j = 0 ; j < 128 ; j ++ ) {
    for ( i = 0 ; i < 32  ; i ++ ) {
      float *ptr = (float *) CMAC_ADDR + (((j*32)+i) * sizeof(float));
      *ptr = 1.0526;
    }
  }


  /*
   for ( i = 0 ; i < 10 ; i ++ ) {
    qLearning.cmac.u[i] = 1.0526 ;
  }

  
  uart_printf ( "\nMem Size: %d", _memorySize ) ;

  for ( i = 0 ; i < 10 ; i ++ ) {
    uart_printf ( "\nCMAC[%d]: %d", i, qLearning.cmac.u[i] ) ; 
  }
*/


  /*
  for ( j = 0 ; j < 128 ; j ++ ) {
    for ( i = 0 ; i < 32  ; i ++ ) {
      float val = read_dram_32_f( CMAC_ADDR + (((j*32)+i) * sizeof(float)) ) ; 
      uart_printf ( "CMAC[%d]: %f", j*32+i, val ) ; 
    }
  }
  predict() ;
  update(1.235) ;  
  */

}

/*
//Set float input array at given index to given value
void setFloatInput(int index, float value ) { 
  qLearning.cmac.floatInputs[index] = value;
}

//Set int input array at given index to given value
void setIntInput(int index, int value ) { 
  qLearning.cmac.intInputs[index] = value;
}
*/

//Predict target for current input vector
float predict()
{
  unsigned char i ;

  //Populate tiles array with tile indices
  GetTiles(qLearning.cmac.tiles, qLearning.cmac.numTilings, qLearning.cmac.memorySize, qLearning.cmac.floatInputs, 
	   qLearning.cmac.numFloatInputs, qLearning.cmac.intInputs, qLearning.cmac.numIntInputs);
 
  
  //Calculate the sum of all indexed tiles
  float sum = 0;
  for(i=0; i< qLearning.cmac.numTilings; i++){
    uint16_t index = qLearning.cmac.tiles[i] ;
    float *ptr = (float *) CMAC_ADDR + (index * sizeof(float));
    float value = *ptr;
    sum += value ; 
    //uint16_t_t index = qLearning.cmac.tiles[i] ;
    //sum += qLearning.cmac.u[qLearning.cmac.tiles[i]];

    //if ( debug_cmac) 
    //uart_printf ( "predict Iter: %d\tindex: %d\tValue: %f", i, index,value) ;  
    //uart_printf ( "predict Iter: %d\tindex: %d\tValue: %f", i, index, qLearning.cmac.u[qLearning.cmac.tiles[i]]  ) ;    
  }

  //uart_printf ( "Q Value Sum: %f" , sum ) ;
  //Return the sum
  return sum;

}



//Train CMAC on current input towards given target
void update(float target)
{
  unsigned char i ;
  //Populate tiles array with tile indices
  GetTiles(qLearning.cmac.tiles, qLearning.cmac.numTilings, qLearning.cmac.memorySize, qLearning.cmac.floatInputs, 
	   qLearning.cmac.numFloatInputs, qLearning.cmac.intInputs, qLearning.cmac.numIntInputs);
  
  //Calculate the sum of all indexed tiles
  float pred = 0;

  
  for(i=0; i< qLearning.cmac.numTilings; i++){
    uint16_t index = qLearning.cmac.tiles[i] ;
    float *ptr = (float *) CMAC_ADDR + (index * sizeof(float));
    float value = *ptr;
    pred += value ; 
    //uint16_t_t index = qLearning.cmac.tiles[i] ;
    //pred += qLearning.cmac.u[qLearning.cmac.tiles[i]];
    
    //if ( debug_cmac) 
    //uart_printf ( "Before Update Iter: %d\tindex: %d\tValue: %f", i, index, value ) ;
    //uart_printf ( "Before Update Iter: %d\tindex: %d\tValue: %f", i, index, qLearning.cmac.u[qLearning.cmac.tiles[i]] ) ;
  } 
  
  //Train CMAC memory cells
  for( i=0; i<qLearning.cmac.numTilings; i++){
    uint16_t index = qLearning.cmac.tiles[i] ;
    float *ptr = (float *) CMAC_ADDR + (index * sizeof(float));
    float value = *ptr;
    value += ((qLearning.cmac.alpha / qLearning.cmac.numTilings) * (target - pred));
    *ptr = value;
    //qLearning.cmac.u[qLearning.cmac.tiles[i]] += ((qLearning.cmac.alpha / qLearning.cmac.numTilings) * (target - pred));
  }

  /*
  for(i=0; i< qLearning.cmac.numTilings; i++){
    uint16_t_t index = qLearning.cmac.tiles[i] ;
    float value = read_dram_32_f( CMAC_ADDR + (index * sizeof(float)) ) ;
    pred += value ; 
    //uint16_t_t index = qLearning.cmac.tiles[i] ;
    //pred += qLearning.cmac.u[qLearning.cmac.tiles[i]];
    //if ( debug_cmac) 
    //uart_printf ( "After Update Iter: %d\tindex: %d\tValue: %f", i, index, value ) ;    
    //uart_printf ( "After Update Iter: %d\tindex: %d\tValue: %f", i, index, qLearning.cmac.u[qLearning.cmac.tiles[i]] ) ;    
  }
  */
}


