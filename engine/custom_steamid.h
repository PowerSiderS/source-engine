#ifndef CUSTOM_STEAMID_H
#define CUSTOM_STEAMID_H

#ifdef _WIN32
#pragma once
#endif

#include "steam/steamclientpublic.h"
#include "netadr.h"

const char *GetDeviceUUID();
CSteamID GenerateSteamIDFromUUID( const char *pszUUID );
CSteamID GenerateSteamIDFromAddress( const netadr_t &adr );
CSteamID GetLocalDeviceSteamID();

#endif
