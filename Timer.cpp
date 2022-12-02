/**
 * @file Timer.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-07-15
 * 
 * Copyright (c) 2022 Peerjuice
 * 
 */

#include "Timer.h"
#include "utils.h"

Timer::Timer() { reset(); }

Timer::~Timer() {}

bool Timer::check(unsigned long ms) { return (millis() - _timer >= ms); }
void Timer::reset(void) { _timer = millis(); };
