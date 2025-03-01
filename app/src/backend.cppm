module;
export module qcm.backend;

import ncrequest;

namespace qcm
{
export class Backend {
public:
    Backend();
    ~Backend();

private:
    ncrequest::WebSocketClient client;
};
} // namespace qcm