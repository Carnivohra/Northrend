#pragma once

namespace Northrend {

class Input;
class Window;

class BackendApplication {
public:
    virtual ~BackendApplication() = default;

    virtual void started(const Window& window) = 0;
    virtual void resumed() = 0;
    virtual void suspended() = 0;
    virtual void tick(const Input& input) = 0;
    virtual void stopped() = 0;
};

}
