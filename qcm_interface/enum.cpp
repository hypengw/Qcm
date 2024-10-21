#include "qcm_interface/enum.h"
#include "core/strv_helper.h"

namespace qcm
{
}

IMPL_CONVERT(std::string_view, qcm::enums::CollectionType) {
    switch (in) {
    case in_type::CTAlbum: {
        out = "album"sv;
    }
    }
}