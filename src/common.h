#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <utility>

#include "bbmp/Endian.h"

namespace bbmp
{

namespace common
{

using PropsMap = std::multimap<std::string, std::string>;

const std::string& emptyString();
const std::string& nameStr();
const std::string& displayNameStr();
const std::string& idStr();
const std::string& versionStr();
const std::string& descriptionStr();
const std::string& endianStr();
const std::string& bigStr();
const std::string& littleStr();
const std::string& fieldsStr();
const std::string& messagesStr();
const std::string& messageStr();
const std::string& frameStr();
const std::string& framesStr();
const std::string& intStr();
const std::string& floatStr();
const std::string& typeStr();
const std::string& defaultValueStr();
const std::string& unitsStr();
const std::string& scalingStr();
const std::string& lengthStr();
const std::string& bitLengthStr();
const std::string& serOffsetStr();
const std::string& validRangeStr();
const std::string& validFullRangeStr();
const std::string& validValueStr();
const std::string& specialStr();
const std::string& valStr();
const std::string& metaStr();
const std::string& validMinStr();
const std::string& validMaxStr();
const std::string& nanStr();
const std::string& infStr();
const std::string& negInfStr();
const std::string& bitfieldStr();

unsigned strToUnsigned(const std::string& str, bool* ok = nullptr, int base = 0);
std::intmax_t strToIntMax(const std::string& str, bool* ok = nullptr, int base = 0);
std::uintmax_t strToUintMax(const std::string& str, bool* ok = nullptr, int base = 0);
double strToDouble(const std::string& str, bool* ok = nullptr, bool allowSpecials = true);
bool strToBool(const std::string& str);


const std::string& getStringProp(
    const PropsMap& map,
    const std::string prop,
    const std::string& defaultValue = emptyString());

Endian parseEndian(const std::string& value, Endian defaultEndian);

void toLower(std::string& str);
std::string toLowerCopy(const std::string& str);
std::pair<std::string, std::string> parseRange(const std::string& str, bool* ok = nullptr);

} // namespace common

} // namespace bbmp
