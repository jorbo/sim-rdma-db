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
#include "rocev2_config.hpp"
#include "rocev2.hpp"


//#define CACHE_DEBUG

//TODO is packet aligned to 32bytes??
//TODO move onto memory write path or where CRC is checked
template <int WIDTH>
void extract_icrc(	stream<net_axis<WIDTH> >&		input,
#ifdef DISABLE_CRC_CHECK
					stream<net_axis<WIDTH> >&		output)
#else
					stream<net_axis<WIDTH> >&		output,
					stream<ap_uint<32> >&	rx_crcFifo)
#endif
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum stateType {FIRST, PKG, LAST};
	static stateType ei_state = FIRST;
	static net_axis<WIDTH> ei_prevWord;
	ap_uint<32> crc;
	net_axis<WIDTH> currWord;

	switch (ei_state)
	{
	case FIRST:
		if (!input.empty())
		{
			input.read(currWord);
			ei_prevWord = currWord;
			ei_state = PKG;
			if (currWord.last)
			{
				ei_state = LAST;
			}
		}
		break;
	case PKG:
		if (!input.empty())
		{
			input.read(currWord);
			if (currWord.last)
			{
				if (currWord.keep[4] == 0)
				{
					ei_prevWord.last = 0x1;
					crc = currWord.data(31, 0);
#ifndef DISABLE_CRC_CHECK
					rx_crcFifo.write(crc);
#endif
					ei_state = FIRST;
				}
				else
				{
					ei_state = LAST;
				}
			}
			output.write(ei_prevWord);
			ei_prevWord = currWord;
		}
		break;
	case LAST:
		ap_uint<64> keep = ei_prevWord.keep; //this is required to make the case statement work for all widths
		switch(keep)
		{
		case 0xF:
			//This should not occur
			crc = ei_prevWord.data(31, 0);
			break;
		case 0xFF:
			crc = ei_prevWord.data(63, 32);
			ei_prevWord.keep(7,4) = 0x0;
			break;
		case 0xFFF:
			crc = ei_prevWord.data(95, 64);
			ei_prevWord.keep(11,8) = 0x0;
			break;
		case 0xFFFF:
			crc = ei_prevWord.data(127, 96);
			ei_prevWord.keep(15,12) = 0x0;
			break;
		case 0xFFFFF:
			crc = ei_prevWord.data(159, 128);
			ei_prevWord.keep(19,16) = 0x0;
			break;
		case 0xFFFFFF:
			crc = ei_prevWord.data(191, 160);
			ei_prevWord.keep(23,20) = 0x0;
			break;
		case 0xFFFFFFF:
			crc = ei_prevWord.data(223, 192);
			ei_prevWord.keep(27,24) = 0x0;
			break;
		case 0xFFFFFFFF:
			crc = ei_prevWord.data(255, 224);
			ei_prevWord.keep(31,28) = 0x0;
			break;
		case 0xFFFFFFFFF:
			crc = ei_prevWord.data(287, 256);
			ei_prevWord.keep(35,32) = 0x0;
			break;
		case 0xFFFFFFFFFF:
			crc = ei_prevWord.data(319, 288);
			ei_prevWord.keep(39,36) = 0x0;
			break;
		case 0xFFFFFFFFFFF:
			crc = ei_prevWord.data(351, 320);
			ei_prevWord.keep(43,40) = 0x0;
			break;
		case 0xFFFFFFFFFFFF:
			crc = ei_prevWord.data(383, 352);
			ei_prevWord.keep(47,44) = 0x0;
			break;
		case 0xFFFFFFFFFFFFF:
			crc = ei_prevWord.data(415, 384);
			ei_prevWord.keep(51,48) = 0x0;
			break;
		case 0xFFFFFFFFFFFFFF:
			crc = ei_prevWord.data(447, 416);
			ei_prevWord.keep(55,52) = 0x0;
			break;
		case 0xFFFFFFFFFFFFFFF:
			crc = ei_prevWord.data(479, 448);
			ei_prevWord.keep(59,56) = 0x0;
			break;
		case 0xFFFFFFFFFFFFFFFF:
			crc = ei_prevWord.data(511, 480);
			ei_prevWord.keep(63,60) = 0x0;
			break;
		} //switch
		output.write(ei_prevWord);
		std::cout << "CRC: " << std::hex << crc << std::endl;
#ifndef DISABLE_CRC_CHECK
		rx_crcFifo.write(crc);
#endif
		ei_state = FIRST;
		break;
	}

}

/*
 * Append ICRC over GRH, IBA Headers & Payload
 * RoCEv2: Replace in IPv6: traffic class, flow label, hop_limit, udp checksum with '1'
 * BTH: ignore Resv8a
 * hop limit is always FF
 */
