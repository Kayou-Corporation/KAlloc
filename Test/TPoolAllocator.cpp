#include <iostream>
#include <vector>

#include "TrackedAllocator.hpp"
#include "LinearAllocator.hpp"
#include "PoolAllocator.hpp"


#pragma region tests
int main()
{
    using namespace Kayou;

    std::cout << "===== KAlloc Test =====\n\n";

    // ---------------------------
    // LinearAllocator + Tracking
    // ---------------------------
    {
        std::cout << "[LinearAllocator + Tracked]\n";

        Kayou::Memory::TrackedAllocator<Kayou::Memory::LinearAllocator> linear(1024);

        [[maybe_unused]] void* a = linear.Alloc(128, Kayou::Memory::MemoryTag::General);
        [[maybe_unused]] void* b = linear.Alloc(256, Kayou::Memory::MemoryTag::Renderer);
        [[maybe_unused]] void* c = linear.Alloc(64, Kayou::Memory::MemoryTag::Physics);

        linear.PrintUsage();
        Kayou::Memory::MemoryTracker::PrintReport();

        std::cout << "-- Reset Linear --\n";
        linear.Reset();

        linear.PrintUsage();
        Kayou::Memory::MemoryTracker::PrintReport();
    }

    std::cout << "\n---------------------------\n\n";

    // ---------------------------
    // PoolAllocator + Tracking
    // ---------------------------
    {
        std::cout << "[PoolAllocator + Tracked]\n";

        Kayou::Memory::TrackedAllocator<Kayou::Memory::PoolAllocator> pool(128, 8);

        std::vector<void*> ptrs;

        // Allocate all blocks
        for (int i = 0; i < 8; ++i)
        {
            void* p = pool.Alloc(64, Kayou::Memory::MemoryTag::ECS);
            if (p)
                ptrs.push_back(p);
        }

        pool.PrintUsage();
        Kayou::Memory::MemoryTracker::PrintReport();

        // Free half
        std::cout << "-- Free half --\n";
        for (size_t i = 0; i < ptrs.size() / 2; ++i)
            pool.Free(ptrs[i]);

        pool.PrintUsage();
        Kayou::Memory::MemoryTracker::PrintReport();

        // Allocate again
        std::cout << "-- Reallocate --\n";
        for (size_t i = 0; i < ptrs.size() / 2; ++i)
            pool.Alloc(32, Kayou::Memory::MemoryTag::Audio);

        pool.PrintUsage();
        Kayou::Memory::MemoryTracker::PrintReport();

        // Reset
        std::cout << "-- Reset Pool --\n";
        pool.Reset();

        pool.PrintUsage();
        Kayou::Memory::MemoryTracker::PrintReport();
    }

    std::cout << "\n===== END =====\n";

    getchar(); // Required on some platforms to prevent the console from instantly closing
    return 0;
}