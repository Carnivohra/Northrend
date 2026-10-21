#pragma once

#include "Backend/application.hpp"

namespace Northrend {

class Backend {
public:
    virtual ~Backend() = default;

    virtual void run(BackendApplication& application) = 0;
};

}
