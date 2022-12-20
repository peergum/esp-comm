/**
 * @file SoftTimer.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-07-15
 * 
 * Copyright (c) 2022 Peerjuice
 * 
 */

#include "SoftTimer.h"
#include "utils.h"

SoftTimer::SoftTimer() { reset(); }

SoftTimer::~SoftTimer() {}

bool SoftTimer::check(unsigned long ms) { return (millis() - _timer >= ms); }
void SoftTimer::reset(void) { _timer = millis(); };
