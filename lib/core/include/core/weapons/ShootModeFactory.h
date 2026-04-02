//
// Created by user on 4/1/2026.
//

#ifndef BLASTER_FIRMWARE_SHOOTMODEFACTORY_H
#define BLASTER_FIRMWARE_SHOOTMODEFACTORY_H

#pragma once

#include <memory>

#include "core/weapons/ShootContext.h"

class IShootMode;

std::unique_ptr<IShootMode> createShootMode(const ShootContext &ctx);

#endif //BLASTER_FIRMWARE_SHOOTMODEFACTORY_H
