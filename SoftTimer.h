/**
 * @file SoftTimer.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-07-15
 * 
 * Copyright (c) 2022 Peerjuice
 * 
 */

#ifndef __SOFTTIMER_H_
#define __SOFTTIMER_H_

class SoftTimer {
public:
  SoftTimer();
  ~SoftTimer();
  bool check(unsigned long ms);
  void reset(void);

 private:
  unsigned long _timer;
};

#endif  // __SOFTTIMER_H_