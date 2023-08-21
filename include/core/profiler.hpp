/*
 * profiler.h
 *
 *  Created on: Nov 26, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include <chrono>
#include <string>
#include "config.h"

class ProfilerNode;

class ProfilerSection
{
public:
    explicit ProfilerSection(std::string name, ProfilerSection *parent = nullptr);

    void OnNodeDeath(ProfilerNode &node);

    float m_min;
    float m_max;
    float m_sum;
    unsigned int m_spewcount;
    unsigned int m_calls;
    float m_log;
    std::string m_name;
    ProfilerSection *m_parent;
};

class ProfilerNode
{
public:
    explicit ProfilerNode(ProfilerSection &section);
    ~ProfilerNode();

    float m_start;
    ProfilerSection &m_section;
};

#if ENABLE_PROFILER
#define PROF_SECTION(id)                          \
    static ProfilerSection __PROFILER__##id(#id); \
    volatile ProfilerNode __PROFILER_NODE__##id(__PROFILER__##id);
#else
#define PROF_SECTION(id)
#endif
