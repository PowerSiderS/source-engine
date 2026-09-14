#include "revemu_ticket.h"
#include "rijndael.h"
#include "sha256.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

uint32_t RevEmu_Hash( const char *str )
{
	if ( !str || !*str )
		return 0;

	uint32_t nHash = 0x4E67C6A7;
	int i = 0;
	int c = (unsigned char)str[i++];
	while ( c )
	{
		nHash = nHash ^ ((nHash >> 2) + (nHash << 5) + c);
		c = (unsigned char)str[i++];
	}
	return nHash;
}

static const char aes_key_rand[32] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V' };
static const char aes_key_rev[33] = "_YOU_SERIOUSLY_NEED_TO_GET_LAID_";

static bool Rev_WriteAES( void *dest, const char *src, const char *key )
{
	char src_pad[32];
	memset( src_pad, 0, sizeof(src_pad) );
	size_t len = strlen(src);
	if ( len > 32 ) len = 32;
	memcpy( src_pad, src, len );

	rijndael_ctx ctx;
	rijndael_init( &ctx );
	if ( rijndael_make_key( &ctx, key, rijndael_get_zero_chain(), 32, 32 ) < 0 )
		return false;

	if ( rijndael_encrypt_block( &ctx, src_pad, (char*)dest ) < 0 )
		return false;

	return true;
}

static bool Rev_WriteSHA( void *dest, const char *src )
{
	char src_pad[32];
	memset( src_pad, 0, sizeof(src_pad) );
	size_t len = strlen(src);
	if ( len > 32 ) len = 32;
	memcpy( src_pad, src, len );

	SHA256_CTX ctx;
	sha256_init( &ctx );
	sha256_update( &ctx, (const BYTE*)src_pad, 32 );
	sha256_final( &ctx, (BYTE*)dest );
	return true;
}

int RevEmu_GenerateTicket2013( void *dest, int max_len, const char *hwid_seed, uint64_t *pOutSteamID )
{
	if ( !dest || max_len < 202 )
		return 0;

	char hwid[33];
	memset( hwid, 0, sizeof(hwid) );
	if ( hwid_seed && hwid_seed[0] )
	{
		size_t slen = strlen(hwid_seed);
		if ( slen > 32 ) slen = 32;
		memcpy( hwid, hwid_seed, slen );
	}
	else
	{
		memcpy( hwid, "Samsung-860EVO-SN123456789", 26 );
	}

	uint32_t hash = RevEmu_Hash( hwid );
	uint32_t sid_low = (hash << 1);
	uint32_t sid_high = 0x01100001;
	uint64_t steamid64 = ((uint64_t)sid_high << 32) | (uint64_t)sid_low;

	if ( pOutSteamID )
		*pOutSteamID = steamid64;

	*(uint64_t*)dest = steamid64;

	uint8_t *ticket_b = (uint8_t*)dest + sizeof(uint64_t);
	int *ticket = (int*)ticket_b;
	memset( ticket_b, 0, 194 );

	int cur_time = (int)time(NULL);

	ticket[0] = 'S';                               // +0
	ticket[1] = (int)hash;                         // +4
	ticket[2] = 0x00726576;                        // +8, 'rev'
	ticket[3] = 0;                                 // +12
	ticket[4] = (int)sid_low;                      // +16
	ticket[5] = (int)sid_high;                     // +20
	ticket[6] = cur_time + 90123;                  // +24
	ticket_b[27] = ~(ticket_b[27] + ticket_b[24]); // +27
	ticket[7] = ~cur_time;                         // +28
	ticket[8] = (int)(hash * 2 >> 3);              // +32
	ticket[9] = 0;                                 // +36

	if ( !Rev_WriteAES( &ticket_b[40], hwid, aes_key_rand ) )
		return 0;

	if ( !Rev_WriteAES( &ticket_b[72], aes_key_rand, aes_key_rev ) )
		return 0;

	if ( !Rev_WriteSHA( &ticket_b[104], hwid ) )
		return 0;

	return 202; // 8 bytes SteamID + 194 bytes RevEmu ticket
}

int RevEmu_GenerateTicketSC2009( void *dest, int max_len, const char *hwid_seed, uint64_t *pOutSteamID )
{
	if ( !dest || max_len < 186 )
		return 0;

	char hwid[33];
	memset( hwid, 0, sizeof(hwid) );
	if ( hwid_seed && hwid_seed[0] )
	{
		size_t slen = strlen(hwid_seed);
		if ( slen > 32 ) slen = 32;
		memcpy( hwid, hwid_seed, slen );
	}
	else
	{
		memcpy( hwid, "Samsung-860EVO-SN123456789", 26 );
	}

	uint32_t hash = RevEmu_Hash( hwid );
	uint32_t sid_low = (hash << 1);
	uint32_t sid_high = 0x01100001;
	uint64_t steamid64 = ((uint64_t)sid_high << 32) | (uint64_t)sid_low;

	if ( pOutSteamID )
		*pOutSteamID = steamid64;

	*(uint64_t*)dest = steamid64;

	uint8_t *ticket_b = (uint8_t*)dest + sizeof(uint64_t);
	int *ticket = (int*)ticket_b;
	memset( ticket_b, 0, 178 );

	ticket[0] = 'S';                               // +0
	ticket[1] = (int)hash;                         // +4
	ticket[2] = 0x00726576;                        // +8, 'rev'
	ticket[3] = 0;                                 // +12
	ticket[4] = (int)sid_low;                      // +16
	ticket[5] = (int)sid_high;                     // +20

	if ( !Rev_WriteAES( &ticket_b[24], hwid, aes_key_rand ) )
		return 0;

	if ( !Rev_WriteAES( &ticket_b[56], aes_key_rand, aes_key_rev ) )
		return 0;

	if ( !Rev_WriteSHA( &ticket_b[88], hwid ) )
		return 0;

	return 186; // 8 bytes SteamID + 178 bytes SC2009 ticket
}
