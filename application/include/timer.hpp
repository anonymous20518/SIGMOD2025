#pragma once

#include <chrono>
#include <iostream>

class Time
{
public:

    Time(const std::string& description)
    {
        m_description = description;
        m_start = std::chrono::steady_clock::now();
    }

    ~Time()
    {
        std::chrono::_V2::steady_clock::time_point end = std::chrono::steady_clock::now();
        int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
        std::cout << m_description << ": " << duration << " (ms)\n";
    }

private:

    std::string m_description;
    std::chrono::_V2::steady_clock::time_point m_start;
};
