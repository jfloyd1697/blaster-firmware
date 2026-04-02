#include "../../include/core/weapons/WeaponBehaviorLoadHelpers.h"

#include <sstream>
#include <stdexcept>

#include "core/text_resource_loader/ITextResourceLoader.h"
#include "weapon_behavior/WeaponBehaviorParser.h"
#include "weapon_behavior/WeaponBehaviorValidation.h"

namespace weapon_behavior {

    WeaponBehaviorDef loadWeaponBehavior(
        ITextResourceLoader& loader,
        const std::string& path)
    {
        const std::string text = loader.loadText(path);

        WeaponBehaviorDef def = WeaponBehaviorParser::parseFromText(text);

        const ValidationResult validation = validateWeaponBehavior(def);
        if (!validation.ok) {
            std::ostringstream ss;
            ss << "loadWeaponBehavior: validation failed for '" << path << "'";
            for (const auto& err : validation.errors) {
                ss << "\n  - " << err;
            }
            throw std::runtime_error(ss.str());
        }

        return def;
    }

    LoadWeaponBehaviorResult tryLoadWeaponBehavior(
        ITextResourceLoader& loader,
        const std::string& path)
    {
        LoadWeaponBehaviorResult result;

        try {
            const std::string text = loader.loadText(path);

            WeaponBehaviorDef def = WeaponBehaviorParser::parseFromText(text);

            const ValidationResult validation = validateWeaponBehavior(def);
            if (!validation.ok) {
                result.ok = false;
                result.errors = validation.errors;
                return result;
            }

            result.ok = true;
            result.behavior = std::move(def);
            result.errors = validation.warnings;
            return result;

        } catch (const std::exception& ex) {
            result.ok = false;
            result.errors.emplace_back(ex.what());
            return result;
        }
    }

} // namespace weapon_behavior