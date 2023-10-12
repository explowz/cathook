/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#pragma once

#include <settings/Bool.hpp>
#include "common.hpp"
#include "DetourHook.hpp"

#define MENU_COLOR (menu_color)

// This is a temporary file to put code that needs moving/refactoring in.
extern bool ignoreKeys;

// Replacement for boost::circular_buffer
template <typename T> class CCircularBuffer
{
public:
    CCircularBuffer() : maxSize(0), currentIndex(0), count(0)
    {
    }

    explicit CCircularBuffer(size_t size) : buffer(size), maxSize(size), currentIndex(0), count(0)
    {
    }

    void resize(size_t newSize)
    {
        buffer.resize(newSize);
        maxSize = newSize;

        if (currentIndex >= newSize)
        {
            currentIndex = 0;
        }

        if (count > newSize)
        {
            count = newSize;
        }
    }

    void push_back(const T &value)
    {
        if (count < maxSize)
        {
            ++count;
        }

        buffer[currentIndex] = value;
        currentIndex         = (currentIndex + 1) % maxSize;
    }

    void push_front(const T &value)
    {
        if (count < maxSize)
        {
            ++count;
        }

        currentIndex         = (currentIndex - 1 + maxSize) % maxSize;
        buffer[currentIndex] = value;
    }

    T &front()
    {
        if (count == 0)
        {
            throw std::out_of_range("Buffer is empty");
        }

        return buffer[(currentIndex - count + maxSize) % maxSize];
    }

    const T &front() const
    {
        if (count == 0)
        {
            throw std::out_of_range("Buffer is empty");
        }

        return buffer[(currentIndex - count + maxSize) % maxSize];
    }

    T &back()
    {
        if (count == 0)
        {
            throw std::out_of_range("Buffer is empty");
        }

        return buffer[(currentIndex - 1 + maxSize) % maxSize];
    }

    const T &back() const
    {
        if (count == 0)
        {
            throw std::out_of_range("Buffer is empty");
        }

        return buffer[(currentIndex - 1 + maxSize) % maxSize];
    }

    bool full() const
    {
        return count == maxSize;
    }

    size_t capacity() const
    {
        return maxSize;
    }

    void clear()
    {
        currentIndex = 0;
        count        = 0;
    }

    T &operator[](size_t index)
    {
        return buffer[(currentIndex + index) % maxSize];
    }

    const T &operator[](size_t index) const
    {
        return buffer[(currentIndex + index) % maxSize];
    }

    T &at(size_t index)
    {
        if (index >= count)
        {
            throw std::out_of_range("Index out of range");
        }

        return buffer[(currentIndex + index) % maxSize];
    }

    const T &at(size_t index) const
    {
        if (index >= count)
        {
            throw std::out_of_range("Index out of range");
        }

        return buffer[(currentIndex + index) % maxSize];
    }

    void fill(const T &value)
    {
        std::fill(buffer.begin(), buffer.end(), value);
        count = maxSize;
    }

    size_t size() const
    {
        return count;
    }

private:
    std::vector<T> buffer;
    size_t maxSize;
    size_t currentIndex;
    size_t count;
};

//-------------------------
// Shared Sentry State
//-------------------------
enum
{
    SENTRY_STATE_INACTIVE = 0,
    SENTRY_STATE_SEARCHING,
    SENTRY_STATE_ATTACKING,
    SENTRY_STATE_UPGRADING,

    SENTRY_NUM_STATES
};

extern Timer DelayTimer;
extern bool firstcm;
extern bool ignoredc;
extern bool user_sensitivity_ratio_set;

extern bool calculated_can_shoot;
#if ENABLE_VISUALS
extern int spectator_target;
extern bool freecam_is_toggled;
#endif
extern settings::Boolean clean_chat;

extern settings::Boolean clean_screenshots;
extern settings::Boolean crypt_chat;
extern settings::Boolean nolerp;
extern float backup_lerp;
extern settings::Int fakelag_amount;
extern settings::Boolean fakelag_midair;
extern settings::Boolean no_zoom;
extern settings::Boolean no_scope;
extern settings::Boolean disable_visuals;
extern settings::Int print_r;
extern settings::Int print_g;
extern settings::Int print_b;
extern Color menu_color;
extern int stored_buttons;
typedef void (*CL_SendMove_t)();
extern DetourHook cl_warp_sendmovedetour;
extern DetourHook cl_nospread_sendmovedetour;
namespace hooked_methods
{
void sendIdentifyMessage(bool reply);
extern settings::Boolean identify;
} // namespace hooked_methods
