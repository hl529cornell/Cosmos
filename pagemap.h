//////////////////////////////////////////////////////////////////////////////////
// page_map.h for Cosmos OpenSSD
// Copyright (c) 2014 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//                Gyeongyong Lee <gylee@enc.hanyang.ac.kr>
//			      Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
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
// Module Name: Page Mapping
// File Name: page_map.h
//
// Version: v2.1.0
//
// Description:
//   - define data structure of map tables
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v2.1.0
//   - add page buffer to decrease formatting time
//
// * v2.0.1
//   - add constant to check bad blocks
//
// * v2.0.0
//   - modify/add tables to support garbage collection
//
// * v1.1.0
//   - add bad flag
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#ifndef PAGEMAP_H_
#define PAGEMAP_H_

#include "host_controller.h"
#include "ftl.h"

typedef enum {
	INACTIVE = 0,
	FREE = 1,
	ACTIVE_FREE = 2,
	ACTIVE = 3,
	BAD = 4
} state;

#define STATE_NUM 4
#define BIN_NUM 3

struct pmEntry {
	u32 ppn;	// Physical Page Number (PPN) to which a logical page is mapped

	u32 valid	: 1;	// validity of a physical page
	u32 lpn		: 31;	// Logical Page Number (LPN) of a physical page
};

struct pmArray {
	struct pmEntry pmEntry[DIE_NUM][PAGE_NUM_PER_DIE];
};

int max_erase_cnt;

/*
 * Block Status Table Entry
 *
 * The block status table stores the state, number of valid pages
 * and the number of erases for all blocks.
 */
struct bstEntry{
	u32 bad				: 1;
	u32 usable			: 1;  // Whether we can use this block to store data or not. Some blocks are
							  // reserved for other uses such as metadata storage.
	state s;
	u32 validPageCnt	: 16;
	unsigned char curBin: 4;
	u32 eraseCnt		: 30;
	u32 invalidPageCnt : 16;
	u32 currentPage    : 16;
	u32 prevBlock;
	u32 nextBlock;
};

struct bstArray {
	struct bstEntry bstEntry[DIE_NUM][BLOCK_NUM_PER_DIE];
};

struct bfsmEntry {
	u32 head;
	u32 tail;
};

struct bfsmTable {
	// State is in order of INACTIVE, ACTIVE, FREE, BAD
	struct bfsmEntry bfsmEntry[DIE_NUM][STATE_NUM][BIN_NUM];
};

struct dieEntry {
	u32 currentBlock;
	u32 freeBlock;
};

struct dieArray {
	struct dieEntry dieEntry[DIE_NUM];
};

struct gcEntry {
	u32 head;
	u32 tail;
};

struct gcArray {
	struct gcEntry gcEntry[DIE_NUM][PAGE_NUM_PER_BLOCK+1];
};

struct pmArray* pageMap;
struct bstArray* blockStatusTable;
struct bfsmTable* blockFSMTable;
struct dieArray* dieBlock;
struct gcArray* gcMap;

// memory addresses for map tables
#define PAGE_MAP_ADDR	(RAM_DISK_BASE_ADDR + (0x1 << 27))
#define BST_ADDR		(PAGE_MAP_ADDR + sizeof(struct pmEntry) * PAGE_NUM_PER_SSD)//SRAM0_BASE_ADDR
#define BFSM_ADDR		(BST_ADDR + sizeof(struct bstEntry) * DIE_NUM * BLOCK_NUM_PER_DIE)//(PAGE_MAP_ADDR + sizeof(struct pmEntry) * PAGE_NUM_PER_SSD)
#define DIE_MAP_ADDR	(BFSM_ADDR + sizeof(struct bfsmEntry) * DIE_NUM * STATE_NUM * BIN_NUM)
#define GC_MAP_ADDR		(DIE_MAP_ADDR + sizeof(struct dieEntry) * DIE_NUM)

// ===============================================================
// Adding Janani's code
// ===============================================================

#define CMAC_ADDR		(GC_MAP_ADDR + sizeof(struct gcEntry) * DIE_NUM*(PAGE_NUM_PER_BLOCK + 1))
#define CMAC_BYTES		(sizeof(float) * 4096)

#define BAD_BLOCK_MARK_POSITION	(7972)
#define METADATA_BLOCK_PPN	 	0x00000000 // write metadata to Block0 of Die0
#define EMPTY_BYTE				0xff


// memory address of buffer for GC migration
#define GC_BUFFER_ADDR (CMAC_ADDR + CMAC_BYTES)

extern u32 BAD_BLOCK_SIZE;

// buffered LPN in the page buffer
u32 pageBufLpn;

