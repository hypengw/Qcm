#pragma once

#include <QMetaType>

namespace helper
{

inline bool is_floating_point_metatype_id(int id) {
    switch (id) {
    case QMetaType::Float16:
    case QMetaType::Float:
    case QMetaType::Double: return true;
    default: return false;
    }
}

inline bool is_integer_metatype_id(int id) {
    switch (id) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::Bool: return true;
    default: return false;
    }
}

inline bool is_numeric_metatype_id(int id) {
    switch (id) {
    case QMetaType::Float:
    case QMetaType::Double: return true;
    default: return is_integer_metatype_id(id);
    }
}

inline bool is_numeric_metatype(QMetaType type) { return is_numeric_metatype_id(type.id()); }

} // namespace helper