#include "heap.h"
#include <stdint.h>
#include <memory.h>
#include "custom_unistd.h"
#include <stdio.h>

struct memory_manager_t memory_manager;

int h_s;

int heap_setup(void){
    void *memory;
    memory=custom_sbrk(4092);
    if(memory==(void*)-1) return -1;
    memory_manager.first_memory_chunk=NULL;
    memory_manager.memory_size=4092;
    memory_manager.memory_start=memory;
    memory_manager.exist=1;
    h_s=1;
    return 0;
}
void heap_clean(void){
    custom_sbrk((long)-memory_manager.memory_size);
    memory_manager.first_memory_chunk=NULL;
    memory_manager.memory_size=0;
    memory_manager.memory_start=NULL;
    memory_manager.exist=0;
    h_s=0;
}
unsigned int bit_count(struct memory_chunk_t *pstr){
    unsigned int bit=0;
    unsigned char* t = ( unsigned char*)pstr;
    for(unsigned int i=0;i<sizeof(struct memory_chunk_t)-sizeof(unsigned int);i++){
        bit=bit+t[i];
    }
    return bit;
}
int heap_validate(void){
    if(memory_manager.exist==0)return 2;
    struct memory_chunk_t *pfirst = memory_manager.first_memory_chunk;
    for(;pfirst!=NULL;pfirst=pfirst->next) {
        if(pfirst->free==1)continue;
        if(pfirst->counter != bit_count(pfirst))return 3;
        void *pom = ((uint8_t *) pfirst + sizeof(struct memory_chunk_t));
        void *end = ((uint8_t *) pfirst + sizeof(struct memory_chunk_t) + 4 + pfirst->size);
        if(memcmp(pom,end,4) != 0)return 1;
    }
    return 0;
}
void* heap_malloc(size_t size){
    if(size==0){
        return NULL;
    }
    struct memory_chunk_t *pfirst;
    struct memory_manager_t *man =&memory_manager;
    if(man->first_memory_chunk==NULL){
        void* increase = custom_sbrk((long)(size+sizeof(struct memory_chunk_t)+8));
        if(increase==(void*)-1)return NULL;
        memory_manager.memory_size=(size_t)custom_sbrk_get_reserved_memory();
        pfirst =(struct memory_chunk_t*)(man->memory_start);
        man->first_memory_chunk=pfirst;
        man->first_memory_chunk->size=size;
        pfirst->prev=NULL;
        pfirst->next=NULL;
        pfirst->free=0;
        pfirst->counter=bit_count(pfirst);
        void* ptr = (uint8_t*)pfirst+sizeof(struct memory_chunk_t)+4;
        void* str = ((uint8_t*)pfirst+sizeof(struct memory_chunk_t));
        memset((char*)str,'#',4);
        void* pom =(uint8_t*)ptr+pfirst->size;
        memset((char*)pom,'#',4);
        return ptr;
    }
    for(pfirst = man->first_memory_chunk;pfirst!=NULL;pfirst=pfirst->next){
        if(pfirst->free==1 && (pfirst->size>= size + sizeof(struct memory_chunk_t)+8))break;
    }
    if(pfirst!=NULL){
        pfirst->free=0;
        pfirst->size=size;
        pfirst->counter=bit_count(pfirst);
    }
    else{
        struct memory_chunk_t*pcur=man->first_memory_chunk;
        void* increase = custom_sbrk((long)(size+sizeof(struct memory_chunk_t)+8));
        if(increase==(void*)-1)return NULL;
        memory_manager.memory_size=(size_t)custom_sbrk_get_reserved_memory();
        for(;pcur->next!=NULL;pcur=pcur->next){};
        struct memory_chunk_t *pnew=(struct memory_chunk_t*)((uint8_t*)pcur+sizeof(struct memory_chunk_t)+4+pcur->size+4);
        pnew->next=NULL;
        pnew->prev=pcur;
        pcur->next=pnew;
        pnew->free=0;
        pnew->size=size;
        pcur->counter = bit_count(pcur);
        pnew->counter = bit_count(pnew);
        void* ptr = (uint8_t*)pnew+sizeof(struct memory_chunk_t)+4;
        void* str = (uint8_t*)pnew+sizeof(struct memory_chunk_t);
        memset((char*)str,'#',4);
        void* pom =(uint8_t*)ptr+pnew->size;
        memset((char*)pom,'#',4);
        return ptr;
    }
    void* ptr = (uint8_t*)pfirst+sizeof(struct memory_chunk_t)+4;
    void* str = ((uint8_t*)pfirst+sizeof(struct memory_chunk_t));
    memset((char*)str,'#',4);
    void* pom =(uint8_t*)ptr+pfirst->size;
    memset((char*)pom,'#',4);
    return ptr;
}
void* heap_calloc(size_t number, size_t size){
    if(number==0)return NULL;
    void* new = heap_malloc(number*size);
    if(new==NULL)return NULL;
    memset(new,0x0,number*size);
    return new;
}
void* heap_realloc(void* memblock, size_t count){
    if(h_s!=1)return NULL;
    if(heap_validate()==2||heap_validate()==1)return NULL;
    if(memblock==NULL && count==0)return NULL;
    if( memblock==NULL && count>0){
        memblock = heap_malloc(count);
        return memblock;
    }
    struct memory_chunk_t *pstr=(struct memory_chunk_t*)((uint8_t*)memblock-4-sizeof(struct memory_chunk_t));
    int correct=0;
    for(struct memory_chunk_t *c=memory_manager.first_memory_chunk;c!=NULL;c=c->next){
        if(c==pstr){
            correct=1;
            break;
        }
    }
    if(correct==0)return NULL;
    if(pstr->free==0 && count==0){
        heap_free(memblock);
        return NULL;
    }
    if(count<pstr->size){
        pstr->size = count;
        void *pom = (uint8_t*)pstr+sizeof(struct memory_chunk_t);
        memset(pom,'#',4);
        void *pom2 = (uint8_t*)pstr+sizeof(struct memory_chunk_t)+4+pstr->size;
        memset(pom2,'#',4);
        pstr->counter= bit_count(pstr);
        return memblock;
    }
    if(pstr->size==count)return memblock;
    if(pstr->size<count){
        if(pstr->next!=NULL){
            if(pstr->size+pstr->next->size>count && pstr->next->free==1){
                pstr->size=count;
                if(pstr->next->next!=NULL){
                    pstr->next->next->prev=pstr;
                    pstr->next=pstr->next->next;
                    pstr->next->counter= bit_count(pstr->next);
                }
                else{
                    pstr->next=NULL;
                }
                void *pom = (uint8_t*)pstr+sizeof(struct memory_chunk_t);
                memset(pom,'#',4);
                void *pom2 = (uint8_t*)pstr+sizeof(struct memory_chunk_t)+4+pstr->size;
                memset(pom2,'#',4);
                pstr->counter= bit_count(pstr);
                return memblock;
            }
            else{
                size_t roz=pstr->size;
                void *mem=memblock;
                heap_free(memblock);
                void*pom=heap_malloc(count);
                if(pom==NULL){
                    memblock=heap_malloc(roz);
                    memcpy(memblock,mem,roz);
                    return NULL;
                }
                memcpy(pom,mem,roz);
                return pom;
            }
        }
        else{
            size_t roz=pstr->size;
            void *mem=memblock;
            heap_free(memblock);
            void*pom=heap_malloc(count);
            if(pom==NULL){
                memblock=heap_malloc(roz);
                memcpy(memblock,mem,roz);
                return NULL;
            }
            memcpy(pom,mem,roz);
            return pom;
        }
    }
    return NULL;
}

