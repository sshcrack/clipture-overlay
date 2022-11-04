#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <windows.h>
#include <WinUser.h>

#include "sl_overlay_api.h"

#include <node_api.h>
#include <uv.h>

struct win_event_t;

napi_status napi_create_and_set_named_property(napi_env& env, napi_value& obj, const char* value_name, const int value) noexcept;

struct callback_win_method_t
{
	napi_ref js_this;

	napi_async_context async_context;
	uv_async_t uv_async_this;
	napi_env env_this;

	bool initialized;
	bool completed;
	bool success;

	int result_int;

	int error;
	napi_callback set_return;
	napi_callback fail;
	napi_value result;

	void callback_method_reset() noexcept;

	napi_status set_args_and_call_callback(napi_env env, napi_value callback, napi_value* result);
	napi_status callback_method_call_tsf(bool block);
	napi_status callback_init(napi_env env, napi_callback_info info, const char* name);
	virtual void set_callback() = 0;
	int use_callback(HWND hwndParam, RECT  rectParam);

	bool ready;
	static bool set_intercept_active(bool) noexcept;

	static bool get_intercept_active() noexcept;

	static void static_async_callback(uv_async_t* handle);
	void async_callback();

	std::mutex send_queue_mutex;
	std::queue<std::shared_ptr<win_event_t>> to_send;
	virtual size_t get_argc_to_cb() = 0;
	virtual napi_value* get_argv_to_cb() = 0;
	virtual napi_status set_callback_args_values(napi_env env)
	{
		return napi_ok;
	};

	callback_win_method_t();

	virtual ~callback_win_method_t() ;
	callback_win_method_t(const callback_win_method_t&) = delete;
	callback_win_method_t& operator=(const callback_win_method_t&) = delete;
	callback_win_method_t(callback_win_method_t&&) = delete;
	callback_win_method_t& operator=(callback_win_method_t&&) = delete;
};

struct callback_win_position_method_t : callback_win_method_t
{
	const static size_t argc_to_cb = 2;
	napi_value argv_to_cb[argc_to_cb];

	size_t get_argc_to_cb() noexcept override
	{
		return argc_to_cb;
	};
	napi_value* get_argv_to_cb() noexcept override
	{
		return argv_to_cb;
	};

	napi_status set_callback_args_values(napi_env env)  override;
	void set_callback() override;
};


extern callback_win_position_method_t* win_position_callback_info;
