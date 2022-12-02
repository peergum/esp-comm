/**
 * @file Timer.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-07-15
 * 
 * Copyright (c) 2022 Peerjuice
 * 
 */

#ifndef __TIMER_H_
#define __TIMER_H_

class Timer {
public:
  Timer();
  ~Timer();
  bool check(unsigned long ms);
  void reset(void);

 private:
  unsigned long _timer;
};

#endif  // __TIMER_H_