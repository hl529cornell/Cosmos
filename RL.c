#include "ftl.h"

// QLearner Functions
void initializeQLearner(UINT8 _numFeatures, UINT8 _numActions, float _delta, float _alpha, UINT16 _memorySize, UINT8 _numTilings, UINT8 _tableDimensionality) {
  qLearning.bellmanError = 0 ;
  qLearning.numFeatures  = _numFeatures ;
  qLearning.numActions   = _numActions ;
  qLearning.delta        = _delta ;
  qLearning.alpha        = _alpha ;
  
  initializeCMAC ( _numFeatures, 2, _memorySize, _alpha, _numTilings, _tableDimensionality);
}

//Initialize CMAC inputs based on given state-action information
void populateCMAC(State s, int a)
{  
  UINT8 i ;
  //Set state information
  for( i=0; i<numFeatures; i++){
    qLearning.cmac.floatInputs[i] =  s.stats[i] ;  
    //setFloatInput(i, 1.0 * s.stats[i] ) ; 
  }
  
  //Set action information
  qLearning.cmac.intInputs[0] = a; 
  //setIntInput(0, a);
  
}

//Predict Q(s,a)
void predictQFactor(State state, int action)
{
  
  //  for ( uint i = 0 ; i < numFeatures ; i ++ ) {
  //printf ( "\nFeature: %d\tStat Value: %f", i, state->getStat(i) ) ;
  //}

  //Populate CMAC inputs
  populateCMAC(state, action);
  
  //Obtain CMAC's prediction
  qValPred = predict(); 

  //Return CMAC's prediction
  //return q;
  
}



//Perform a single q-factor update (sarsa)
void updateSarsa(Tuple tuple)
{
  
  //Get current state
  State s = tuple.currentState ; 

  //Get action
  int a = tuple.currentAction ; 

  //Get reward
  float reward = tuple.reward ;

  //Get next state
  State s_ = tuple.nextState ;
  
  //Get next action
  int a_ = tuple.nextAction ;

  //Get Q(s',a')
  populateCMAC(s_, a_);
  float nextQ = predict();
 
  //SARSA update
  populateCMAC(s,a);
  float oldQFactor = predict();
  update(reward + delta * nextQ);

}




