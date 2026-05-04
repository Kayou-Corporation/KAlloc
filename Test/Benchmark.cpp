#include "FreeListAllocator.hpp"
#include "TrackedAllocator.hpp"
#include "MemoryTracker.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <string_view>
#include <type_traits>
#include <vector>

#include "TLSFAllocator.hpp"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <fstream>
#endif

namespace Benchmark
{
    using Clock = std::chrono::steady_clock;
    using Nanoseconds = std::chrono::nanoseconds;

    static constexpr std::size_t kAllocatorSize = 256ull * 1024ull * 1024ull; // 256 MB
    static constexpr std::size_t kAllocatorAlignment = 64;
    static constexpr std::uint32_t kSeed = 0xC0FFEEu;

    template <typename TAllocator>
    void* AllocRaw(TAllocator& allocator, const std::size_t size, const std::size_t alignment)
    {
        return allocator.Alloc(size, alignment);
    }

    template <typename TDerived>
    void* AllocRaw(Kayou::Memory::TrackedAllocator<TDerived>& allocator, const std::size_t size, const std::size_t alignment)
    {
        return allocator.Alloc(size, Kayou::Memory::MemoryTag::General, alignment);
    }

    struct Timer
    {
        Clock::time_point start {};

        void Reset()
        {
            start = Clock::now();
        }

        [[nodiscard]] Nanoseconds Elapsed() const
        {
            return std::chrono::duration_cast<Nanoseconds>(Clock::now() - start);
        }
    };

    [[nodiscard]] static std::uint64_t GetProcessRSSBytes()
    {
    #ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc {};
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
            return static_cast<std::uint64_t>(pmc.WorkingSetSize);
        return 0;
    #elif defined(__linux__)
        std::ifstream statm("/proc/self/statm");
        if (!statm)
            return 0;

        std::uint64_t totalPages = 0;
        std::uint64_t residentPages = 0;
        statm >> totalPages >> residentPages;

        const long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize <= 0)
            return 0;

