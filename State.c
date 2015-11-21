#include "jasmine.h" 

//Constructor
void initializeState( State s) {
  //Initialize stats array
  for(UINT8 i=0; i<NUM_FEATURE_TYPES; i++){
    s.stats[i] = 0;
  }
}



/*
//TUPLE functions
//Set current state`
void setCurrentState(State _currentState) { currentState = _currentState; }
//Set current action
void setCurrentAction(int _currentAction) { currentAction = _currentAction; }
//Set reward
void setCurrentReward(double _reward) { currentReward = _reward; }
//Set next state
void setNextState(State _nextState) { nextState = _nextState; }
//Set next action
void setNextAction(int _nextAction) { nextAction = _nextAction; }

//Get current state
State getCurrentState() { return currentState; }
//Get current action
int getCurrentAction() { return currentAction; }
//Get reward
double getCurrentReward() { return currentReward; }
//Get next state
State getNextState() { return nextState; }
//Get next action
int getNextAction() { return nextAction; }
*/

//Constructor
void initializeTuple( State _currentState, int _currentAction, double _reward, State _nextState, int _nextAction) {
  t.currentState = _currentState ;
  t.currentAction = _currentAction ;
  t.reward = _reward ;
  t.nextState = _nextState ;
  t.nextAction = _nextAction ;
}
