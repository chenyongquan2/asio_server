#pragma once

#include <asio.hpp>
#include <cstddef>
#include <future>
#include "server.h"

struct RequestContext
{
    RequestContext(size_t task_id, std::string request_data)
        :task_id_(task_id)
        ,request_data_(request_data)
    {

    }
    size_t task_id_;
    std::string request_data_;
};