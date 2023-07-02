#include <unistd.h>
#include <stdint.h>

void* custom_sbrk(intptr_t delta);

#if defined(sbrk)
#undef sbrk
#endif

#if defined(brk)
#undef brk
#endif


#define sbrk(__arg__) (assert("Proszę nie używać standardowej funkcji sbrk()" && 0), (void*)-1)
#define brk(__arg__) (assert("Proszę nie używać standardowej funkcji sbrk()" && 0), -1)

//
// Funkcja testuje płotki ustawione przed i po obszarze przyznanym przez sbrk().
// Funkcja zwraca:
//    0 - płotki są nienaruszone
//  !=0 - kod diagnostyczny (płotki zostały popsute)
int custom_sbrk_check_fences_integrity(void);

//
// Funkcja zwraca ilość (w bajtach) pamięci zarezerwownej przez funkcjię `custom_sbrk`, a nie "zwróconą"
// do systemu operacyjnego.
uint64_t custom_sbrk_get_reserved_memory(void);
