#include "ArenaAllocator.hpp"

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
    using Kayou::Memory::ArenaAllocator;

    std::cout << "ArenaAllocator validation test\n";
    PrintSeparator();

    constexpr std::size_t pageSize = 1024;
    constexpr std::size_t alignment = 64;

    ArenaAllocator allocator(pageSize, alignment);

    std::cout << "Allocator created with:\n";
    std::cout << "  page size = " << pageSize << " bytes\n";
    std::cout << "  alignment = " << alignment << " bytes\n";

    allocator.PrintUsage();
    PrintSeparator();

    std::vector<void*> ptrs;

    std::cout << "PHASE 1 - deterministic allocations\n";
    PrintSeparator();

    const std::vector<std::pair<std::size_t, std::size_t>> tests =
    {
        {64, 8},
        {128, 16},
        {256, 32},
        {200, 64}
    };

    for (const auto& [size, memAlignment] : tests)
    {
        void* ptr = allocator.Alloc(size, memAlignment);
        PrintPointerInfo(ptr, size, memAlignment);

        assert(ptr != nullptr);
        assert(IsAligned(ptr, memAlignment));

        std::memset(ptr, 0xCD, size);
        ptrs.push_back(ptr);
    }

    allocator.PrintUsage();
    assert(allocator.GetPageCount() == 1);

    PrintSeparator();
    std::cout << "PHASE 2 - marker / rollback\n";
    PrintSeparator();

    ArenaAllocator::Marker marker = allocator.SaveMarker();

    void* tempA = allocator.Alloc(128, 16);
    void* tempB = allocator.Alloc(96, 32);

    PrintPointerInfo(tempA, 128, 16);
    PrintPointerInfo(tempB, 96, 32);

    assert(tempA != nullptr);
    assert(tempB != nullptr);

    [[maybe_unused]] const std::size_t usedBeforeRollback = allocator.GetUsedSize();

    allocator.RollbackToMarker(marker);

    std::cout << "After rollback:\n";
    allocator.PrintUsage();

    assert(allocator.GetUsedSize() < usedBeforeRollback);

    PrintSeparator();
    std::cout << "PHASE 3 - allocation after rollback should reuse space\n";
    PrintSeparator();

    void* reused = allocator.Alloc(128, 16);
    PrintPointerInfo(reused, 128, 16);

    assert(reused != nullptr);
    assert(IsAligned(reused, 16));
    assert(reused == tempA && "Arena rollback did not reuse expected memory");

    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 4 - force new page\n";
    PrintSeparator();

    void* large = allocator.Alloc(2048, 64);
    PrintPointerInfo(large, 2048, 64);

    assert(large != nullptr);
    assert(IsAligned(large, 64));
    assert(allocator.GetPageCount() == 2);

    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "PHASE 4.5 - multi-page marker / rollback\n";
    PrintSeparator();

    allocator.Reset();

    auto multiPageMarker = allocator.SaveMarker();

    void* first = allocator.Alloc(256, 16);
    PrintPointerInfo(first, 256, 16);

    void* big = allocator.Alloc(2048, 64);
    PrintPointerInfo(big, 2048, 64);

    void* afterBig = allocator.Alloc(128, 16);
    PrintPointerInfo(afterBig, 128, 16);

    assert(first != nullptr);
    assert(big != nullptr);
    assert(afterBig != nullptr);
    assert(allocator.GetPageCount() == 2);
    assert(allocator.GetUsedSize() > 0);

    allocator.RollbackToMarker(multiPageMarker);

    std::cout << "After multi-page rollback:\n";
    allocator.PrintUsage();

    assert(allocator.GetUsedSize() == 0);

    void* reusedAfterRollback = allocator.Alloc(256, 16);
    PrintPointerInfo(reusedAfterRollback, 256, 16);

    assert(reusedAfterRollback == first && "Arena multi-page rollback did not reuse first page memory");

    PrintSeparator();
    std::cout << "PHASE 5 - invalid requests\n";
    PrintSeparator();

    {
        void* ptr = allocator.Alloc(0, 16);
        std::cout << "Alloc(0, 16) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    {
        void* ptr = allocator.Alloc(64, 128);
        std::cout << "Alloc(64, 128) => " << (ptr == nullptr ? "nullptr" : "NON-NULL") << '\n';
        assert(ptr == nullptr);
    }

    PrintSeparator();
    std::cout << "PHASE 6 - reset\n";
    PrintSeparator();

    allocator.Reset();

    allocator.PrintUsage();

    assert(allocator.GetUsedSize() == 0);
    assert(allocator.GetPageCount() == 2);

    PrintSeparator();
    std::cout << "PHASE 7 - allocation after reset\n";
    PrintSeparator();

    void* afterReset = allocator.Alloc(128, 16);
    PrintPointerInfo(afterReset, 128, 16);

    assert(afterReset != nullptr);
    assert(IsAligned(afterReset, 16));

    allocator.PrintUsage();

    PrintSeparator();
    std::cout << "All validation steps completed.\n";

    return 0;
}