template <int WIDTH, int DUMMY>
void mask_header_fields(stream<net_axis<WIDTH> >& input,
						stream<net_axis<WIDTH> >& dataOut,
						stream<net_axis<WIDTH> >& maskedDataOut)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	//mask containig all of these fields
	const static ap_uint<424> one_mask = 0;
	// traffic class
	one_mask(3, 0) = 0xF;
	one_mask(11, 8) = 0xF;
	// flow label
	one_mask(31, 12) = 0xFFFFF;
	// hop limit TODO: should already be xFF
	one_mask(63, 56) = 0xFF;

	// UDP checksum
	one_mask(383, 368) = 0xFFFF;
	// BTH Resv8a
	one_mask(423,416) = 0xFF;
	const static ap_uint<3> header_length = (424/WIDTH);
	static ap_uint<8> ai_wordCount = 0;

	net_axis<WIDTH> crcWord;
	net_axis<WIDTH> currWord;


	if (!input.empty())
	{
		input.read(currWord);
		crcWord = currWord;

		if (ai_wordCount < header_length)
		{
			//std::cout << "applied mask: " << ai_wordCount << ", range: (" << std::dec << (int) ((ai_wordCount+1)*WIDTH)-1 << "," << (int) (ai_wordCount*WIDTH) << ")" << std::endl;
			crcWord.data = crcWord.data | one_mask(((ai_wordCount+1)*WIDTH)-1, ai_wordCount*WIDTH);
		}
		else if (ai_wordCount == header_length)
		{
			//std::cout << "aaapplied mask: " << ai_wordCount << ", range: (" << std::dec << (int) 423 << "," << (int) (ai_wordCount*WIDTH) << ")" << std::endl;
			crcWord.data((424%WIDTH)-1 , 0) = crcWord.data((424%WIDTH)-1 , 0) | one_mask(423, ai_wordCount*WIDTH);
		}
		maskedDataOut.write(crcWord);
		dataOut.write(currWord);
		ai_wordCount++;
		if (currWord.last)
		{
			ai_wordCount = 0;
		}
	}
}

template <int WIDTH>
void drop_invalid_crc(	stream<net_axis<WIDTH> >& input,
						stream<ap_uint<32> >& crcFifo,
						stream<ap_uint<32> >& calcCrcFifo,
						stream<net_axis<WIDTH> >& output,
						ap_uint<32>& 	 regCrcDropPkgCount)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmType {META, FWD, DROP};
	static fsmType state = META;
	static ap_uint<32> droppedPackts = 0;

	ap_uint<32> act_crc;
	ap_uint<32> calc_crc;
	net_axis<WIDTH> currWord;

	switch (state)
	{
		case META:
			if (!crcFifo.empty() && !calcCrcFifo.empty())
			{
				crcFifo.read(act_crc);
				calcCrcFifo.read(calc_crc);
				calc_crc = ~calc_crc;
				std::cout << std::hex << "actual crc: " << act_crc << " exp crc: " << calc_crc << std::endl;
				if (act_crc == calc_crc)
				{
					state = FWD;
				}
				else
				{
					droppedPackts++;
					regCrcDropPkgCount = droppedPackts;
					std::cout << "CRC PKG DROPED" << std::endl;
					state = FWD; ///TODO hack to avoid CRC drops
				}
			}
			break;
		case FWD:
			if (!input.empty())
			{
				input.read(currWord);
				output.write(currWord);
				if (currWord.last)
				{
					state = META;
				}
			}
			break;
		case DROP:
			if (!input.empty())
			{
				input.read(currWord);
				if (currWord.last)
				{
					state = META;
				}
			}
			break;
	} //switch
}


template <int WIDTH, int DUMMY>
void compute_crc32(	stream<net_axis<WIDTH> >& input,
					stream<ap_uint<32> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum crcFsmStateType {FIRST, SECOND};
	static crcFsmStateType crcState = FIRST;
	const unsigned int polynomial = 0xEDB88320; //Ethernet polynomial: 0x04C11DB7 reversed
	static unsigned int crc = 0xdebb20e3; // 8 bytes of 0xFF with init crc 0xFFFFFFFF
	//static unsigned int crc = 0xFFFFFFFF;
	static unsigned int mask = 0;

	static net_axis<WIDTH> currWord;

	switch (crcState)
	{
	case FIRST:
		if (!input.empty())
		{
			input.read(currWord);
			/*std::cout << "CRC32:";
			print(std::cout, currWord);
			std::cout << std::endl;*/

			//std::cout << "byte: " << std::hex << (uint16_t) byte;// << std::endl;
			for (int i = 0; i < (WIDTH/8/2); i++)
			{
				#pragma HLS UNROLL
				if (currWord.keep[i])
				{
					crc ^= currWord.data(i*8+7, i*8);
					//std::cout << std::hex << std::setw(2) << (uint16_t) currWord.data(i*8+7, i*8);// << " ";
					//std::cout << std::dec <<  ((int) currWord.data(i*8+7, i*8)) << " ";

					for (int j = 0; j < 8; j++)
					{
						#pragma HLS UNROLL
						//crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
						mask = -(crc & 1);
						crc = (crc >> 1) ^ (polynomial & mask);
					}
				}
			}
			crcState = SECOND;
		}
		break;
	case SECOND:
		for (int i = (WIDTH/8/2); i < (WIDTH/8); i++)
		{
			#pragma HLS UNROLL
			if (currWord.keep[i])
			{
				crc ^= currWord.data(i*8+7, i*8);
				for (int j = 0; j < 8; j++)
				{
					#pragma HLS UNROLL
					mask = -(crc & 1);
					crc = (crc >> 1) ^ (polynomial & mask);
				}
			}
		}
		if (currWord.last)
		{
			output.write(crc);
			//std::cout << std::endl;
			std::cout << "CRC["<< DUMMY << "]: "<< std::hex << ~crc << std::endl;
			//reset
			crc = 0xdebb20e3;
			mask = 0;
		}
		crcState = FIRST;
		break;
		//std::cout << std::endl;
	} //switch

}


