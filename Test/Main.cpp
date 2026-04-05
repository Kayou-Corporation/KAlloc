#include "LinearAllocator.hpp"
#include "TrackedAllocator.hpp"
#include "MemoryTracker.hpp"

#include <cassert>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

#ifdef KAYOU_USE_TRACY
#include <tracy/Tracy.hpp>
#endif

int main()
{
    std::cout << "Starting randomized allocator stress test...\n";

    constexpr std::size_t AllocatorSize = 1024 * 1024; // 1 MB
    Kayou::TrackedAllocator<Kayou::LinearAllocator> allocator(AllocatorSize);

    std::mt19937 rng(std::random_device{}());

    std::uniform_int_distribution<int> allocCountDist(20, 120);
    std::uniform_int_distribution<int> sizeDist(32, 2048);
    std::uniform_int_distribution<int> alignPowDist(4, 6); // 2^4=16, 2^5=32, 2^6=64
    std::uniform_int_distribution<int> tagDist(0, static_cast<int>(Kayou::MemoryTag::COUNT) - 1);

    std::uint64_t frame = 0;

    while (true)
    {
#ifdef KAYOU_USE_TRACY
        ZoneScoped;
        FrameMark;
#endif

        int allocationCount = allocCountDist(rng);

        for (int i = 0; i < allocationCount; ++i)
        {
            std::size_t size = sizeDist(rng);
            std::size_t alignment = 1ull << alignPowDist(rng);

            auto tag = static_cast<Kayou::MemoryTag>(tagDist(rng));

            void* ptr = allocator.Alloc(size, tag, alignment);

            if (ptr)
            {
                std::memset(ptr, 0xCD, size);
            }
        }

        if (frame % 120 == 0)
        {
            std::cout << "\n[Frame " << frame << "]\n";
            allocator.PrintUsage();
            Kayou::MemoryTracker::PrintReport();
            std::cout << "-----------------------------\n";
        }

        allocator.Reset();

        ++frame;

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}