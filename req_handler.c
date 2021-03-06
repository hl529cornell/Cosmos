//////////////////////////////////////////////////////////////////////////////////
// req_handler.c for Cosmos OpenSSD
// Copyright (c) 2014 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//                Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//                Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//                Gyeongyong Lee <gylee@enc.hanyang.ac.kr>
//			 	  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
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
// Module Name: Request Handler
// File Name: req_handler.c
//
// Version: v2.2.0
//
// Description:
//   - Handling request commands.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v2.2.0
//   - page buffer initialization
//
// * v2.1.1
//   - Automatically calculate sector count
//
// * v2.1.0
//   - Support shutdown command (not ATA command)
//   - Move sector count information from driver to device firmware
//
// * v2.0.0
//   - support Tutorial FTL function
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include "xparameters.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xil_exception.h"
#include "xil_types.h"
#include "xaxicdma.h"
#include "xaxipcie.h"

#include "mem_map.h"
#include "ata.h"
#include "host_controller.h"
#include "identify.h"
#include "req_handler.h"

#include "ftl.h"
#include "pageMap.h"
#include "lld.h"

extern XAxiPcie devPcie;

extern XAxiCdma devCdma;

P_IDENTIFY_DEVICE_DATA pIdentifyData = (P_IDENTIFY_DEVICE_DATA)IDENTIFY_DEVICE_DATA_BASE_ADDR;
HOST_CMD hostCmd;
XAxiPcie_Config XAxiPcie_ConfigTable[] =
{
	{
		XPAR_PCI_EXPRESS_DEVICE_ID,
		XPAR_PCI_EXPRESS_BASEADDR,
		XPAR_PCI_EXPRESS_AXIBAR_NUM,
		XPAR_PCI_EXPRESS_INCLUDE_BAROFFSET_REG,
		XPAR_PCI_EXPRESS_INCLUDE_RC
	}
};

XAxiCdma_Config XAxiCdma_ConfigTable[] =
{
	{
		XPAR_AXI_CDMA_0_DEVICE_ID,
		XPAR_AXI_CDMA_0_BASEADDR,
		XPAR_AXI_CDMA_0_INCLUDE_DRE,
		XPAR_AXI_CDMA_0_USE_DATAMOVER_LITE,
		XPAR_AXI_CDMA_0_M_AXI_DATA_WIDTH,
		XPAR_AXI_CDMA_0_M_AXI_MAX_BURST_LEN
	}
};

void nextReadyCmd(P_CHILD_CMD *c){
	u32 diesReady[DIE_NUM];
	int i;
	for (i = 0; i < DIE_NUM; i++) {
		diesReady[i] = 0;
	}
	entity_t *curr = tQueue.head;
	entity_t *prev = NULL;
	u32 dieNo;
	while (curr != NULL){
		dieNo = (curr->cmd->reqInfo.CurSect / SECTOR_NUM_PER_PAGE) % DIE_NUM;
		if (!diesReady[dieNo] && SsdReadChWayStatus(dieNo % CHANNEL_NUM, dieNo / CHANNEL_NUM) == 0){
			if (prev == NULL) {
				queue_dequeue(&tQueue, c);
			} else {
				prev->next = curr->next;
				*c = curr->cmd;
				//free(curr);
			}
			return;
		} else {
			diesReady[dieNo] = 1;
			prev = curr;
			curr = curr->next;
		}
	}
	*c = NULL;
}