//packets are multiple of 4 bytes, crc is 4 bytes
template <int WIDTH>
void insert_icrc(
#ifndef DISABLE_CRC_CHECK
					stream<ap_uint<32> >& crcIn,
#endif
					stream<net_axis<WIDTH> >& input,
					stream<net_axis<WIDTH> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

#ifndef DISABLE_CRC_CHECK
	enum fsmState {CRC, FWD, POST};
	static fsmState ii_state = CRC;
#else
	enum fsmState {FWD, POST};
	static fsmState ii_state = FWD;
#endif
	static ap_uint<32> crc = 0xdeadbeef;

	net_axis<WIDTH> currWord;
	net_axis<WIDTH> sendWord;

	switch(ii_state)
	{
#ifndef DISABLE_CRC_CHECK
	case CRC:
		if (!crcIn.empty())
		{
			crcIn.read(crc);
			crc = ~crc;
			ii_state = FWD;
		}
		break;
#endif
	case FWD:
		if (!input.empty())
		{
			input.read(currWord);
			if (currWord.last)
			{
				//Check if word is full
				if (currWord.keep[(WIDTH/8)-1] == 1)
				{
					currWord.last = 0;
					ii_state = POST;
				}
				else
				{
#ifndef DISABLE_CRC_CHECK
					ii_state = CRC;
#endif
					ap_uint<64> keep = currWord.keep; //this is required to make the case statement work for all widths
					switch(keep)
					{
					case 0xF:
						currWord.data(63, 32) = crc;
						currWord.keep(7,4) = 0xF;
						break;
					case 0xFF:
						currWord.data(95, 64) = crc;
						currWord.keep(11,8) = 0xF;
						break;
					case 0xFFF:
						currWord.data(127, 96) = crc;
						currWord.keep(15,12) = 0xF;
						break;
					case 0xFFFF:
						currWord.data(159, 128) = crc;
						currWord.keep(19,16) = 0xF;
						break;
					case 0xFFFFF:
						currWord.data(191, 160) = crc;
						currWord.keep(23,20) = 0xF;
						break;
					case 0xFFFFFF:
						currWord.data(223, 192) = crc;
						currWord.keep(27,24) = 0xF;
						break;
					case 0xFFFFFFF:
						currWord.data(255, 224) = crc;
						currWord.keep(31,28) = 0xF;
						break;
					case 0xFFFFFFFF:
						currWord.data(287, 256) = crc;
						currWord.keep(35,32) = 0xF;
						break;
					case 0xFFFFFFFFF:
						currWord.data(319, 288) = crc;
						currWord.keep(39,36) = 0xF;
						break;
					case 0xFFFFFFFFFF:
						currWord.data(351, 320) = crc;
						currWord.keep(43,40) = 0xF;
						break;
					case 0xFFFFFFFFFFF:
						currWord.data(383, 352) = crc;
						currWord.keep(47,44) = 0xF;
						break;
					case 0xFFFFFFFFFFFF:
						currWord.data(415, 384) = crc;
						currWord.keep(51,48) = 0xF;
						break;
					case 0xFFFFFFFFFFFFF:
						currWord.data(447, 416) = crc;
						currWord.keep(55,52) = 0xF;
						break;
					case 0xFFFFFFFFFFFFFF:
						currWord.data(479, 448) = crc;
						currWord.keep(59,56) = 0xF;
						break;
					case 0xFFFFFFFFFFFFFFF:
						currWord.data(511, 480) = crc;
						currWord.keep(63,60) = 0xF;
						break;
					//case 0xFFFFFFFF:
						//TODO should not be reached
						//break;
					} //switch
				} //keep
			} //last
			output.write(currWord);
		}
		break;
	case POST:
		sendWord.data(31, 0) = crc;
		sendWord.data(WIDTH-1, 32) = 0;
		sendWord.keep(3, 0) = 0xF;
		sendWord.keep((WIDTH/8)-1, 4) = 0;
		sendWord.last = 1;
		output.write(sendWord);
#ifndef DISABLE_CRC_CHECK
		ii_state = CRC;
#else
		ii_state = FWD;
#endif
		break;
	} //switch
}

template <int WIDTH, int DUMMY>
void round_robin_arbiter(stream<net_axis<WIDTH> >& in,
						stream<net_axis<WIDTH> >& out1,
						stream<net_axis<WIDTH> >& out2)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static bool one = true;
	net_axis<WIDTH> currWord;

	if (!in.empty())
	{
		in.read(currWord);
		if (one)
		{
			out1.write(currWord);
		}
		else
		{
			out2.write(currWord);
		}
		if (currWord.last)
		{
			one = !one;
		}
	}
}

template <int DUMMY>
void round_robin_merger(stream<ap_uint<32> >& in1,
						stream<ap_uint<32> >& in2,
						stream<ap_uint<32> >& out)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static bool one = true;

	if (one)
	{
		if (!in1.empty())
		{
			out.write(in1.read());
			one = !one;
		}
	}
	else
	{
		if (!in2.empty())
		{
			out.write(in2.read());
			one = !one;
		}
	}
}

struct cache_block{
	ap_uint<32> data; //4 byte integer
	//Tag bits size = bits in mem address(at least 26 for 62000000) - bits to address cache(log2(128*128))
	ap_uint<12> tag; //note the actual address field in routedmemcmd is 64 bit, so either a larger cache or tag field might be necessary in the future
	bool valid;
	bool dirty;
	
};

