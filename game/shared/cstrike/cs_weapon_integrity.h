#ifndef CS_WEAPON_INTEGRITY_H
#define CS_WEAPON_INTEGRITY_H
bool CSWeaponFilesDigest(char digest[65], int *fileCount = NULL);
#ifndef CLIENT_DLL
bool CSValidateWeaponFiles(edict_t *client, char *reject, int maxReject);
#endif
#endif
