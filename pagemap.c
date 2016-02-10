//////////////////////////////////////////////////////////////////////////////////
// page_map.c for Cosmos OpenSSD
// Copyright (c) 2014 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//                Gyeongyong Lee <gylee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@@enc.hanyang.ac.kr>
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
// File Name: page_map.c
//
// Version: v2.2.0
//
// Description:
//   - initialize map tables
//   - read/write request
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v2.2.0
//   - page buffer utilization in r/w
//   - code of metadata update for overwrite is extracted into the function UpdateMetaForOverwrite()
//
// * v2.1.0
//   - add a function to check bad block
//
// * v2.0.1
//   - replace bitwise operation with decimal operation
//
// * v2.0.0
//   - add garbage collection
//
// * v1.1.0
//   - support static bad block management
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "pagemap.h"
#include "mem_map.h"

#include <assert.h>

#include "lld.h"

u32 BAD_BLOCK_SIZE;

void InitPageMap()
{
	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);

//	xil_printf("PAGE_MAP_ADDR : %8x\r\n", PAGE_MAP_ADDR);

	// page status initialization, allows lpn, ppn access
	int i, j;
	for(i=0 ; i<DIE_NUM ; i++)
	{
		for(j=0 ; j<PAGE_NUM_PER_DIE ; j++)
		{
			pageMap->pmEntry[i][j].ppn = 0xffffffff;

			pageMap->pmEntry[i][j].valid = 1;
			pageMap->pmEntry[i][j].lpn = 0x7fffffff;
		}
	}

	xil_printf("[ ssd page map initialized. ]\r\n");
}

void InitBlockMap()
{
	blockStatusTable = (struct bstArray*)(BST_ADDR);
	blockFSMTable = (struct bfsmTable*)(BFSM_ADDR);
	dieBlock = (struct dieArray*)(DIE_MAP_ADDR);
	gcMap = (struct gcArray*)(GC_MAP_ADDR);
	u32 a = GC_MAP_ADDR;
	u32 b = BFSM_ADDR;
	u32 c = BST_ADDR;
	u32 d = PAGE_MAP_ADDR;

	xil_printf("GC Map ADDR: %x \r\n", a);
	xil_printf("BFSM ADDR: %x \r\n", b);
	xil_printf("BST ADDR: %x \r\n", c);
	xil_printf("Page Map ADDR: %x \r\n", d);
	int k, l, m;


	// Initialize block FSM table
	// head and tail are NULL initially.

	for (k = 0; k < DIE_NUM; k++) {
		for (l = 0; l < STATE_NUM; l++) {
			for (m = 0; m < BIN_NUM; m++) {
				blockFSMTable->bfsmEntry[k][l][m].head = 0xffffffff;
				blockFSMTable->bfsmEntry[k][l][m].tail = 0xffffffff;
			}
		}
	}

	// block status initialization, allows only physical access
	int i, j;
	for(i=0 ; i<BLOCK_NUM_PER_DIE ; i++)
	{
		for(j=0 ; j<DIE_NUM ; j++)
		{
			// initialize block state table
			// All bstEntry are initially in FREE bin 1
			blockStatusTable->bstEntry[j][i].s = FREE;
			blockStatusTable->bstEntry[j][i].curBin = 0;
			blockStatusTable->bstEntry[j][i].validPageCnt = PAGE_NUM_PER_BLOCK;
			blockStatusTable->bstEntry[j][i].invalidPageCnt = 0;
			blockStatusTable->bstEntry[j][i].eraseCnt = 0;
			blockStatusTable->bstEntry[j][i].currentPage = 0x0;

			// Put all block except for the very first block in FREE Bin 1
			// Very first block is ignored because it is only used to store metadata.
			// Note that the prevBlock, nextBlock of bstEntry and head, tail of bfsmEntry are
			// simply indices to the block number of the blockStatusTable.


			if (i == 0) {
				xil_printf("Putting 1st block of die in BAD bin\r\n");
				blockFSMTable->bfsmEntry[j][BAD][0].head = i;
				blockFSMTable->bfsmEntry[j][BAD][0].tail = i;
				blockStatusTable->bstEntry[j][i].nextBlock = 0xffffffff;
				blockStatusTable->bstEntry[j][i].prevBlock = 0xffffffff;
				blockStatusTable->bstEntry[j][i].s = BAD;
				blockStatusTable->bstEntry[j][i].bad = 1;
				blockStatusTable->bstEntry[j][i].curBin = 0;
			}
			else if (i == 1) {
				blockFSMTable->bfsmEntry[j][FREE][0].head = i;
				blockStatusTable->bstEntry[j][i].nextBlock = i + 1;
				blockStatusTable->bstEntry[j][i].prevBlock = 0xffffffff;
			}
			else if (i == BLOCK_NUM_PER_DIE - 1) {
				blockFSMTable->bfsmEntry[j][FREE][0].tail = i;
				blockStatusTable->bstEntry[j][i].prevBlock = i - 1;
				blockStatusTable->bstEntry[j][i].nextBlock = 0xffffffff;
			}
			else{
				blockStatusTable->bstEntry[j][i].nextBlock = i + 1;
				blockStatusTable->bstEntry[j][i].prevBlock = i - 1;
			}
		}
	}

	// DEBUGGING: print head and tail for each state/bin pair
	for (k = 0; k < DIE_NUM; k++) {
		for (l = 0; l < STATE_NUM; l++) {
			for (m = 0; m < BIN_NUM; m++) {
				xil_printf("Head of (Die %d, State %d, Bin %d) is %d\r\n", k, l, m, blockFSMTable->bfsmEntry[k][l][m].head);
				xil_printf("Tail of (Die %d, State %d, Bin %d) is %d\r\n", k, l, m, blockFSMTable->bfsmEntry[k][l][m].tail);
			}
		}
	}

	// DEBUGGING: print all block states for die 0
	for (i = 0; i < BLOCK_NUM_PER_DIE; i++ ) {
		xil_printf("Block %d: (state, curBin, nextBlk, prevBlk) = ( %d, %d, %d, %d)\r\n",
				i,
				blockStatusTable->bstEntry[0][i].s,
				blockStatusTable->bstEntry[0][i].curBin,
				blockStatusTable->bstEntry[0][i].nextBlock,
				blockStatusTable->bstEntry[0][i].prevBlock );
	}

	CheckBadBlock();

	for (i = 0; i < BLOCK_NUM_PER_DIE; ++i)
		for (j = 0; j < DIE_NUM; ++j)
			if (!blockStatusTable->bstEntry[j][i].bad
					&& ((i != METADATA_BLOCK_PPN % DIE_NUM) || (j != (METADATA_BLOCK_PPN / DIE_NUM) / PAGE_NUM_PER_BLOCK)))
			{
				// initial block erase
				WaitWayFree(j % CHANNEL_NUM, j / CHANNEL_NUM);
				SsdErase(j % CHANNEL_NUM, j / CHANNEL_NUM, i);
			}


	xil_printf("[ ssd entire block erasure completed. ]\r\n");

	for(i=0 ; i<DIE_NUM ; i++)
	{
		// initially, 0th block of each die is allocated for storage start point
		blockStatusTable->bstEntry[i][0].usable = 0;
		blockStatusTable->bstEntry[i][0].currentPage = 0xffff;
		// initially, the last block of each die is reserved as free block for GC migration
		blockStatusTable->bstEntry[i][BLOCK_NUM_PER_DIE-1].usable = 0;
	}
	//block0 of die0 is metadata block
	blockStatusTable->bstEntry[0][1].usable = 0;
	blockStatusTable->bstEntry[0][1].currentPage = 0xffff;

	xil_printf("[ ssd block map initialized. ]\r\n");
}

