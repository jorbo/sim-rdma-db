/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "rocev2.hpp"
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h> /* Added for the nonblocking socket */

#include "../axi_utils.hpp" //TODO why is this needed here
#include "rocev2_config.hpp"

using namespace hls;
#include "newFakeDram.hpp"

class fakeDRAM {
public:
	fakeDRAM() :writeState(CMD), readState(INIT) { memory = new ap_uint<8>[65536]; readAddr = 0; }

	template <int WIDTH>
	void process_writes(stream<memCmd>& cmdIn, stream<routed_net_axis<WIDTH> >& dataIn)//, stream<mmStatus>& statusOut)
	{
		//static fsmStateType writeState = CMD;
		memCmd cmd;
		routed_net_axis<WIDTH> inWord;
		//static uint16_t writeAddr;
		static int counter = 0;

		switch (writeState)
		{
			case CMD:
				if (!cmdIn.empty() && (CurrReq == IDLE || CurrReq == WRITES))
				{
					CurrReq = WRITES;
					cmdIn.read(cmd);
					writeAddr = cmd.addr(15, 0) * 4;
					uint16_t tempLen = (uint16_t) cmd.len(15, 0);
					writeLen = (int) tempLen;
					std::cout << "MEMORY WRITE, total length: " << std::dec << writeLen << std::endl;
					writeState = DATA;
				}
				break;
			case DATA: //TODO need to purge stuff here
				if (!dataIn.empty())
				{
					dataIn.read(inWord);
					counter++;
					std::cout << "MEMORY WRITE ADDR: " << std::hex << writeAddr/4 << ", dest: " << inWord.dest << ", counter: " << std::dec << counter << std::endl; //" length: " << keepToLen(inWord.keep) << std::endl;
					print(std::cout, inWord.data);
					std::cout << std::endl;
					for (int i = 0; i < (WIDTH/8); i++)
					{
							if (inWord.keep[i])
							{
								memory[writeAddr] = inWord.data((i*8)+7, i*8);
								writeAddr++;
							}
							else
							{
								break;
							}
					}
					if (inWord.last)
					{
						//mmStatus status;
						//status.okay = 1;
						//statusOut.write(status);
						CurrReq = IDLE;
						writeState = CMD;
					}
				}
				break;
		} //switch
	}
//annoying hack
	template <int WIDTH>
	void process_writes(stream<routedMemCmd>& cmdIn, stream<routed_net_axis<WIDTH> >& dataIn)//, stream<mmStatus>& statusOut)
	{
		//static fsmStateType writeState = CMD;
		routedMemCmd cmd;
		routed_net_axis<WIDTH> inWord;
		//static uint16_t writeAddr;
		static int counter = 0;

		switch (writeState)
		{
			case CMD:
				if (!cmdIn.empty() && (CurrReq == IDLE || CurrReq == WRITES))
				{
					CurrReq = WRITES;
					cmdIn.read(cmd);
					writeAddr = cmd.data.addr(15, 0) * 4;
					uint16_t tempLen = (uint16_t) cmd.data.len(15, 0);
					writeLen = (int) tempLen;
					std::cout << "MEMORY WRITE, total length: " << std::dec << writeLen << std::endl;
					writeState = DATA;
				}
				break;
			case DATA: //TODO need to purge stuff here
				if (!dataIn.empty())
				{
					dataIn.read(inWord);
					counter++;
					std::cout << "MEMORY WRITE ADDR: " << std::hex << writeAddr/4 << ", dest: " << inWord.dest << ", counter: " << std::dec << counter << std::endl; //" length: " << keepToLen(inWord.keep) << std::endl;
					print(std::cout, inWord.data);
					std::cout << std::endl;
					for (int i = 0; i < (WIDTH/8); i++)
					{
							if (inWord.keep[i])
							{
								memory[writeAddr] = inWord.data((i*8)+7, i*8);
							}
							// else
							// {
							// 	break;
							// }
							writeAddr++;
					}
					if (inWord.last)
					{
						//mmStatus status;
						//status.okay = 1;
						//statusOut.write(status);
						CurrReq = IDLE;
						writeState = CMD;
					}
				}
				break;
		} //switch
	}

