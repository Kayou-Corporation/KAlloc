#include "StackAllocator.hpp"

#include <cassert>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t
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
    std::cout << "StackAllocator validation test\n";
    PrintSeparator();

    constexpr std::size_t allocatorSize = 2048;
    Kayou::Memory::StackAllocator allocator(allocatorSize, 64);

    std::cout << "Allocator created with total size = " << allocatorSize << " bytes\n";
    allocator.PrintUsage();
    PrintSeparator();

    struct TestAlloc
    {
        std::size_t size;
        std::size_t alignment;
    };

    const std::vector<TestAlloc> tests =
    {
        {24, 8},
        {64, 16},
        {128, 32},
        {80, 64},
        {200, 16},
        {13, 8},
        {97, 32},
        {256, 64}
    };

    std::vector<void*> allocatedPtrs;
    allocatedPtrs.reserve(tests.size());

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    for (const TestAlloc& test : tests)
    {
        void* ptr = allocator.Alloc(test.size, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment);

        if (ptr != nullptr)
        {
            assert(IsAligned(ptr, test.alignment) && "Returned pointer is not correctly aligned");
            std::memset(ptr, 0xCD, test.size);
        }

        allocatedPtrs.push_back(ptr);
    }

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
    std::cout << "PHASE 4 - free last allocation (valid LIFO)\n";
    PrintSeparator();
    {
        void* lastPtr = allocatedPtrs.back();
        assert(lastPtr != nullptr && "Last allocation should be valid");

        allocator.Free(lastPtr);

        std::cout << "After freeing last allocation:\n";
        allocator.PrintUsage();

        void* ptr = allocator.Alloc(256, 64);
        PrintPointerInfo(ptr, 256, 64);

        assert(ptr != nullptr && "Re-allocation after LIFO free should succeed");
        assert(ptr == lastPtr && "StackAllocator should reuse the exact same top memory after LIFO free");
        assert(IsAligned(ptr, 64) && "Reused pointer is not correctly aligned");

        std::memset(ptr, 0xEF, 256);
        allocatedPtrs.back() = ptr;
    }

    PrintSeparator();
    std::cout << "PHASE 5 - allocator state before reset\n";
    PrintSeparator();
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 6 - reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After Reset():\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 7 - allocations after reset\n";
    PrintSeparator();

    const std::vector<TestAlloc> postResetTests =
    {
        {32, 8},
        {64, 16},
        {128, 32},
        {48, 64}
    };

    for (const TestAlloc& test : postResetTests)
    {
        void* ptr = allocator.Alloc(test.size, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment);

        if (ptr != nullptr)
        {
            assert(IsAligned(ptr, test.alignment) && "Returned pointer is not correctly aligned after reset");
            std::memset(ptr, 0xAB, test.size);
        }
    }

    PrintSeparator();
    std::cout << "Final allocator state before final reset:\n";
    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 8 - final reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After final Reset():\n";
    allocator.PrintUsage();

#ifdef KAYOU_DEBUG
    // Uncomment this block if you explicitly want to validate the LIFO assert in debug.
/*
    PrintSeparator();
    std::cout << "PHASE 9 - invalid free order (DEBUG ASSERT EXPECTED)\n";
    PrintSeparator();

    Kayou::Memory::StackAllocator debugAllocator(512);
    void* a = debugAllocator.Alloc(64, 16);
    void* b = debugAllocator.Alloc(64, 16);

    static_cast<void>(b);
    debugAllocator.Free(a); // Should assert in debug: not LIFO
*/
#endif

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    getchar();
    return 0;
}