template <int WIDTH>
void CACHE(
	//IB2C
	hls::stream<routedMemCmd>&				m_axis_cache_write_cmd,
	hls::stream<routedMemCmd>&				m_axis_cache_read_cmd,
	hls::stream<routed_net_axis<WIDTH> >&	m_axis_cache_write_data,
	hls::stream<net_axis<WIDTH> >&			s_axis_cache_read_data,
	//C2MEM
	hls::stream<routedMemCmd>&				m_axis_mem_write_cmd,
	hls::stream<routedMemCmd>&				m_axis_mem_read_cmd,
	hls::stream<routed_net_axis<WIDTH> >&	m_axis_mem_write_data,
	hls::stream<net_axis<WIDTH> >&			s_axis_mem_read_data,
	//LOC2C
	hls::stream<routedMemCmd>&				m_axis_local_write_cmd,
	hls::stream<routedMemCmd>&				m_axis_local_read_cmd,
	hls::stream<routed_net_axis<WIDTH> >&	m_axis_local_write_data,
	hls::stream<net_axis<WIDTH> >&			s_axis_local_read_data
){
	static cache_block BRAM_MEM_POOL [CACHE_SIZE]; //[128][128];
	enum cacheHandlerState{CMD, WRITE, DIRTY_CHECK, WRITE_BACK, WRITE_FETCH_LINE, READ};
	enum readDataDestination{INIT, LOCAL, REMOTE};
	enum cacheReturnState {WRITE_RET, READ_RET};
	static cacheHandlerState state = CMD;
	static readDataDestination return_stream = INIT;
	static cacheReturnState returnState = WRITE_RET;
	static routedMemCmd command;
	static routedMemCmd main_mem_cmd; 
	static routed_net_axis<WIDTH> write_data; 
	static net_axis<WIDTH>  read_data(0,0,0);
	static ap_uint<14> line_iterator;
	static ap_uint<1> fetchSent = 0;
	static ap_uint<64> starting_line_addr;

	// std::cout << "Entering cache" << std::endl << std::endl;

	static int iteration_check = 0; //Debugging - remove later
	static int write_flag = 0;



	static routedMemCmd writeCmd;
	static routed_net_axis<WIDTH> WR;
	WR.data = 0;

	switch(state) {

		case CMD: {
			//Look into better way to handle streams than chained else-if
			if(!m_axis_local_write_cmd.empty()){
				#ifdef CACHE_DEBUG
					std::cout << "Local write command recieved" << std::endl;
				#endif
				m_axis_local_write_cmd.read(command);
				write_flag = 1;
				//line_iterator = command.data.addr%CACHE_SIZE;
				state = WRITE;
			}
			else if (!m_axis_local_read_cmd.empty()){
				//TODO: Add enum for which stream to return
				#ifdef CACHE_DEBUG
					std::cout << "Local read command recieved" << std::endl;
				#endif
				m_axis_local_read_cmd.read(command);
				//line_iterator = command.data.addr%CACHE_SIZE;
				return_stream = LOCAL;
				state = READ; 
			}
			else if (!m_axis_cache_write_cmd.empty()) {

				#ifdef CACHE_DEBUG
					std::cout << "Write command recieved" << std::endl;
				#endif
				m_axis_cache_write_cmd.read(command);
				//line_iterator = command.data.addr%CACHE_SIZE;
				write_flag = 0;
				state = WRITE; 
			} else if (!m_axis_cache_read_cmd.empty()) {
				#ifdef CACHE_DEBUG
					std::cout << "Read command recieved" << std::endl;
				#endif
				m_axis_cache_read_cmd.read(command);
				state = READ; 
				return_stream = REMOTE;
			}
			break; 
		}

		case WRITE: {
			
            if(command.data.addr >> 14 == BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].tag){
				if(write_flag){
					if(!m_axis_local_write_data.empty()){
						m_axis_local_write_data.read(write_data);

						#ifdef CACHE_DEBUG
							std::cout << "Memory addr match: " << command.data.addr << std::endl;
							ap_uint<32> print_friendly_data = write_data.data & 0xFFFFFFFF;
							std::cout << "Attempting to write " << std::hex << print_friendly_data << " to block ["<<command.data.addr%CACHE_SIZE<<"]" << std::endl;
						
						#endif

						BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].data = write_data.data & 0xFFFFFFFF;
						BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].dirty = true;
						BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].valid = true;
						state = CMD;
                	}
				}
				else{
					if(!m_axis_cache_write_data.empty()){
						m_axis_cache_write_data.read(write_data);

						#ifdef CACHE_DEBUG
							std::cout << "Memory addr match: " << command.data.addr << std::endl;
							ap_uint<32> print_friendly_data = write_data.data & 0xFFFFFFFF;
							std::cout << "Attempting to write " << std::hex << print_friendly_data << " to block ["<<command.data.addr%CACHE_SIZE<<"]" << std::endl;
						
						#endif

						BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].data = write_data.data & 0xFFFFFFFF;
						BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].dirty = true;
						BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].valid = true;
						state = CMD;
					}
				}
            }
            else{
				#ifdef CACHE_DEBUG
					std::cout << "No match to mem addr in cache" << std::endl;
					std::cout << "Tag in block(decimal): " << BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].tag << std::endl; 
				
				#endif

                line_iterator = command.data.addr%CACHE_SIZE - command.data.addr%LINE_SIZE;
                state = DIRTY_CHECK;
                returnState = WRITE_RET;
            }
			break; 
		}
        case DIRTY_CHECK: {
            static ap_uint<1> dirty = 0;
            for(int i = 0; i < LINE_SIZE; i++){
                if(BRAM_MEM_POOL[line_iterator + i].dirty == true){
                    dirty = 1;
                    break;
                }
            }
            if(dirty){
                state = WRITE_BACK;
            }
            else{
				starting_line_addr = command.data.addr - command.data.addr%LINE_SIZE;
                state = WRITE_FETCH_LINE;
            }
        }

        case WRITE_BACK: {
			//write back
			
			

			std::cout << "Writing back.." <<std::endl;
			if(!m_axis_mem_write_cmd.full() && !m_axis_mem_write_data.full()){
				//Figure out the relative block position in the cache line, increment through the line and write every valid && dirty block
				
				// writeCmd.data.addr = (BRAM_MEM_POOL[line_iterator].tag << 14) + line_iterator;


				
				// writeCmd.data.len = 64; //could change to LINE_SIZE, likely have to move it to prev state
				// m_axis_mem_write_cmd.write(writeCmd);
                //Should make this be a single burst write of length LINE_SIZE*4 that happens once on the first transition to this state
				int tempLen = 0;
				int i = 0;


				#ifdef CACHE_DEBUG
						std::cout << "Writing back to address: " << std::hex << line_iterator << std::endl;
						// std::cout << "Cache_block["<< line_iterator <<"]" <<" tag: " <<BRAM_MEM_POOL[line_iterator].tag << std::endl;
				#endif
				while(!BRAM_MEM_POOL[line_iterator].valid && (line_iterator+1)%LINE_SIZE){
					line_iterator++;
				}

				writeCmd.data.addr = (BRAM_MEM_POOL[line_iterator].tag << 14) + line_iterator;
				
				while((line_iterator+1)%LINE_SIZE != 0 && i < 16){
					if(!BRAM_MEM_POOL[line_iterator+i].valid){
						WR.last = 1;
						break;
					}
					WR.data(31+(32*i), 32*i) = BRAM_MEM_POOL[line_iterator].data;
					WR.keep(3+(4*i), 4*i) = 0xF;
					tempLen+=4;
					i++;
					line_iterator++;
				}
				
				line_iterator += tempLen/4;
				writeCmd.data.len = tempLen;
				// m_axis_mem_write_cmd.write(writeCmd);
				// WR.keep = 0xFFFFFFFFFFFFFFFF;
                // line_iterator += 16;
				#ifdef CACHE_DEBUG
					std::cout << "write packet data: " << std::hex << WR.data << std::endl;
				#endif
				if((line_iterator+1)%LINE_SIZE == 0){
					//send fetch command on transition, so here
					
					starting_line_addr = command.data.addr - command.data.addr%LINE_SIZE;
					line_iterator -= LINE_SIZE - 1;
					WR.last = 1;
					state = WRITE_FETCH_LINE;
				}
				if(tempLen != 0){
					m_axis_mem_write_cmd.write(writeCmd);
					m_axis_mem_write_data.write(WR);
				}
			}
			break;
		}

        case WRITE_FETCH_LINE: {
            routedMemCmd fetchCmd(starting_line_addr, 4*LINE_SIZE); //possibly should conditionally load this data only once for efficiency
            if(!m_axis_mem_read_cmd.full() && !fetchSent){
                m_axis_mem_read_cmd.write(fetchCmd);
				#ifdef CACHE_DEBUG
					std::cout << "Fetching new line from main memory" <<std::endl;
					std::cout << "New line begins with addr: " << std::hex << starting_line_addr << std::endl;
					ap_uint<64> end_addr = starting_line_addr + LINE_SIZE -1;
					std::cout << "And ends with addr: " << std::hex << end_addr  << std::endl;
				#endif
                fetchSent = 1;
            }

            if(!s_axis_mem_read_data.empty()){
                net_axis<WIDTH> fetchedPacket;
                s_axis_mem_read_data.read(fetchedPacket);

				#ifdef CACHE_DEBUG
					std::cout << "mem_read_data data: " <<std::hex << fetchedPacket.data << std::endl; 
					std::cout << "mem_read_data keep bits: " << std::hex << fetchedPacket.keep << std::endl;
					std::cout << "mem_read_data last bit: " << fetchedPacket.last << std::endl;
				#endif


                for(int i = 0; i < 16; i++){
                    BRAM_MEM_POOL[line_iterator].data = (fetchedPacket.data >> i*32) & 0xFFFFFFFF; //hoping the memory manager doesn't do something weird with keep bits
                    BRAM_MEM_POOL[line_iterator].tag  = (starting_line_addr) >> 14;
                    BRAM_MEM_POOL[line_iterator].valid = true;
                    BRAM_MEM_POOL[line_iterator].dirty = false;

					#ifdef CACHE_DEBUG
						std::cout << "Cache_block[" << std::hex << line_iterator <<"]" << " data:" <<BRAM_MEM_POOL[line_iterator].data << std::endl;
						std::cout << "Cache_block["<< line_iterator <<"]" <<" tag: " <<BRAM_MEM_POOL[line_iterator].tag << std::endl;
					#endif
                    starting_line_addr+=4;
                    line_iterator++;

                }
                if(fetchedPacket.last==1){
					fetchSent = 0;
					if(returnState == WRITE_RET){
						state = WRITE;
					}
					else{
						state = READ;
					}
                }
				// #ifdef CACHE_DEBUG
				// 		std::cout << "Writing back to address: " << std::hex << line_iterator << " data:" <<BRAM_MEM_POOL[line_iterator].data << std::endl;
				// 		std::cout << "Cache_block["<< line_iterator <<"]" <<" tag: " <<BRAM_MEM_POOL[line_iterator].tag << std::endl;
				// #endif
            }
            break;
        }

		case READ: {
			//check that outgoing fifo isn't full
			if (!s_axis_cache_read_data.full()) {
				#ifdef CACHE_DEBUG
					std::cout << "Attempting to read addr: " << std::hex << command.data.addr << std::endl;
					ap_uint<26> addr2tag_checker = (command.data.addr & 0x3FFFFFF) >> 14;
					std::cout << "Command addr corresponds to tag: " << std::hex << addr2tag_checker << std::endl;
					std::cout << "Tag at corresponding block [" <<command.data.addr%CACHE_SIZE << "]" <<" is: " << std::hex << BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].tag << std::endl;
				#endif
				if ((BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].tag == ((command.data.addr & 0x3FFFFFF) >> 14)) && BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].valid == true) {
                    //if only single ints need to get returned at a time we can just return the 1 int packet immediately
					#ifdef CACHE_DEBUG
						std::cout << "Addr found in cache" << std::endl << "Value: " << std::hex << BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].data <<std::endl;
					#endif

                    read_data.data(31, 0) = BRAM_MEM_POOL[command.data.addr%CACHE_SIZE].data;
                    read_data.keep = 0xF;
                    read_data.last = 1;
					//LOC2C stream
					if(return_stream == LOCAL){
						s_axis_local_read_data.write(read_data);
					}
					//REM2C stream
					else if(return_stream == REMOTE){
						s_axis_cache_read_data.write(read_data);
					}
					state = CMD;
				}
				else{
				#ifdef CACHE_DEBUG
					std::cout << "Addr not found in cache. Fetching from main mem" << std::endl; 
				#endif

					line_iterator = command.data.addr%CACHE_SIZE - command.data.addr%LINE_SIZE; //Should require this to set the iterator to the correct block, but it's jumping forward...
					state = DIRTY_CHECK;
                    returnState = READ_RET;
				}
			}

			break;
		}

	}


}




