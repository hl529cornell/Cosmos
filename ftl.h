//////////////////////////////////////////////////////////////////////////////////
// ftl.h for Cosmos OpenSSD
// Copyright (c) 2014 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//                Gyeongyong Lee <gylee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// This file is part of Cosmos OpenSSD.
//
// Cosmos OpenSSD is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// Cosmos OpenSSD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cosmos OpenSSD; see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Company: ENC Lab. <http://enc.hanyang.ac.kr>
// Engineer: Gyeongyong Lee <gylee@enc.hanyang.ac.kr>
//			 Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// Project Name: Cosmos OpenSSD
// Design Name: Greedy FTL
// Module Name: Flash Translation Layer
// File Name: ftl.h
//
// Version: v1.0.2
//
// Description:
//   - define NAND flash memory and SSD parameters
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.2
//   - add constant to calculate ssd size
//
// * v1.0.1
//   - replace bitwise operation with decimal operation
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#ifndef	FTL_H_
#define	FTL_H_

#define	SECTOR_SIZE_FTL			512

#define	PAGE_SIZE				8192  //8KB
#define	PAGE_NUM_PER_BLOCK		256
#define	BLOCK_NUM_PER_DIE		4096
#define	BLOCK_SIZE_MB			((PAGE_SIZE * PAGE_NUM_PER_BLOCK) / (1024 * 1024))

#define	CHANNEL_NUM				4
#define	WAY_NUM					4
#define	DIE_NUM					(CHANNEL_NUM * WAY_NUM)

#define	SECTOR_NUM_PER_PAGE		(PAGE_SIZE / SECTOR_SIZE_FTL)

#define	PAGE_NUM_PER_DIE		(PAGE_NUM_PER_BLOCK * BLOCK_NUM_PER_DIE)
#define	PAGE_NUM_PER_CHANNEL	(PAGE_NUM_PER_DIE * WAY_NUM)
#define	PAGE_NUM_PER_SSD		(PAGE_NUM_PER_CHANNEL * CHANNEL_NUM)

#define	BLOCK_NUM_PER_CHANNEL	(BLOCK_NUM_PER_DIE * WAY_NUM)
#define	BLOCK_NUM_PER_SSD		(BLOCK_NUM_PER_CHANNEL * CHANNEL_NUM)

#define SSD_SIZE				(BLOCK_NUM_PER_SSD * BLOCK_SIZE_MB) //MB
#define FREE_BLOCK_SIZE			(DIE_NUM * BLOCK_SIZE_MB)	//MB
#define METADATA_BLOCK_SIZE		(1 * BLOCK_SIZE_MB)	//MB

void InitNandReset();
void InitFtlMapTable();


///////////////////////////////
// FTL RL data structures
///////////////////////////////
#define NUM_FEATURE_TYPES 4
#define NUM_FEATURES      4
#define NUM_TILINGS       32
#define ISSUE_GC  1
#define ISSUE_NOP 2


UINT16 MEM_SIZE ;

//State information
typedef struct State {
  //Array that tracks stats
  float stats[NUM_FEATURE_TYPES];
} State ;

typedef struct Tuple {

  //Current state
  State currentState;
  //Action taken
  UINT8 currentAction;
  //Reward
  float reward;

  //Next state for sarsa updates
  State nextState;
  //Next action for sarsa updates
  UINT8 nextAction;

} Tuple ;


typedef struct CMAC {

  //Learning rate
  float alpha;

  //Input float vector
  float floatInputs[NUM_FEATURES];

  //Number of float inputs
  UINT8 numFloatInputs;

  //Input int vector
  UINT16 intInputs[2];

  //Number of int inputs
  UINT8 numIntInputs;

  //Tile indices (after hashing)
  UINT16 tiles[NUM_TILINGS];

  //CMAC array
  //float u[MEM_SIZE];
  float u[1];

  //Size of CMAC array
  UINT16 memorySize;

  //Number of tilings (after hashing)
  UINT8 numTilings;

  //Dimensionality of distinct tables
  UINT8 tableDimensionality;
} CMAC ;

//Q-Learning
typedef struct QLearning {

  //Largest Bellman error of the current sweep
  float bellmanError;

  //Number of input features
  UINT8 numFeatures;
  //Number of actions
  UINT8 numActions;
  //Discount rate
  float delta;
  //Learning rate
  float alpha;

  //CMAC across state-action space
  CMAC cmac;

  //Features selected for tiling
  UINT8 features[NUM_FEATURES];

} QLearning;


QLearning qLearning ;

//Random action probability of RL scheduler
float epsilon;

//Current state
State currentState;

//Next state
State nextState;

//Current action
UINT8 currentAction;

//Next action
UINT8 nextAction;

//Current reward
float currentReward;

//Next reward
float nextReward;

//Tuple for <s,a,r,s',a'>
Tuple t;

UINT8  numFeatures ;
UINT8  features[NUM_FEATURES] ;

UINT8  numActions  ;
float rewardGC ;
float rewardNOP ;

float delta ;
float alpha ;
float epsilon ;

UINT16 memorySize ;
UINT16 numTilings ;
UINT8  tableDim ;

float qValPred ;



// ===============================================================
// Adding Janani's code
// ===============================================================

#define CMAC_ADDR		(FCOUNT_ADDR + FCOUNT_BYTES)
#define CMAC_BYTES		(sizeof(float) * 4096)


///////////////////////////////
// FTL RL functions
///////////////////////////////

void initializeRL() ;


// STATE FUNCTIONS
//Constructor
void initializeState( State s ) ;

//QLEARNER FUNCTIONS
void initializeQLearner(UINT8 _numFeatures, UINT8 _numActions, float _delta, float _alpha, UINT16 _memorySize, UINT8 _numTilings, UINT8 _tableDimensionality) ;


//CMAC FUNCTIONS
void initializeCMAC( UINT8 _numFloatInputs, UINT8 _numIntInputs, UINT16 _memorySize, float _alpha, UINT8 _numTilings, UINT8 _tableDimensionality) ;
void setFloatInput(int index, float value ) ;
void setIntInput(int index, int value ) ;
float predict() ;
void update(float target) ;
void GetTiles(
    UINT16 tiles[],     // provided array contains returned tiles (tile indices)
    UINT8 num_tilings,  // number of tile indices to be returned in tiles
    UINT16 memory_size, // total number of possible tiles
    float floats[],     // array of floating point variables
    UINT8 num_floats,   // number of floating point variables
    UINT16 ints[],      // array of integer variables
    UINT8 num_ints) ;   // number of integer variables


#endif /* FTL_H_ */
