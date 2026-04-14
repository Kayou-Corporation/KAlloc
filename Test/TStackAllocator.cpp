#include "StackAllocator.hpp"
#include "TrackedAllocator.hpp"

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

    std::cout << "TrackedAllocator<StackAllocator> validation test\n";
    PrintSeparator();

    constexpr std::size_t allocatorSize = 4096;
    constexpr std::size_t allocatorAlignment = 64;

    TrackedAllocator<StackAllocator> allocator(allocatorSize, allocatorAlignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  total size = " << allocatorSize << " bytes\n";
    std::cout << "  base alignment = " << allocatorAlignment << " bytes\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();
    PrintSeparator();

    struct TestAlloc
    {
        std::size_t size;
        std::size_t alignment;
        MemoryTag tag;
    };

    const std::vector<TestAlloc> tests =
    {
        {24,  8,  MemoryTag::General},
        {64,  16, MemoryTag::Renderer},
        {128, 32, MemoryTag::Physics},
        {80,  64, MemoryTag::ECS},
        {200, 16, MemoryTag::Audio},
        {13,  8,  MemoryTag::Renderer},
        {97,  32, MemoryTag::General},
        {256, 64, MemoryTag::Physics}
    };

    std::vector<void*> allocatedPtrs;
    allocatedPtrs.reserve(tests.size());

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    for (const TestAlloc& test : tests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

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
        void* bigPtr = allocator.Alloc(100000, MemoryTag::Renderer, 16);
        std::cout << "Alloc(100000, Renderer, 16) => " << (bigPtr == nullptr ? "nullptr" : "NON-NULL (unexpected)") << '\n';
        assert(bigPtr == nullptr && "Huge allocation should overflow and return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 4 - free last allocation (valid LIFO)\n";
    PrintSeparator();
    {
        auto it = std::find_if(allocatedPtrs.rbegin(), allocatedPtrs.rend(),
            [](void* p)
            {
                return p != nullptr;
            });

        assert(it != allocatedPtrs.rend() && "No valid allocation available to free");

        void* lastPtr = *it;
        allocator.Free(lastPtr);

        std::cout << "After freeing last allocation:\n";
        allocator.PrintUsage();
        MemoryTracker::PrintReport();

        const TestAlloc retry {256, 64, MemoryTag::Physics};
        void* ptr = allocator.Alloc(retry.size, retry.tag, retry.alignment);
        PrintPointerInfo(ptr, retry.size, retry.alignment, retry.tag);

        assert(ptr != nullptr && "Re-allocation after LIFO free should succeed");
        assert(ptr == lastPtr && "StackAllocator should reuse the exact same top memory after LIFO free");
        assert(IsAligned(ptr, retry.alignment) && "Reused pointer is not correctly aligned");

        std::memset(ptr, 0xEF, retry.size);
        *it = ptr;
    }

    PrintSeparator();
    std::cout << "PHASE 5 - allocator state before reset\n";
    PrintSeparator();
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 6 - reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After Reset():\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 7 - allocations after reset\n";
    PrintSeparator();

    const std::vector<TestAlloc> postResetTests =
    {
        {32,  8,  MemoryTag::General},
        {64,  16, MemoryTag::Renderer},
        {128, 32, MemoryTag::Physics},
        {48,  64, MemoryTag::Audio}
    };

    for (const TestAlloc& test : postResetTests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        if (ptr != nullptr)
        {
            assert(IsAligned(ptr, test.alignment) && "Returned pointer is not correctly aligned after reset");
            std::memset(ptr, 0xAB, test.size);
        }
    }

    PrintSeparator();
    std::cout << "Final allocator state before final reset:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 8 - final reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After final Reset():\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

#ifdef KAYOU_DEBUG
    /*
    PrintSeparator();
    std::cout << "PHASE 9 - invalid free order (DEBUG ASSERT EXPECTED)\n";
    PrintSeparator();

    TrackedAllocator<StackAllocator> debugAllocator(1024, 64);
    void* a = debugAllocator.Alloc(64, MemoryTag::General, 16);
    void* b = debugAllocator.Alloc(64, MemoryTag::Renderer, 16);

    (void)b;
    debugAllocator.Free(a); // Should assert in debug: not LIFO
    */
#endif

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    getchar();
    return 0;
}