void CheckBadBlock()
{
	blockStatusTable = (struct bstArray*)(BST_ADDR);
	blockFSMTable = (struct bfsmTable*)(BFSM_ADDR);
	u32 dieNo, diePpn, blockNo, tempBuffer, badBlockCount;
	u8* shifter;
	u8* markPointer;
	int loop;

	markPointer = (u8*)(RAM_DISK_BASE_ADDR + BAD_BLOCK_MARK_POSITION);

	//read badblock marks
	loop = DIE_NUM *BLOCK_NUM_PER_DIE;
	dieNo = METADATA_BLOCK_PPN % DIE_NUM;
	diePpn = METADATA_BLOCK_PPN / DIE_NUM;

	tempBuffer = RAM_DISK_BASE_ADDR;
	while(loop > 0)
	{
		SsdRead(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, diePpn, tempBuffer);
		WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);

		diePpn++;
		tempBuffer += PAGE_SIZE;
		loop -= PAGE_SIZE;
	}

	shifter= (u8*)(RAM_DISK_BASE_ADDR);
	badBlockCount = 0;
	//if(*shifter == EMPTY_BYTE)	//check whether badblock marks exist
	//{
		// static bad block management
		for(blockNo=0; blockNo < BLOCK_NUM_PER_DIE; blockNo++)
			for(dieNo=0; dieNo < DIE_NUM; dieNo++)
			{
				// Assume block is not bad
				if (blockNo != 0)
					blockStatusTable->bstEntry[dieNo][blockNo].bad = 0;

				SsdRead(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, (blockNo*PAGE_NUM_PER_BLOCK+1), RAM_DISK_BASE_ADDR);
				WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);

				// if we detect that block is bad, mark as bad block.
				if(CountBits(*markPointer)<4 && blockStatusTable->bstEntry[dieNo][blockNo].s == FREE)
				{
					xil_printf("Bad block is detected on: Ch %d Way %d Block %d \r\n",dieNo%CHANNEL_NUM, dieNo/CHANNEL_NUM, blockNo);
					// change block state
					blockStatusTable->bstEntry[dieNo][blockNo].bad = 1;
					blockStatusTable->bstEntry[dieNo][blockNo].s = BAD;

					// Remove bad block from FREE Bin 1. We know that the block is already in the FREE Bin 1
					// since that is where we initialize all blocks to go to.
					//
					// There are four possible conditions:
					// 1. bstEntry is in middle of FREE Bin 1 linked list
					// 2. bstEntry is in end of FREE Bin 1 linked list
					// 3. bstEntry is in beginning of FREE Bin 1 linked list
					// 4. bstEntry is the only member of FREE Bin 1 linked list

					MoveBSTEntry(dieNo, blockNo, FREE, 0, BAD, 0);

					badBlockCount++;
				}
				shifter= (u8*)(GC_BUFFER_ADDR + blockNo + dieNo *BLOCK_NUM_PER_DIE );//gather badblock mark at GC buffer
				*shifter = blockStatusTable->bstEntry[dieNo][blockNo].bad;
			}

		// save bad block mark
		loop = DIE_NUM *BLOCK_NUM_PER_DIE;
		dieNo = METADATA_BLOCK_PPN % DIE_NUM;
		diePpn = METADATA_BLOCK_PPN / DIE_NUM;
		blockNo = diePpn / PAGE_NUM_PER_BLOCK;

		SsdErase(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, blockNo);
		WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);

		tempBuffer = GC_BUFFER_ADDR;
		while(loop>0)
		{
			WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
			SsdProgram(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, diePpn, tempBuffer);
			diePpn++;
			tempBuffer += PAGE_SIZE;
			loop -= PAGE_SIZE;
		}
		xil_printf("[ Bad block Marks are saved. ]\r\n");
	//}

	/*else	//read existing bad block marks
	{
		for(blockNo=0; blockNo<BLOCK_NUM_PER_DIE; blockNo++)
			for(dieNo=0; dieNo<DIE_NUM; dieNo++)
			{
				shifter = (u8*)(RAM_DISK_BASE_ADDR + blockNo + dieNo *BLOCK_NUM_PER_DIE );
				blockStatusTable->bstEntry[dieNo][blockNo].bad = *shifter;
				if(blockStatusTable->bstEntry[dieNo][blockNo].bad)
				{
					xil_printf("Bad block mark is checked at: Ch %d Way %d Block %d  \r\n",dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, blockNo );

					// set block status as BAD.
					blockStatusTable->bstEntry[dieNo][blockNo].s = BAD;

					// Remove bad block from FREE Bin 1. We know that the block is already in the FREE Bin 1
					// since that is where we initialize all blocks to go to.
					//
					// There are four possible conditions:
					// 1. bstEntry is in middle of FREE Bin 1 linked list
					// 2. bstEntry is in end of FREE Bin 1 linked list
					// 3. bstEntry is in beginning of FREE Bin 1 linked list
					// 4. bstEntry is the only member of FREE Bin 1 linked list
					if((blockStatusTable->bstEntry[dieNo][blockNo].nextBlock != 0xffffffff)
							&& (blockStatusTable->bstEntry[dieNo][blockNo].prevBlock != 0xffffffff))
					{
						xil_printf("bstEntry in middle of DLL\n");
						blockStatusTable->bstEntry[dieNo][blockStatusTable->bstEntry[dieNo][blockNo].prevBlock].nextBlock
							= blockStatusTable->bstEntry[dieNo][blockNo].nextBlock;
						blockStatusTable->bstEntry[dieNo][blockStatusTable->bstEntry[dieNo][blockNo].nextBlock].prevBlock
							= blockStatusTable->bstEntry[dieNo][blockNo].prevBlock;
					}
					else if((blockStatusTable->bstEntry[dieNo][blockNo].nextBlock == 0xffffffff)
							&& (blockStatusTable->bstEntry[dieNo][blockNo].prevBlock != 0xffffffff))
					{
						xil_printf("bstEntry in end of DLL\n");
						blockStatusTable->bstEntry[dieNo][blockStatusTable->bstEntry[dieNo][blockNo].prevBlock].nextBlock = 0xffffffff;
						blockFSMTable->bfsmEntry[dieNo][FREE][0].tail = blockStatusTable->bstEntry[dieNo][blockNo].prevBlock;
					}
					else if((blockStatusTable->bstEntry[dieNo][blockNo].nextBlock != 0xffffffff)
							&& (blockStatusTable->bstEntry[dieNo][blockNo].prevBlock == 0xffffffff))
					{
						xil_printf("bstEntry in start of DLL\n");
						blockStatusTable->bstEntry[dieNo][blockStatusTable->bstEntry[dieNo][blockNo].nextBlock].prevBlock = 0xffffffff;
						blockFSMTable->bfsmEntry[dieNo][FREE][0].head = blockStatusTable->bstEntry[dieNo][blockNo].nextBlock;
					}
					else
					{
						xil_printf("THIS SHOULD NOT HAPPEN FREE 2\n");
						blockFSMTable->bfsmEntry[dieNo][FREE][0].head = 0xffffffff;
						blockFSMTable->bfsmEntry[dieNo][FREE][0].tail = 0xffffffff;
					}


					// Move bad block to BAD Bin 1 (we only use bin 1 for BAD blocks)
					if (blockFSMTable->bfsmEntry[dieNo][BAD][0].tail == 0xffffffff) {
						blockStatusTable->bstEntry[dieNo][blockNo].nextBlock = 0xffffffff;
						blockStatusTable->bstEntry[dieNo][blockNo].prevBlock = 0xffffffff;
						blockFSMTable->bfsmEntry[dieNo][BAD][0].head = blockNo;
						blockFSMTable->bfsmEntry[dieNo][BAD][0].tail = blockNo;
					} else {
						blockStatusTable->bstEntry[dieNo][blockNo].nextBlock = 0xffffffff;
						blockStatusTable->bstEntry[dieNo][blockNo].prevBlock = blockFSMTable->bfsmEntry[dieNo][BAD][0].tail;
						blockStatusTable->bstEntry[dieNo][blockFSMTable->bfsmEntry[dieNo][BAD][0].tail].nextBlock = blockNo;
						blockFSMTable->bfsmEntry[dieNo][BAD][0].tail = blockNo;
					}

					badBlockCount++;
				}
			}

		xil_printf("[ Bad blocks are checked. ]\r\n");
	}*/

	// save bad block size
	BAD_BLOCK_SIZE = badBlockCount * BLOCK_SIZE_MB;
}

