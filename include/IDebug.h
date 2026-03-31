//
// Created by user on 3/26/2026.
//

#ifndef BLASTERFIRMWARE_IDEBUG_H
#define BLASTERFIRMWARE_IDEBUG_H

#pragma once
#include <string>

struct IDebug {
    virtual ~IDebug() = default;
    virtual void log(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
};

#endif //BLASTERFIRMWARE_IDEBUG_H