template <int WIDTH>
void rocev2(hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,

				hls::stream<txMeta>&	s_axis_tx_meta,
				hls::stream<net_axis<WIDTH> >&	s_axis_tx_data,
				hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
				//Memory
				hls::stream<routedMemCmd>&		m_axis_mem_write_cmd,
				hls::stream<routedMemCmd>&		m_axis_mem_read_cmd,
				hls::stream<routed_net_axis<WIDTH> >&	m_axis_mem_write_data,
				hls::stream<net_axis<WIDTH> >&	s_axis_mem_read_data,
				
				hls::stream<routedMemCmd>&		m_axis_local_write_cmd,
				hls::stream<routedMemCmd>&		m_axis_local_read_cmd,
				hls::stream<routed_net_axis<WIDTH> >&	m_axis_local_write_data,
				hls::stream<net_axis<WIDTH> >&	s_axis_local_read_data,
				//Interface
				hls::stream<qpContext>&	s_axis_qp_interface,
				hls::stream<ifConnReq>&	s_axis_qp_conn_interface,

				//pointer chasing
#if POINTER_CHASING_EN
				hls::stream<ptrChaseMeta>&	m_axis_rx_pcmeta,
				hls::stream<ptrChaseMeta>&	s_axis_tx_pcmeta,
#endif

				ap_uint<128>		local_ip_address,

				//Debug output
				ap_uint<32>& 	 regCrcDropPkgCount,
				ap_uint<32>& 	 regInvalidPsnDropCount)
{
//IB2Cache streams
	static stream<routedMemCmd>		s_axis_cache_write_cmd;
	static stream<routedMemCmd>		s_axis_cache_read_cmd;
	static stream<routed_net_axis<WIDTH> > s_axis_cache_write_data;
	static stream<net_axis<WIDTH> >	m_axis_cache_read_data;

#pragma HLS INLINE

	//metadata fifos
	static stream<ipUdpMeta>	rx_ipUdpMetaFifo("rx_ipUdpMetaFifo");
	static stream<ipUdpMeta>	tx_ipUdpMetaFifo("tx_ipUdpMetaFifo");
	#pragma HLS STREAM depth=8 variable=rx_ipUdpMetaFifo
	#pragma HLS STREAM depth=2 variable=tx_ipUdpMetaFifo

	//IP
	static stream<net_axis<WIDTH> >		rx_crc2ipFifo("rx_crc2ipFifo");
	static stream<net_axis<WIDTH> >		rx_udp2ibFifo("rx_udp2ibFifo");
	static stream<net_axis<WIDTH> >		tx_ib2udpFifo("tx_ib2udpFifo");
	static stream<net_axis<WIDTH> >		tx_ip2crcFifo("tx_ip2crcFifo");
	#pragma HLS STREAM depth=2 variable=rx_crc2ipFifo
	#pragma HLS STREAM depth=2 variable=rx_udp2ibFifo
	#pragma HLS STREAM depth=2 variable=tx_ib2udpFifo
	#pragma HLS STREAM depth=2 variable=tx_ip2crcFifo

	static stream<ipMeta>	rx_ip2udpMetaFifo("rx_ip2udpMetaFifo");
	static stream<net_axis<WIDTH> >	rx_ip2udpFifo("rx_ip2udpFifo");
	//static stream<net_axis<WIDTH> >	rx_ip2udpFifo("rx_ip2udpFifo");
	static stream<ipMeta>	tx_udp2ipMetaFifo("tx_udp2ipMetaFifo");
	static stream<net_axis<WIDTH> >	tx_udp2ipFifo("tx_udp2ipFifo");
	#pragma HLS STREAM depth=2 variable=rx_ip2udpMetaFifo
	#pragma HLS STREAM depth=2 variable=rx_ip2udpFifo
	#pragma HLS STREAM depth=2 variable=tx_udp2ipMetaFifo
	#pragma HLS STREAM depth=2 variable=tx_udp2ipFifo

	/*
	 * IPv6 & UDP
	 */
#if IP_VERSION == 6
	ipv6(	rx_crc2ipFifo,
			rx_ip2udpMetaFifo,
			rx_ip2udpFifo,
			tx_udp2ipMetaFifo,
			tx_udp2ipFifo,
			tx_ip2crcFifo,
			local_ip_address);

	/*
	 * IPv4 & UDP
	 */
#else
	ipv4<WIDTH>(	rx_crc2ipFifo,
			rx_ip2udpMetaFifo,
			rx_ip2udpFifo,
			tx_udp2ipMetaFifo,
			tx_udp2ipFifo,
			tx_ip2crcFifo,
			local_ip_address(127,96),
			UDP_PROTOCOL);
#endif

	udp<WIDTH>(rx_ip2udpMetaFifo,
			rx_ip2udpFifo,
		rx_ipUdpMetaFifo,
		rx_udp2ibFifo,
		tx_ipUdpMetaFifo,
		tx_ib2udpFifo,
		tx_udp2ipMetaFifo,
		tx_udp2ipFifo,
		local_ip_address,
		RDMA_DEFAULT_PORT);

	/*
	 * IB PROTOCOL
	 */
	ib_transport_protocol<WIDTH>(		rx_ipUdpMetaFifo,
								rx_udp2ibFifo,
								//m_axis_rx_data,
								s_axis_tx_meta,
								s_axis_tx_data,
								tx_ipUdpMetaFifo,
								tx_ib2udpFifo,
								s_axis_cache_write_cmd,
								s_axis_cache_read_cmd,
								s_axis_cache_write_data,
								m_axis_cache_read_data,
								s_axis_qp_interface,
								s_axis_qp_conn_interface,
#if POINTER_CHASING_EN
								m_axis_rx_pcmeta,
								s_axis_tx_pcmeta,
#endif
								regInvalidPsnDropCount);


	/*
	*  Cache
	*/
	CACHE(
		//IB2C
		s_axis_cache_write_cmd,
		s_axis_cache_read_cmd,
		s_axis_cache_write_data,
		m_axis_cache_read_data,
		//C2MEM
		m_axis_mem_write_cmd,
		m_axis_mem_read_cmd,
		m_axis_mem_write_data,
		s_axis_mem_read_data,
		//LOC2C
		m_axis_local_write_cmd,
		m_axis_local_read_cmd,
		m_axis_local_write_data,
		s_axis_local_read_data
	);

	/*
	 * Check ICRC of incoming packets
	 */
#ifndef DISABLE_CRC_CHECK
	static stream<net_axis<WIDTH> > rx_dataFifo("rx_dataFifo");
	static stream<net_axis<WIDTH> > rx_maskedDataFifo("rx_maskedDataFifo");
	static stream<net_axis<WIDTH> > rx_maskedDataFifo1("rx_maskedDataFifo1");
	static stream<net_axis<WIDTH> > rx_maskedDataFifo2("rx_maskedDataFifo2");
	static stream<net_axis<WIDTH> > rx_crcDataFifo("rx_crcDataFifo");
	static stream<ap_uint<32> > rx_crcFifo("rx_crcFifo");
	static stream<ap_uint<32> > rx_calcCrcFifo("rx_calcCrcFifo");
	static stream<ap_uint<32> > rx_calcCrcFifo1("rx_calcCrcFifo1");
	static stream<ap_uint<32> > rx_calcCrcFifo2("rx_calcCrcFifo2");
	#pragma HLS STREAM depth=4 variable=rx_dataFifo
	#pragma HLS STREAM depth=4 variable=rx_maskedDataFifo
	#pragma HLS STREAM depth=192 variable=rx_maskedDataFifo1 //TODO maybe increase for better TP
	#pragma HLS STREAM depth=192 variable=rx_maskedDataFifo2
	#pragma HLS STREAM depth=512 variable=rx_crcDataFifo // 1536 bytes, 48 for WIDTH = 256
	#pragma HLS STREAM depth=32 variable=rx_crcFifo
	#pragma HLS STREAM depth=32 variable=rx_calcCrcFifo
	#pragma HLS STREAM depth=2 variable=rx_calcCrcFifo1
	#pragma HLS STREAM depth=2 variable=rx_calcCrcFifo2
#endif

#ifdef DISABLE_CRC_CHECK
	regCrcDropPkgCount = 0;
	extract_icrc(s_axis_rx_data, rx_crc2ipFifo);
//	net_axis<WIDTH> rx_crc2ip;
//	rx_crc2ipFifo.read(rx_crc2ip);
//        std::cout << "[rocev2] crc2ip: ";
//        print(std::cout, rx_crc2ip);
//        std::cout << std::endl;

#else
	extract_icrc(s_axis_rx_data, rx_dataFifo, rx_crcFifo);
	mask_header_fields<1>(rx_dataFifo, rx_crcDataFifo, rx_maskedDataFifo);
	round_robin_arbiter<1>(rx_maskedDataFifo, rx_maskedDataFifo1, rx_maskedDataFifo2);
	compute_crc32<1>(rx_maskedDataFifo1, rx_calcCrcFifo1);
	compute_crc32<2>(rx_maskedDataFifo2, rx_calcCrcFifo2);
	round_robin_merger<1>(rx_calcCrcFifo1, rx_calcCrcFifo2, rx_calcCrcFifo);
	drop_invalid_crc(rx_crcDataFifo, rx_crcFifo, rx_calcCrcFifo, rx_crc2ipFifo, regCrcDropPkgCount);
#endif

	/*
	 * Append ICRC after IPv6
	 */
#ifndef DISABLE_CRC_CHECK
	static stream<net_axis<WIDTH> > tx_maskedDataFifo("tx_maskedDataFifo");
	static stream<net_axis<WIDTH> > tx_maskedDataFifo1("tx_maskedDataFifo1");
	static stream<net_axis<WIDTH> > tx_maskedDataFifo2("tx_maskedDataFifo2");
	static stream<net_axis<WIDTH> > tx_crcDataFifo("tx_crcDataFifo");
	static stream<ap_uint<32> > crcFifo("crcFifo");
	static stream<ap_uint<32> > crcFifo1("crcFifo1");
	static stream<ap_uint<32> > crcFifo2("crcFifo2");
	#pragma HLS STREAM depth=4 variable=tx_maskedDataFifo
	#pragma HLS STREAM depth=128 variable=tx_maskedDataFifo1 //increase size for better TP
	#pragma HLS STREAM depth=128 variable=tx_maskedDataFifo2
	#pragma HLS STREAM depth=384 variable=tx_crcDataFifo // 1536 bytes, 48 for WIDTH = 256
	#pragma HLS STREAM depth=2 variable=crcFifo
	#pragma HLS STREAM depth=2 variable=crcFifo1
	#pragma HLS STREAM depth=2 variable=crcFifo2
#endif

#ifdef DISABLE_CRC_CHECK
	insert_icrc(tx_ip2crcFifo, m_axis_tx_data);
#else
	mask_header_fields<2>(tx_ip2crcFifo, tx_crcDataFifo, tx_maskedDataFifo);
	round_robin_arbiter<2>(tx_maskedDataFifo, tx_maskedDataFifo1, tx_maskedDataFifo2);
	compute_crc32<3>(tx_maskedDataFifo1, crcFifo1);
	compute_crc32<4>(tx_maskedDataFifo2, crcFifo2);
	round_robin_merger<2>(crcFifo1, crcFifo2, crcFifo);
	insert_icrc(crcFifo, tx_crcDataFifo, m_axis_tx_data);
#endif
}