int CountBits(u8 i)
{
	int count;
	count=0;
	while(i!=0)
	{
		count+=i%2;
		i/=2;
	}
	return count;
}


void InitDieBlock()
{
	dieBlock = (struct dieArray*)(DIE_MAP_ADDR);

//	xil_printf("DIE_MAP_ADDR : %8x\r\n", DIE_MAP_ADDR);

	int i;
	for(i=0 ; i<DIE_NUM ; i++)
	{
		if(i==0) // prevent to write at meta data block
			dieBlock->dieEntry[i].currentBlock = 1;
		else
			dieBlock->dieEntry[i].currentBlock = 0;
		dieBlock->dieEntry[i].freeBlock = BLOCK_NUM_PER_DIE - 1;
	}

	xil_printf("[ ssd die map initialized. ]\r\n");
}

void InitGcMap()
{
	gcMap = (struct gcArray*)(GC_MAP_ADDR);

//	xil_printf("GC_MAP_ADDR : %8x\r\n", GC_MAP_ADDR);

	// gc table status initialization
	int i, j;
	for(i=0 ; i<DIE_NUM ; i++)
	{
		for(j=0 ; j<PAGE_NUM_PER_BLOCK+1 ; j++)
		{
			gcMap->gcEntry[i][j].head = 0xffffffff;
			gcMap->gcEntry[i][j].tail = 0xffffffff;
		}
	}

	xil_printf("[ ssd gc map initialized. ]\r\n");
}

