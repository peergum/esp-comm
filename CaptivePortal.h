/**
 * @file CaptivePortal.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-07-15
 * 
 * Copyright (c) 2022 Peerjuice
 * 
 */

#ifndef __CAPTIVEPORTAL_H_
#define __CAPTIVEPORTAL_H_

#include "WebServer.h"
#include "Config.h"

class CaptivePortal: public WebServer {
public:
  CaptivePortal(Config &config, bool useFAT = false);
  ~CaptivePortal();
private:
};

#endif  // __CAPTIVEPORTAL_H_