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

#define MIR_LOG_COMPONENT "program_factory"

#include "program_factory.h"
#include <mir/graphics/egl_error.h>
#include <mir/log.h>
#include <sstream>

namespace
{
const GLchar* const vertex_shader_src = R"(
attribute vec3 position;
attribute vec2 texcoord;

uniform mat4 screen_to_gl_coords;
uniform mat4 display_transform;
uniform mat4 workspace_transform;
uniform mat4 transform;
uniform vec2 centre;

varying vec2 v_texcoord;

void main() {
   vec4 mid = vec4(centre, 0.0, 0.0);
   vec4 transformed = (transform * (vec4(position, 1.0) - mid)) + mid;
   gl_Position = display_transform * screen_to_gl_coords * workspace_transform * transformed;
   v_texcoord = texcoord;
}
)";

const GLchar* const mode_scale_integration = R"(
uniform int mode;

vec4 resolve_color(vec4 v) {
    if (mode == 1) {
        float color =  0.299 * v.x + 0.587 * v.y + 0.114 * v.z;
        return vec4(color, color, color, v.w);
    }
    else {
        return v;
    }
}
)";

}

miracle::ProgramData::ProgramData(GLuint program_id)
{
    id = program_id;
    position_attr = glGetAttribLocation(id, "position");
    if (position_attr < 0)
        mir::log_warning("Program is missing position_attr");
    texcoord_attr = glGetAttribLocation(id, "texcoord");
    if (position_attr < 0)
        mir::log_warning("Program is missing texcoord_attr");
    for (auto i = 0u; i < tex_uniforms.size(); ++i)
    {
        /* You can reference uniform arrays as tex[0], tex[1], tex[2], … until you
         * hit the end of the array, which will return -1 as the location.
         */
        auto const uniform_name = std::string { "tex[" } + std::to_string(i) + "]";
        tex_uniforms[i] = glGetUniformLocation(id, uniform_name.c_str());
    }
    centre_uniform = glGetUniformLocation(id, "centre");
    if (centre_uniform < 0)
        mir::log_warning("Program is missing centre_uniform");

    display_transform_uniform = glGetUniformLocation(id, "display_transform");
    if (display_transform_uniform < 0)
        mir::log_warning("Program is missing display_transform_uniform");

    workspace_transform_uniform = glGetUniformLocation(id, "workspace_transform");
    if (workspace_transform_uniform < 0)
        mir::log_warning("Program is missing workspace_transform_uniform");

    transform_uniform = glGetUniformLocation(id, "transform");
    if (transform_uniform < 0)
        mir::log_warning("Program is missing transform_uniform");

    screen_to_gl_coords_uniform = glGetUniformLocation(id, "screen_to_gl_coords");
    if (screen_to_gl_coords_uniform < 0)
        mir::log_warning("Program is missing screen_to_gl_coords_uniform");

    alpha_uniform = glGetUniformLocation(id, "alpha");
    if (alpha_uniform < 0)
        mir::log_warning("Program is missing alpha_uniform");

    mode_uniform = glGetUniformLocation(id, "mode");
    if (mode_uniform < 0)
        mir::log_warning("Program is missing mode_uniform");

    outline_color_uniform = glGetUniformLocation(id, "outline_color");
    if (outline_color_uniform < 0)
        mir::log_warning("Program is missing outline_color_uniform");
}

miracle::Program::Program(
    ProgramHandle&& opaque_shader, ProgramHandle&& alpha_shader, ProgramHandle&& outline_shader) :
    opaque_handle(std::move(opaque_shader)),
    alpha_handle(std::move(alpha_shader)),
    outline_handle(std::move(outline_shader)),
    opaque { opaque_handle },
    alpha { alpha_handle },
    outline(outline_handle)
{
}

miracle::ProgramFactory::ProgramFactory() :
    vertex_shader { compile_shader(GL_VERTEX_SHADER, vertex_shader_src) }
{
}

mir::graphics::gl::Program& miracle::ProgramFactory::compile_fragment_shader(
    void const* id,
    char const* extension_fragment,
    char const* fragment_fragment)
{
    /* NOTE: This does not lock the programs vector as there is one ProgramFactory instance
     * per rendering thread.
     */

    for (auto const& pair : programs)
    {
        if (pair.first == id)
        {
            return *pair.second;
        }
    }

    std::stringstream opaque_fragment;
    opaque_fragment
        << extension_fragment
        << "\n"
        << "#ifdef GL_ES\n"
           "precision mediump float;\n"
           "#endif\n"
        << "\n"
        << fragment_fragment
        << "\n"
        << mode_scale_integration
        << "varying vec2 v_texcoord;\n"
           "void main() {\n"
           "    gl_FragColor = resolve_color(sample_to_rgba(v_texcoord));\n"
           "}\n";

    std::stringstream alpha_fragment;
    alpha_fragment
        << extension_fragment
        << "\n"
        << "#ifdef GL_ES\n"
           "precision mediump float;\n"
           "#endif\n"
        << "\n"
        << fragment_fragment
        << "\n"
        << mode_scale_integration
        << "varying vec2 v_texcoord;\n"
           "uniform float alpha;\n"
           "void main() {\n"
           "    gl_FragColor = alpha * resolve_color(sample_to_rgba(v_texcoord));\n"
           "}\n";

    std::stringstream outline_shader_src;
    outline_shader_src
        << "\n"
        << "#ifdef GL_ES\n"
           "precision mediump float;\n"
           "#endif\n"
        << "\n"
        << mode_scale_integration
        << "uniform float alpha;\n"
        << "uniform vec4 outline_color;\n"
        << "void main() {\n"
        << "    gl_FragColor = alpha * resolve_color(outline_color);\n"
        << "}\n";

    // GL shader compilation is *not* threadsafe, and requires external synchronisation
    std::lock_guard lock { compilation_mutex };

    ShaderHandle const opaque_shader {
        compile_shader(GL_FRAGMENT_SHADER, opaque_fragment.str().c_str())
    };
    ShaderHandle const alpha_shader {
        compile_shader(GL_FRAGMENT_SHADER, alpha_fragment.str().c_str())
    };
    ShaderHandle const outline_shader {
        compile_shader(GL_FRAGMENT_SHADER, outline_shader_src.str().c_str())
    };

    programs.emplace_back(id, std::make_unique<miracle::Program>(link_shader(vertex_shader, opaque_shader), link_shader(vertex_shader, alpha_shader), link_shader(vertex_shader, outline_shader)));

    return *programs.back().second;

    // We delete opaque_shader and alpha_shader here. This is fine; it only marks them
    // for deletion. GL will only delete them once the GL Program they're linked in is destroyed.
}

GLuint miracle::ProgramFactory::compile_shader(GLenum type, GLchar const* src)
{
    GLuint id = glCreateShader(type);
    if (!id)
    {
        throw std::runtime_error("Failed to create shader");
    }

    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);
    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024] = "(No log info)";
        glGetShaderInfoLog(id, sizeof log, NULL, log);
        glDeleteShader(id);
        throw std::runtime_error(std::string("Compile failed: ") + log + " for:\n" + src);
    }
    return id;
}

miracle::ProgramHandle miracle::ProgramFactory::link_shader(
    ShaderHandle const& vertex_shader,
    ShaderHandle const& fragment_shader)
{
    ProgramHandle program { glCreateProgram() };
    glAttachShader(program, fragment_shader);
    glAttachShader(program, vertex_shader);
    glLinkProgram(program);
    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024];
        glGetProgramInfoLog(program, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        throw std::runtime_error(std::string("Linking GL shader failed: ") + log);
    }

    return program;
}