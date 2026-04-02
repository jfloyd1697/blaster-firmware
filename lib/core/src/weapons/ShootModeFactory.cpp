#include "core/weapons/ShootModeFactory.h"

#include "core/weapons/BurstFireMode.h"
#include "core/weapons/ChargeShotMode.h"
#include "core/weapons/RapidFireMode.h"
#include "core/weapons/SingleShotMode.h"
#include "core/weapons/WeaponProfile.h"

std::unique_ptr<IShootMode> createShootMode(const ShootContext &ctx) {
    switch (ctx.profile->shootMode.type) {
        case ShootModeType::Single:
            return std::make_unique<SingleShotMode>(ctx);

        case ShootModeType::Charge:
            return std::make_unique<ChargeShotMode>(ctx);

        case ShootModeType::Rapid:
            return std::make_unique<RapidFireMode>(ctx);

        case ShootModeType::Burst:
            return std::make_unique<BurstFireMode>(ctx);
    }

    return nullptr;
}