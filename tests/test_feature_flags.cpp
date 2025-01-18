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

#include "feature_flags.h"
#include <gtest/gtest.h>

class FeatureFlagsTest : public testing::Test
{
public:
};

TEST_F(FeatureFlagsTest, DragAndDropIsFalse)
{
    EXPECT_EQ(MIRACLE_FEATURE_FLAG_DRAG_AND_DROP, true);
}