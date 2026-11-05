#include "DynamicPoolAllocator.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

bool IsAligned(const void* ptr, const std::size_t alignment)
{
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}

void PrintSeparator()
{
    std::cout << "============================================================\n";
}

void PrintPointerInfo(const void* ptr, const std::size_t size, const std::size_t alignment)
{
    std::cout
        << "Alloc request | size=" << std::setw(4) << size
        << " | align=" << std::setw(3) << alignment;

    if (ptr == nullptr)
    {
        std::cout << " | ptr=nullptr\n";
        return;
    }

    std::cout
        << " | ptr=0x" << std::hex << reinterpret_cast<std::uintptr_t>(ptr) << std::dec
        << " | aligned=" << (IsAligned(ptr, alignment) ? "YES" : "NO")
        << '\n';
}

int main()
{
    using Kayou::Memory::DynamicPoolAllocator;

    std::cout << "DynamicPoolAllocator validation test\n";
    PrintSeparator();

    constexpr std::size_t blockCapacity = 128;
    constexpr std::size_t blocksPerPage = 4;
    constexpr std::size_t alignment = 64;

    DynamicPoolAllocator allocator(blockCapacity, blocksPerPage, alignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  block capacity = " << blockCapacity << " bytes\n";
    std::cout << "  blocks/page    = " << blocksPerPage << '\n';
    std::cout << "  alignment      = " << alignment << " bytes\n";

    allocator.PrintUsage();
    PrintSeparator();

    std::vector<void*> ptrs;

    std::cout << "PHASE 1 - allocate more than one page\n";
    PrintSeparator();

    for (std::size_t i = 0; i < 10; ++i)
    {
        void* ptr = allocator.Alloc(64, 16);
        PrintPointerInfo(ptr, 64, 16);

        assert(ptr != nullptr);
        assert(IsAligned(ptr, 16));

        std::memset(ptr, static_cast<int>(i), 64);
        ptrs.push_back(ptr);
    }

    allocator.PrintUsage();
    assert(allocator.GetPageCount() == 3);
    assert(allocator.GetUsedBlocks() == 10);
    assert(allocator.GetFreeBlocks() == 2);

    PrintSeparator();
    std::cout << "PHASE 2 - invalid requests\n";
    PrintSeparator();

    {
        void* ptr = allocator.Alloc(0, 16);
        std::cout << "Alloc(0, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    {
        void* ptr = allocator.Alloc(256, 16);
        std::cout << "Alloc(256, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    {
        void* ptr = allocator.Alloc(64, 128);
        std::cout << "Alloc(64, 128) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    PrintSeparator();
    std::cout << "PHASE 3 - free half\n";
    PrintSeparator();

    for (std::size_t i = 0; i < ptrs.size(); i += 2)
    {
        allocator.Free(ptrs[i]);
        ptrs[i] = nullptr;
    }

    allocator.PrintUsage();
    assert(allocator.GetUsedBlocks() == 5);

    PrintSeparator();
    std::cout << "PHASE 4 - reuse freed blocks\n";
    PrintSeparator();

    for (std::size_t i = 0; i < 5; ++i)
    {
        void* ptr = allocator.Alloc(64, 16);
        PrintPointerInfo(ptr, 64, 16);

        assert(ptr != nullptr);
        ptrs.push_back(ptr);
    }

    allocator.PrintUsage();
    assert(allocator.GetUsedBlocks() == 10);

    PrintSeparator();
    std::cout << "PHASE 5 - free all\n";
    PrintSeparator();

    for (void*& ptr : ptrs)
    {
        if (ptr != nullptr)
        {
            allocator.Free(ptr);
            ptr = nullptr;
        }
    }

    allocator.PrintUsage();
    assert(allocator.GetUsedBlocks() == 0);

    PrintSeparator();
    std::cout << "PHASE 6 - reset\n";
    PrintSeparator();

    allocator.Reset();
    allocator.PrintUsage();

    assert(allocator.GetUsedBlocks() == 0);
    assert(allocator.GetPageCount() == 3);

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    return 0;
}