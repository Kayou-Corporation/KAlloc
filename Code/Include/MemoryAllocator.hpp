#pragma once

#include <concepts>
#include <cstddef>



namespace Kayou::Memory
{
    template <typename A>
    concept Allocator = requires(A a, std::size_t size, std::size_t memAlignment)
    {
        { a.Alloc(size, memAlignment) } -> std::same_as<void*>;
    };


    template <typename A>
    concept FreeableAllocator = Allocator<A> && requires(A a, void* ptr)
    {
        { a.Free(ptr) } -> std::same_as<void>;
    };


    template <typename A>
    concept ResettableAllocator = Allocator<A> && requires(A a)
    {
        { a.Reset() } -> std::same_as<void>;
    };


    template <typename A>
    concept PrintableAllocator = requires(A a)
    {
        { a.PrintUsage() } -> std::same_as<void>;
    };
}