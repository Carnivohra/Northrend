#pragma once

#include "Backend/application.hpp"

namespace Northrend {

class Engine final : public BackendApplication {
public:
    Engine();

    void started() override;
    void resumed() override;
    void suspended() override;
    void update() override;
    void stopped() override;

    void tick();
};

}
