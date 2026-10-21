#pragma once

namespace Northrend {

class BackendApplication {
public:
    virtual ~BackendApplication() = default;

    virtual void started() = 0;
    virtual void resumed() = 0;
    virtual void suspended() = 0;
    virtual void update() = 0;
    virtual void stopped() = 0;
};

}