int FindFreePage(u32 dieNo)
{
	blockStatusTable = (struct bstArray*)(BST_ADDR);
	dieBlock = (struct dieArray*)(DIE_MAP_ADDR);
	int j;
	for (j = 0; j < BIN_NUM; j++) {
		if (blockFSMTable->bfsmEntry[dieNo][FREE][j].head != 0xffffffff) {
			xil_printf("Free page in (dieNo, state, bin)\r\n", dieNo, FREE, j);

			int block_num = blockFSMTable->bfsmEntry[dieNo][FREE][j].head;
			MoveBSTEntry(dieNo, blockFSMTable->bfsmEntry[dieNo][FREE][j].head, FREE, j, ACTIVE_FREE, j);
			blockStatusTable->bstEntry[dieNo][block_num].currentPage++;
			return block_num * PAGE_NUM_PER_BLOCK + blockStatusTable->bstEntry[dieNo][block_num].currentPage - 1;
		}
	}

	for (j = 0; j < BIN_NUM; j++) {
		if (blockFSMTable->bfsmEntry[dieNo][ACTIVE_FREE][j].head != 0xffffffff)
		{
			xil_printf("Free page in (dieNo, state, bin)\r\n", dieNo, ACTIVE_FREE, j);

			int block_num = blockFSMTable->bfsmEntry[dieNo][ACTIVE_FREE][j].head;
			blockStatusTable->bstEntry[dieNo][block_num].currentPage++;
			return block_num * PAGE_NUM_PER_BLOCK + blockStatusTable->bstEntry[dieNo][block_num].currentPage - 1;
		}
	}

	return -1;
}

int PrePmRead(P_HOST_CMD hostCmd, u32 bufferAddr)
{
	u32 lpn;
	u32 dieNo;
	u32 dieLpn;

	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);
	lpn = hostCmd->reqInfo.CurSect / SECTOR_NUM_PER_PAGE;

	if (lpn != pageBufLpn)
	{
		FlushPageBuf(pageBufLpn, bufferAddr);

		if((((hostCmd->reqInfo.CurSect)%SECTOR_NUM_PER_PAGE) != 0)
					|| ((hostCmd->reqInfo.CurSect / SECTOR_NUM_PER_PAGE) == (((hostCmd->reqInfo.CurSect)+(hostCmd->reqInfo.ReqSect))/SECTOR_NUM_PER_PAGE)))
		{
			dieNo = lpn % DIE_NUM;
			dieLpn = lpn / DIE_NUM;

			if(pageMap->pmEntry[dieNo][dieLpn].ppn != 0xffffffff)
			{
//				xil_printf("PrePmRead pdie, ppn = %d, %d\r\n", dieNo, pageMap->pmEntry[dieNo][dieLpn].ppn);

				WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
				SsdRead(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, pageMap->pmEntry[dieNo][dieLpn].ppn, bufferAddr);
				// we don't see the point of this second WaitWayFree
				//WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);

				pageBufLpn = lpn;
			}
		}
	}

	if(((((hostCmd->reqInfo.CurSect)+(hostCmd->reqInfo.ReqSect))% SECTOR_NUM_PER_PAGE) != 0)
			&& ((hostCmd->reqInfo.CurSect / SECTOR_NUM_PER_PAGE) != (((hostCmd->reqInfo.CurSect)+(hostCmd->reqInfo.ReqSect))/SECTOR_NUM_PER_PAGE)))
	{
		lpn = ((hostCmd->reqInfo.CurSect)+(hostCmd->reqInfo.ReqSect))/SECTOR_NUM_PER_PAGE;
		dieNo = lpn % DIE_NUM;
		dieLpn = lpn / DIE_NUM;

		if(pageMap->pmEntry[dieNo][dieLpn].ppn != 0xffffffff)
		{

//			xil_printf("PrePmRead pdie, ppn = %d, %d\r\n", dieNo, pageMap->pmEntry[dieNo][dieLpn].ppn);

			WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
			SsdRead(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, pageMap->pmEntry[dieNo][dieLpn].ppn,
					bufferAddr + ((((hostCmd->reqInfo.CurSect)% SECTOR_NUM_PER_PAGE) + hostCmd->reqInfo.ReqSect)/SECTOR_NUM_PER_PAGE*PAGE_SIZE));
			// we don't see the point of this second WaitWayFree
			//WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
		}
	}

	return 0;
}

