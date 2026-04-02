#pragma once

#include "MemoryAllocator.hpp"



namespace Kayou
{
    class LinearAllocator : public MemoryAllocator
    {
    public:
        explicit LinearAllocator(std::size_t size);
        ~LinearAllocator() override;

        virtual void* Alloc(std::size_t size, std::size_t memAlignment) override;
        virtual void PrintUsage() const override;

        void Reset();


    private:
        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;
    };
}
