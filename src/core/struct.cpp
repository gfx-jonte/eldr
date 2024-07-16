/**
 * Struct implementation adapted from the Mitsuba project
 */
#include <eldr/core/hash.hpp>
#include <eldr/core/math.hpp>
#include <eldr/core/struct.hpp>

#include <spdlog/fmt/ostr.h>

#include <cmath>

namespace eldr {
constexpr bool hasFlag(uint32_t flags, Struct::Flags f)
{
  return (flags & static_cast<uint32_t>(f)) != 0;
}

Struct::Struct(bool pack, Struct::ByteOrder byte_order)
  : pack_(pack), byte_order_(byte_order)
{
  if (byte_order_ == Struct::ByteOrder::HostByteOrder)
    byte_order_ = host_byte_order();
}

Struct::Struct(const Struct& s)
  : fields_(s.fields_), pack_(s.pack_), byte_order_(s.byte_order_)
{
}

size_t Struct::size() const
{
  if (fields_.empty())
    return 0;
  auto const& last = fields_[fields_.size() - 1];
  size_t      size = last.offset + last.size;
  if (!pack_) {
    size_t align = alignment();
    size += glm::abs((align - size) % align);
  }
  return size;
}

size_t Struct::alignment() const
{
  if (pack_)
    return 1;
  size_t size = 1;
  for (auto const& field : fields_)
    size = std::max(size, field.size);
  return size;
}

bool Struct::hasField(const std::string& name) const
{
  for (auto const& field : fields_)
    if (field.name == name)
      return true;
  return false;
}

Struct& Struct::append(const std::string& name, Struct::Type type,
                       uint32_t flags, double default_)
{
  Field f;
  f.name     = name;
  f.type     = type;
  f.flags    = flags;
  f.default_ = default_;
  if (fields_.empty()) {
    f.offset = 0;
  }
  else {
    auto const& last = fields_[fields_.size() - 1];
    f.offset         = last.offset + last.size;
  }
  switch (type) {
    case Type::Int8:
    case Type::UInt8:
      f.size = 1;
      break;
    case Type::Int16:
    case Type::UInt16:
    case Type::Float16:
      f.size = 2;
      break;
    case Type::Int32:
    case Type::UInt32:
    case Type::Float32:
      f.size = 4;
      break;
    case Type::Int64:
    case Type::UInt64:
    case Type::Float64:
      f.size = 8;
      break;
    default:
      Throw("Struct::append(): invalid field type!");
  }
  if (!pack_)
    f.offset += glm::abs((f.size - f.offset) % f.size);
  fields_.push_back(f);
  return *this;
}

std::ostream& operator<<(std::ostream& os, const Struct::Type& value)
{
  switch (value) {
    case Struct::Type::Int8:
      os << "int8";
      break;
    case Struct::Type::UInt8:
      os << "uint8";
      break;
    case Struct::Type::Int16:
      os << "int16";
      break;
    case Struct::Type::UInt16:
      os << "uint16";
      break;
    case Struct::Type::Int32:
      os << "int32";
      break;
    case Struct::Type::UInt32:
      os << "uint32";
      break;
    case Struct::Type::Int64:
      os << "int64";
      break;
    case Struct::Type::UInt64:
      os << "uint64";
      break;
    case Struct::Type::Float16:
      os << "float16";
      break;
    case Struct::Type::Float32:
      os << "float32";
      break;
    case Struct::Type::Float64:
      os << "float64";
      break;
    case Struct::Type::Invalid:
      os << "invalid";
      break;
    default:
      Throw("Struct: operator<<: invalid field type!");
  }
  return os;
}

std::string Struct::toString() const
{
  std::ostringstream os;
  os << "Struct<" << size() << ">[" << std::endl;
  for (size_t i = 0; i < fields_.size(); ++i) {
    auto const& f = fields_[i];
    if (i > 0) {
      size_t padding = f.offset - (fields_[i - 1].offset + fields_[i - 1].size);
      if (padding > 0)
        os << "  // " << padding << " byte" << (padding > 1 ? "s" : "")
           << " of padding." << std::endl;
    }
    os << "  " << f.type;
    os << " " << f.name << "; // @" << f.offset;
    if (hasFlag(f.flags, Flags::Normalized))
      os << ", normalized";
    if (hasFlag(f.flags, Flags::Gamma))
      os << ", gamma";
    if (hasFlag(f.flags, Flags::Weight))
      os << ", weight";
    if (hasFlag(f.flags, Flags::Alpha))
      os << ", alpha";
    if (hasFlag(f.flags, Flags::PremultipliedAlpha))
      os << ", premultiplied alpha";
    if (hasFlag(f.flags, Flags::Default))
      os << ", default=" << f.default_;
    if (hasFlag(f.flags, Flags::Assert))
      os << ", assert=" << f.default_;
    if (!f.blend.empty()) {
      os << ", blend = <";
      for (size_t j = 0; j < f.blend.size(); ++j) {
        os << f.blend[j].second << " * " << f.blend[j].first;
        if (j + 1 < f.blend.size())
          os << " + ";
      }
      os << ">";
    }
    os << "\n";
  }
  if (!fields_.empty()) {
    size_t padding = size() - (fields_[fields_.size() - 1].offset +
                               fields_[fields_.size() - 1].size);
    if (padding > 0)
      os << "  // " << padding << " byte" << (padding > 1 ? "s" : "")
         << " of padding." << std::endl;
  }
  os << "]";
  return os.str();
}

const Struct::Field& Struct::field(const std::string& name) const
{
  for (auto const& field : fields_)
    if (field.name == name)
      return field;
  Throw("Unable to find field \"%s\"", name);
}

Struct::Field& Struct::field(const std::string& name)
{
  for (auto& field : fields_)
    if (field.name == name)
      return field;
  Throw("Unable to find field \"%s\"", name);
}

std::pair<double, double> Struct::range(Struct::Type type)
{
  std::pair<double, double> result;

#define COMPUTE_RANGE(key, type)                                               \
  case key:                                                                    \
    result = std::make_pair((double) std::numeric_limits<type>::min(),         \
                            (double) std::numeric_limits<type>::max());        \
    break;

  switch (type) {
    COMPUTE_RANGE(Type::UInt8, uint8_t);
    COMPUTE_RANGE(Type::Int8, int8_t);
    COMPUTE_RANGE(Type::UInt16, uint16_t);
    COMPUTE_RANGE(Type::Int16, int16_t);
    COMPUTE_RANGE(Type::UInt32, uint32_t);
    COMPUTE_RANGE(Type::Int32, int32_t);
    COMPUTE_RANGE(Type::UInt64, uint64_t);
    COMPUTE_RANGE(Type::Int64, int64_t);
    COMPUTE_RANGE(Type::Float32, float);
    COMPUTE_RANGE(Type::Float64, double);

    case Type::Float16:
      result = std::make_pair(-65504, 65504);
      break;

    default:
      Throw("Internal error: invalid field type");
  }

  if (isInteger(type)) {
    // Account for rounding errors in the conversions above.
    // (we want the bounds to be conservative)
    if (result.first != 0)
      result.first = std::nextafter(
        result.first, result.first + std::numeric_limits<double>::epsilon());
    result.second = std::nextafter(
      result.second, result.second - std::numeric_limits<double>::epsilon());
  }

  return result;
}

size_t hash(const Struct::Field& f)
{
  size_t value = hash(f.name);
  value        = hashCombine(value, hash(f.type));
  value        = hashCombine(value, hash(f.size));
  value        = hashCombine(value, hash(f.offset));
  value        = hashCombine(value, hash(f.flags));
  value        = hashCombine(value, hash(f.default_));
  return value;
}

size_t hash(const Struct& s)
{
  return hashCombine(hashCombine(hash(s.fields_), hash(s.pack_)),
                     hash(s.byte_order_));
}
} // namespace eldr
