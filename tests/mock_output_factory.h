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

#ifndef MIRACLE_WM_MOCK_OUTPUT_FACTORY_H
#define MIRACLE_WM_MOCK_OUTPUT_FACTORY_H

#include "mock_output.h"
#include "output.h"
#include "output_factory.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{
    class MockOutputFactory : public OutputFactory
    {
    public:
        MockOutputFactory()
        {
            output = new ::testing::NiceMock<test::MockOutput>();
        }

        std::unique_ptr<Output> create(
            std::string name,
            int id,
            mir::geometry::Rectangle area,
            OutputManager* output_manager)
        {
            return std::unique_ptr<Output>(output);
        }

        test::MockOutput* output;
    };
}
}

#endif // MIRACLE_WM_MOCK_OUTPUT_FACTORY_H
