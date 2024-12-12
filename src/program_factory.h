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

#ifndef MIRACLE_WM_PROGRAM_FACTORY_H
#define MIRACLE_WM_PROGRAM_FACTORY_H

#include <GLES2/gl2.h>
#include <array>
#include <mir/graphics/program.h>
#include <mir/graphics/program_factory.h>
#include <mutex>
#include <vector>

namespace miracle
{

template <void (*deleter)(GLuint)>
class GLHandle
{
public:
    explicit GLHandle(GLuint id) :
        id { id }
    {
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(id);
    }

    GLHandle(GLHandle const&) = delete;

    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from) :
        id { from.id }
    {
        from.id = 0;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using ProgramHandle = GLHandle<&glDeleteProgram>;
using ShaderHandle = GLHandle<&glDeleteShader>;

struct ProgramData
{
    GLuint id = 0;
    /* 8 is the minimum number of texture units a GL implementation can provide
     * and should comfortably provide enough textures for any conceivable buffer
     * format
     */
    std::array<GLint, 8> tex_uniforms;
    GLint position_attr = -1;
    GLint texcoord_attr = -1;
    GLint topleft_uniform = -1;
    GLint display_transform_uniform = -1;
    GLint workspace_transform_uniform = -1;
    GLint transform_uniform = -1;
    GLint screen_to_gl_coords_uniform = -1;
    GLint alpha_uniform = -1;
    GLint mode_uniform = -1;
    GLint outline_color_uniform = -1;
    mutable long long last_used_frameno = 0;

    ProgramData(GLuint program_id);
};

struct Program : public mir::graphics::gl::Program
{
public:
    Program(ProgramHandle&& opaque_shader, ProgramHandle&& alpha_shader, ProgramHandle&& outline_shader);
    ProgramHandle opaque_handle, alpha_handle, outline_handle;
    ProgramData opaque, alpha, outline;
};

class ProgramFactory : public mir::graphics::gl::ProgramFactory
{
public:
    ProgramFactory();
    mir::graphics::gl::Program& compile_fragment_shader(
        void const* id,
        char const* extension_fragment,
        char const* fragment_fragment) override;

private:
    static GLuint compile_shader(GLenum type, GLchar const* src);
    static ProgramHandle link_shader(
        ShaderHandle const& vertex_shader,
        ShaderHandle const& fragment_shader);

    ShaderHandle const vertex_shader;
    std::vector<std::pair<void const*, std::unique_ptr<Program>>> programs;
    // GL requires us to synchronise multi-threaded access to the shader APIs.
    std::mutex compilation_mutex;
};

} // miracle

#endif // MIRACLE_WM_PROGRAM_FACTORY_H
