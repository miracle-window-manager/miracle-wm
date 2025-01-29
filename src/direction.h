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

#ifndef MIRACLEWM_DIRECTION_H
#define MIRACLEWM_DIRECTION_H

namespace miracle
{
enum class Direction
{
    up,
    left,
    down,
    right,
    MAX
};

inline bool is_negative_direction(Direction direction)
{
    return direction == Direction::left || direction == Direction::up;
}

inline bool is_vertical_direction(Direction direction)
{
    return direction == Direction::up || direction == Direction::down;
}
}

#endif // MIRACLEWM_DIRECTION_H