int PmRead(P_HOST_CMD hostCmd, u32 bufferAddr)
{
	u32 tempBuffer = bufferAddr;
	
	u32 lpn = hostCmd->reqInfo.CurSect / SECTOR_NUM_PER_PAGE;
	int loop = (hostCmd->reqInfo.CurSect % SECTOR_NUM_PER_PAGE) + hostCmd->reqInfo.ReqSect;
	
	u32 dieNo;
	u32 dieLpn;

	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);

	if (lpn == pageBufLpn)
	{
		lpn++;
		tempBuffer += PAGE_SIZE;
		loop -= SECTOR_NUM_PER_PAGE;
	}
	else
	{
		dieNo = lpn % DIE_NUM;
		dieLpn = lpn / DIE_NUM;

		if(pageMap->pmEntry[dieNo][dieLpn].ppn != 0xffffffff)
		{
			FlushPageBuf(pageBufLpn, bufferAddr);
			pageBufLpn = lpn;
		}
	}

	while(loop > 0)
	{
		dieNo = lpn % DIE_NUM;
		dieLpn = lpn / DIE_NUM;

//		xil_printf("requested read lpn = %d\r\n", lpn);
//		xil_printf("read pdie, ppn = %d, %d\r\n", dieNo, pageMap->pmEntry[dieNo][dieLpn].ppn);

		if(pageMap->pmEntry[dieNo][dieLpn].ppn != 0xffffffff)
		{
//			xil_printf("read at (%d, %2d, %4x)\r\n", dieNo%CHANNEL_NUM, dieNo/CHANNEL_NUM, pageMap->pmEntry[dieNo][dieLpn].ppn);

			WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
			SsdRead(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, pageMap->pmEntry[dieNo][dieLpn].ppn, tempBuffer);
		}

		lpn++;
		tempBuffer += PAGE_SIZE;
		loop -= SECTOR_NUM_PER_PAGE;
	}

	int i;
	for(i=0 ; i<DIE_NUM ; ++i)
		WaitWayFree(i%CHANNEL_NUM, i/CHANNEL_NUM);

	return 0;
}

int PmWrite(P_HOST_CMD hostCmd, u32 bufferAddr)
{
	blockStatusTable = (struct bstArray*)(BST_ADDR);

	u32 tempBuffer = bufferAddr;
	
	u32 lpn = hostCmd->reqInfo.CurSect / SECTOR_NUM_PER_PAGE;
	
	int loop = (hostCmd->reqInfo.CurSect % SECTOR_NUM_PER_PAGE) + hostCmd->reqInfo.ReqSect;
	
	u32 dieNo;
	u32 dieLpn;
	u32 freePageNo;

	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);

	// page buffer utilization
	if (lpn != pageBufLpn)
		pageBufLpn = lpn;

	UpdateMetaForOverwrite(lpn);

	// pageMap update
	dieNo = lpn % DIE_NUM;
	dieLpn = lpn / DIE_NUM;
	pageMap->pmEntry[dieNo][dieLpn].ppn = 0xffffffff;

	lpn++;
	tempBuffer += PAGE_SIZE;
	loop -= SECTOR_NUM_PER_PAGE;

	while(loop > 0)
	{
		dieNo = lpn % DIE_NUM;
		dieLpn = lpn / DIE_NUM;
		freePageNo = FindFreePage(dieNo);

		xil_printf("free page: %6d(%d, %4d)\r\n", freePageNo, dieNo, freePageNo/PAGE_NUM_PER_BLOCK);

		WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
		SsdProgram(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, freePageNo, tempBuffer);
		
		UpdateMetaForOverwrite(lpn);

		// pageMap update
		pageMap->pmEntry[dieNo][dieLpn].ppn = freePageNo;
		pageMap->pmEntry[dieNo][freePageNo].lpn = dieLpn;

		u32 diePbn = pageMap->pmEntry[dieNo][dieLpn].ppn / PAGE_NUM_PER_BLOCK;

		// block status table update
		u32 nValidPages = blockStatusTable->bstEntry[dieNo][diePbn].validPageCnt;
		state block_s = blockStatusTable->bstEntry[dieNo][diePbn].s;
		unsigned char curBin = blockStatusTable->bstEntry[dieNo][diePbn].curBin;

		// check to see if this block is full - then it needs to be transitioned to the ACTIVE state
		if (blockStatusTable->bstEntry[dieNo][diePbn].currentPage == PAGE_NUM_PER_BLOCK - 1) {
			if (nValidPages < ( PAGE_NUM_PER_BLOCK >> 2 ))
				MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE, 0);
			else if ( nValidPages < ( PAGE_NUM_PER_BLOCK >> 1 ) )
				MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE, 1);
			else
				MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE, 2);
		}

		lpn++;
		tempBuffer += PAGE_SIZE;
		loop -= SECTOR_NUM_PER_PAGE;
	}

	// TODO: Why does it do this?
	//int i;
	//for(i=0 ; i<DIE_NUM ; ++i)
	//	WaitWayFree(i%CHANNEL_NUM, i/CHANNEL_NUM);

	return 0;
}

