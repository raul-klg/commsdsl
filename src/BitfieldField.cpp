#include "bbmp/BitfieldField.h"

#include <cassert>

#include "BitfieldFieldImpl.h"

namespace bbmp
{

namespace
{

const BitfieldFieldImpl* cast(const FieldImpl* ptr)
{
    assert(ptr != nullptr);
    return static_cast<const BitfieldFieldImpl*>(ptr);
}

} // namespace

BitfieldField::BitfieldField(const BitfieldFieldImpl* impl)
  : Base(impl)
{
}

BitfieldField::BitfieldField(Field field)
  : Base(field)
{
    assert(kind() == Kind::Bitfield);
}

Endian BitfieldField::endian() const
{
    return cast(m_pImpl)->endian();
}

std::size_t BitfieldField::length() const
{
    return cast(m_pImpl)->length();
}

std::size_t BitfieldField::bitLength() const
{
    return cast(m_pImpl)->bitLength();
}

const BitfieldField::Members& BitfieldField::members() const
{
    return cast(m_pImpl)->members();
}

} // namespace bbmp
