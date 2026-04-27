#include "LinearAllocator.hpp"
#include "TrackedAllocator.hpp"

#include <cassert>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t
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


void PrintPointerInfo(const void* ptr, std::size_t size, std::size_t alignment, Kayou::Memory::MemoryTag tag)
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
    std::cout << "TrackedAllocator<LinearAllocator> validation test\n";
    PrintSeparator();

    constexpr std::size_t allocatorSize = 4096;
    Kayou::Memory::TrackedAllocator<Kayou::Memory::LinearAllocator> allocator(allocatorSize);

    std::cout << "Allocator created with total size = " << allocatorSize << " bytes\n";
    allocator.PrintUsage();
    Kayou::Memory::MemoryTracker::PrintReport();
    PrintSeparator();

    struct TestAlloc
    {
        std::size_t size;
        std::size_t alignment;
        Kayou::Memory::MemoryTag tag;
    };

    const std::vector<TestAlloc> tests =
    {
        {24, 8, Kayou::Memory::MemoryTag::General},
        {64, 16, Kayou::Memory::MemoryTag::Renderer},
        {128, 32, Kayou::Memory::MemoryTag::Physics},
        {80, 64, Kayou::Memory::MemoryTag::ECS},
        {200, 16, Kayou::Memory::MemoryTag::Audio},
        {13, 8, Kayou::Memory::MemoryTag::Renderer},
        {97, 32, Kayou::Memory::MemoryTag::General},
        {256, 64, Kayou::Memory::MemoryTag::Physics}
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
    Kayou::Memory::MemoryTracker::PrintReport();
    PrintSeparator();

    std::cout << "PHASE 2 - zero-sized allocation\n";
    PrintSeparator(); {
        void* ptr = allocator.Alloc(0, Kayou::Memory::MemoryTag::General, 16);
        std::cout << "Alloc(0, General, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL (unexpected)") << '\n';
        assert(ptr == nullptr && "Zero-sized allocation should return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 3 - overflow test\n";
    PrintSeparator(); {
        void* bigPtr = allocator.Alloc(100000, Kayou::Memory::MemoryTag::Renderer, 16);
        std::cout << "Alloc(100000, Renderer, 16) => " << (bigPtr == nullptr ? "nullptr" : "NON-NULL (unexpected)")
                << '\n';
        assert(bigPtr == nullptr && "Huge allocation should overflow and return nullptr");
    }

    PrintSeparator();
    std::cout << "PHASE 4 - allocator state before reset\n";
    PrintSeparator();

    allocator.PrintUsage();
    Kayou::Memory::MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 5 - reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After Reset():\n";
    allocator.PrintUsage();
    Kayou::Memory::MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 6 - allocations after reset\n";
    PrintSeparator();

    const std::vector<TestAlloc> postResetTests =
    {
        {32, 8, Kayou::Memory::MemoryTag::General},
        {64, 16, Kayou::Memory::MemoryTag::Renderer},
        {128, 32, Kayou::Memory::MemoryTag::Physics},
        {48, 64, Kayou::Memory::MemoryTag::Audio}
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
    Kayou::Memory::MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "PHASE 7 - final reset\n";
    PrintSeparator();

    allocator.Reset();

    std::cout << "After final Reset():\n";
    allocator.PrintUsage();
    Kayou::Memory::MemoryTracker::PrintReport();

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    getchar(); // Required on some platforms to prevent the console from instantly closing
    return 0;
}
