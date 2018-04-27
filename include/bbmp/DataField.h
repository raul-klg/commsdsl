#pragma once

#include <vector>

#include "Field.h"

namespace bbmp
{

class DataFieldImpl;
class BBMP_API DataField : public Field
{
    using Base = Field;
public:

    using ValueType = std::vector<std::uint8_t>;

    explicit DataField(const DataFieldImpl* impl);
    explicit DataField(Field field);

    const ValueType& defaultValue() const;
    unsigned fixedLength() const;
    bool hasLengthPrefixField() const;
    Field lengthPrefixField() const;
};

} // namespace bbmp
