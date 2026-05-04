#include "TLSFAllocator.hpp"
#include "TrackedAllocator.hpp"
#include "MemoryTracker.hpp"

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

void PrintPointerInfo(const void* ptr, const std::size_t size, const std::size_t alignment, const Kayou::Memory::MemoryTag tag)
{
    std::cout
        << "Alloc request | size=" << std::setw(4) << size
        << " | align=" << std::setw(3) << alignment
        << " | tag=" << std::setw(8) << Kayou::Memory::MemoryTracker::GetTagName(tag);

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
    using Kayou::Memory::MemoryTag;
    using Kayou::Memory::TLSFAllocator;
    using Kayou::Memory::TrackedAllocator;
    using Kayou::Memory::MemoryTracker;

    std::cout << "TrackedAllocator<TLSFAllocator> validation test\n";
    PrintSeparator();

    constexpr std::size_t allocatorSize = 4096;
    constexpr std::size_t allocatorAlignment = 64;

    TrackedAllocator<TLSFAllocator> allocator(allocatorSize, allocatorAlignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  total size     = " << allocatorSize << " bytes\n";
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
        {128, 16, MemoryTag::General},
        {256, 32, MemoryTag::Renderer},
        {64,   8, MemoryTag::Physics},
        {200, 64, MemoryTag::ECS}
    };

    std::vector<void*> ptrs;
    ptrs.reserve(tests.size());

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    for (const TestAlloc& test : tests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        assert(ptr != nullptr && "Tracked TLSF allocation failed unexpectedly");
        assert(IsAligned(ptr, test.alignment) && "Tracked TLSF returned misaligned pointer");

        std::memset(ptr, 0xCD, test.size);
        ptrs.push_back(ptr);
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
        std::cout << "Alloc(0, General, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    PrintSeparator();
    std::cout << "PHASE 3 - overflow test\n";
    PrintSeparator();

    {
        void* ptr = allocator.Alloc(100000, MemoryTag::Audio, 16);
        std::cout << "Alloc(100000, Audio, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    PrintSeparator();
    std::cout << "PHASE 4 - free middle block and reallocate\n";
    PrintSeparator();

    allocator.Free(ptrs[1]);
    ptrs[1] = nullptr;

    std::cout << "After freeing block b:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    void* reused = allocator.Alloc(128, MemoryTag::Audio, 16);
    PrintPointerInfo(reused, 128, 16, MemoryTag::Audio);

    assert(reused != nullptr);
    assert(IsAligned(reused, 16));

    ptrs[1] = reused;

    PrintSeparator();
    std::cout << "Allocator state after reuse attempt:\n";
    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 5 - free all remaining allocations\n";
    PrintSeparator();

    for (void*& ptr : ptrs)
    {
        if (ptr != nullptr)
        {
            allocator.Free(ptr);
            ptr = nullptr;
        }
    }

    std::cout << "After freeing all blocks:\n";
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

    const std::vector<TestAlloc> resetTests =
    {
        {64,   8, MemoryTag::General},
        {128, 16, MemoryTag::Renderer},
        {256, 32, MemoryTag::Physics},
        {96,  64, MemoryTag::Audio}
    };

    for (const TestAlloc& test : resetTests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        assert(ptr != nullptr && "Tracked TLSF allocation after reset failed unexpectedly");
        assert(IsAligned(ptr, test.alignment));

        std::memset(ptr, 0xAB, test.size);
        ptrs.push_back(ptr);
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

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    return 0;
}