#pragma once

#include <coroutine>

namespace slg {
	struct coroutine {
		coroutine();

		struct promise_type {
			coroutine get_return_object() const noexcept;
			void return_void() const noexcept;
			std::suspend_never initial_suspend() const noexcept;
			std::suspend_never final_suspend() const noexcept;
			void unhandled_exception() const noexcept;
		};
	};
}