void connect(struct memory_chunk_t* temp){
    temp->free=1;
    struct memory_chunk_t *pl=temp->next->next;
    temp->size+=temp->next->size+ sizeof(struct memory_chunk_t);
    if(pl==NULL){
        temp->next=NULL;
    }
    else{
        temp->next=pl;
    }

    if(pl!=NULL){
        pl->prev=temp;
        pl->counter=bit_count(pl);
    }
    temp->counter=bit_count(temp);
}
void  heap_free(void* memblock){
    if(memblock==NULL)return;
    int exist=0;
    struct memory_chunk_t *pfree = (struct memory_chunk_t*)((uint8_t*)memblock-sizeof(struct memory_chunk_t)-4);
    for(struct memory_chunk_t *t=memory_manager.first_memory_chunk;t!=NULL;t=t->next) {
        if (pfree == t) {
            exist = 1;
            break;
        }
    }
    if(exist==0)return;
    pfree->free=1;
    if(pfree->next!=NULL){
        pfree->size=(size_t)((uint8_t*)pfree->next-((uint8_t*)pfree));
    }
    pfree->counter=bit_count(pfree);
    if(pfree==memory_manager.first_memory_chunk&&pfree->next==NULL){
        memory_manager.first_memory_chunk=NULL;
        return;
    }
    if(pfree->next!=NULL){
        if(pfree->next->free){
            connect(pfree);
        }
    }
    // free=1 wolne //free=0 zajęte
    if(pfree->prev!=NULL) {
        if (pfree->prev->free) {
            connect(pfree->prev);
        }
    }
    int is=0;
    for(struct memory_chunk_t *u=memory_manager.first_memory_chunk;u!=NULL;u=u->next){
        if(u->free==0){
            is=1;
            int delete=1;
            for(struct memory_chunk_t *c=u->next;c!=NULL;c=c->next){
                if(c->free==0)delete=0;
            }
            if(delete==1){
                u->next=NULL;
                u->counter= bit_count(u);
                break;
            }
        }
    }
    if(is==0){
        memory_manager.first_memory_chunk=NULL;
    }
}
enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer==NULL) return pointer_null;

    if(heap_validate() != 0) return pointer_heap_corrupted;

    struct memory_chunk_t *pstr=memory_manager.first_memory_chunk;

    while(pstr!=NULL)
    {
        if(pstr->next != NULL) {
            if (pointer >= (void *) (intptr_t) pstr && pointer < (void *) (intptr_t) pstr->next) {
                if (pstr->free == 1) return pointer_unallocated;

                if (pointer == (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4)) return pointer_valid;

                if (pointer >= (void *) (intptr_t) pstr &&
                    pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t)))
                    return pointer_control_block;
                else if (pointer >= (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t)) &&
                         pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4))
                    return pointer_inside_fences;
                else if (pointer > (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4) &&
                         pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size))
                    return pointer_inside_data_block;
                else if (pointer >= (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size) &&
                         pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size + 4))
                    return pointer_inside_fences;
                else if (pointer >= (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size + 4) &&
                         pointer < (void *) (intptr_t) pstr->next)
                    return pointer_unallocated;
            }
        }else {
            if (pstr->free == 1) return pointer_unallocated;

            if (pointer == (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4)) return pointer_valid;

            if (pointer >= (void *) (intptr_t) pstr &&
                pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t)))
                return pointer_control_block;
            else if (pointer >= (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t)) &&
                     pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4))
                return pointer_inside_fences;
            else if (pointer > (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4) &&
                     pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size))
                return pointer_inside_data_block;
            else if (pointer >= (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size) &&
                     pointer < (void *) ((intptr_t) pstr + sizeof(struct memory_chunk_t) + 4 + pstr->size + 4))
                return pointer_inside_fences;
        }
        pstr=pstr->next;

    }

    return pointer_unallocated;
}


size_t heap_get_largest_used_block_size(void){
    if(heap_validate())return 0;
    size_t biggest=0;
    for(struct memory_chunk_t* pfirst=memory_manager.first_memory_chunk;pfirst!=NULL;pfirst=pfirst->next){
        if(pfirst->free==0){
            if(pfirst->size>biggest){
                biggest=pfirst->size;
            }
        }
    }
    return biggest;
}