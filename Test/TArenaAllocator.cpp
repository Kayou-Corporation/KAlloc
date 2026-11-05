#include "ArenaAllocator.hpp"
#include "TrackedAllocator.hpp"
#include "MemoryTracker.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

const char* TagToString(Kayou::Memory::MemoryTag tag)
{
    using Kayou::Memory::MemoryTag;

    switch (tag)
    {
        case MemoryTag::General:  return "General";
        case MemoryTag::Renderer: return "Renderer";
        case MemoryTag::Physics:  return "Physics";
        case MemoryTag::ECS:      return "ECS";
        case MemoryTag::Audio:    return "Audio";
        default:                  return "Unknown";
    }
}

bool IsAligned(const void* ptr, const std::size_t alignment)
{
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}

void PrintSeparator()
{
    std::cout << "============================================================\n";
}

void PrintPointerInfo(
    const void* ptr,
    const std::size_t size,
    const std::size_t alignment,
    const Kayou::Memory::MemoryTag tag)
{
    std::cout
        << "Alloc request | size=" << std::setw(4) << size
        << " | align=" << std::setw(3) << alignment
        << " | tag=" << std::setw(8) << TagToString(tag);

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
    using Kayou::Memory::ArenaAllocator;
    using Kayou::Memory::TrackedAllocator;
    using Kayou::Memory::MemoryTag;
    using Kayou::Memory::MemoryTracker;

    std::cout << "TrackedAllocator<ArenaAllocator> validation test\n";
    PrintSeparator();

    constexpr std::size_t pageSize = 1024;
    constexpr std::size_t alignment = 64;

    TrackedAllocator<ArenaAllocator> allocator(pageSize, alignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  page size = " << pageSize << " bytes\n";
    std::cout << "  alignment = " << alignment << " bytes\n";

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
        {64,   8, MemoryTag::General},
        {128, 16, MemoryTag::Renderer},
        {256, 32, MemoryTag::Physics},
        {200, 64, MemoryTag::ECS}
    };

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    std::vector<void*> ptrs;
    ptrs.reserve(tests.size());

    for (const TestAlloc& test : tests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        assert(ptr != nullptr);
        assert(IsAligned(ptr, test.alignment));

        std::memset(ptr, 0xCD, test.size);
        ptrs.push_back(ptr);
    }

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
    std::cout << "PHASE 3 - force new page\n";
    PrintSeparator();

    void* large = allocator.Alloc(2048, MemoryTag::Audio, 64);
    PrintPointerInfo(large, 2048, 64, MemoryTag::Audio);

    assert(large != nullptr);
    assert(IsAligned(large, 64));
    assert(allocator.GetAllocator().GetPageCount() == 2);

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 4 - reset clears tracked allocations\n";
    PrintSeparator();

    allocator.Reset();

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetUsedSize() == 0);

    PrintSeparator();
    std::cout << "PHASE 5 - allocations after reset\n";
    PrintSeparator();

    const std::vector<TestAlloc> resetTests =
    {
        {32,   8, MemoryTag::General},
        {64,  16, MemoryTag::Renderer},
        {128, 32, MemoryTag::Physics},
        {96,  64, MemoryTag::Audio}
    };

    for (const TestAlloc& test : resetTests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        assert(ptr != nullptr);
        assert(IsAligned(ptr, test.alignment));

        std::memset(ptr, 0xAB, test.size);
    }

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 6 - final reset\n";
    PrintSeparator();

    allocator.Reset();

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetUsedSize() == 0);

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    return 0;
}