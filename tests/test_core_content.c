#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../core_content.h"

int main(void) {
    const char *file="b1-test-content.bin";
    const unsigned char bytes[]={0x4e,0x45,0x53,0x1a,0,1,2,3};
    FILE *f=fopen(file,"wb");
    assert(f);
    assert(fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes));
    assert(fclose(f)==0);

    CoreContent c={0};
    assert(core_content_open(&c,file,false));
    assert(c.path && strcmp(c.path,file)==0);
    assert(c.data && c.size==sizeof(bytes));
    assert(memcmp(c.data,bytes,sizeof(bytes))==0);
    core_content_release(&c);
    assert(!c.data && !c.path && !c.size);

    assert(core_content_open(&c,file,true));
    assert(c.path && strcmp(c.path,file)==0);
    assert(c.data==NULL && c.size==0);
    core_content_release(&c);

    assert(!core_content_open(&c,"b1-file-does-not-exist.bin",true));
    assert(!c.path && !c.data && c.size==0);
    assert(!core_content_open(&c,NULL,false));
    assert(!core_content_open(NULL,file,false));

    assert(remove(file)==0);
    puts("B1: carregamento de conteudo (memoria/fullpath) OK");
    return 0;
}
