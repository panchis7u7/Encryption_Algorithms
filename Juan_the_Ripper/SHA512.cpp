/*----------------------------------------------------------------------------
MIT License

Copyright (c) 2019 Filip Dobrosavljevic

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
-----------------------------------------------------------------------------*/

#include "SHA512.hpp"
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <cstdint>
#include <sstream>

namespace Local {

	typedef unsigned long long uint64;
	__extension__ typedef unsigned __int128 uint128;
	
	SHA512::SHA512(){
	}
	
	SHA512::~SHA512(){
	}
	
	char* SHA512::hashSSL(char* msg){
		unsigned char hash[SHA512_DIGEST_LENGTH]; //64 bytes.
		SHA512_CTX sha512;
		SHA512_Init(&sha512);
		SHA512_Update(&sha512, msg, strlen(msg));
		SHA512_Final(hash, &sha512);
		std::stringstream ss;
		for (size_t i = 0; i < SHA512_DIGEST_LENGTH; i++)
		{
			ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
		}
		std::string cryptMsg = ss.str();
		char* shaMsg = (char*)malloc(sizeof(char)*cryptMsg.size());
		strncpy(shaMsg, cryptMsg.c_str(), cryptMsg.size());
		return shaMsg;
	}
	
	char* SHA512::hash(const char* input){
		size_t nBuffer; 	//amt of message blocks
		uint64** buffer; 	//message blocks of size 1024bits wtih 16 64bit words
		uint64* h = new uint64[8];
		buffer = preprocess((unsigned char*)input, nBuffer);
		process(buffer, nBuffer, h);
		freeBuffer(buffer, nBuffer);
		std::string msg = digest(h);
		char* shaMsg = (char*)malloc(sizeof(char)*msg.length());
		strncpy(shaMsg, msg.c_str(), msg.length());
		return shaMsg;
	}
	
	uint64** SHA512::preprocess(const unsigned char* input, size_t &nBuffer){
		size_t mLen = strlen((const char*) input);
		size_t kLen = (895-(mLen*8))%1024;
		nBuffer = (mLen*8+1+kLen+128) / 1024;
	
		uint64** buffer = new uint64*[nBuffer];
	
		for(size_t i=0; i<nBuffer; i++){
			buffer[i] = new uint64[SEQUENCE_LEN];
		}
	
		for(size_t i=0; i<nBuffer; i++){
			for(size_t j=0; j<SEQUENCE_LEN; j++){
				uint64 in = 0x0ULL;
				for(size_t k=0; k<8; k++){
					if(i*128+j*8+k < mLen){
						in = in<<8 | (uint64)input[i*128+j*8+k];
					}else if(i*128+j*8+k == mLen){
						in = in<<8 | 0x80ULL;
					}else{
						in = in<<8 | 0x0ULL;
					}
				}
				buffer[i][j] = in;
			}
		}
	
		
		appendLen(mLen, 8, buffer[nBuffer-1][SEQUENCE_LEN-1], buffer[nBuffer-1][SEQUENCE_LEN-2]);
		return buffer;
	}
	
	void SHA512::process(uint64** buffer, size_t nBuffer, uint64* h){
		uint64 s[8];
		uint64 w[80];
	
		memcpy(h, hPrime, 8*sizeof(uint64));
	
		for(size_t i=0; i<nBuffer; i++){
			//message schedule
			memcpy(w, buffer[i], 16*sizeof(uint64));
	
			for(size_t j=16; j<80; j++){
				w[j] = w[j-16] + sig0(w[j-15]) + w[j-7] + sig1(w[j-2]);
			}
			//init
			memcpy(s, h, 8*sizeof(uint64));
			//compression
			for(size_t j=0; j<80; j++){
				uint64 temp1 = s[7] + Sig1(s[4]) + Ch(s[4], s[5], s[6]) + k[j] + w[j];
				uint64 temp2 = Sig0(s[0]) + Maj(s[0], s[1], s[2]);
	
				s[7] = s[6];
				s[6] = s[5];
				s[5] = s[4];
				s[4] = s[3] + temp1;
				s[3] = s[2];
				s[2] = s[1];
				s[1] = s[0];
				s[0] = temp1 + temp2;
			}
	
			for(size_t j=0; j<8; j++){
				h[j] += s[j];
			}
		}
	
	}
	
	void SHA512::appendLen(uint64 mLen, uint64 mp, uint64& lo, uint64& hi){
		uint128 prod = mLen*mp;
		lo = prod;
		hi = prod>>64;
	}
	
	std::string SHA512::digest(uint64* h){
		std::stringstream ss;
		for(size_t i=0; i<8; i++){
			ss << std::hex << h[i];
		}
		delete[] h;
		return ss.str();
	}
	
	void SHA512::freeBuffer(uint64** buffer, size_t nBuffer){
		for(size_t i=0; i<nBuffer; i++){
			delete[] buffer[i];
		}
	
		delete[] buffer;
	}

}