	template <int WIDTH>
	void process_reads(stream<memCmd>& cmdIn, stream<net_axis<WIDTH> >& dataOut)//, stream<mmStatus>& status)
	{
		memCmd cmd;
		net_axis<WIDTH> outWord;
		//uint32_t count = 0;
		//uint32_t* uPtr;

		switch (readState)
		{
			case INIT:
				/*count = 13;
				uPtr = (uint32_t*)memory;
				for (int i = 0; i < 4096; i += 4)
				{
					*uPtr = count;
					uPtr++;
					count++;
				}*/
				readState = CMD;
			break;
			case CMD:
				if (!cmdIn.empty() && (CurrReq == IDLE || CurrReq == READS))
				{
					CurrReq = READS;
					cmdIn.read(cmd);
					readAddr = cmd.addr(15, 0) * 4;
					uint16_t tempLen = (uint16_t) cmd.len(15, 0);
					readLen = (int) tempLen;
					std::cout << "MEMORY READ, addr: " << readAddr << ", length" << readLen << std::endl;
					readState = DATA;
				}
				break;
			case DATA:
				int i = 0;
				outWord.keep = 0;
				while (readLen > 0 && i < (WIDTH/8))
				{
					outWord.data((i*8)+7, i*8) = memory[readAddr];
					outWord.keep = (outWord.keep << 1);
					outWord.keep++;
					readLen--;
					readAddr++;
					i++;
				}
				outWord.last = (readLen == 0);
				/*std::cout << "READ FROM MEMORY: ";
				print(std::cout, outWord);
				std::cout << std::endl;*/
				dataOut.write(outWord);
				if (outWord.last)
				{
					CurrReq = IDLE;
					readState = CMD;
				}
				break;
		} //switch
	}
//annoying hack
	template<int WIDTH>
	void process_reads(stream<routedMemCmd>& cmdIn, stream<net_axis<WIDTH> >& dataOut)//, stream<mmStatus>& status)
	{
		routedMemCmd cmd;
		net_axis<WIDTH> outWord;
		//uint32_t count = 0;
		//uint32_t* uPtr;

		switch (readState)
		{
			case INIT:
				/*count = 13;
				uPtr = (uint32_t*)memory;
				for (int i = 0; i < 4096; i += 4)
				{
					*uPtr = count;
					uPtr++;
					count++;
				}*/
				readState = CMD;
			break;
			case CMD:
				if (!cmdIn.empty() && (CurrReq == IDLE || CurrReq == READS))
				{
					CurrReq = READS;
					cmdIn.read(cmd);
					readAddr = cmd.data.addr(15, 0) * 4;
					uint16_t tempLen = (uint16_t) cmd.data.len(15, 0);
					readLen = (int) tempLen;
					std::cout << "MEMORY READ, addr: " << readAddr << ", length: " << readLen << " true length: " << cmd.data.len << std::endl;
					readState = DATA;
				}
				break;
			case DATA:
				int i = 0;
				outWord.keep = 0;
				while (readLen > 0 && i < (WIDTH/8))
				{
					outWord.data((i*8)+7, i*8) = memory[readAddr];
					outWord.keep = (outWord.keep << 1);
					outWord.keep++;
					readLen--;
					readAddr++;
					i++;
				}
				outWord.last = (readLen == 0);
				/*std::cout << "READ FROM MEMORY: ";
				print(std::cout, outWord);
				std::cout << std::endl;*/
				dataOut.write(outWord);
				if (outWord.last)
				{
					CurrReq = IDLE;
					readState = CMD;
				}
				break;
		} //switch
	}
private:
	enum fsmStateType {INIT, CMD, DATA};
	enum MemProc {IDLE, READS, WRITES}; //Janky fix to guarantee sequential read/write behavior
	MemProc CurrReq = IDLE;				//Could probably just have the two fsms check the others state
	fsmStateType writeState;
	fsmStateType readState;
	ap_uint<8>* memory;
	uint16_t writeAddr;
	uint16_t writeLen;
	uint16_t readAddr;
	uint16_t readLen;
};

