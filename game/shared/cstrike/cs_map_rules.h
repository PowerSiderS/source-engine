#ifndef CS_MAP_RULES_H
#define CS_MAP_RULES_H
#include <stddef.h>
#include <string.h>
inline bool CSMapNameValid(const char *name)
{
    if(!name || strlen(name)<4 || strlen(name)>=128)return false;
    if(strncmp(name,"de_",3) && strncmp(name,"cs_",3))return false;
    for(const char *p=name;*p;++p)if(!(*p>='a'&&*p<='z') && !(*p>='0'&&*p<='9') && *p!='_' && *p!='-')return false;
    return true;
}
#endif
