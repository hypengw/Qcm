#pragma once

#include <string>
#include <string_view>

namespace request
{

class URI {
public:
    URI();
    URI(std::string_view);
    ~URI();
    URI(const URI&);
    URI& operator=(const URI&);

    static URI from(std::string_view);

    std::string_view uri;
    std::string_view scheme;
    std::string_view authority;
    std::string_view userinfo;
    std::string_view host;
    std::string_view port;
    std::string_view path;
    std::string_view query;
    std::string_view fragment;

    bool valid() const;
private:
    bool m_valid;
    std::string m_holder;
};

} // namespace request