int test_rx(fakeDRAM* memory, std::ifstream& inputFile, std::ofstream& outputFile, ap_uint<128>& local_ip_address, ap_uint<128>& remote_ip_address)
{
#pragma HLS inline region off

	stream<net_axis<128> >		s_axis_data128("testrx_s_axis_data128");
	stream<net_axis<DATA_WIDTH> >		s_axis_rx_data("s_axis_rx_data");
	//stream<ipUdpMeta>	m_axis_rx_meta("m_axis_rx_udp_meta");
	stream<net_axis<DATA_WIDTH> >		m_axis_rx_data("m_axis_rx_data");
	stream<txMeta>		s_axis_tx_meta("s_axis_tx_ibh_meta");
	stream<net_axis<DATA_WIDTH> >		s_axis_tx_data("s_axis_tx_data");
	stream<net_axis<DATA_WIDTH> >		m_axis_tx_data("m_axis_tx_data");
	stream<net_axis<128> >		m_axis_tx_data128("m_axis_tx_data128");
	//memory
	stream<routedMemCmd>		m_axis_mem_write_cmd("m_axis_mem_write_cmd");
	stream<routedMemCmd>		m_axis_mem_read_cmd("m_axis_mem_read_cmd");
	//stream<mmStatus>	s_axis_mem_write_status("s_axis_mem_write_status");
	stream<routed_net_axis<DATA_WIDTH> >		m_axis_mem_write_data("m_axis_mem_write_data");
	stream<net_axis<DATA_WIDTH> >		s_axis_mem_read_data("s_axis_mem_read_data");
	stream<routedMemCmd>				m_axis_local_write_cmd("m_axis_local_write_cmd");
	stream<routedMemCmd>				m_axis_local_read_cmd("m_axis_local_read_cmd");
	stream<routed_net_axis<DATA_WIDTH> >		m_axis_local_write_data("m_axis_local_write_data");
	stream<net_axis<DATA_WIDTH> >			s_axis_local_read_data("s_axis_local_read_data");
			
	
	//interface
	stream<qpContext>	s_axis_qp_interface("s_axis_qp_interface");
	stream<ifConnReq>	s_axis_qp_conn_interface("s_axis_qp_conn_interface");
	//pointer chasing
#ifdef POINTER_CHASING_EN
	stream<ptrChaseMeta>	m_axis_rx_pcmeta("m_axis_rx_pcmeta");
	stream<ptrChaseMeta>	s_axis_tx_pcmeta("s_axis_tx_pcmeta");
#endif
	ap_uint<32> regCrcDropPkgCount;
	ap_uint<32> regInvalidPsnDropCount;


	net_axis<128> inWord;

	//fix ip address
	local_ip_address(127, 96) = 0xD2D4010B;
	remote_ip_address(127, 96) = 0xD1D4010B;

	std::cout << "local ip address ";
	print(std::cout, local_ip_address);
	std::cout << std::endl;
	std::cout << "remote ip address ";
	print(std::cout, remote_ip_address);
	std::cout << std::endl;
	//Create initial QP
	qpContext context;
	context.newState = READY_RECV;
	context.qp_num = 0x11;
	context.remote_psn = 0x9dbe5d; //0x8c2a19d6;
	context.local_psn = 0x9dbe5d;
	context.r_key = 0x0;
	context.virtual_address = 0x0;
	s_axis_qp_interface.write(context);

	ifConnReq connInfo;
	connInfo.qpn = context.qp_num;
	connInfo.remote_qpn = 0x12; //TODO should not be the same
	connInfo.remote_ip_address = remote_ip_address;
	connInfo.remote_udp_port = 0x4853;
	s_axis_qp_conn_interface.write(connInfo);



	int count = 0;
	//Make sure it is initialized
	while (count < 10)
	{
		rocev2<DATA_WIDTH>(	s_axis_rx_data,
				//m_axis_rx_data,
				s_axis_tx_meta,
				s_axis_tx_data,
				m_axis_tx_data,
				m_axis_mem_write_cmd,
				m_axis_mem_read_cmd,
				//s_axis_mem_write_status,
				m_axis_mem_write_data,
				s_axis_mem_read_data,
				m_axis_local_write_cmd,
				m_axis_local_read_cmd,
				m_axis_local_write_data,
				s_axis_local_read_data,
				s_axis_qp_interface,
				s_axis_qp_conn_interface,
#if POINTER_CHASING_EN
				m_axis_rx_pcmeta,
				s_axis_tx_pcmeta,
#endif

				local_ip_address,
				regCrcDropPkgCount,
				regInvalidPsnDropCount);
		count++;
	}
	//start packet processing
	while (scan(inputFile, inWord))
	{
		s_axis_data128.write(inWord);
		convertStreamWidth<128, 25>(s_axis_data128, s_axis_rx_data);
		if (inWord.last)
			std::cout << "inWOrd last" << std::endl;

		rocev2<DATA_WIDTH>(	s_axis_rx_data,
				//m_axis_rx_data,
				s_axis_tx_meta,
				s_axis_tx_data,
				m_axis_tx_data,
				m_axis_mem_write_cmd,
				m_axis_mem_read_cmd,
				//s_axis_mem_write_status,
				m_axis_mem_write_data,
				s_axis_mem_read_data,
				m_axis_local_write_cmd,
				m_axis_local_read_cmd,
				m_axis_local_write_data,
				s_axis_local_read_data,
				s_axis_qp_interface,
				s_axis_qp_conn_interface,
#if POINTER_CHASING_EN
				m_axis_rx_pcmeta,
				s_axis_tx_pcmeta,
#endif
				local_ip_address,
				regCrcDropPkgCount,
				regInvalidPsnDropCount);
	}
	routedMemCmd local_write_cmd_packet;
	routedMemCmd local_read_cmd_packet;
	net_axis<DATA_WIDTH> 		local_read_data_packet;
	routed_net_axis<DATA_WIDTH> local_write_data_packet;

	//data.data = 108; //6C
	local_write_data_packet.data = 0x0;
	local_write_data_packet.keep = 0xff;
	local_write_data_packet.last = true;

	net_axis<DATA_WIDTH> local_read_data;
	//data.data = 108; //6C
	// local_read_data.data = 0x0;
	// local_read_data.keep = 0xff;
	// local_data.last = true;
	int i,j,k = 1000;

	while (count < 800000)
	{
	local_write_cmd_packet.data.addr = i;
	local_write_cmd_packet.data.len = 4; //
	local_read_cmd_packet.data.addr = j;
	// local_read_data_packet;
	// local_write_data_packet;

		convertStreamWidth<128, 25>(s_axis_data128, s_axis_rx_data);
		convertStreamWidth<DATA_WIDTH, 26>(m_axis_tx_data, m_axis_tx_data128);
		rocev2<DATA_WIDTH>(	s_axis_rx_data,
				//m_axis_rx_data,
				s_axis_tx_meta,
				s_axis_tx_data,
				m_axis_tx_data,
				m_axis_mem_write_cmd,
				m_axis_mem_read_cmd,
				//s_axis_mem_write_status,
				m_axis_mem_write_data,
				s_axis_mem_read_data,
				m_axis_local_write_cmd,
				m_axis_local_read_cmd,
				m_axis_local_write_data,
				s_axis_local_read_data,
				s_axis_qp_interface,
				s_axis_qp_conn_interface,
#if POINTER_CHASING_EN
				m_axis_rx_pcmeta,
				s_axis_tx_pcmeta,
#endif
				local_ip_address,
				regCrcDropPkgCount,
				regInvalidPsnDropCount);

		memory->process_writes<DATA_WIDTH>(m_axis_mem_write_cmd, m_axis_mem_write_data);
		// if(!m_axis_mem_write_cmd.empty()){
		// 	routedMemCmd checkCMD;
		// 	m_axis_mem_write_cmd.read(checkCMD);
		// 	std::out <<
		// }
		memory->process_reads<DATA_WIDTH>(m_axis_mem_read_cmd, s_axis_mem_read_data);
		count++;
	}

	net_axis<128> outWord;
	//outputFile << "[DATA]" << std::endl;
	while(!m_axis_tx_data128.empty())
	{
		m_axis_tx_data128.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
	}

	//TODO diff
	return 0;
}



