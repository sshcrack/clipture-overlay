/******************************************************************************
    Copyright (C) 2016-2019 by Streamlabs (General Workings Inc)
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <WinUser.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <windows.h>
#include "overlay_logging.h"
#include "win_position_callback.h"

callback_win_position_method_t* win_position_callback_info = nullptr;

struct win_event_t
{
	HWND hwndParam;
	LPRECT rectParam;

	win_event_t(HWND _hwndParam, LPRECT _rectParam) noexcept : hwndParam(_hwndParam), rectParam(_rectParam) {}
};

callback_win_method_t::callback_win_method_t()
{
	ready = false;
}

callback_win_method_t ::~callback_win_method_t()
{
	log_debug << "APP: ~callback_win_method_t" << std::endl;
	napi_delete_reference(env_this, js_this);
	napi_async_destroy(env_this, async_context);
}

void callback_win_method_t::callback_method_reset() noexcept
{
	initialized = false;
	completed = false;
	success = false;
	error = 0;
	result_int = 0;
}

bool is_intercept_active = false;
bool callback_win_method_t::set_intercept_active(bool new_state) noexcept
{
	is_intercept_active = new_state;
	return new_state;
}

bool callback_win_method_t::get_intercept_active() noexcept
{
	return is_intercept_active;
}

napi_status callback_win_position_method_t::set_callback_args_values(napi_env env)
{
	log_info << "APP: callback_keyboard_method_t::set_callback_args_values" << std::endl;
	napi_status status = napi_ok;

	std::shared_ptr<win_event_t> event;

	{
		std::lock_guard<std::mutex> lock(send_queue_mutex);
		event = to_send.front();
		to_send.pop();
	}

	if (status == napi_ok)
	{
		status = napi_create_int32(env, (int)event->hwndParam, &argv_to_cb[0]);
	}

	if (status == napi_ok)
	{
		napi_value bottom, top, right, left;
		status = napi_create_object(env, &argv_to_cb[1]);
		long r_bottom = event->rectParam->bottom;
		long r_top = event->rectParam->top;
		long r_right = event->rectParam->right;
		long r_left = event->rectParam->left;


		status = napi_create_int64(env, r_bottom, &bottom);
		if(status != napi_ok) {
			return status;
		}

		status = napi_create_int64(env, r_top, &top);
		if(status != napi_ok) {
			return status;
		}

		status = napi_create_int64(env, r_right, &right);
		if(status != napi_ok) {
			return status;
		}

		status = napi_create_int64(env, r_left, &left);
		if(status != napi_ok) {
			return status;
		}

		status = napi_set_named_property(env, argv_to_cb[1], "bottom", bottom);
		if(status != napi_ok) {
			return status;
		}

		status = napi_set_named_property(env, argv_to_cb[1], "top", top);
		if(status != napi_ok) {
			return status;
		}

		status = napi_set_named_property(env, argv_to_cb[1], "right", right);
		if(status != napi_ok) {
			return status;
		}

		status = napi_set_named_property(env, argv_to_cb[1], "left", left);
		if(status != napi_ok) {
			return status;
		}

		//status = napi_create_string_utf8(env, keyCode.c_str(), keyCode.size(), &argv_to_cb[1]);
	}

	return status;
}

int callback_win_method_t::use_callback(HWND hwndParam, RECT rectParam)
{
	log_info << "APP: use_callback called" << std::endl;

	int ret = -1;

	{
		std::lock_guard<std::mutex> lock(send_queue_mutex);
		to_send.push(std::make_shared<win_event_t>(hwndParam, rectParam));
	}

	uv_async_send(&uv_async_this);

	return ret;
}

int use_callback_win_position(HWND hwndParam, RECT rectParam)
{
	log_info << "APP: use_callback_window_position  " << std::endl;

	int ret = -1;

	callback_win_method_t* method = win_position_callback_info;
	if (method != nullptr)
	{
		ret = method->use_callback(hwndParam, rectParam);
	}

	return ret;
}

void callback_finalize(napi_env env, void* data, void* hint)
{
	log_info << "APP: callback_finalize " << std::endl;
}

napi_status callback_win_method_t::callback_init(napi_env env, napi_callback_info info, const char* name)
{
	size_t argc = 1;
	napi_value argv[1];
	napi_value async_name = nullptr;
	napi_status status;

	status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

	if (status == napi_ok)
	{
		status = napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &async_name);
	}

	if (status == napi_ok)
	{
		status = napi_async_destroy(env_this, async_context);

		status = napi_async_init(env, argv[0], async_name, &async_context);
		uv_async_init(uv_default_loop(), &uv_async_this, &static_async_callback);
		uv_async_this.data = this;
		env_this = env;

		log_info << "APP: window_pos_callback_method_init status = " << status << std::endl;

		if (status == napi_ok)
		{
			set_callback();
			ready = true;
		}
	}

	return status;
}

void callback_win_method_t::static_async_callback(uv_async_t* handle)
{
	try
	{
		static_cast<callback_win_method_t*>(handle->data)->async_callback();
	} catch (std::exception&)
	{
	} catch (...)
	{}
}

void callback_win_method_t::async_callback()
{
	napi_status status;
	napi_value js_cb;
	napi_value ret_value;
	napi_value recv;

	napi_handle_scope scope;

	status = napi_open_handle_scope(env_this, &scope);
	if (status == napi_ok)
	{
		status = napi_get_reference_value(env_this, js_this, &js_cb);
		if (status == napi_ok)
		{
			while (to_send.size() > 0)
			{
				status = set_callback_args_values(env_this);
				if (status == napi_ok)
				{
					status = napi_create_object(env_this, &recv);
					if (status == napi_ok)
					{
						if (async_context)
						{
							status = napi_make_callback(env_this, async_context, recv, js_cb, get_argc_to_cb(), get_argv_to_cb(), &ret_value);
						} else
						{
							napi_value global;
							status = napi_get_global(env_this, &global);
							if (status == napi_ok)
							{
								status = napi_call_function(env_this, global, js_cb, get_argc_to_cb(), get_argv_to_cb(), &ret_value);
							}
						}
					}
				}
			}
		}

		napi_close_handle_scope(env_this, scope);
	}

	if (status != napi_ok)
	{
		log_error << "APP: failed async_callback to send callback with status " << status << std::endl;
		while (to_send.size() > 0)
		{
			to_send.pop();
		}
	}
}

void callback_win_position_method_t::set_callback()
{
	set_callback_for_window_position(&use_callback_win_position);
}

napi_status napi_create_and_set_named_property(napi_env& env, napi_value& obj, const char* value_name, const int value) noexcept
{
	napi_status status;
	napi_value set_value;
	status = napi_create_int32(env, value, &set_value);
	if (status == napi_ok)
	{
		status = napi_set_named_property(env, obj, value_name, set_value);
	}
	return status;
}