void rocev2_top(
				//local streams
				stream<routedMemCmd>&		s_axis_local_bram_write_cmd,
				stream<routedMemCmd>&		s_axis_local_bram_read_cmd,
				stream<routed_net_axis<DATA_WIDTH> >&	s_axis_local_bram_write_data,
				stream<net_axis<DATA_WIDTH> >&	m_axis_local_bram_read_data,
				//stream<ipUdpMeta>&	s_axis_rx_meta,
				stream<net_axis<DATA_WIDTH> >&	s_axis_rx_data,
				//stream<net_axis<DATA_WIDTH> >&	m_axis_rx_data,

				stream<txMeta>&	s_axis_tx_meta,
				stream<net_axis<DATA_WIDTH> >&	s_axis_tx_data,
				stream<net_axis<DATA_WIDTH> >&	m_axis_tx_data,
				//Memory
				stream<routedMemCmd>&		m_axis_mem_write_cmd,
				stream<routedMemCmd>&		m_axis_mem_read_cmd,
				//stream<mmStatus>&	s_axis_mem_write_status,
				stream<routed_net_axis<DATA_WIDTH> >&	m_axis_mem_write_data,
				stream<net_axis<DATA_WIDTH> >&	s_axis_mem_read_data,

				//Interface
				stream<qpContext>&	s_axis_qp_interface,
				stream<ifConnReq>&	s_axis_qp_conn_interface,

				//pointer chasing
#if POINTER_CHASING_EN
				stream<ptrChaseMeta>&	m_axis_rx_pcmeta,
				stream<ptrChaseMeta>&	s_axis_tx_pcmeta,
#endif

				ap_uint<128>		local_ip_address,

				//Debug output
				ap_uint<32>& 	 regCrcDropPkgCount,
				ap_uint<32>& 	 regInvalidPsnDropCount)
{
#pragma HLS DATAFLOW disable_start_propagation
#pragma HLS INTERFACE ap_ctrl_none port=return

	//DATA
	#pragma HLS INTERFACE axis register port=s_axis_rx_data
	#pragma HLS INTERFACE axis register port=s_axis_tx_data
	#pragma HLS INTERFACE axis register port=s_axis_tx_data
	#pragma HLS INTERFACE axis register port=m_axis_tx_meta
	#pragma HLS INTERFACE axis register port=m_axis_tx_data
	#pragma HLS DATA_PACK variable=s_axis_tx_meta

	//MEMORY
	#pragma HLS INTERFACE axis register port=m_axis_mem_write_cmd
	#pragma HLS INTERFACE axis register port=m_axis_mem_read_cmd
	#pragma HLS INTERFACE axis register port=m_axis_mem_write_data
	#pragma HLS INTERFACE axis register port=s_axis_mem_read_data

	#pragma HLS INTERFACE axis register port=s_axis_local_bram_write_cmd
	#pragma HLS INTERFACE axis register port=s_axis_local_bram_read_cmd
	#pragma HLS INTERFACE axis register port=s_axis_local_bram_write_data
	#pragma HLS INTERFACE axis register port=m_axis_local_read_data

	//CONTROL
	#pragma HLS INTERFACE axis register port=s_axis_qp_interface
	#pragma HLS INTERFACE axis register port=s_axis_qp_conn_interface
	#pragma HLS DATA_PACK variable=s_axis_qp_interface
	#pragma HLS DATA_PACK variable=s_axis_qp_conn_interface

	//Pointer chasing
#if POINTER_CHASING_EN
	#pragma HLS INTERFACE axis register port=m_axis_rx_pcmeta
	#pragma HLS INTERFACE axis register port=s_axis_tx_pcmeta
	#pragma HLS DATA_PACK variable=m_axis_rx_pcmeta
	#pragma HLS DATA_PACK variable=s_axis_tx_pcmeta
#endif

	#pragma HLS INTERFACE ap_none register port=local_ip_address

	//DEBUG
	#pragma HLS INTERFACE ap_vld port=regCrcDropPkgCount

   rocev2<DATA_WIDTH>(			s_axis_rx_data,
								s_axis_tx_meta,
								s_axis_tx_data,
								m_axis_tx_data,
								m_axis_mem_write_cmd,
								m_axis_mem_read_cmd,
								m_axis_mem_write_data,
								s_axis_mem_read_data,
								s_axis_local_bram_write_cmd,
								s_axis_local_bram_read_cmd,
								s_axis_local_bram_write_data,
								m_axis_local_bram_read_data,
								s_axis_qp_interface,
								s_axis_qp_conn_interface,
								local_ip_address,
								regCrcDropPkgCount,
								regInvalidPsnDropCount);
}