// int test_tx_debug(std::ifstream& inputFile, std::ofstream& outputFile, ap_uint<128>& ip_address, ap_uint<128>& remote_ip_address)
// {
// #pragma HLS inline region off

// 	stream<net_axis<128> >		s_axis_data128("tx_s_axis_data128");
// 	stream<net_axis<256> >		s_axis_data256("tx_s_axis_data256");
// 	stream<net_axis<DATA_WIDTH> >		s_axis_rx_data("s_axis_rx_data");
// 	//stream<ipUdpMeta>	m_axis_rx_meta("m_axis_rx_udp_meta");
// 	//stream<net_axis<DATA_WIDTH> >		m_axis_rx_data("m_axis_rx_data");
// 	stream<txMeta>		s_axis_tx_meta("s_axis_tx_ibh_meta");
// 	stream<net_axis<DATA_WIDTH> >		s_axis_tx_data("s_axis_tx_data");
// 	stream<net_axis<DATA_WIDTH> >		m_axis_tx_data("m_axis_tx_data");
// 	stream<net_axis<128> >		m_axis_tx_data128("m_axis_tx_data128");
// 	stream<net_axis<256> >		m_axis_tx_data256("m_axis_tx_data256");
// 	//memory
// 	stream<routedMemCmd>		m_axis_mem_write_cmd("m_axis_mem_write_cmd");
// 	stream<routedMemCmd>		m_axis_mem_read_cmd("m_axis_mem_read_cmd");
// 	//stream<mmStatus>	s_axis_mem_write_status("s_axis_mem_write_status");
// 	stream<routed_net_axis<DATA_WIDTH> >		m_axis_mem_write_data("m_axis_mem_write_data");
// 	stream<net_axis<DATA_WIDTH> >		s_axis_mem_read_data("s_axis_mem_read_data");
// 	//interface
// 	stream<qpContext>	s_axis_qp_interface("s_axis_qp_interface");
// 	stream<ifConnReq>	s_axis_qp_conn_interface("s_axis_qp_conn_interface");
// 	//pointer chasing
// #if POINTER_CHASING_EN
// 	stream<ptrChaseMeta>	m_axis_rx_pcmeta("m_axis_rx_pcmeta");
// 	stream<ptrChaseMeta>	s_axis_tx_pcmeta("s_axis_tx_pcmeta");
// #endif
// 	stream<routedMemCmd>		m_axis_local_write_cmd("m_axis_local_write_cmd");
// 	stream<routedMemCmd>		m_axis_local_read_cmd("m_axis_local_read_cmd");
// 	stream<routed_net_axis<DATA_WIDTH> >	m_axis_local_write_data("m_axis_local_write_data");
// 	stream<net_axis<DATA_WIDTH> >	s_axis_local_read_data("s_axis_local_read_data");
// 	ap_uint<32> regCrcDropPkgCount;
// 	ap_uint<32> regInvalidPsnDropCount;


// 	net_axis<128> inWord;

	//fix ip address
	// ip_address(127, 96) = 0xD2D4010B;
	// remote_ip_address(127, 96) = 0xD1D4010B;

	// //Create initial QP
	// qpContext context;
	// context.newState = READY_RECV;
	// context.qp_num = 0x11;
	// //context.remote_psn = 0x8bcb01;
	// context.remote_psn = 0x9dbe5d; //0x8c2a19d6;
	// context.local_psn = 0x9dbe5d;
	// context.r_key = 0;
	// context.virtual_address = 0x0;
	// s_axis_qp_interface.write(context);

	// ifConnReq connInfo;
	// connInfo.qpn = 0x11;
	// connInfo.remote_qpn = 0x12;
	// connInfo.remote_ip_address = remote_ip_address;
	// connInfo.remote_udp_port = 0x12b7;
