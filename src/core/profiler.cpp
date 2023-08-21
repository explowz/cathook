/*
 * profiler.cpp
 *
 *  Created on: Nov 26, 2016
 *      Author: nullifiedcat
 */

#include <utility>

#include "common.hpp"

unsigned int g_spewcount{ 0 };

static CatCommand profiler_begin("profiler_spew", "Spew and reset", []() { g_spewcount++; });

ProfilerSection::ProfilerSection(std::string name, ProfilerSection *parent)
{
    m_name      = std::move(name);
    m_calls     = 0;
    m_log       = g_IEngine->Time();
    m_min       = 0.0f;
    m_max       = 0.0f;
    m_sum       = 0.0f;
    m_spewcount = 0;
    m_parent    = parent;
}

void ProfilerSection::OnNodeDeath(ProfilerNode &node)
{
    auto dur = g_IEngine->Time() - node.m_start;
    if (m_min == 0.0f || dur < m_min)
        m_min = dur;

    if (m_max == 0.0f || dur > m_max)
        m_max = dur;
    m_sum += dur;
    m_calls++;

    if (g_spewcount > m_spewcount)
    {
        logging::Info("[P],'%-32s',%12f,%12f,%12f,%12f,%u", m_name.c_str(), m_sum, m_sum / static_cast<float>(m_calls ? m_calls : 1), m_min, m_max, m_calls);
        m_log       = g_IEngine->Time();
        m_min       = 0.0f;
        m_max       = 0.0f;
        m_sum       = 0.0f;
        m_calls     = 0;
        m_spewcount = g_spewcount;
    }
}

ProfilerNode::ProfilerNode(ProfilerSection &section) : m_section(section)
{
    m_start = g_IEngine->Time();
}

ProfilerNode::~ProfilerNode()
{
    m_section.OnNodeDeath(*this);
}