void EraseBlock(u32 dieNo, u32 blockNo)
{
	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);
	blockStatusTable = (struct bstArray*)(BST_ADDR);

	// block status table indicated blockNo initialization
	blockStatusTable->bstEntry[dieNo][blockNo].usable = 1;
	blockStatusTable->bstEntry[dieNo][blockNo].eraseCnt++;
	blockStatusTable->bstEntry[dieNo][blockNo].invalidPageCnt = 0;
	blockStatusTable->bstEntry[dieNo][blockNo].validPageCnt = PAGE_NUM_PER_BLOCK;
	blockStatusTable->bstEntry[dieNo][blockNo].currentPage = 0x0;

	// Move the bstEntry from its list to the FREE list
	state cur_state = blockStatusTable->bstEntry[dieNo][blockNo].s;
	unsigned char cur_bin = blockStatusTable->bstEntry[dieNo][blockNo].curBin;
	u32 num_erases = blockStatusTable->bstEntry[dieNo][blockNo].eraseCnt;
	unsigned char next_bin;

	if (num_erases < (max_erase_cnt >> 2))
		next_bin = 0;
	else if (num_erases < (max_erase_cnt >> 1))
		next_bin = 1;
	else
		next_bin = 2;

	MoveBSTEntry(dieNo, blockNo, cur_state, cur_bin, FREE, next_bin);

	int i;
	for(i=0 ; i<PAGE_NUM_PER_BLOCK ; i++)
	{
		pageMap->pmEntry[dieNo][(blockNo * PAGE_NUM_PER_BLOCK) + i].valid = 1;
		pageMap->pmEntry[dieNo][(blockNo * PAGE_NUM_PER_BLOCK) + i].lpn = 0x7fffffff;
	}

	WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
	SsdErase(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, blockNo);
}

// TODO: Change this code to work with BST and BFSMT

/* Garbage collecting a given die involves the following steps:
 * 1. Identify an INACTIVE or ACTIVE block from the BFSMT - this is the victim block
 * 2. Identify a FREE block
 * 3. Move valid pages from the victim block to the FREE block
 * 4. Erase the victim block - this marks it as FREE and moves it to the FREE list.
 */
u32 GarbageCollection(u32 dieNo)
{
	xil_printf("GC occurs!\r\n");

	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);
	blockStatusTable = (struct bstArray*)(BST_ADDR);
	blockFSMTable = (struct bfsmTable*)(BFSM_ADDR);
	dieBlock = (struct dieArray*)(DIE_MAP_ADDR);
	gcMap = (struct gcArray*)(GC_MAP_ADDR);

	u32 victimBlock;
	u32 freeBlock;
	int j;

	// 1. Identify victim block
	if (blockFSMTable->bfsmEntry[dieNo][INACTIVE][0].head != 0xffffffff)
		victimBlock = blockFSMTable->bfsmEntry[dieNo][INACTIVE][0].head;
	else if (blockFSMTable->bfsmEntry[dieNo][INACTIVE][1].head != 0xffffffff)
		victimBlock = blockFSMTable->bfsmEntry[dieNo][INACTIVE][1].head;
	else if (blockFSMTable->bfsmEntry[dieNo][INACTIVE][2].head != 0xffffffff)
		victimBlock = blockFSMTable->bfsmEntry[dieNo][INACTIVE][2].head;
	else if (blockFSMTable->bfsmEntry[dieNo][ACTIVE][0].head != 0xffffffff)
		victimBlock = blockFSMTable->bfsmEntry[dieNo][ACTIVE][0].head;
	else if (blockFSMTable->bfsmEntry[dieNo][ACTIVE][1].head != 0xffffffff)
		victimBlock = blockFSMTable->bfsmEntry[dieNo][ACTIVE][1].head;
	else if (blockFSMTable->bfsmEntry[dieNo][ACTIVE][2].head != 0xffffffff)
		victimBlock = blockFSMTable->bfsmEntry[dieNo][ACTIVE][2].head;
	else
		// No block to garbage collect
		assert(!"[WARNING] There are no free blocks. Abort terminate this ssd. [WARNING]");

	// 2. Identify FREE block
	if (blockFSMTable->bfsmEntry[dieNo][FREE][0].head != 0xffffffff)
		freeBlock = blockFSMTable->bfsmEntry[dieNo][FREE][0].head;
	else if (blockFSMTable->bfsmEntry[dieNo][FREE][1].head != 0xffffffff)
		freeBlock = blockFSMTable->bfsmEntry[dieNo][FREE][1].head;
	else if (blockFSMTable->bfsmEntry[dieNo][FREE][2].head != 0xffffffff)
		freeBlock = blockFSMTable->bfsmEntry[dieNo][FREE][2].head;
	else
		// No more free blocks
		assert(!"[WARNING] There are no free blocks. Abort terminate this ssd. [WARNING]");

	// 3. Copy valid pages from victim block to the free block, if victim was an ACTIVE block
	struct bstEntry victimBlkEntry = blockStatusTable->bstEntry[dieNo][victimBlock];

	if (victimBlkEntry.s == ACTIVE) {
		for(j=0 ; j<PAGE_NUM_PER_BLOCK ; j++)
		{
			if(pageMap->pmEntry[dieNo][(victimBlock * PAGE_NUM_PER_BLOCK) + j].valid)
			{
				// page copy process
				u32 validPage = victimBlock*PAGE_NUM_PER_BLOCK + j;
				// u32 freeBlock = dieBlock->dieEntry[dieNo].freeBlock;
				u32 freePage = freeBlock*PAGE_NUM_PER_BLOCK + blockStatusTable->bstEntry[dieNo][freeBlock].currentPage;

				WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
				SsdRead(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, validPage, GC_BUFFER_ADDR);
				WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
				SsdProgram(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, freePage, GC_BUFFER_ADDR);

				// pageMap, blockStatusTable update
				u32 lpn = pageMap->pmEntry[dieNo][validPage].lpn;

				pageMap->pmEntry[dieNo][lpn].ppn = freePage;
				pageMap->pmEntry[dieNo][freePage].lpn = lpn;
				blockStatusTable->bstEntry[dieNo][freeBlock].currentPage++;
			}
		}
	}

	// Move free block to the ACTIVE_FREE list
	struct bstEntry free_blk_entry = blockStatusTable->bstEntry[dieNo][freeBlock];
	MoveBSTEntry(dieNo, freeBlock, free_blk_entry.s, free_blk_entry.curBin, ACTIVE_FREE, free_blk_entry.curBin);

	// Erase the victim block
	EraseBlock(dieNo, victimBlock);

	return victimBlock;
}

