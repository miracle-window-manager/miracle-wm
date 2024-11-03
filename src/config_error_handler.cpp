/**
Copyright (C) 2024  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#define MIR_LOG_COMPONENT "config_error_handler"

#include "config_error_handler.h"
#include <mir/log.h>
#include <sstream>

using namespace miracle;

namespace
{
std::string configuration_info_to_string(ConfigurationInfo const& info)
{
    std::ostringstream out;
    out << info.filename << ':' << (info.line + 1) << ':' << info.column << ':' << ' ' << info.message;
    return out.str();
}
}

ConfigurationInfo::ConfigurationInfo(
    int line,
    int column,
    Level level,
    std::string const& filename,
    std::string message) :
    line { line },
    column { column },
    level { level },
    filename { filename },
    message { std::move(message) }
{
}

void ConfigErrorHandler::add_error(ConfigurationInfo const&& error)
{
    info.push_back(error);
}

void ConfigErrorHandler::on_complete()
{
    for (auto const& i : info)
    {
        mir::log_error(configuration_info_to_string(i));
    }

    info.clear();
}
