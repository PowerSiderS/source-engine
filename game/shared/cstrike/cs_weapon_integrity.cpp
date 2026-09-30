#include "cbase.h"
#include "cs_weapon_integrity.h"
#include "cs_sha256.h"
#include "filesystem.h"
#include "igamesystem.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlstring.h"
#include "tier0/memdbgon.h"

static int ScriptOrder(const CUtlString *a, const CUtlString *b) { return Q_stricmp(a->String(),b->String()); }
bool CSWeaponFilesDigest(char digest[65], int *fileCount)
{
	digest[0]=0; if (fileCount) *fileCount=0;
	CUtlVector<CUtlString> names; FileFindHandle_t handle;
	for (const char *name=filesystem->FindFirstEx("scripts/weapon_*.txt","GAME",&handle); name; name=filesystem->FindNext(handle))
	{
		if (filesystem->FindIsDirectory(handle) || Q_strlen(name)>120 || Q_strstr(name,"/") || Q_strstr(name,"\\")) continue;
		CUtlString normalized(name); Q_strlower(normalized.GetForModify());
		bool duplicate=false; for (int i=0;i<names.Count();++i) if (!Q_strcmp(names[i].String(),normalized.String())) duplicate=true;
		if (!duplicate) names.AddToTail(normalized);
	}
	filesystem->FindClose(handle); names.Sort(ScriptOrder);
	if (!names.Count() || names.Count()>512) return false;
	CSSha256 hash; const char domain[]="SourceAdvancedWeaponFiles/SHA256/v1"; hash.Update(domain,sizeof(domain));
	for (int i=0;i<names.Count();++i)
	{
		char path[160]; Q_snprintf(path,sizeof(path),"scripts/%s",names[i].String()); CUtlBuffer bytes;
		if (!filesystem->ReadFile(path,"GAME",bytes) || bytes.TellPut()<1 || bytes.TellPut()>1024*1024) return false;
		hash.Update(names[i].String(),Q_strlen(names[i].String())+1);
		uint32_t count=bytes.TellPut(); unsigned char size[4]={(unsigned char)count,(unsigned char)(count>>8),(unsigned char)(count>>16),(unsigned char)(count>>24)};
		hash.Update(size,sizeof(size)); hash.Update(bytes.Base(),count);
	}
	hash.Finish(digest); if (fileCount) *fileCount=names.Count(); return true;
}

#ifdef CLIENT_DLL
static ConVar cl_weapon_files_sha256("cl_weapon_files_sha256","",FCVAR_USERINFO|FCVAR_HIDDEN,"Effective weapon definitions fingerprint.");
#else
static char s_MasterDigest[65];
#endif
class CWeaponFileIntegrity : public CAutoGameSystem
{
public:
	CWeaponFileIntegrity() : CAutoGameSystem("SourceAdvancedWeaponFileIntegrity") {}
	bool Init() { Refresh(); return true; }
	void LevelInitPreEntity() { Refresh(); }
	void Refresh()
	{
		char digest[65]; int count=0; bool valid=CSWeaponFilesDigest(digest,&count);
#ifdef CLIENT_DLL
		cl_weapon_files_sha256.SetValue(valid ? digest : "unavailable");
#else
		Q_strncpy(s_MasterDigest,valid ? digest : "",sizeof(s_MasterDigest));
#endif
		Msg("Weapon integrity: %d effective scripts, SHA256 %s\n",count,valid ? digest : "unavailable");
	}
};
static CWeaponFileIntegrity s_WeaponIntegrity;

#ifndef CLIENT_DLL
bool CSValidateWeaponFiles(edict_t *client, char *reject, int maxReject)
{
	const char *reported=engine->GetClientConVarValue(engine->IndexOfEdict(client),"cl_weapon_files_sha256");
	if (!s_MasterDigest[0]) s_WeaponIntegrity.Refresh();
	if (s_MasterDigest[0] && reported && Q_strlen(reported)==64 && !Q_strcmp(reported,s_MasterDigest)) return true;
	Q_strncpy(reject,"Client weapon files modified",maxReject); return false;
}
#endif