// 	s_axis_qp_conn_interface.write(connInfo);

// 	newFakeDRAM<DATA_WIDTH> memory;



// 	int count = 0;
// 	//Make sure it is initialized
// 	while (count < 10)
// 	{
// 		rocev2<DATA_WIDTH>(	s_axis_rx_data,
// 				//m_axis_rx_data,
// 				s_axis_tx_meta,
// 				s_axis_tx_data,
// 				m_axis_tx_data,
// 				m_axis_mem_write_cmd,
// 				m_axis_mem_read_cmd,
// 				//s_axis_mem_write_status,
// 				m_axis_mem_write_data,
// 				s_axis_mem_read_data,
// 				s_axis_qp_interface,
// 				s_axis_qp_conn_interface,
// #if POINTER_CHASING_EN
// 				m_axis_rx_pcmeta,
// 				s_axis_tx_pcmeta,
// #endif
// #if IP_VERSION == 6
// 				ip_address,
// #else
// 				ip_address(127,96),
// #endif
// 				regCrcDropPkgCount,
	// 			regInvalidPsnDropCount);
	// 	count++;
	// }
	//create packet
	//int pkgLen = 16384;
	// int pkgLen = 128;
	// uint64_t* dataPtr = (uint64_t*) memory.createChunk(pkgLen);
	// //populate chunk
	// uint64_t* uPtr = dataPtr;
	// for (int j = 0; j < pkgLen; j += 8)
	// {
	// 	*uPtr = j + 20;
	// 	uPtr++;
	// }

	//s_axis_tx_meta.write(txMeta(APP_WRITE, 0x11, 0x0, 0x0, 0x8));
	// net_axis<DATA_WIDTH> local_data;

	// //data.data = 108; //6C
	// local_data.data = 0x0;
	// local_data.keep = 0xff;
	// data.last = true; 	

	// s_axis_tx_data.write(data);

	// s_axis_tx_meta.write(txMeta(APP_WRITE, 0x11, 0x0, 0x0 + CACHE_SIZE, 0x8));

	// data.data = 198; //C6
	// data.keep = 0xff;
	// data.last = true;

	// for(ap_uint<16> i = 0; i<CACHE_SIZE*2; i+=CACHE_SIZE){
	// 	s_axis_tx_meta.write(txMeta(APP_WRITE, 0x11, 0x0, i, 0x8));
	// 	data.data = i;
	// 	data.keep = 0xff;
	// 	data.last = true;
	// 	s_axis_tx_data.write(data);
	// }

	// data.data = 0xdeadbeefdeadbeef;
	// data.data = data.data << 64;
	// data.data += 0xaabbccddeeff1122;
	// data.data = data.data << 64;
	// data.data += 0x33445566778899ab;
	// data.data = data.data << 64;
	// data.data += 0xcdef123456789a44;
	// data.keep = 0xffffffff;
	// data.last = true; 	


	//process write packet
// 	int pkgCount = 0;
// 	count = 0;
// 	routedMemCmd writeCmd;
// 	routedMemCmd readCmd;
// 	bool writeCmdReady = false;
// 	int firstAcks = 0;
// 	int readCmdCounter = 0;
// 	while (count < 200000)
// 	{
// 		convertStreamWidth<128, 37>(s_axis_data128, s_axis_rx_data);
// 		convertStreamWidth<DATA_WIDTH, 36>(m_axis_tx_data, m_axis_tx_data128);

// /*		if (count >= 20 && count < (pkgLen/8)+20)
// 		{
// 			uint64_t value = pkgCount+20;
// 			pkgCount+=8;
// 			s_axis_tx_data.write(net_axis<DATA_WIDTH>(value, 0xFF, ((pkgCount % PMTU == 0) || pkgCount == pkgLen)));
// 			outputFile << "[count] " << pkgCount << "[value] " << value << std::endl;
// 			if (pkgCount % PMTU == 0)
// 			{
// 				int len = pkgLen-pkgCount;
// 				if (len > PMTU)
// 					len = PMTU;
// 				s_axis_tx_meta.write(txMeta(APP_WRITE, 0x11, 0, 0x019d5040, len));
// 				outputFile << "[count] " << pkgCount  << "[len] " << len << std::endl;
// 			}
// 		}
// */		if (count > 10000) {
// 			if (scan(inputFile, inWord))
// 			{
// 				s_axis_data128.write(inWord);
// 				if (inWord.last) {
// 					firstAcks++;
// 				}
// 			}
// 		}
// 		/*if (readCmdCounter > 1)
// 		{
// 			if (scan(inputFile, inWord))
// 			{
// 				s_axis_data128.write(inWord);
// 			}
// 		}*/
// 		rocev2<DATA_WIDTH>(	s_axis_rx_data,
// 				//m_axis_rx_data,
// 				s_axis_tx_meta,
// 				s_axis_tx_data,
// 				m_axis_tx_data,
// 				m_axis_mem_write_cmd,
// 				m_axis_mem_read_cmd,
// 				//s_axis_mem_write_status,
// 				m_axis_mem_write_data,
// 				s_axis_mem_read_data,
// 				s_axis_qp_interface,
// 				s_axis_qp_conn_interface,
// #if POINTER_CHASING_EN

