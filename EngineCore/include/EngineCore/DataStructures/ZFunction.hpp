#pragma once
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include <assert.h>
#include <utility>
#include <type_traits>

namespace StateEngine::EngineCore::DataStructures {
	template <typename R, typename... Args>
	struct FunctionVTable {
		R(*invoke)(void* ptr, Args&&... args);
		void (*destroy_dtor)(void* ptr);
		void (*clone_placement)(const void* src, void* dest_buffer);
		void* (*clone_heap)(const void* src);
	};

	template <typename Functor, typename R, typename... Args>
	struct VTableFactory {
		static R invoke(void* ptr, Args&&... args) {
			return (*static_cast<Functor*>(ptr))(std::forward<Args>(args)...);
		}

		static void destroy_dtor(void* ptr) {
			static_cast<Functor*>(ptr)->~Functor();
		}

		static void clone_placement(const void* src, void* dest_buffer) {
			new (dest_buffer) Functor(*static_cast<const Functor*>(src));
		}

		static void* clone_heap(const void* src) {
			void* ptr = MemoryAllocator_AlignedAllocate(sizeof(Functor), alignof(Functor));
			new (ptr) Functor(*static_cast<const Functor*>(src));
			return ptr;
		}

		static constexpr FunctionVTable<R, Args...> table = {
			invoke,
			destroy_dtor,
			clone_placement,
			clone_heap
		};
	};
    template <typename Signature>
    class ZFunction;

    template <typename R, typename... Args>
    class ZFunction<R(Args...)> {
    private:
        using VTableType = FunctionVTable<R, Args...>;

        static constexpr size_t kSmallObjectSize = 32;
        static constexpr size_t kAlignment = alignof(std::max_align_t);

        const VTableType* vtable_{ nullptr };
        bool isSmallObject_{ false };

        union Storage {
            void* ptr;
            std::aligned_storage_t<kSmallObjectSize, kAlignment> buffer;
        } data_;

        void* get_obj_ptr() {
            return isSmallObject_ ? (void*)&data_.buffer : data_.ptr;
        }
        const void* get_obj_ptr() const {
            return isSmallObject_ ? (const void*)&data_.buffer : data_.ptr;
        }
    private:
        void move_from(ZFunction&& other) {
            if (!other.vtable_) {
                vtable_ = nullptr;
                return;
            }

            vtable_ = other.vtable_;
            isSmallObject_ = other.isSmallObject_;

            if (isSmallObject_) {
                vtable_->clone_placement(&other.data_.buffer, &data_.buffer);

                other.destroy_current();
            }
            else {
                data_.ptr = other.data_.ptr;
                other.data_.ptr = nullptr;
                other.vtable_ = nullptr;
            }
        }
        void copy_from(const ZFunction& other) {
            if (!other.vtable_) {
                vtable_ = nullptr;
                return;
            }

            vtable_ = other.vtable_;
            isSmallObject_ = other.isSmallObject_;

            if (isSmallObject_) {
                vtable_->clone_placement(&other.data_.buffer, &data_.buffer);
            }
            else {
                data_.ptr = vtable_->clone_heap(other.data_.ptr);
            }
        }
        void destroy_current() {
            if (vtable_) {
                void* obj_ptr = get_obj_ptr();
                vtable_->destroy_dtor(obj_ptr);

                if (!isSmallObject_) {
                    MemoryAllocator_Deallocate(data_.ptr);
                }
            }
            vtable_ = nullptr;
        }

    public:
        ZFunction() = default;

        template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, ZFunction>>>
        ZFunction(F&& func) {
            using FunctorType = std::decay_t<F>;

            vtable_ = &VTableFactory<FunctorType, R, Args...>::table;

            constexpr bool fits_in_buffer =
                sizeof(FunctorType) <= kSmallObjectSize &&
                alignof(FunctorType) <= kAlignment;

            if constexpr (fits_in_buffer) {
                new (&data_.buffer) FunctorType(std::forward<F>(func));
                isSmallObject_ = true;
            }
            else {
                data_.ptr = MemoryAllocator_AlignedAllocate(sizeof(FunctorType), alignof(FunctorType));
                new (data_.ptr) FunctorType(std::forward<F>(func));
                isSmallObject_ = false;
            }
        }

        ~ZFunction() {
            destroy_current();
        }
        
        ZFunction(const ZFunction& other) {
            copy_from(other);
        }
        ZFunction& operator=(const ZFunction& other) {
            if (this != &other) {
                destroy_current();
                copy_from(other);
            }
            return *this;
        }

        ZFunction(ZFunction&& other) noexcept {
            move_from(std::move(other));
        }
        ZFunction& operator=(ZFunction&& other) noexcept {
            if (this != &other) {
                destroy_current();
                move_from(std::move(other));
            }
            return *this;
        }

        R operator()(Args... args) const {
            assert(vtable_ && "Call to empty ZFunction");
            return vtable_->invoke(const_cast<void*>(get_obj_ptr()), std::forward<Args>(args)...);
        }

        explicit operator bool() const noexcept {
            return vtable_ != nullptr;
        }

        bool operator==(const ZFunction& other) const noexcept {
            return vtable_ == other.vtable_;
		}
        bool operator!=(const ZFunction& other) const noexcept {
            return vtable_ != other.vtable_;
        }
    };
}