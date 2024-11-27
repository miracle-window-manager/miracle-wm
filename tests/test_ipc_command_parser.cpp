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

#include "ipc_command.h"
#include <gtest/gtest.h>

using namespace miracle;

class IpcCommandParserTest : public testing::Test
{
};

TEST_F(IpcCommandParserTest, TestClassParsing)
{
    const char* v = "[class=\"XYZ\"]";
    IpcCommandParser parser(v);
    auto scope = parser.parse();
    ASSERT_EQ(scope.scope[0].type, IpcScopeType::class_);
    ASSERT_EQ(scope.scope[0].value, "XYZ");
}

TEST_F(IpcCommandParserTest, TestAllParsing)
{
    const char* v = "[all]";
    IpcCommandParser parser(v);
    auto scope = parser.parse();
    ASSERT_EQ(scope.scope[0].type, IpcScopeType::all);
}

TEST_F(IpcCommandParserTest, TestMultipleParsing)
{
    const char* v = "[class=\"Firefox\" window_role=\"About\"]";
    IpcCommandParser parser(v);
    auto scope = parser.parse();
    ASSERT_EQ(scope.scope[0].type, IpcScopeType::class_);
    ASSERT_EQ(scope.scope[0].value, "Firefox");
    ASSERT_EQ(scope.scope[1].type, IpcScopeType::window_role);
    ASSERT_EQ(scope.scope[1].value, "About");
}

TEST_F(IpcCommandParserTest, TestComplexClassParsing)
{
    const char* v = "[class=\"^(?i)(?!firefox)(?!gnome-terminal).*\"]";
    IpcCommandParser parser(v);
    auto scope = parser.parse();
    ASSERT_EQ(scope.scope[0].type, IpcScopeType::class_);
    ASSERT_EQ(scope.scope[0].value, "^(?i)(?!firefox)(?!gnome-terminal).*");
}

TEST_F(IpcCommandParserTest, TestTilingParsing)
{
    const char* v = "[tiling]";
    IpcCommandParser parser(v);
    auto scope = parser.parse();
    ASSERT_EQ(scope.scope[0].type, IpcScopeType::tiling);
}

TEST_F(IpcCommandParserTest, TestFloatingParsing)
{
    const char* v = "[floating ]";
    IpcCommandParser parser(v);
    auto scope = parser.parse();
    ASSERT_EQ(scope.scope[0].type, IpcScopeType::floating);
}

TEST_F(IpcCommandParserTest, CanParseSingleI3Command)
{
    const char* v = "exec gedit";
    IpcCommandParser parser(v);
    auto commands = parser.parse();
    ASSERT_EQ(commands.commands.size(), 1);
    ASSERT_EQ(commands.commands[0].type, IpcCommandType::exec);
    ASSERT_EQ(commands.commands[0].arguments[0], "gedit");
}

TEST_F(IpcCommandParserTest, CanParseExecCommandWithNoStartupId)
{
    const char* v = "exec --no-startup-id gedit";
    IpcCommandParser parser(v);
    auto commands = parser.parse();
    ASSERT_EQ(commands.commands.size(), 1);
    ASSERT_EQ(commands.commands[0].type, IpcCommandType::exec);
    ASSERT_EQ(commands.commands[0].options[0], "--no-startup-id");
    ASSERT_EQ(commands.commands[0].arguments[0], "gedit");
}

TEST_F(IpcCommandParserTest, CanParseSplitCommand)
{
    const char* v = "split vertical";
    IpcCommandParser parser(v);
    auto commands = parser.parse();
    ASSERT_EQ(commands.commands.size(), 1);
    ASSERT_EQ(commands.commands[0].type, IpcCommandType::split);
    ASSERT_EQ(commands.commands[0].arguments[0], "vertical");
}

TEST_F(IpcCommandParserTest, CanParseStringLiteralArguments)
{
    const char* v = "workspace  \"1:first\"";
    IpcCommandParser parser(v);
    auto commands = parser.parse();
    ASSERT_EQ(commands.commands.size(), 1);
    ASSERT_EQ(commands.commands[0].type, IpcCommandType::workspace);
    ASSERT_EQ(commands.commands[0].arguments[0], "1:first");
}

TEST_F(IpcCommandParserTest, CanParseTwoCommands)
{
    const char* v = "workspace  \"1:first\"; layout --opt1 splith";
    IpcCommandParser parser(v);
    auto commands = parser.parse();
    ASSERT_EQ(commands.commands.size(), 2);
    ASSERT_EQ(commands.commands[0].type, IpcCommandType::workspace);
    ASSERT_EQ(commands.commands[0].arguments[0], "1:first");
    ASSERT_EQ(commands.commands[1].type, IpcCommandType::layout);
    ASSERT_EQ(commands.commands[1].options[0], "--opt1");
    ASSERT_EQ(commands.commands[1].arguments[0], "splith");
}

TEST_F(IpcCommandParserTest, CanParseThreeCommands)
{
    const char* v = "workspace  \"1:first\"; layout --opt1 splith; layout --opt2 splitv";
    IpcCommandParser parser(v);
    auto commands = parser.parse();
    ASSERT_EQ(commands.commands.size(), 3);
    ASSERT_EQ(commands.commands[0].type, IpcCommandType::workspace);
    ASSERT_EQ(commands.commands[0].arguments[0], "1:first");
    ASSERT_EQ(commands.commands[1].type, IpcCommandType::layout);
    ASSERT_EQ(commands.commands[1].options[0], "--opt1");
    ASSERT_EQ(commands.commands[1].arguments[0], "splith");
    ASSERT_EQ(commands.commands[2].type, IpcCommandType::layout);
    ASSERT_EQ(commands.commands[2].options[0], "--opt2");
    ASSERT_EQ(commands.commands[2].arguments[0], "splitv");
}