void FlushPageBuf(u32 lpn, u32 bufAddr)
{
	if (lpn == 0xffffffff)
		return;

	u32 dieNo = lpn % DIE_NUM;
	u32 dieLpn = lpn / DIE_NUM;
	u32 ppn = pageMap->pmEntry[dieNo][dieLpn].ppn;

	if (ppn == 0xffffffff)
	{
		u32 freePageNo = FindFreePage(dieNo);

//		xil_printf("free page: %6d(%d, %d, %4d)\r\n", freePageNo, dieNo%CHANNEL_NUM, dieNo/CHANNEL_NUM, freePageNo/PAGE_NUM_PER_BLOCK);

		WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);
		SsdProgram(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM, freePageNo, bufAddr);
		WaitWayFree(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM);

		// pageMap update
		pageMap->pmEntry[dieNo][dieLpn].ppn = freePageNo;
		pageMap->pmEntry[dieNo][freePageNo].lpn = dieLpn;
	}
}

/* Update metadata in case of writes. Update the block status table and the
 * block FSM table.
 *
 * We check which physical page and block the corresponding logical page that was
 * just written is mapped to, and then use that information to change our Block FSM
 * Table. Note that there are a complex set of cases for which we have to move the
 * bstEntry between the different state-bin pairs of the BFSMT. Each case is listed below:
 *
 * 1. Block is in FREE state:
 *    - Here, regardless of the bin number we move the block into the ACTIVE FREE state,
 *    checking which bin to go into according to its erase count.
 * 2. Block is in ACTIVE FREE state:
 *    - Here, we check whether the block should be moved into the ACTIVE state,
 *    based on whether the block is full or not.
 * 3. Block is in ACTIVE state:
 *    - This should never occur! Throw out error message.
 */
void UpdateMetaForOverwrite(u32 lpn)
{
	pageMap = (struct pmArray*)(PAGE_MAP_ADDR);
	blockStatusTable = (struct bstArray*)(BST_ADDR);
	blockFSMTable = (struct bfsmTable*)(BFSM_ADDR);
	gcMap = (struct gcArray*)(GC_MAP_ADDR);

	u32 dieNo = lpn % DIE_NUM;
	u32 dieLpn = lpn / DIE_NUM;

	if ( pageMap->pmEntry[dieNo][dieLpn].ppn != 0xffffffff ) {

		// GC victim block list management
		// The physical block number in which an update occured.
		u32 diePbn = pageMap->pmEntry[dieNo][dieLpn].ppn / PAGE_NUM_PER_BLOCK;

		state block_s = blockStatusTable->bstEntry[dieNo][diePbn].s;
		unsigned char curBin = blockStatusTable->bstEntry[dieNo][diePbn].curBin;
		u32 curPage = blockStatusTable->bstEntry[dieNo][diePbn].currentPage;

		// If there was a previous page mapping, invalidate it and update valid page count.
		if(pageMap->pmEntry[dieNo][dieLpn].ppn != 0xffffffff)
		{
			// invalidation update
			pageMap->pmEntry[dieNo][pageMap->pmEntry[dieNo][dieLpn].ppn].valid = 0;
			blockStatusTable->bstEntry[dieNo][diePbn].validPageCnt--;
		}

		u32 nValidPages = blockStatusTable->bstEntry[dieNo][diePbn].validPageCnt;

		if (block_s == FREE) {
			MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE_FREE, curBin);
		}
		else if ( block_s == ACTIVE_FREE && curPage == PAGE_NUM_PER_BLOCK - 1 ) {
			if (nValidPages < ( PAGE_NUM_PER_BLOCK >> 2 ))
				MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE, 0);
			else if ( nValidPages < ( PAGE_NUM_PER_BLOCK >> 1 ) )
				MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE, 1);
			else
				MoveBSTEntry( dieNo, diePbn, block_s, curBin, ACTIVE, 2);
		}
		else if ( block_s != ACTIVE_FREE){
			xil_printf("ERROR! No block other than FREE or ACTIVE FREE should be written to.\r\n");
			xil_printf("WRITTEN BLOCK (dieNo, state, bin): (%d, %d, %d)", dieNo, block_s, curBin);
		}
	}

}

/* Helper function to reorganize the DLL data structure of the BFSMT
 * Changes the BFSMT entry and the state and bin of each block.
 */
