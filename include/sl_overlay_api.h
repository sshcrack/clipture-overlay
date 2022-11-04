#pragma once
#include "stdafx.h"
#include <string>
#include <windows.h>
#include <WinUser.h>

struct overlay_frame;
class smg_overlays;

// char* params like url - functions get ownership of that pointer and clean memory when finish with it
int WINAPI start_overlays_thread();
int WINAPI stop_overlays_thread();
std::string get_thread_status_name();

int WINAPI show_overlays();
int WINAPI hide_overlays();
bool WINAPI is_overlays_hidden();

int WINAPI add_overlay_by_hwnd(const void* hwnd_array, size_t array_size);
std::shared_ptr<smg_overlays> WINAPI get_overlays();
int WINAPI get_overlays_count();
int WINAPI remove_overlay(int id);

int WINAPI set_overlay_position(int id, int x, int y, int width, int height);
int WINAPI paint_overlay_from_buffer(int overlay_id, const void* image_array, size_t array_size, int width, int height);
int WINAPI paint_overlay_cached_buffer(int overlay_id, std::shared_ptr<overlay_frame>, int width, int height);
int WINAPI set_overlay_transparency(int id, int transparency);
int WINAPI set_overlay_visibility(int id, bool visibility);
int WINAPI set_overlay_autohide(int id, int autohide_timeout, int autohide_transparency);

int WINAPI set_callback_for_keyboard_input(int (*ptr)(WPARAM, LPARAM));
int WINAPI set_callback_for_window_position(int (*ptr)(HWND, RECT));
int WINAPI set_callback_for_mouse_input(int (*ptr)(WPARAM, LPARAM));
int WINAPI set_callback_for_switching_input(int (*ptr)());

int WINAPI use_callback_for_keyboard_input(WPARAM wParam, LPARAM lParam);
int WINAPI use_callback_for_window_position(HWND hwndParam, RECT rectParam);
int WINAPI use_callback_for_mouse_input(WPARAM wParam, LPARAM lParam);
int WINAPI use_callback_for_switching_input();

int WINAPI switch_overlays_user_input(bool mode_active);