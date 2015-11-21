#include "jasmine.h"
#define debug_init_rl 1

void initializeRL() {
  UINT8 i ;

  MEM_SIZE = 4096 ;
  uart_printf ( "\nInitialize RL" ) ;
  numFeatures = 4 ;
  if ( debug_init_rl ) 
    uart_printf ( "NumFeatures: %d", numFeatures ) ;    
  for (  i = 0 ; i < numFeatures ; i ++ ) {
    features[i] = i ;
    if ( debug_init_rl ) 
      uart_printf ( "Feature[%d]: %d", i, features[i] ) ;      
  }

  numActions = 2 ;
  // 50%
  rewardGC   = -3.26 ;
  rewardNOP  = -1.76 ;

  // 25%
  //rewardGC   = -4.17 ;
  //rewardNOP  = -1.93 ;


  //  rewardGC   = 3.68 ;
  //rewardNOP  = -2.13 ;

  //rewardGC   = 3.85 ;
  //rewardNOP  = 3.68 ;

  //rewardGC   = -1.39 ;
  //rewardNOP  = 0.22 ;


  if ( debug_init_rl ) {
    uart_printf ( "Num Actions: %d", numActions ) ; 
    uart_printf ( "rewardGC : %f", rewardGC ) ; 
    uart_printf ( "rewardNOP: %f", rewardNOP ) ;   
  }

  delta = 0.95 ; // discount factor gamma
  alpha = 0.20 ; // learning rate

  memorySize = MEM_SIZE ;
  memorySize = 4096 ;
  numTilings = NUM_TILINGS ;
  tableDim   = 2 ;
  epsilon    = 0.05 ;

  UINT16 msize = 4096 ;
  memorySize = msize ;

  if ( debug_init_rl ) {
    uart_printf ( "Delta: %f", delta ) ; 
    uart_printf ( "Alpha: %f", alpha ) ; 
    //uart_printf ( "Memory Size: %d", memorySize ) ; 
    uart_printf ( "Num Tilings: %d", numTilings ) ; 
    uart_printf ( "Table Dim: %d", tableDim ) ; 
    uart_printf ( "Epsilon: %f", epsilon ) ; 
    uart_printf ( "CMAC Size: %d", msize  ) ;
    uart_printf ( "CMAC Size: %d", memorySize  ) ;
  }  

  initializeQLearner(numFeatures, numActions, delta, alpha, memorySize, numTilings, tableDim);

  if ( debug_init_rl ) {
    uart_printf ( "Q Learner Delta: %f", qLearning.delta ) ; 
    uart_printf ( "Q Learner Alpha: %f", qLearning.alpha ) ; 
    uart_printf ( "Q Learner Num Features: %d", qLearning.numFeatures ) ; 
    uart_printf ( "Q Learner Num Actionss: %d", qLearning.numActions ) ; 
  }

  initializeState ( currentState ) ;
  initializeState ( nextState ) ;

  for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
    currentState.stats[i] = i + 1.0 ;
    nextState.stats[i] = i + 2.0 ;
  }
  
  currentAction = ISSUE_NOP ;
  nextAction    = ISSUE_NOP ;
  
  currentReward = rewardNOP ;
  nextReward    = rewardNOP ;

  initializeTuple ( currentState, currentAction, currentReward, nextState, nextAction ) ;

  if ( debug_init_rl ) {
    uart_printf ( "\nTuple Initialization: " ) ;
    for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
      uart_printf ( "t.currentState.stats[%d]: %f", i, t.currentState.stats[i] ) ;
    }
    for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
      uart_printf ( "t.nextState.stats[%d]: %f", i, t.nextState.stats[i] ) ;
    }

    uart_printf ( "t.reward: %f", t.reward ) ;
    uart_printf ( "t.currentAction: %d", t.currentAction ) ; 
    uart_printf ( "t.nextAction: %d", t.nextAction ) ; 

    State cs, ns ;
    for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
      cs.stats[i] = i + 5.0 ;
      ns.stats[i] = i + 6.0 ;
    }
    currentState = cs ;
    //setCurrentState ( cs ) ;
    currentReward = rewardNOP ;
    //setCurrentReward ( 1.567) ;
    currentAction = ISSUE_GC ;
    //setCurrentAction ( ISSUE_GC ) ;
    nextState = ns ;
    //setNextState ( ns ) ;
    nextAction = ISSUE_GC ;
    //setNextAction ( ISSUE_GC ) ;


    initializeTuple ( currentState, currentAction, currentReward, nextState, nextAction ) ;
    uart_printf ( "\nTuple Initialization: " ) ;
    for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
      uart_printf ( "t.currentState.stats[%d]: %f", i, t.currentState.stats[i] ) ;
    }
    for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
      uart_printf ( "t.nextState.stats[%d]: %f", i, t.nextState.stats[i] ) ;
    }

    uart_printf ( "t.reward: %f", t.reward ) ;
    uart_printf ( "t.currentAction: %d", t.currentAction ) ; 
    uart_printf ( "t.nextAction: %d", t.nextAction ) ; 
  }

  for ( i = 0 ; i < NUM_FEATURES ; i ++ ) {
    currentState.stats[i] = 0 ; 
    nextState.stats[i] = 0 ; 
  }
  currentReward = rewardNOP ;
  nextReward    = rewardNOP ;
  currentAction = ISSUE_NOP ;
  nextAction    = ISSUE_NOP ;
  
  for ( i = 0 ; i < numFeatures ; i ++ ) {
    qLearning.features[i] = i ;
  }

  predict() ;
  predict() ;

}