// 				m_axis_rx_pcmeta,
// 				s_axis_tx_pcmeta,
// #endif
// #if IP_VERSION == 6
// 				ip_address,
// #else
// 				ip_address(127,96),
// #endif
// 				regCrcDropPkgCount,
// 				regInvalidPsnDropCount);

// 		//handle writes
// 		if (!m_axis_mem_write_cmd.empty() && !writeCmdReady)
// 		{
// 			m_axis_mem_write_cmd.read(writeCmd);
// 			writeCmdReady = true;
// 		}
// 		if (writeCmdReady && m_axis_mem_write_data.size() >= (writeCmd.data.len/8))
// 		{
// 			memory.processWrite(writeCmd, m_axis_mem_write_data);
// 			writeCmdReady = false;
// 		}
// 		//handle reads
// 		if (!m_axis_mem_read_cmd.empty())
// 		{
// 			std::cout << "read cmd: counter" << readCmdCounter << std::endl;
// 			m_axis_mem_read_cmd.read(readCmd);
// 			memory.processRead(readCmd, s_axis_mem_read_data);
// 			readCmdCounter++;
// 		}
// 		count++;
// 	}

// 	net_axis<128> outWord;
// 	//outputFile << "[DATA]" << std::endl;
// 	while(!m_axis_tx_data128.empty())
// 	{
// 		m_axis_tx_data128.read(outWord);
// 		print(outputFile, outWord);
// 		outputFile << std::endl;
// 	}

// 	//TODO diff
// 	return 0;
// }


// int test_tx_debug_1(std::ifstream& inputFile, std::ofstream& outputFile, ap_uint<128>& ip_address, ap_uint<128>& remote_ip_address)
// {
// #pragma HLS inline region off

// 	stream<net_axis<128> >		s_axis_data128("tx_s_axis_data128");
// 	stream<net_axis<256> >		s_axis_data256("tx_s_axis_data256");
// 	stream<net_axis<DATA_WIDTH> >		s_axis_rx_data("s_axis_rx_data");
// 	//stream<ipUdpMeta>	m_axis_rx_meta("m_axis_rx_udp_meta");
// 	//stream<net_axis<DATA_WIDTH> >		m_axis_rx_data("m_axis_rx_data");
// 	stream<txMeta>		s_axis_tx_meta("s_axis_tx_ibh_meta");
// 	stream<net_axis<DATA_WIDTH> >		s_axis_tx_data("s_axis_tx_data");
// 	stream<net_axis<DATA_WIDTH> >		m_axis_tx_data("m_axis_tx_data");
// 	stream<net_axis<128> >		m_axis_tx_data128("m_axis_tx_data128");
// 	stream<net_axis<256> >		m_axis_tx_data256("m_axis_tx_data256");
// 	//memory
// 	stream<routedMemCmd>		m_axis_mem_write_cmd("m_axis_mem_write_cmd");
// 	stream<routedMemCmd>		m_axis_mem_read_cmd("m_axis_mem_read_cmd");
// 	//stream<mmStatus>	s_axis_mem_write_status("s_axis_mem_write_status");
// 	stream<routed_net_axis<DATA_WIDTH> >		m_axis_mem_write_data("m_axis_mem_write_data");
// 	stream<net_axis<DATA_WIDTH> >		s_axis_mem_read_data("s_axis_mem_read_data");
// 	//interface
// 	stream<qpContext>	s_axis_qp_interface("s_axis_qp_interface");
// 	stream<ifConnReq>	s_axis_qp_conn_interface("s_axis_qp_conn_interface");
// 	//pointer chasing
// #if POINTER_CHASING_EN
// 	stream<ptrChaseMeta>	m_axis_rx_pcmeta("m_axis_rx_pcmeta");
// 	stream<ptrChaseMeta>	s_axis_tx_pcmeta("s_axis_tx_pcmeta");
// #endif
// 	ap_uint<32> regCrcDropPkgCount;
// 	ap_uint<32> regInvalidPsnDropCount;


// 	net_axis<128> inWord;

// 	//fix ip address
// 	ip_address(127, 96) = 0xD1D4010B;
// 	remote_ip_address(127, 96) = 0xD2D4010B;

