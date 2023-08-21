/*
 * entry.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "hack.hpp"

#include <thread>
#include <mutex>
#include <atomic>

std::mutex mutex_quit;
std::thread thread_main;
std::atomic_bool is_stopping{ false };

bool IsStopping()
{
    std::unique_lock lock(mutex_quit, std::try_to_lock);
    if (!lock.owns_lock()) [[unlikely]]
    {
        logging::Info("Shutting down, unlocking mutex");
        return true;
    }
    else
        return false;
}

void MainThread()
{
    hack::Initialize();
    logging::Info("Init done...");
    while (!IsStopping()) [[likely]]
        hack::Think();
    hack::Shutdown();
    logging::Shutdown();
}

void attach()
{
    thread_main = std::thread(MainThread);
}

// FIXME: Currently also closes the game
void detach()
{
    logging::Info("Detaching...");
    {
        std::scoped_lock lock(mutex_quit);
        is_stopping = true;
    }
    thread_main.join();
}

void __attribute__((constructor)) construct()
{
    attach();
}

void __attribute__((destructor)) deconstruct()
{
    detach();
}

CatCommand cat_detach("detach", "Detach cathook from TF2",
                      []()
                      {
                          hack::game_shutdown = false;
                          detach();
                      });
