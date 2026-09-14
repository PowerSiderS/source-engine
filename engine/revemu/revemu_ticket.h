#ifndef REVEMU_TICKET_H
#define REVEMU_TICKET_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t RevEmu_Hash( const char *str );

int RevEmu_GenerateTicket2013( void *dest, int max_len, const char *hwid_seed, uint64_t *pOutSteamID );

int RevEmu_GenerateTicketSC2009( void *dest, int max_len, const char *hwid_seed, uint64_t *pOutSteamID );

#ifdef __cplusplus
}
#endif

#endif // REVEMU_TICKET_H