void InitPageMap();
void InitBlockMap();
void InitDieBlock();
void InitGcMap();

int FindFreePage(u32 dieNo);
int PrePmRead(P_HOST_CMD hostCmd, u32 bufferAddr);
int PmRead(P_HOST_CMD hostCmd, u32 bufferAddr);
int PmWrite(P_HOST_CMD hostCmd, u32 bufferAddr);

void EraseBlock(u32 dieNo, u32 blockNo);
u32 GarbageCollection(u32 dieNo);

void CheckBadBlock();
int CountBits(u8 i);

void FlushPageBuf(u32 lpn, u32 bufAddr);
void UpdateMetaForOverwrite(u32 lpn);
void MoveBSTEntry(u32 dieNo, u32 blockNo, state curState, unsigned char curBin, state nextState, unsigned char nextBin);
//void MvData(u32* src, u32* dst, u32 sectSize);

///////////////////////////////
// FTL RL data structures
///////////////////////////////
#define NUM_FEATURE_TYPES 4
#define NUM_FEATURES      4
#define NUM_TILINGS       32
#define ISSUE_GC  1
#define ISSUE_NOP 2


unsigned short MEM_SIZE ;

//State information
typedef struct State {
  //Array that tracks stats
  float stats[NUM_FEATURE_TYPES];
} State ;

typedef struct Tuple {

  //Current state
  State currentState;
  //Action taken
  unsigned char currentAction;
  //Reward
  float reward;

  //Next state for sarsa updates
  State nextState;
  //Next action for sarsa updates
  unsigned char nextAction;

} Tuple ;


typedef struct CMAC {

  //Learning rate
  float alpha;

  //Input float vector
  float floatInputs[NUM_FEATURES];

  //Number of float inputs
  unsigned char numFloatInputs;

  //Input int vector
  unsigned short intInputs[2];

  //Number of int inputs
  unsigned char numIntInputs;

  //Tile indices (after hashing)
  unsigned short tiles[NUM_TILINGS];

  //CMAC array
  //float u[MEM_SIZE];
  float u[1];

  //Size of CMAC array
  unsigned short memorySize;

  //Number of tilings (after hashing)
  unsigned char numTilings;

  //Dimensionality of distinct tables
  unsigned char tableDimensionality;
} CMAC ;

//Q-Learning
typedef struct QLearning {

  //Largest Bellman error of the current sweep
  float bellmanError;

  //Number of input features
  unsigned char numFeatures;
  //Number of actions
  unsigned char numActions;
  //Discount rate
  float delta;
  //Learning rate
  float alpha;

  //CMAC across state-action space
  CMAC cmac;

  //Features selected for tiling
  unsigned char features[NUM_FEATURES];

} QLearning;


QLearning qLearning ;

//Random action probability of RL scheduler
float epsilon;

//Current state
State currentState;

//Next state
State nextState;

//Current action
unsigned char currentAction;

//Next action
unsigned char nextAction;

//Current reward
float currentReward;

//Next reward
float nextReward;

//Tuple for <s,a,r,s',a'>
Tuple t;

unsigned char  numFeatures ;
unsigned char  features[NUM_FEATURES] ;

unsigned char  numActions  ;
float rewardGC ;
float rewardNOP ;

float delta ;
float alpha ;
float epsilon ;

unsigned short memorySize ;
unsigned short numTilings ;
unsigned char  tableDim ;

float qValPred ;

///////////////////////////////
// FTL RL functions
///////////////////////////////

void initializeRL() ;


// STATE FUNCTIONS
//Constructor
void initializeState( State s ) ;

//QLEARNER FUNCTIONS
void initializeQLearner(unsigned char _numFeatures, unsigned char _numActions, float _delta, float _alpha, unsigned short _memorySize, unsigned char _numTilings, unsigned char _tableDimensionality) ;


//CMAC FUNCTIONS
void initializeCMAC( unsigned char _numFloatInputs, unsigned char _numIntInputs, unsigned short _memorySize, float _alpha, unsigned char _numTilings, unsigned char _tableDimensionality) ;
void setFloatInput(int index, float value ) ;
void setIntInput(int index, int value ) ;
float predict() ;
void update(float target) ;
void GetTiles(
    unsigned short tiles[],     // provided array contains returned tiles (tile indices)
    unsigned char num_tilings,  // number of tile indices to be returned in tiles
    unsigned short memory_size, // total number of possible tiles
    float floats[],     // array of floating point variables
    unsigned char num_floats,   // number of floating point variables
    unsigned short ints[],      // array of integer variables
    unsigned char num_ints) ;   // number of integer variables

#endif /* PAGEMAP_H_ */
