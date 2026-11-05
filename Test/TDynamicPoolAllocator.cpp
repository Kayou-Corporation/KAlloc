#include "DynamicPoolAllocator.hpp"
#include "TrackedAllocator.hpp"
#include "MemoryTracker.hpp"
#include "AllocationHeader.hpp"

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
    using Kayou::Memory::DynamicPoolAllocator;
    using Kayou::Memory::TrackedAllocator;
    using Kayou::Memory::MemoryTag;
    using Kayou::Memory::MemoryTracker;

    std::cout << "TrackedAllocator<DynamicPoolAllocator> validation test\n";
    PrintSeparator();

    constexpr std::size_t userBlockCapacity = 128;
    constexpr std::size_t blocksPerPage = 4;
    constexpr std::size_t alignment = 64;

    constexpr std::size_t trackedBlockCapacity =
        userBlockCapacity
        + sizeof(Kayou::Memory::Internal::AllocationHeader)
        + alignment;

    TrackedAllocator<DynamicPoolAllocator> allocator(
        trackedBlockCapacity,
        blocksPerPage,
        alignment
    );

    std::cout << "Allocator created with:\n";
    std::cout << "  user block capacity    = " << userBlockCapacity << " bytes\n";
    std::cout << "  tracked block capacity = " << trackedBlockCapacity << " bytes\n";
    std::cout << "  blocks/page            = " << blocksPerPage << '\n';
    std::cout << "  alignment              = " << alignment << " bytes\n";

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
        {64,  16, MemoryTag::General},
        {96,  32, MemoryTag::Renderer},
        {128, 64, MemoryTag::Physics},
        {32,   8, MemoryTag::ECS},
        {80,  16, MemoryTag::Audio},
        {48,  32, MemoryTag::General},
        {72,   8, MemoryTag::Renderer},
        {120, 64, MemoryTag::Physics},
        {16,  16, MemoryTag::ECS},
        {100, 32, MemoryTag::Audio}
    };

    std::vector<void*> ptrs;
    ptrs.reserve(tests.size());

    std::cout << "PHASE 1 - allocate more than one page\n";
    PrintSeparator();

    for (const TestAlloc& test : tests)
    {
        void* ptr = allocator.Alloc(test.size, test.tag, test.alignment);
        PrintPointerInfo(ptr, test.size, test.alignment, test.tag);

        assert(ptr != nullptr && "Tracked DynamicPool allocation failed unexpectedly");
        assert(IsAligned(ptr, test.alignment));

        std::memset(ptr, 0xCD, test.size);
        ptrs.push_back(ptr);
    }

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetPageCount() == 3);
    assert(allocator.GetAllocator().GetUsedBlocks() == 10);
    assert(allocator.GetAllocator().GetFreeBlocks() == 2);

    PrintSeparator();
    std::cout << "PHASE 2 - invalid requests\n";
    PrintSeparator();

    {
        void* ptr = allocator.Alloc(0, MemoryTag::General, 16);
        std::cout << "Alloc(0, General, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    {
        void* ptr = allocator.Alloc(256, MemoryTag::General, 16);
        std::cout << "Alloc(256, General, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    {
        void* ptr = allocator.Alloc(64, MemoryTag::General, 128);
        std::cout << "Alloc(64, General, 128) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
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
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetUsedBlocks() == 5);

    PrintSeparator();
    std::cout << "PHASE 4 - reuse freed blocks\n";
    PrintSeparator();

    for (std::size_t i = 0; i < 5; ++i)
    {
        void* ptr = allocator.Alloc(64, MemoryTag::Audio, 16);
        PrintPointerInfo(ptr, 64, 16, MemoryTag::Audio);

        assert(ptr != nullptr);
        assert(IsAligned(ptr, 16));

        ptrs.push_back(ptr);
    }

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetUsedBlocks() == 10);
    assert(allocator.GetAllocator().GetPageCount() == 3);

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
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetUsedBlocks() == 0);

    PrintSeparator();
    std::cout << "PHASE 6 - reset\n";
    PrintSeparator();

    allocator.Reset();

    allocator.PrintUsage();
    MemoryTracker::PrintReport();

    assert(allocator.GetAllocator().GetUsedBlocks() == 0);
    assert(allocator.GetAllocator().GetPageCount() == 3);

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    return 0;
}