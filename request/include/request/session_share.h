#pragma once
#include <filesystem>

#include "core/core.h"

namespace request
{
class SessionShare {
public:
    class Private;
    SessionShare();
    ~SessionShare();

    auto handle() const -> voidp;
    void load(const std::filesystem::path& p);
    void save(const std::filesystem::path& p) const;

private:
    rc<Private> d_ptr;
    C_DECLARE_PRIVATE_FUNC(SessionShare, d_ptr);
};
} // namespace request