// 	//Create initial QP
// 	qpContext context;
// 	context.newState = READY_RECV;
// 	context.qp_num = 0x12;
// 	//context.remote_psn = 0x8bcb01;
// 	context.remote_psn = 0x9dbe5d; //0x8c2a19d6;
// 	context.local_psn = 0x9dbe5d;
// 	context.r_key = 0;
// 	context.virtual_address = 0x0;
// 	s_axis_qp_interface.write(context);

// 	ifConnReq connInfo;
// 	connInfo.qpn = 0x12;
// 	connInfo.remote_qpn = 0x11;
// 	connInfo.remote_ip_address = remote_ip_address;
// 	connInfo.remote_udp_port = 0x1111;
// 	s_axis_qp_conn_interface.write(connInfo);

// 	newFakeDRAM<DATA_WIDTH> memory;



// 	int count = 0;
// 	//Make sure it is initialized
// 	while (count < 10)
// 	{
// 		rocev2<DATA_WIDTH>(	s_axis_rx_data,
// 				//m_axis_rx_data,
// 				s_axis_tx_meta,
// 				s_axis_tx_data,
// 				m_axis_tx_data,
// 				m_axis_mem_write_cmd,
// 				m_axis_mem_read_cmd,
// 				//s_axis_mem_write_status,
// 				m_axis_mem_write_data,
// 				s_axis_mem_read_data,
// 				s_axis_qp_interface,
// 				s_axis_qp_conn_interface,
// #if POINTER_CHASING_EN
// 				m_axis_rx_pcmeta,
// 				s_axis_tx_pcmeta,
// #endif
// 				ip_address,
// 				regCrcDropPkgCount,
// 				regInvalidPsnDropCount);
// 		count++;
// 	}
// 	//create packet
// 	//int pkgLen = 16384;
// 	int pkgLen = 128;
// 	uint64_t* dataPtr = (uint64_t*) memory.createChunk(pkgLen);
// 	//populate chunk
// 	uint64_t* uPtr = dataPtr;
// 	for (int j = 0; j < pkgLen; j += 8)
// 	{
// 		*uPtr = j + 20;
// 		uPtr++;
// 	}

// 	s_axis_tx_meta.write(txMeta(APP_READ, 0x12, 0x7fc292600000, pkgLen));

// 	//process write packet
// 	int pkgCount = 0;
// 	count = 0;
// 	routedMemCmd writeCmd;
// 	routedMemCmd readCmd;
// 	bool writeCmdReady = false;
// 	int firstAcks = 0;
// 	int readCmdCounter = 0;
// 	while (count < 50000)
// 	{
// 		convertStreamWidth<128, 37>(s_axis_data128, s_axis_rx_data);
// 		convertStreamWidth<DATA_WIDTH, 36>(m_axis_tx_data, m_axis_tx_data128);

// /*		if (count >= 20 && count < (pkgLen/8)+20)
// 		{
// 			uint64_t value = pkgCount+20;
// 			pkgCount+=8;
// 			s_axis_tx_data.write(net_axis<DATA_WIDTH>(value, 0xFF, ((pkgCount % PMTU == 0) || pkgCount == pkgLen)));
// 			outputFile << "[count] " << pkgCount << "[value] " << value << std::endl;
// 			if (pkgCount % PMTU == 0)
// 			{
// 				int len = pkgLen-pkgCount;
// 				if (len > PMTU)
// 					len = PMTU;
// 				s_axis_tx_meta.write(txMeta(APP_WRITE, 0x11, 0, 0x019d5040, len));
// 				outputFile << "[count] " << pkgCount  << "[len] " << len << std::endl;
// 			}
// 		}
// */		if (count > 10000) {
// 			if (scan(inputFile, inWord))
// 			{
// 				s_axis_data128.write(inWord);
// 				if (inWord.last) {
// 					firstAcks++;
// 				}
// 			}
// 		}
// 		/*if (readCmdCounter > 1)
// 		{
// 			if (scan(inputFile, inWord))
// 			{
// 				s_axis_data128.write(inWord);
// 			}
// 		}*/
// 		rocev2<DATA_WIDTH>(	s_axis_rx_data,
// 				//m_axis_rx_data,
// 				s_axis_tx_meta,
// 				s_axis_tx_data,
// 				m_axis_tx_data,
// 				m_axis_mem_write_cmd,
// 				m_axis_mem_read_cmd,
// 				//s_axis_mem_write_status,
// 				m_axis_mem_write_data,
// 				s_axis_mem_read_data,
// 				s_axis_qp_interface,
// 				s_axis_qp_conn_interface,
// #if POINTER_CHASING_EN

// 				m_axis_rx_pcmeta,
// 				s_axis_tx_pcmeta,
// #endif
// 				ip_address,
// 				regCrcDropPkgCount,
// 				regInvalidPsnDropCount);

