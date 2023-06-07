// Created by clara on 6/7/23.

#ifndef EVEREST_BASE_TOUTF8EXCEPTION_H
#define EVEREST_BASE_TOUTF8EXCEPTION_H

#pragma once

#include <exception>
#include <string>

namespace ResponseActions::RACommon
{
    class toUtf8Exception : public std::exception
    {
    public:
        explicit toUtf8Exception(std::string message);

        const char* what() const noexcept override;

    private:
        std::string m_message;
    };
}

#endif // EVEREST_BASE_TOUTF8EXCEPTION_H