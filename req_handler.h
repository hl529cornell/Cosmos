//////////////////////////////////////////////////////////////////////////////////
// req_handler.h for Cosmos OpenSSD
// Copyright (c) 2014 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//                Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//                Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//                Gyeongyong Lee <gylee@enc.hanyang.ac.kr>
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
//
// Project Name: Cosmos OpenSSD
// Design Name: Greedy FTL
// Module Name: Request Handler
// File Name: req_handler.h
//
// Version: v1.0.0
//
// Description:
//   - Handling request commands.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#ifndef REQ_HANDLER_H_
#define REQ_HANDLER_H_

#include "host_controller.h"

// Reward defines
#define RD_RWD               -4.45
#define WR_RWD               -0.89
#define RELOC_RD_RWD         -4.9
#define RELOC_WR_RWD          2.57
#define ERASE_RWD             3.71
#define WAIT_BEFORE_ERASE_RWD 1.09

// Transaction Queue
P_HOST_CMD tQueue[128];
int head, tail;

// CMACs
int CMAC_1[10][10][10][10][10];

void ReqHandler(void);

#endif /* REQ_HANDLER_H_ */
