/*
 * timer.hpp
 *
 *  Created on: Jul 29, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "platform.h"

class Timer
{
private:
    int last{};

public:
    constexpr Timer() noexcept = default;

    inline bool check(unsigned int ms) const noexcept
    {
        return Plat_MSTime() - last >= ms;
    }

    bool test_and_set(unsigned int ms) noexcept
    {
        if (check(ms))
        {
            update();
            return true;
        }
        return false;
    }

    inline void update() noexcept
    {
        last = static_cast<int>(Plat_MSTime());
    }

    static inline unsigned int sec_to_ms(unsigned int sec)
    {
        return sec * 1000;
    }

    inline void operator-=(unsigned int ms)
    {
        last -= static_cast<int>(ms);
    }

    inline void operator+=(unsigned int ms)
    {
        last += static_cast<int>(ms);
    }
};
