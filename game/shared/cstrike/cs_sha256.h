#ifndef CS_SHA256_H
#define CS_SHA256_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Small, allocation-free SHA-256 implementation shared by the two game DLLs.
class CSSha256
{
public:
	CSSha256() : m_Bytes(0), m_Used(0)
	{
		const uint32_t initial[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
		memcpy(m_State,initial,sizeof(initial));
	}
	void Update(const void *data, size_t length)
	{
		const unsigned char *bytes = (const unsigned char *)data;
		m_Bytes += length;
		while (length)
		{
			size_t count = length < 64-m_Used ? length : 64-m_Used;
			memcpy(m_Buffer+m_Used,bytes,count); m_Used+=count; bytes+=count; length-=count;
			if (m_Used==64) { Transform(m_Buffer); m_Used=0; }
		}
	}
	void Finish(char digest[65])
	{
		uint64_t bits=m_Bytes*8; unsigned char padding[64]={0x80};
		Update(padding,m_Used<56 ? 56-m_Used : 120-m_Used);
		unsigned char length[8]; for (int i=0;i<8;++i) length[7-i]=(unsigned char)(bits>>(i*8));
		Update(length,8);
		const char hex[]="0123456789abcdef";
		for (int i=0;i<32;++i) { unsigned char byte=(unsigned char)(m_State[i/4]>>(24-(i%4)*8)); digest[i*2]=hex[byte>>4]; digest[i*2+1]=hex[byte&15]; }
		digest[64]=0;
	}
private:
	static uint32_t Rotate(uint32_t v,unsigned int bits) { return (v>>bits)|(v<<(32-bits)); }
	void Transform(const unsigned char *block)
	{
		static const uint32_t k[64]={
			0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
			0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
			0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
			0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
			0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
			0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
			0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
			0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
		uint32_t w[64];
		for (int i=0;i<16;++i) w[i]=(uint32_t(block[i*4])<<24)|(uint32_t(block[i*4+1])<<16)|(uint32_t(block[i*4+2])<<8)|block[i*4+3];
		for (int i=16;i<64;++i) { uint32_t a=w[i-15],b=w[i-2]; w[i]=w[i-16]+(Rotate(a,7)^Rotate(a,18)^(a>>3))+w[i-7]+(Rotate(b,17)^Rotate(b,19)^(b>>10)); }
		uint32_t a=m_State[0],b=m_State[1],c=m_State[2],d=m_State[3],e=m_State[4],f=m_State[5],g=m_State[6],h=m_State[7];
		for (int i=0;i<64;++i) { uint32_t t=h+(Rotate(e,6)^Rotate(e,11)^Rotate(e,25))+((e&f)^(~e&g))+k[i]+w[i]; uint32_t u=(Rotate(a,2)^Rotate(a,13)^Rotate(a,22))+((a&b)^(a&c)^(b&c)); h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u; }
		m_State[0]+=a;m_State[1]+=b;m_State[2]+=c;m_State[3]+=d;m_State[4]+=e;m_State[5]+=f;m_State[6]+=g;m_State[7]+=h;
	}
	uint32_t m_State[8]; uint64_t m_Bytes; size_t m_Used; unsigned char m_Buffer[64];
};
#endif
