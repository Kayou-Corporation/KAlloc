#include "FreeListAllocator.hpp"
#include "TrackedAllocator.hpp"
#include "MemoryTracker.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>


const char* ToString(Kayou::Memory::MemoryTag tag)
{
    switch (tag)
    {
        case Kayou::Memory::MemoryTag::General:
            return "General";
        case Kayou::Memory::MemoryTag::Renderer:
            return "Renderer";
        case Kayou::Memory::MemoryTag::Physics:
            return "Physics";
        case Kayou::Memory::MemoryTag::ECS:
            return "ECS";
        case Kayou::Memory::MemoryTag::Audio:
            return "Audio";
        default:
            return "Unknown";
    }
}


bool IsAligned(const void* ptr, const std::size_t alignment)
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(ptr);
    return (address % alignment) == 0;
}


void PrintSeparator()
{
    std::cout << "============================================================\n";
}


void PrintPointerInfo(const void* ptr, const std::size_t size, const std::size_t alignment, Kayou::Memory::MemoryTag tag)
{
    std::cout
        << "Alloc request | size=" << std::setw(4) << size
        << " | align=" << std::setw(3) << alignment
        << " | tag=" << std::setw(8) << ToString(tag);

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

    std::cout << "TrackedAllocator<FreeListAllocator> validation test\n";
    PrintSeparator();

    constexpr std::size_t allocatorSize = 4096;
    constexpr std::size_t allocatorAlignment = 64;

    TrackedAllocator<FreeListAllocator> allocator(allocatorSize, allocatorAlignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  total size     = " << allocatorSize << " bytes\n";
    std::cout << "  base alignment = " << allocatorAlignment << " bytes\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();
    PrintSeparator();

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    void* a = allocator.Alloc(128, MemoryTag::General, 16);
    PrintPointerInfo(a, 128, 16, MemoryTag::General);
    assert(a != nullptr && IsAligned(a, 16));
    std::memset(a, 0xAA, 128);

    void* b = allocator.Alloc(256, MemoryTag::Renderer, 32);
    PrintPointerInfo(b, 256, 32, MemoryTag::Renderer);
    assert(b != nullptr && IsAligned(b, 32));
    std::memset(b, 0xBB, 256);

    void* c = allocator.Alloc(64, MemoryTag::Physics, 8);
    PrintPointerInfo(c, 64, 8, MemoryTag::Physics);
    assert(c != nullptr && IsAligned(c, 8));
    std::memset(c, 0xCC, 64);

    void* d = allocator.Alloc(200, MemoryTag::ECS, 64);
    PrintPointerInfo(d, 200, 64, MemoryTag::ECS);
    assert(d != nullptr && IsAligned(d, 64));
    std::memset(d, 0xDD, 200);

    PrintSeparator();
    std::cout << "After deterministic allocations:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 2 - zero-sized allocation\n";
    PrintSeparator();
    {
        void* ptr = allocator.Alloc(0, MemoryTag::General, 16);
        std::cout << "Alloc(0, General, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL (unexpected)") << '\n';
        assert(ptr == nullptr && "Zero-sized allocation should return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 3 - overflow test\n";
    PrintSeparator();
    {
        void* bigPtr = allocator.Alloc(100000, MemoryTag::Audio, 16);
        std::cout << "Alloc(100000, Audio, 16) => " << (bigPtr == nullptr ? "nullptr" : "NON-NULL (unexpected)") << '\n';
        assert(bigPtr == nullptr && "Huge allocation should overflow and return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 4 - free middle block and reallocate into reclaimed space\n";
    PrintSeparator();

    allocator.Free(b);

    std::cout << "After freeing block b:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    void* e = allocator.Alloc(128, MemoryTag::Audio, 16);
    PrintPointerInfo(e, 128, 16, MemoryTag::Audio);
    assert(e != nullptr && IsAligned(e, 16));
    std::memset(e, 0xEE, 128);

    PrintSeparator();
    std::cout << "Allocator state after reuse attempt:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 5 - free adjacent blocks to exercise merge\n";
    PrintSeparator();

    allocator.Free(c);
    allocator.Free(d);

    std::cout << "After freeing blocks c and d:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    void* f = allocator.Alloc(400, MemoryTag::Physics, 32);
    PrintPointerInfo(f, 400, 32, MemoryTag::Physics);
    assert(f != nullptr && IsAligned(f, 32));
    std::memset(f, 0xF0, 400);

    PrintSeparator();
    std::cout << "Allocator state after merged-space allocation:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 6 - free remaining live allocations\n";
    PrintSeparator();

    allocator.Free(a);
    allocator.Free(e);
    allocator.Free(f);

    std::cout << "After freeing all remaining blocks:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 7 - full-size reallocation after complete free\n";
    PrintSeparator();

    void* g = allocator.Alloc(2048, MemoryTag::Renderer, 64);
    PrintPointerInfo(g, 2048, 64, MemoryTag::Renderer);
    assert(g != nullptr && IsAligned(g, 64));
    std::memset(g, 0x11, 2048);

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 8 - reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After Reset():\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 9 - allocations after reset\n";
    PrintSeparator();

    struct TestAlloc
    {
        std::size_t size;
        std::size_t alignment;
        MemoryTag tag;
    };

    const std::vector<TestAlloc> postResetTests =
    {
        {64,  8,  MemoryTag::General},
        {128, 16, MemoryTag::Renderer},
        {256, 32, MemoryTag::Physics},
        {96,  64, MemoryTag::Audio}
    };

    for (const TestAlloc& test : postResetTests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        assert(ptr != nullptr && "Post-reset allocation failed unexpectedly");
        assert(IsAligned(ptr, test.alignment) && "Returned pointer is not correctly aligned after reset");
        std::memset(ptr, 0x5A, test.size);
    }

    PrintSeparator();
    std::cout << "Final allocator state before final reset:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 10 - final reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After final Reset():\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

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