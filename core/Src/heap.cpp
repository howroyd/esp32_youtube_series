#include <cstddef>
#include "esp_heap_caps.h"

void* operator new(std::size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

void* operator new[](std::size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

void operator delete(void* p)
{
	heap_caps_free(p); // NULL safe
}

void operator delete(void* p, std::size_t)
{
	heap_caps_free(p); // NULL safe
}

void operator delete[](void* p)
{
	heap_caps_free(p); // NULL safe
}

void operator delete[](void* p, std::size_t)
{
	heap_caps_free(p); // NULL safe
}

