#ifndef CS_GAMEPLAY_AUDIT_H
#define CS_GAMEPLAY_AUDIT_H
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "filesystem.h"
extern IFileSystem *filesystem;
#include <stdarg.h>
// Explicit opt-in diagnostics. No drawing or gameplay changes.
extern ConVar sv_gameplay_audit;
enum CSGameplayAuditFlags { CS_AUDIT_MOVEMENT=1, CS_AUDIT_SHOTS=2, CS_AUDIT_GRENADES=4 };
// Independent diagnostics remain complete while RCON redirects console output.
inline void CSGameplayAuditPrint(const char *format,...)
{
 char text[1024];va_list arguments;va_start(arguments,format);Q_vsnprintf(text,sizeof(text),format,arguments);va_end(arguments);
 if(filesystem) { FileHandle_t file=filesystem->Open("gameplay_audit.log","a","MOD"); if(file!=FILESYSTEM_INVALID_HANDLE) { filesystem->Write(text,Q_strlen(text),file);filesystem->Close(file); } }
 Msg("%s",text);
}
#endif
