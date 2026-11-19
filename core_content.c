#include "core_content.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void core_content_release(CoreContent *content) {
    if(!content) return;
    free(content->data);
    free(content->path);
    *content=(CoreContent){0};
}

bool core_content_open(CoreContent *out, const char *path, bool need_fullpath) {
    if(!out || out->path || out->data || out->size || !path || !path[0])
        return false;

    FILE *file=fopen(path,"rb");
    if(!file) return false;

    CoreContent prepared={0};
    size_t len=strlen(path);
    prepared.path=(char*)malloc(len+1);
    if(!prepared.path) goto error;
    memcpy(prepared.path,path,len+1);

    if(!need_fullpath) {
        /* Primeira etapa B1: ROMs em memoria ainda usam fseek/ftell.
           Imagens grandes devem usar um core que peça need_fullpath. */
        if(fseek(file,0,SEEK_END)!=0) goto error;
        long file_size=ftell(file);
        if(file_size<=0 || fseek(file,0,SEEK_SET)!=0) goto error;
        prepared.size=(size_t)file_size;
        prepared.data=malloc(prepared.size);
        if(!prepared.data) goto error;
        if(fread(prepared.data,1,prepared.size,file)!=prepared.size) goto error;
    }

    fclose(file);
    *out=prepared;
    return true;

error:
    fclose(file);
    core_content_release(&prepared);
    return false;
}
