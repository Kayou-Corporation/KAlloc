#pragma once

#include <cstddef>



namespace Kayou
{
    class MemoryAllocator
    {
    public:
        virtual ~MemoryAllocator() = default;
        virtual void* alloc(std::size_t size) = 0;
        virtual void free(void* ptr, std::size_t size) = 0;
        virtual std::size_t GetTotalAllocatedSize() const = 0;
        virtual void PrintUsage() const = 0;


    };
}