void MoveBSTEntry(u32 dieNo, u32 blockNo, state curState, unsigned char curBin, state nextState, unsigned char nextBin) {

	char* cur_state_str;
	char* next_state_str;

	switch (curState)
	{
		case FREE:
			cur_state_str = "FREE";
			break;
		case ACTIVE_FREE:
			cur_state_str = "ACTIVE_FREE";
			break;
		case ACTIVE:
			cur_state_str = "ACTIVE";
			break;
		case INACTIVE:
			cur_state_str = "INACTIVE";
			break;
		case BAD:
			cur_state_str = "BAD";
			break;
		default:
			cur_state_str = "UNKNOWN";
	}

	switch (nextState)
	{
		case FREE:
			next_state_str = "FREE";
			break;
		case ACTIVE_FREE:
			next_state_str = "ACTIVE_FREE";
			break;
		case ACTIVE:
			next_state_str = "ACTIVE";
			break;
		case INACTIVE:
			next_state_str = "INACTIVE";
			break;
		case BAD:
			next_state_str = "BAD";
			break;
		default:
			next_state_str = "UNKNOWN";
	}

	xil_printf("Moving (%d, %d) from [%s, %c] to [%s, %c]\r\n", dieNo, blockNo, cur_state_str, curBin, next_state_str, nextBin);
	// Move block entry from curState, curBin to nextState, nextBin.
	//
	// There are four possible conditions for removal from linked list:
	// 1. bstEntry is in middle of linked list
	// 2. bstEntry is in end of linked list
	// 3. bstEntry is in beginning of linked list
	// 4. bstEntry is the only member of linked list

	// prev block of current block
	u32 pBlock = blockStatusTable->bstEntry[dieNo][blockNo].prevBlock;
	// next block of current block
	u32 nBlock = blockStatusTable->bstEntry[dieNo][blockNo].nextBlock;

	if((nBlock != 0xffffffff) && (pBlock != 0xffffffff))
	{
		/* Before:
		 *        HEAD                                                          TAIL
		 * 		  ... | PrevBlock | < --- > | CurBlock | < --- > | NextBlock | ...
		 *
		 * After: HEAD                                    TAIL
		 *        ... | PrevBlock | < --- > | NextBlock | ...
		 */
		xil_printf("MoveBSTEntry: bstEntry in middle of DLL\r\n");
		blockStatusTable->bstEntry[dieNo][pBlock].nextBlock = nBlock;
		blockStatusTable->bstEntry[dieNo][nBlock].prevBlock = pBlock;
	}
	else if((nBlock == 0xffffffff) && (pBlock != 0xffffffff))
	{
		/* Before:
		 *          HEAD                                    TAIL
		 * 		    ...   < --- > | PrevBlock | < --- > | CurBlock | --- > NULL
		 *
		 * After:   HEAD               TAIL
		 *          ...   < --- > | PrevBlock | --- > NULL
		 */
		xil_printf("MoveBSTEntry: bstEntry in end of DLL\r\n");
		blockStatusTable->bstEntry[dieNo][pBlock].nextBlock = 0xffffffff;
		blockFSMTable->bfsmEntry[dieNo][curState][curBin].tail = pBlock;
	}
	else if((nBlock != 0xffffffff) && (pBlock == 0xffffffff))
	{
		/* Before:
		 *                         HEAD                                    TAIL
		 * 		    NULL < --- | CurBlock | < --- > | NextBlock | < --- > ...
		 *
		 * After:                                        HEAD              TAIL
		 *                               NULL < --- | NextBlock |  < --- > ...
		 */
		xil_printf("MoveBSTEntry: bstEntry in start of DLL\r\n");
		blockStatusTable->bstEntry[dieNo][nBlock].prevBlock = 0xffffffff;
		blockFSMTable->bfsmEntry[dieNo][curState][curBin].head = nBlock;
	}
	else
	{
		/* Before:
		 *                     HEAD == TAIL
		 * 		    NULL < --- | CurBlock | --- > NULL
		 *
		 * After:   HEAD = NULL, TAIL = NULLL
		 *                 EMPTY
		 */
		xil_printf("MoveBSTEntry: bstEntry is the only entry\r\n");
		blockFSMTable->bfsmEntry[dieNo][curState][curBin].head = 0xffffffff;
		blockFSMTable->bfsmEntry[dieNo][curState][curBin].tail = 0xffffffff;
	}


	// Move block entry to next state, next bin
	if (blockFSMTable->bfsmEntry[dieNo][nextState][nextBin].tail == 0xffffffff) {
		blockStatusTable->bstEntry[dieNo][blockNo].nextBlock = 0xffffffff;
		blockStatusTable->bstEntry[dieNo][blockNo].prevBlock = 0xffffffff;
		blockStatusTable->bstEntry[dieNo][blockNo].s = nextState;
		blockStatusTable->bstEntry[dieNo][blockNo].curBin = nextBin;
		blockFSMTable->bfsmEntry[dieNo][nextState][nextBin].head = blockNo;
		blockFSMTable->bfsmEntry[dieNo][nextState][nextBin].tail = blockNo;
	} else {
		blockStatusTable->bstEntry[dieNo][blockNo].nextBlock = 0xffffffff;
		blockStatusTable->bstEntry[dieNo][blockNo].prevBlock = blockFSMTable->bfsmEntry[dieNo][nextState][nextBin].tail;
		blockStatusTable->bstEntry[dieNo][blockFSMTable->bfsmEntry[dieNo][nextState][nextBin].tail].nextBlock = blockNo;
		blockStatusTable->bstEntry[dieNo][blockNo].s = nextState;
		blockStatusTable->bstEntry[dieNo][blockNo].curBin = nextBin;
		blockFSMTable->bfsmEntry[dieNo][nextState][nextBin].tail = blockNo;
	}
}


//void MvData(u32* src, u32* dst, u32 sectSize)
//{
//	int i;
//	for (i = 0; i < sectSize*(SECTOR_SIZE/4); ++i)
//		dst[i] = src[i];
//}
