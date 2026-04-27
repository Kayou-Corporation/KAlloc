#include "FreeListAllocator.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>


bool IsAligned(const void* ptr, const std::size_t alignment)
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(ptr);
    return (address % alignment) == 0;
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

    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(ptr);

    std::cout
        << " | ptr=0x" << std::hex << address << std::dec
        << " | aligned=" << (IsAligned(ptr, alignment) ? "YES" : "NO")
        << '\n';
}


int main()
{
    using namespace Kayou::Memory;

    std::cout << "FreeListAllocator validation test\n";
    PrintSeparator();

    constexpr std::size_t allocatorSize = 4096;
    constexpr std::size_t allocatorAlignment = 64;

    FreeListAllocator allocator(allocatorSize, allocatorAlignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  total size     = " << allocatorSize << " bytes\n";
    std::cout << "  base alignment = " << allocatorAlignment << " bytes\n";
    allocator.PrintUsage();
    PrintSeparator();

    struct TestAlloc
    {
        std::size_t size;
        std::size_t alignment;
    };

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    void* a = allocator.Alloc(128, 16);
    PrintPointerInfo(a, 128, 16);
    assert(a != nullptr && IsAligned(a, 16));
    std::memset(a, 0xAA, 128);

    void* b = allocator.Alloc(256, 32);
    PrintPointerInfo(b, 256, 32);
    assert(b != nullptr && IsAligned(b, 32));
    std::memset(b, 0xBB, 256);

    void* c = allocator.Alloc(64, 8);
    PrintPointerInfo(c, 64, 8);
    assert(c != nullptr && IsAligned(c, 8));
    std::memset(c, 0xCC, 64);

    void* d = allocator.Alloc(200, 64);
    PrintPointerInfo(d, 200, 64);
    assert(d != nullptr && IsAligned(d, 64));
    std::memset(d, 0xDD, 200);

    PrintSeparator();
    std::cout << "After deterministic allocations:\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 2 - zero-sized allocation\n";
    PrintSeparator();
    {
        void* ptr = allocator.Alloc(0, 16);
        std::cout << "Alloc(0, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL (unexpected)") << '\n';
        assert(ptr == nullptr && "Zero-sized allocation should return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 3 - overflow test\n";
    PrintSeparator();
    {
        void* bigPtr = allocator.Alloc(100000, 16);
        std::cout << "Alloc(100000, 16) => " << (bigPtr == nullptr ? "nullptr" : "NON-NULL (unexpected)") << '\n';
        assert(bigPtr == nullptr && "Huge allocation should overflow and return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 4 - free middle block and reallocate into reclaimed space\n";
    PrintSeparator();

    allocator.Free(b);
    std::cout << "After freeing block b:\n";
    allocator.PrintUsage();

    void* e = allocator.Alloc(128, 16);
    PrintPointerInfo(e, 128, 16);
    assert(e != nullptr && IsAligned(e, 16));
    std::memset(e, 0xEE, 128);

    PrintSeparator();
    std::cout << "Allocator state after reuse attempt:\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 5 - free adjacent blocks to exercise merge\n";
    PrintSeparator();

    allocator.Free(c);
    allocator.Free(d);

    std::cout << "After freeing blocks c and d:\n";
    allocator.PrintUsage();

    void* f = allocator.Alloc(400, 32);
    PrintPointerInfo(f, 400, 32);
    assert(f != nullptr && IsAligned(f, 32));
    std::memset(f, 0xF0, 400);

    PrintSeparator();
    std::cout << "Allocator state after merged-space allocation:\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 6 - free remaining live allocations\n";
    PrintSeparator();

    allocator.Free(a);
    allocator.Free(e);
    allocator.Free(f);

    std::cout << "After freeing all remaining blocks:\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 7 - full-size reallocation after complete free\n";
    PrintSeparator();

    void* g = allocator.Alloc(2048, 64);
    PrintPointerInfo(g, 2048, 64);
    assert(g != nullptr && IsAligned(g, 64));
    std::memset(g, 0x11, 2048);

    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 8 - reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After Reset():\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 9 - allocations after reset\n";
    PrintSeparator();

    const std::vector<TestAlloc> postResetTests =
    {
        {64, 8},
        {128, 16},
        {256, 32},
        {96, 64}
    };

    for (const TestAlloc& test : postResetTests)
    {
        void* ptr = allocator.Alloc(test.size, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment);

        assert(ptr != nullptr && "Post-reset allocation failed unexpectedly");
        assert(IsAligned(ptr, test.alignment) && "Returned pointer is not correctly aligned after reset");
        std::memset(ptr, 0x5A, test.size);
    }

    PrintSeparator();
    std::cout << "Final allocator state before final reset:\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 10 - final reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After final Reset():\n";
    allocator.PrintUsage();

#ifdef KAYOU_DEBUG
    /*
    PrintSeparator();
    std::cout << "PHASE 11 - invalid free (DEBUG ASSERT EXPECTED)\n";
    PrintSeparator();

    int stackValue = 42;
    allocator.Free(&stackValue); // Should assert: pointer does not belong to this allocator
    */
#endif

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    getchar();
    return 0;
}