        return residentPages * static_cast<std::uint64_t>(pageSize);
    #else
        return 0;
    #endif
    }

    [[nodiscard]] static double NsToMs(const Nanoseconds ns)
    {
        return static_cast<double>(ns.count()) / 1'000'000.0;
    }

    struct BenchResult
    {
        std::string_view name {};
        std::size_t allocationCount = 0;
        std::size_t requestedBytes = 0;
        std::size_t successfulAllocs = 0;

        Nanoseconds allocTime {};
        Nanoseconds freeTime {};
        Nanoseconds totalTime {};

        std::uint64_t rssBefore = 0;
        std::uint64_t rssAfterAlloc = 0;
        std::uint64_t rssAfterFree = 0;
        std::uint64_t rssPeakObserved = 0;

        std::size_t allocatorUsedAfterAlloc = 0;
        std::size_t allocatorPeakAfterAlloc = 0;

        std::size_t freeBlockCount = 0;
        std::size_t largestFreeBlock = 0;
        std::size_t totalFreeSize = 0;
        double fragmentationRatio = 0.0;
    };

    static void PrintBytes(const std::uint64_t bytes)
    {
        constexpr double kb = 1024.0;
        constexpr double mb = 1024.0 * 1024.0;
        constexpr double gb = 1024.0 * 1024.0 * 1024.0;

        std::cout << std::fixed << std::setprecision(2);

        if (bytes >= static_cast<std::uint64_t>(gb))
            std::cout << (bytes / gb) << " GB";
        else if (bytes >= static_cast<std::uint64_t>(mb))
            std::cout << (bytes / mb) << " MB";
        else if (bytes >= static_cast<std::uint64_t>(kb))
            std::cout << (bytes / kb) << " KB";
        else
            std::cout << bytes << " B";
    }

    static void PrintResult(const BenchResult& result)
    {
        std::cout << "------------------------------------------------------------\n";
        std::cout << result.name << "\n";
        std::cout << "  allocations           : " << result.allocationCount << '\n';
        std::cout << "  successful allocations: " << result.successfulAllocs << '\n';
        std::cout << "  requested bytes       : ";
        PrintBytes(result.requestedBytes);
        std::cout << '\n';

        std::cout << "  alloc time            : " << NsToMs(result.allocTime) << " ms\n";
        std::cout << "  free time             : " << NsToMs(result.freeTime) << " ms\n";
        std::cout << "  total time            : " << NsToMs(result.totalTime) << " ms\n";

        const double allocOpsPerSec =
            result.allocTime.count() > 0
                ? (static_cast<double>(result.successfulAllocs) * 1'000'000'000.0 / static_cast<double>(result.allocTime.count()))
                : 0.0;

        const double freeOpsPerSec =
            result.freeTime.count() > 0
                ? (static_cast<double>(result.successfulAllocs) * 1'000'000'000.0 / static_cast<double>(result.freeTime.count()))
                : 0.0;

        std::cout << "  alloc throughput      : " << allocOpsPerSec << " alloc/s\n";
        std::cout << "  free throughput       : " << freeOpsPerSec << " free/s\n";

        if (result.rssBefore != 0 || result.rssAfterAlloc != 0 || result.rssAfterFree != 0)
        {
            std::cout << "  RSS before            : ";
            PrintBytes(result.rssBefore);
            std::cout << '\n';

            std::cout << "  RSS after alloc       : ";
            PrintBytes(result.rssAfterAlloc);
            std::cout << '\n';

            std::cout << "  RSS after free        : ";
            PrintBytes(result.rssAfterFree);
            std::cout << '\n';

            std::cout << "  RSS peak observed     : ";
            PrintBytes(result.rssPeakObserved);
            std::cout << '\n';
        }

        if (result.allocatorPeakAfterAlloc != 0 || result.allocatorUsedAfterAlloc != 0)
        {
            std::cout << "  allocator used        : ";
            PrintBytes(result.allocatorUsedAfterAlloc);
            std::cout << '\n';

            std::cout << "  allocator peak        : ";
            PrintBytes(result.allocatorPeakAfterAlloc);
            std::cout << '\n';
        }

        if (result.totalFreeSize != 0 || result.freeBlockCount != 0)
        {
            std::cout << "  free blocks           : " << result.freeBlockCount << '\n';

            std::cout << "  largest free block    : ";
            PrintBytes(result.largestFreeBlock);
            std::cout << '\n';

            std::cout << "  total free size       : ";
            PrintBytes(result.totalFreeSize);
            std::cout << '\n';

            std::cout << "  fragmentation         : "
                      << std::fixed << std::setprecision(2)
                      << result.fragmentationRatio * 100.0 << "%\n";
        }
    }

    struct GameObject
    {
        std::array<std::byte, 256> payload {};
        std::uint64_t id = 0;
        double a = 0.0;
        double b = 0.0;
        double c = 0.0;

        explicit GameObject(const std::uint64_t value)
            : id(value)
            , a(static_cast<double>(value))
            , b(static_cast<double>(value) * 2.0)
            , c(static_cast<double>(value) * 3.0)
        {
            payload[0] = static_cast<std::byte>(value & 0xFFu);
        }
    };

    template <typename TAllocator>
    BenchResult RunObjectBenchmarkAllocator(const std::string_view name, const std::size_t count)
    {
        TAllocator allocator(kAllocatorSize, kAllocatorAlignment);

        std::vector<GameObject*> objects;
        objects.reserve(count);

        BenchResult result {};
        result.name = name;
        result.allocationCount = count;
        result.requestedBytes = count * sizeof(GameObject);

        std::mt19937 rng(kSeed);
        std::vector<std::size_t> indices(count);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);

        result.rssBefore = GetProcessRSSBytes();
        result.rssPeakObserved = result.rssBefore;

        Timer totalTimer;
        totalTimer.Reset();

        Timer allocTimer;
        allocTimer.Reset();

        for (std::size_t i = 0; i < count; ++i)
        {
            void* memory = AllocRaw(allocator, sizeof(GameObject), alignof(GameObject));
            if (memory == nullptr)
                break;

            auto* object = new (memory) GameObject(static_cast<std::uint64_t>(i));
            objects.push_back(object);

            ++result.successfulAllocs;

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.allocTime = allocTimer.Elapsed();
        result.rssAfterAlloc = GetProcessRSSBytes();
        result.rssPeakObserved = std::max(result.rssPeakObserved, result.rssAfterAlloc);

        if constexpr (requires(TAllocator a) { a.GetUsedSize(); a.GetPeakSize(); })
        {
            result.allocatorUsedAfterAlloc = allocator.GetUsedSize();
            result.allocatorPeakAfterAlloc = allocator.GetPeakSize();
        }
        else if constexpr (requires(TAllocator a) { a.GetAllocator().GetUsedSize(); a.GetAllocator().GetPeakSize(); })
        {
            result.allocatorUsedAfterAlloc = allocator.GetAllocator().GetUsedSize();
            result.allocatorPeakAfterAlloc = allocator.GetAllocator().GetPeakSize();
        }

        Timer freeTimer;
        freeTimer.Reset();

        for (const std::size_t shuffledIndex : indices)
        {
            if (shuffledIndex >= objects.size() || objects[shuffledIndex] == nullptr)
                continue;

            GameObject* object = objects[shuffledIndex];
            object->~GameObject();
            allocator.Free(object);

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.freeTime = freeTimer.Elapsed();
        result.totalTime = totalTimer.Elapsed();
        result.rssAfterFree = GetProcessRSSBytes();
        CaptureFreeListStats(allocator, result);

        return result;
    }

    BenchResult RunObjectBenchmarkNewDelete(const std::size_t count)
    {
        std::vector<GameObject*> objects;
        objects.reserve(count);

        BenchResult result {};
        result.name = "new/delete - GameObject";
        result.allocationCount = count;
        result.requestedBytes = count * sizeof(GameObject);

        std::mt19937 rng(kSeed);
        std::vector<std::size_t> indices(count);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);

        result.rssBefore = GetProcessRSSBytes();
        result.rssPeakObserved = result.rssBefore;

        Timer totalTimer;
        totalTimer.Reset();

        Timer allocTimer;
        allocTimer.Reset();

        for (std::size_t i = 0; i < count; ++i)
        {
            GameObject* object = new GameObject(static_cast<std::uint64_t>(i));
            objects.push_back(object);
            ++result.successfulAllocs;

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.allocTime = allocTimer.Elapsed();
        result.rssAfterAlloc = GetProcessRSSBytes();
        result.rssPeakObserved = std::max(result.rssPeakObserved, result.rssAfterAlloc);

        Timer freeTimer;
        freeTimer.Reset();

        for (const std::size_t shuffledIndex : indices)
        {
            delete objects[shuffledIndex];

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.freeTime = freeTimer.Elapsed();
        result.totalTime = totalTimer.Elapsed();
        result.rssAfterFree = GetProcessRSSBytes();

        return result;
    }

    struct BufferAllocation
    {
        void* ptr = nullptr;
        std::size_t size = 0;
        std::size_t alignment = 0;
    };

    template <typename TAllocator>
    BenchResult RunBufferBenchmarkAllocator(const std::string_view name, const std::size_t count)
    {
        TAllocator allocator(kAllocatorSize, kAllocatorAlignment);

        std::vector<BufferAllocation> allocations;
        allocations.reserve(count);

        std::mt19937 rng(kSeed);
        std::uniform_int_distribution<std::size_t> sizeDist(8, 2048);
        const std::array<std::size_t, 5> alignments { 8, 16, 32, 64, alignof(std::max_align_t) };
        std::uniform_int_distribution<std::size_t> alignDist(0, alignments.size() - 1);

        BenchResult result {};
        result.name = name;
        result.allocationCount = count;

        result.rssBefore = GetProcessRSSBytes();
        result.rssPeakObserved = result.rssBefore;

        Timer totalTimer;
        totalTimer.Reset();

        Timer allocTimer;
        allocTimer.Reset();

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t size = sizeDist(rng);
            const std::size_t alignment = alignments[alignDist(rng)];
            result.requestedBytes += size;

            void* memory = AllocRaw(allocator, size, alignment);
            if (memory == nullptr)
                continue;

            std::memset(memory, static_cast<int>(i & 0xFFu), size);

            allocations.push_back({ memory, size, alignment });
            ++result.successfulAllocs;

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.allocTime = allocTimer.Elapsed();
        result.rssAfterAlloc = GetProcessRSSBytes();
        result.rssPeakObserved = std::max(result.rssPeakObserved, result.rssAfterAlloc);

        std::shuffle(allocations.begin(), allocations.end(), rng);

        if constexpr (requires(TAllocator a) { a.GetUsedSize(); a.GetPeakSize(); })
        {
            result.allocatorUsedAfterAlloc = allocator.GetUsedSize();
            result.allocatorPeakAfterAlloc = allocator.GetPeakSize();
        }
        else if constexpr (requires(TAllocator a) { a.GetAllocator().GetUsedSize(); a.GetAllocator().GetPeakSize(); })
        {
            result.allocatorUsedAfterAlloc = allocator.GetAllocator().GetUsedSize();
            result.allocatorPeakAfterAlloc = allocator.GetAllocator().GetPeakSize();
        }

        Timer freeTimer;
        freeTimer.Reset();

        for (const BufferAllocation& alloc : allocations)
        {
            allocator.Free(alloc.ptr);

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.freeTime = freeTimer.Elapsed();
        result.totalTime = totalTimer.Elapsed();
        result.rssAfterFree = GetProcessRSSBytes();
        CaptureFreeListStats(allocator, result);

        return result;
    }

    BenchResult RunBufferBenchmarkNewDelete(const std::size_t count)
    {
        std::vector<BufferAllocation> allocations;
        allocations.reserve(count);

        std::mt19937 rng(kSeed);
        std::uniform_int_distribution<std::size_t> sizeDist(8, 2048);
        const std::array<std::size_t, 5> alignments { 8, 16, 32, 64, alignof(std::max_align_t) };
        std::uniform_int_distribution<std::size_t> alignDist(0, alignments.size() - 1);

        BenchResult result {};
        result.name = "new/delete - variable buffers";
        result.allocationCount = count;

        result.rssBefore = GetProcessRSSBytes();
        result.rssPeakObserved = result.rssBefore;

        Timer totalTimer;
        totalTimer.Reset();

        Timer allocTimer;
        allocTimer.Reset();

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t size = sizeDist(rng);
            const std::size_t alignment = alignments[alignDist(rng)];
            result.requestedBytes += size;

            void* memory = ::operator new(size, std::align_val_t(alignment), std::nothrow);
            if (memory == nullptr)
                continue;

            std::memset(memory, static_cast<int>(i & 0xFFu), size);
            allocations.push_back({ memory, size, alignment });
            ++result.successfulAllocs;

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.allocTime = allocTimer.Elapsed();
        result.rssAfterAlloc = GetProcessRSSBytes();
        result.rssPeakObserved = std::max(result.rssPeakObserved, result.rssAfterAlloc);

        std::shuffle(allocations.begin(), allocations.end(), rng);

        Timer freeTimer;
        freeTimer.Reset();

        for (const BufferAllocation& alloc : allocations)
        {
            ::operator delete(alloc.ptr, std::align_val_t(alloc.alignment));

            const std::uint64_t rss = GetProcessRSSBytes();
            result.rssPeakObserved = std::max(result.rssPeakObserved, rss);
        }

        result.freeTime = freeTimer.Elapsed();
        result.totalTime = totalTimer.Elapsed();
        result.rssAfterFree = GetProcessRSSBytes();

        return result;
    }


    template <typename TAllocator>
    void CaptureFreeListStats(TAllocator& allocator, BenchResult& result)
    {
        if constexpr (requires(TAllocator a)
        {
            a.GetFreeBlockCount();
            a.GetLargestFreeBlockSize();
            a.GetTotalFreeSize();
            a.GetFragmentationRatio();
        })
        {
            result.freeBlockCount = allocator.GetFreeBlockCount();
            result.largestFreeBlock = allocator.GetLargestFreeBlockSize();
            result.totalFreeSize = allocator.GetTotalFreeSize();
            result.fragmentationRatio = allocator.GetFragmentationRatio();
        }
        else if constexpr (requires(TAllocator a)
        {
            a.GetAllocator().GetFreeBlockCount();
            a.GetAllocator().GetLargestFreeBlockSize();
            a.GetAllocator().GetTotalFreeSize();
            a.GetAllocator().GetFragmentationRatio();
        })
        {
            result.freeBlockCount = allocator.GetAllocator().GetFreeBlockCount();
            result.largestFreeBlock = allocator.GetAllocator().GetLargestFreeBlockSize();
            result.totalFreeSize = allocator.GetAllocator().GetTotalFreeSize();
            result.fragmentationRatio = allocator.GetAllocator().GetFragmentationRatio();
        }
    }
}

int main()
{
    using namespace Benchmark;
    using Kayou::Memory::FreeListAllocator;
    using Kayou::Memory::TrackedAllocator;
    using Kayou::Memory::TLSFAllocator;

    constexpr std::size_t kObjectCount = 5000;
    constexpr std::size_t kBufferCount = 5000;

    std::cout << "================ MEMORY BENCHMARK ================\n";
    std::cout << "Allocator size : " << kAllocatorSize / (1024ull * 1024ull) << " MB\n";
    std::cout << "Object count   : " << kObjectCount << '\n';
    std::cout << "Buffer count   : " << kBufferCount << "\n\n";

    const BenchResult objectNewDelete = RunObjectBenchmarkNewDelete(kObjectCount);
    const BenchResult objectFreeList = RunObjectBenchmarkAllocator<FreeListAllocator>("FreeListAllocator - GameObject", kObjectCount);
    const BenchResult objectTrackedFreeList = RunObjectBenchmarkAllocator<TrackedAllocator<FreeListAllocator>>("TrackedAllocator<FreeListAllocator> - GameObject", kObjectCount);

    const BenchResult bufferNewDelete = RunBufferBenchmarkNewDelete(kBufferCount);
    const BenchResult bufferFreeList = RunBufferBenchmarkAllocator<FreeListAllocator>("FreeListAllocator - variable buffers", kBufferCount);
    const BenchResult bufferTrackedFreeList = RunBufferBenchmarkAllocator<TrackedAllocator<FreeListAllocator>>("TrackedAllocator<FreeListAllocator> - variable buffers", kBufferCount);
    const BenchResult objectTLSF = RunObjectBenchmarkAllocator<TLSFAllocator>("TLSFAllocator - GameObject", kObjectCount);
    const BenchResult objectTrackedTLSF = RunObjectBenchmarkAllocator<TrackedAllocator<TLSFAllocator>>("TrackedAllocator<TLSFAllocator> - GameObject", kObjectCount);
    const BenchResult bufferTLSF = RunBufferBenchmarkAllocator<TLSFAllocator>("TLSFAllocator - variable buffers", kBufferCount);
    const BenchResult bufferTrackedTLSF = RunBufferBenchmarkAllocator<TrackedAllocator<TLSFAllocator>>("TrackedAllocator<TLSFAllocator> - variable buffers", kBufferCount);

    std::cout << "\nOBJECT BENCHMARKS\n";
    PrintResult(objectNewDelete);
    PrintResult(objectFreeList);
    PrintResult(objectTrackedFreeList);
    PrintResult(objectTLSF);
    PrintResult(objectTrackedTLSF);

    std::cout << "\nVARIABLE BUFFER BENCHMARKS\n";
    PrintResult(bufferNewDelete);
    PrintResult(bufferFreeList);
    PrintResult(bufferTrackedFreeList);
    PrintResult(bufferTLSF);
    PrintResult(bufferTrackedTLSF);

    std::cout << "\n================ DONE ================\n";

    getchar();
    return 0;
}