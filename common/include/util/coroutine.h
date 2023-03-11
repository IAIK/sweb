#pragma once

#include <cstddef>

#include <EASTL/functional.h>

#include "assert.h"
#include "debug.h"

namespace std
{
    template<class, class...>
    struct coroutine_traits { };

    template<class R, class... Args>
    requires requires { typename R::promise_type; }
    struct coroutine_traits<R, Args...>
    {
        using promise_type = typename R::promise_type;
    };


    template< class Promise = void>
    struct coroutine_handle;

    template<>
    struct coroutine_handle<void>
    {
        constexpr coroutine_handle() noexcept : coro_frame_ptr_(nullptr) { }
        constexpr coroutine_handle(std::nullptr_t) noexcept : coro_frame_ptr_(nullptr) { }

        coroutine_handle(const coroutine_handle&) = default;
        coroutine_handle(coroutine_handle&&) = default;
        coroutine_handle& operator=(const coroutine_handle&) = default;
        coroutine_handle& operator=(coroutine_handle&&) = default;

        coroutine_handle& operator=(std::nullptr_t np) noexcept
        {
            coro_frame_ptr_ = np;
            return *this;
        }

        bool done() const noexcept
        {
            return __builtin_coro_done(coro_frame_ptr_);
        }

        void resume()
        {
            __builtin_coro_resume(coro_frame_ptr_);
        }

        void operator()()
        {
            resume();
        }

        void destroy()
        {
            __builtin_coro_destroy(coro_frame_ptr_);
            coro_frame_ptr_ = nullptr;
        }

        explicit operator bool() const noexcept
        {
            return bool(address());
        }

        constexpr void* address() const noexcept
        {
            return coro_frame_ptr_;
        }

        constexpr static coroutine_handle from_address(void* address) noexcept
        {
            return {address};
        }

        friend auto operator<=>(const coroutine_handle&, const coroutine_handle&) = default;
    protected:
        constexpr coroutine_handle(void* addr) : coro_frame_ptr_(addr) { }

        void* coro_frame_ptr_ = nullptr;
    };

    template<typename Promise>
    struct coroutine_handle : coroutine_handle<void>
    {
        constexpr coroutine_handle() noexcept { }
        constexpr coroutine_handle(std::nullptr_t) noexcept { }

        coroutine_handle(const coroutine_handle&) = default;
        coroutine_handle(coroutine_handle&&) = default;
        coroutine_handle& operator=(const coroutine_handle&) = default;
        coroutine_handle& operator=(coroutine_handle&&) = default;

        coroutine_handle& operator=(std::nullptr_t np) noexcept
        {
            coro_frame_ptr_ = np;
            return *this;
        }

        Promise& promise() const
        {
            return *static_cast<Promise*>(__builtin_coro_promise (coro_frame_ptr_, __alignof(Promise), false));
        }

        static coroutine_handle from_promise(Promise& promise)
        {
            return {__builtin_coro_promise((char*) &promise, __alignof(Promise), true)};
        }

        constexpr static coroutine_handle from_address(void* address) noexcept
        {
            return {address};
        }

    protected:
        constexpr coroutine_handle(void* addr) : coroutine_handle<void>(addr) { }
    };

    struct noop_coroutine_promise { };

    template <>
    struct coroutine_handle<noop_coroutine_promise>
    {
        constexpr operator coroutine_handle<>() const noexcept
        {
            return coroutine_handle<>::from_address(address());
        }

        constexpr explicit operator bool() const noexcept
        {
            return true;
        }

        constexpr bool done() const noexcept
        {
            return false;
        }

        void operator()() const noexcept { }

        void resume() const noexcept { }

        void destroy() const noexcept { }

        noop_coroutine_promise& promise() const noexcept
        {
            return dummy_noop_coro_frame_.__p;
        }

        constexpr void* address() const noexcept
        {
            return coro_frame_ptr_;
        }

    private:
        friend coroutine_handle noop_coroutine() noexcept;

        struct __frame
        {
            static void __dummy_resume_destroy() { }

            void (*__r)() = __dummy_resume_destroy;
            void (*__d)() = __dummy_resume_destroy;
            struct noop_coroutine_promise __p;
        };

        static __frame dummy_noop_coro_frame_;

        explicit coroutine_handle() noexcept = default;

        void* coro_frame_ptr_ = &dummy_noop_coro_frame_;
    };

    using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

    inline noop_coroutine_handle noop_coroutine() noexcept
    {
        return noop_coroutine_handle();
    }


    struct suspend_always
    {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_suspend(std::coroutine_handle<>) noexcept { }
        constexpr void await_resume() noexcept { }
    };

    struct suspend_never
    {
        constexpr bool await_ready() const noexcept { return true; }
        constexpr void await_suspend(std::coroutine_handle<>) noexcept { }
        constexpr void await_resume() noexcept { }
    };


};

template<typename Promise>
struct eastl::hash<std::coroutine_handle<Promise>>
{
    size_t operator()(const std::coroutine_handle<Promise>& h) noexcept
    {
        return reinterpret_cast<size_t>(h.address());
    }
};


template<typename T>
class generator
{
private:
    using value = T;
public:
    using yielded = T;

    struct iterator;

    struct promise_type
    {
        generator get_return_object() { return generator(std::coroutine_handle<promise_type>::from_promise(*this)); }
        constexpr std::suspend_always initial_suspend() noexcept { return {}; }
        constexpr std::suspend_always final_suspend() noexcept { return {}; }

        constexpr void return_void() const noexcept {}
        std::suspend_always yield_value(const yielded& val)
        {
            value_ = &val;
            return {};
        }

        void await_transform() = delete;
        void unhanded_exception() {}

    private:
        const T* value_ = nullptr;
        friend iterator;
    };

    struct iterator : eastl::iterator<eastl::input_iterator_tag, T>
    {
        struct sentinel_t {};

        iterator(const std::coroutine_handle<promise_type>& h) : coroutine_(h) { }

        iterator& operator++()
        {
            assert(coroutine_);
            coroutine_.resume();
            if (coroutine_.done())
            {
                coroutine_ = nullptr;
            }
            return *this;
        }

        const T& operator*() const
        {
            return *coroutine_.promise().value_;
        }

        bool operator==(sentinel_t)
        {
            return !coroutine_ || coroutine_.done();
        }

    private:
        std::coroutine_handle<promise_type> coroutine_;
    };

    iterator begin()
    {
        if (coroutine_)
        {
            coroutine_.resume();
        }

        return iterator(coroutine_);
    }

    constexpr iterator::sentinel_t end()
    {
        return {};
    }

    generator(const generator&) = delete;
    generator(generator&& other) noexcept;

    explicit generator(std::coroutine_handle<promise_type> h) : coroutine_(h) { }

    ~generator() noexcept
    {
        if (coroutine_)
        {
            coroutine_.destroy();
        }
    }

private:
    std::coroutine_handle<promise_type> coroutine_ = nullptr;
};
