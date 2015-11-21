/*
 * State.h
 *
 *  Created on: Nov 20, 2015
 *      Author: Ryan
 */

#ifndef STATE_H_
#define STATE_H_

void initializeState( State s);
void initializeTuple( State _currentState, int _currentAction, double _reward, State _nextState, int _nextAction);

#endif /* STATE_H_ */
