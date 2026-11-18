export module qcm.activation;

import qextra.qt;

export namespace qcm::activation
{
auto take_token() -> QString;

class WindowActivation {
public:
    explicit WindowActivation(QStringView token);
    ~WindowActivation();
    WindowActivation(const WindowActivation&)            = delete;
    WindowActivation& operator=(const WindowActivation&) = delete;

    void activate(QWindow* window) const;

private:
    bool m_wayland_token {};
    bool m_x11_token {};
};
} // namespace qcm::activation