// 		//handle writes
// 		if (!m_axis_mem_write_cmd.empty() && !writeCmdReady)
// 		{
// 			m_axis_mem_write_cmd.read(writeCmd);
// 			writeCmdReady = true;
// 		}
// 		if (writeCmdReady && m_axis_mem_write_data.size() >= (writeCmd.data.len/8))
// 		{
// 			memory.processWrite(writeCmd, m_axis_mem_write_data);
// 			writeCmdReady = false;
// 		}
// 		//handle reads
// 		if (!m_axis_mem_read_cmd.empty())
// 		{
// 			std::cout << "read cmd: counter" << readCmdCounter << std::endl;
// 			m_axis_mem_read_cmd.read(readCmd);
// 			memory.processRead(readCmd, s_axis_mem_read_data);
// 			readCmdCounter++;
// 		}
// 		count++;
// 	}

// 	net_axis<128> outWord;
// 	//outputFile << "[DATA]" << std::endl;
// 	while(!m_axis_tx_data128.empty())
// 	{
// 		m_axis_tx_data128.read(outWord);
// 		print(outputFile, outWord);
// 		outputFile << std::endl;
// 	}

// 	//TODO diff
// 	return 0;
// }



int main(int argc, char* argv[])
{
#pragma HLS inline region off

	// constants can only be up to 64bit
	//local address
	ap_uint<128>	reg_ip_address;
	//remote address
	ap_uint<128>	remote_ip_address;
#if IP_VERSION == 6
	//IPv6
	reg_ip_address(127, 64) = 0xfe80000000000000;
	reg_ip_address(63, 0)   = 0x020f53fffe089ff4;
	remote_ip_address(127, 64) = 0xfe80000000000000;
	remote_ip_address(63, 0)   = 0x92e2bafffe3a7665;
#else
	//IPv4
	reg_ip_address(127, 64) = 0xfe80000000000000;
	reg_ip_address(63, 0)   = 0x020f53ff0b01d4d1;
	remote_ip_address(127, 64) = 0xfe80000000000000;
	remote_ip_address(63, 0)   = 0x92e2baff0b01d4d2;
#endif
	ap_uint<128>	rev_ip_address = reverse(reg_ip_address);
	ap_uint<128>	rev_remote_ip_address = reverse(remote_ip_address);

	fakeDRAM dram;

	std::cout << "ip address";
	print(std::cout, reg_ip_address);
	std::cout << std::endl;

	std::cout << "rev address";
	print(std::cout, rev_ip_address);
	std::cout << std::endl;

	std::ifstream rxInputFile;
	std::ofstream rxOutputFile;
	std::ifstream txInputFile;
	std::ofstream txOutputFile;

	if (argc < 3)
	{
		std::cout << "[ERROR] missing arguments." << std::endl;
		return -1;
	}

	rxInputFile.open(argv[1]);
	if (!rxInputFile)
	{
		std::cout << "[ERROR] could not open test rx input file." << std::endl;
		return -1;
	}

	rxOutputFile.open(argv[2]);
	if (!rxOutputFile)
	{
		std::cout << "[ERROR] could not open test rx output file." << std::endl;
		return -1;
	}

	txInputFile.open(argv[3]);
	if (!txInputFile)
	{
		std::cout << "[ERROR] could not open test tx input file." << std::endl;
		return -1;
	}

	txOutputFile.open(argv[4]);
	if (!txOutputFile)
	{
		std::cout << "[ERROR] could not open test tx output file." << std::endl;
		return -1;
	}

	int ret = 0;
	// Test RX path
	ret = test_rx(&dram, rxInputFile, rxOutputFile, rev_ip_address, rev_remote_ip_address);
	//ret = get_network_input(&dram, rxInputFile, rxOutputFile);
	//ret = test_rx_nak(&dram, rxInputFile, rxOutputFile, rev_ip_address, rev_remote_ip_address);
	std::cout << "Test:\tRX path\t\t";
	if (ret == 0)
	{
		std::cout << "[PASSED]";
	}
	else
	{
		std::cout << "[FAILED]";
	}
	std::cout << std::endl << std::endl;

	//ret = test_tx_host(&dram, txInputFile, txOutputFile, rev_ip_address, rev_remote_ip_address);
	//ret = test_tx_latency(txInputFile, txOutputFile, rev_ip_address, rev_remote_ip_address);
	//ret = test_tx_read(txInputFile, txOutputFile, rev_ip_address, rev_remote_ip_address);
	// ret = test_tx_debug(txInputFile, txOutputFile, rev_ip_address, rev_remote_ip_address);
	std::cout << "Test:\tTX path\t\t";
	if (ret == 0)
	{
		std::cout << "[PASSED]";
	}
	else
	{
		std::cout << "[FAILED]";
	}
	std::cout << std::endl;


	std::cout << "Endinaness test" << std::endl;
	ap_uint<32> temp;
	temp(3, 0) = 0xFF;
	temp(31, 24) = 0x11;

	std::cout << "lowest addr: " << std::hex  << (uint16_t)(reinterpret_cast<char*>(&temp))[0] << std::endl;
	std::cout << "highest addr: " << std::hex << (uint16_t)(reinterpret_cast<char*>(&temp))[3] << std::endl;

	return 0;
}