void ReqHandler(void)
{
	u32 deviceAddr;
	u32 reqSize, scatterLength;
	u32 checkRequest;
	u32 storageSize;

	//initialize controller registers
	Xil_Out32(CONFIG_SPACE_REQUEST_START, 0);
	Xil_Out32(CONFIG_SPACE_SHUTDOWN, 0);
	
	//initialize AXI bridge for PCIe
	XAxiPcie_CfgInitialize(&devPcie, XAxiPcie_ConfigTable, XPAR_PCI_EXPRESS_BASEADDR);

	//initialize CentralDMA
	XAxiCdma_CfgInitialize(&devCdma, XAxiCdma_ConfigTable, XPAR_AXI_CDMA_0_BASEADDR);

	InitIdentifyData(pIdentifyData);

	InitNandReset();
	InitFtlMapTable();

	xil_printf("Before initializing transaction queue \r\n");
	// Initialize Transaction Queue
	tQueue.head = NULL;
	tQueue.tail = NULL;
	tQueue.length = 0;

	xil_printf("Before initializing holding queue \r\n");
	// Initialize Holding Queue
	holdingQueue.head = NULL;
	holdingQueue.tail = NULL;
	holdingQueue.length = 0;

	printf("[ Initialization is completed. ]\r\n");

	storageSize = SSD_SIZE - FREE_BLOCK_SIZE - BAD_BLOCK_SIZE - METADATA_BLOCK_SIZE;
	xil_printf("[ Bad block size : %dMB. ]\r\n", BAD_BLOCK_SIZE);
	xil_printf("[ Storage size : %dMB. ]\r\n",storageSize);
	Xil_Out32(CONFIG_SPACE_SECTOR_COUNT, storageSize * Mebibyte);

	pageBufLpn = 0xffffffff;	// page buffer initialization

	while(1)
	{

		checkRequest = CheckRequest();

		if(checkRequest == 0)
		{
			//shutdown handling
			print("------ Shutdown ------\r\n");
		}
		else if (checkRequest == 1)
		{
			/*DebugPrint("CONFIG_SPACE_STATUS = 0x%x\n\r", 					Xil_In32(CONFIG_SPACE_STATUS));
			DebugPrint("CONFIG_SPACE_INTERRUPT_SET = 0x%x\n\r", 			Xil_In32(CONFIG_SPACE_INTERRUPT_SET));
			DebugPrint("CONFIG_SPACE_REQUEST_BASE_ADDR_U = 0x%x\n\r", 		Xil_In32(CONFIG_SPACE_REQUEST_BASE_ADDR_U));
			DebugPrint("CONFIG_SPACE_REQUEST_BASE_ADDR_L = 0x%x\n\r", 		Xil_In32(CONFIG_SPACE_REQUEST_BASE_ADDR_L));
			DebugPrint("CONFIG_SPACE_REQUEST_HEAD_PTR_SET = 0x%x\n\r", 		Xil_In32(CONFIG_SPACE_REQUEST_HEAD_PTR_SET));
			DebugPrint("CONFIG_SPACE_REQUEST_TAIL_PTR = 0x%x\n\r", 			Xil_In32(CONFIG_SPACE_REQUEST_TAIL_PTR));
			DebugPrint("CONFIG_SPACE_COMPLETION_BASE_ADDR_U = 0x%x\n\r", 	Xil_In32(CONFIG_SPACE_COMPLETION_BASE_ADDR_U));
			DebugPrint("CONFIG_SPACE_COMPLETION_BASE_ADDR_L = 0x%x\n\r", 	Xil_In32(CONFIG_SPACE_COMPLETION_BASE_ADDR_L));
			DebugPrint("CONFIG_SPACE_COMPLETION_HEAD_PTR = 0x%x\n\r",	 	Xil_In32(CONFIG_SPACE_COMPLETION_HEAD_PTR));*/

			GetRequestCmd(&hostCmd);

			hostCmd.CmdStatus = COMMAND_STATUS_SUCCESS;
			hostCmd.ErrorStatus = IDE_ERROR_NOTHING;



			// u32 lpn = hostCmd.reqInfo.CurSect / SECTOR_NUM_PER_PAGE;

			int numPages = (hostCmd.reqInfo.CurSect % SECTOR_NUM_PER_PAGE) + hostCmd.reqInfo.ReqSect;
			int numLoop = 0;
			
			CHILD_CMD cCmd;
			while (numPages > 0) {
				// divide up command
				cCmd.parent = &hostCmd;
				cCmd.reqInfo.Cmd = hostCmd.reqInfo.Cmd;
				cCmd.reqInfo.CurSect = (hostCmd.reqInfo.CurSect + (SECTOR_NUM_PER_PAGE * numLoop)) / SECTOR_NUM_PER_PAGE;
				// TODO: not sure what to do about these, they are used for the DMA transfer

				cCmd.reqInfo.HostScatterAddrL;
				cCmd.reqInfo.HostScatterAddrU;
				cCmd.reqInfo.HostScatterNum;

				// we want just enough to get to the next page
				cCmd.reqInfo.ReqSect = SECTOR_NUM_PER_PAGE - (cCmd.reqInfo.CurSect % SECTOR_NUM_PER_PAGE);

				if (tQueue.length == TQUEUE_MAX) {
					queue_append(&holdingQueue, &cCmd);
				} else {
					// Put command in transaction queue
					queue_append(&tQueue, &cCmd);
				}
				numLoop++;
				numPages -= SECTOR_NUM_PER_PAGE;
			}
			//TODO: What to do with the parent command? Where does it go?
		}

		if (tQueue.length > 0) {
			if (holdingQueue.length > 0) {
				while (tQueue.length < TQUEUE_MAX) {
					P_CHILD_CMD cmd;
					queue_dequeue(&holdingQueue, &cmd);
					queue_append(&tQueue, cmd);
				}
			}
			P_HOST_CMD commandToIssue;
			nextReadyCmd(&commandToIssue);

			if((commandToIssue->reqInfo.Cmd == IDE_COMMAND_WRITE_DMA) ||  (commandToIssue->reqInfo.Cmd == IDE_COMMAND_WRITE))
			{
				//				xil_printf("write(%d, %d)\r\n", hostCmd.reqInfo.CurSect, hostCmd.reqInfo.ReqSect);

				PrePmRead(commandToIssue, RAM_DISK_BASE_ADDR);

				deviceAddr = RAM_DISK_BASE_ADDR + (commandToIssue->reqInfo.CurSect % SECTOR_NUM_PER_PAGE)*SECTOR_SIZE;
				reqSize = commandToIssue->reqInfo.ReqSect * SECTOR_SIZE;
				scatterLength = commandToIssue->reqInfo.HostScatterNum;

				DmaHostToDevice(commandToIssue, deviceAddr, reqSize, scatterLength);

				PmWrite(commandToIssue, RAM_DISK_BASE_ADDR);

				CompleteCmd(commandToIssue);
			}

			else if((commandToIssue->reqInfo.Cmd == IDE_COMMAND_READ_DMA) || (commandToIssue->reqInfo.Cmd == IDE_COMMAND_READ))
			{
				//				xil_printf("read(%d, %d)\r\n", commandToIssue->reqInfo.CurSect, commandToIssue->reqInfo.ReqSect);

				PmRead(commandToIssue, RAM_DISK_BASE_ADDR);

				deviceAddr = RAM_DISK_BASE_ADDR + (commandToIssue->reqInfo.CurSect % SECTOR_NUM_PER_PAGE)*SECTOR_SIZE;
				reqSize = commandToIssue->reqInfo.ReqSect * SECTOR_SIZE;
				scatterLength = commandToIssue->reqInfo.HostScatterNum;

				DmaDeviceToHost(commandToIssue, deviceAddr, reqSize, scatterLength);

				CompleteCmd(commandToIssue);
			}
			else if( commandToIssue->reqInfo.Cmd == IDE_COMMAND_FLUSH_CACHE )
			{
				DebugPrint("flush command\r\n");
				CompleteCmd(commandToIssue);
			}
			else if( commandToIssue->reqInfo.Cmd == IDE_COMMAND_IDENTIFY )
			{
				reqSize = commandToIssue->reqInfo.ReqSect * SECTOR_SIZE;
				scatterLength = commandToIssue->reqInfo.HostScatterNum;

				DmaDeviceToHost(commandToIssue, IDENTIFY_DEVICE_DATA_BASE_ADDR, reqSize, scatterLength);
				CompleteCmd(commandToIssue);
			}
			else if( commandToIssue->reqInfo.Cmd == IDE_COMMAND_SET_FEATURE )
			{
				SetIdentifyData(pIdentifyData, commandToIssue);
				CompleteCmd(commandToIssue);
			}
			else if( commandToIssue->reqInfo.Cmd == IDE_COMMAND_SECURITY_FREEZE_LOCK )
			{
				SetIdentifyData(pIdentifyData, commandToIssue);
				CompleteCmd(commandToIssue);
			}
			else if( commandToIssue->reqInfo.Cmd == IDE_COMMAND_SMART )
			{
				DebugPrint("not support IDE_COMMAND_SMART:%x\r\n", commandToIssue->reqInfo.Cmd);
				commandToIssue->CmdStatus = COMMAND_STATUS_INVALID_REQUEST;
				CompleteCmd(commandToIssue);
			}
			else if( commandToIssue->reqInfo.Cmd == IDE_COMMAND_ATAPI_IDENTIFY )
			{
				DebugPrint("not support IDE_COMMAND_ATAPI_IDENTIFY:%x\r\n", commandToIssue->reqInfo.Cmd);
				commandToIssue->CmdStatus = COMMAND_STATUS_INVALID_REQUEST;
				CompleteCmd(commandToIssue);
			}
			else
			{
				DebugPrint("not support command:%x\r\n", commandToIssue->reqInfo.Cmd);
				commandToIssue->CmdStatus = COMMAND_STATUS_INVALID_REQUEST;
				CompleteCmd(commandToIssue);
			}
		}
	}
}

