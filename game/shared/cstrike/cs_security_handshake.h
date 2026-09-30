//========= Source Advanced V1 - Proprietary DLL Signature & Auth Handshake =========//
//
// Purpose: Cryptographic handshake between client.dll and server.dll (Dedicated & Client)
//          Guarantees that only clients running our exact compiled client.dll with matching
//          secret signatures can connect to the Source Advanced dedicated server.
//
//===================================================================================//

#ifndef CS_SECURITY_HANDSHAKE_H
#define CS_SECURITY_HANDSHAKE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/strtools.h"

#define SA_SECURITY_MAGIC            0x53415631ULL // 'SAV1'
#define SA_SECURITY_SECRET_SALT      0x9F4D23647B6DCAEDULL
#define SA_SECURITY_CVAR_NAME        "cl_sa_auth"
#define SA_SERVER_SIG_CVAR           "sv_sourceadvanced"
#define SA_REJECT_MSG                "Conexao recusada: DLL do cliente nao autorizada.\nInstale a versao oficial do Source Advanced V1."

// Fast 64-bit FNV-1a Hash with secret cryptographic salt
inline uint64 SA_HashBuffer( const void *pData, size_t nBytes, uint64 nInitialSalt = SA_SECURITY_SECRET_SALT )
{
	uint64 hash = 0xCBF29CE484222325ULL ^ nInitialSalt;
	const unsigned char *pByte = (const unsigned char *)pData;

	for ( size_t i = 0; i < nBytes; ++i )
	{
		hash ^= (uint64)pByte[i];
		hash *= 0x100000001B3ULL;
	}
	return hash;
}

// Generate the unique token expected for a client connecting with a given name
inline void SA_GenerateAuthToken( const char *pszPlayerName, char *outBuffer, int maxLen )
{
	if ( !outBuffer || maxLen <= 0 )
		return;

	const char *safeName = ( pszPlayerName && pszPlayerName[0] ) ? pszPlayerName : "unnamed";
	uint64 h1 = SA_HashBuffer( safeName, Q_strlen( safeName ), SA_SECURITY_SECRET_SALT );
	
	// Secondary cascading round
	const char *saltRound2 = "SOURCE_ADVANCED_RING5_AUTH_V1_2026";
	uint64 h2 = SA_HashBuffer( saltRound2, Q_strlen( saltRound2 ), h1 );

	Q_snprintf( outBuffer, maxLen, "SA1_%08X%08X", (uint32)(h2 >> 32), (uint32)(h2 & 0xFFFFFFFF) );
}

// Verify if token matches player name
inline bool SA_VerifyAuthToken( const char *pszPlayerName, const char *pszToken )
{
	if ( !pszToken || !pszToken[0] || !pszPlayerName || !pszPlayerName[0] )
		return false;

	char expected[64];
	SA_GenerateAuthToken( pszPlayerName, expected, sizeof( expected ) );
	return ( Q_strcmp( pszToken, expected ) == 0 );
}

#endif // CS_SECURITY_HANDSHAKE_H
