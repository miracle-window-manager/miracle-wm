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

#ifndef MIRACLE_CONFIG_ERROR_HANDLER_H
#define MIRACLE_CONFIG_ERROR_HANDLER_H

#include <cstdint>
#include <string>
#include <vector>

namespace miracle
{
class ConfigurationInfo
{
public:
    enum class Level
    {
        warning,
        error
    };

    ConfigurationInfo(
        int line,
        int column,
        Level level,
        std::string const& filename,
        std::string message);

    int const line;
    int const column;
    Level const level;
    std::string const filename;
    std::string const message;
};

class ConfigErrorHandler
{
public:
    void add_error(ConfigurationInfo const&& info);
    void on_complete();

private:
    std::vector<ConfigurationInfo> info;
};
}

#endif // MIRACLE_CONFIG_ERROR_HANDLER_H
