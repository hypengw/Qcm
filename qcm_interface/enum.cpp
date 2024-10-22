#include "qcm_interface/enum.h"
#include "core/strv_helper.h"
#include "core/log.h"

namespace qcm
{
}

IMPL_CONVERT(std::string_view, qcm::enums::CollectionType) {
    switch (in) {
    case in_type::CTAlbum: {
        out = "album"sv;
        break;
    }
    case in_type::CTArtist: {
        out = "artist"sv;
        break;
    }
    default: {
        _assert_rel_(false);
    }
    }
}