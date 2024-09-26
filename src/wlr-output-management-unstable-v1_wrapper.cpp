/*
 * AUTOGENERATED - DO NOT EDIT
 *
 * This file is generated from wlr-output-management-unstable-v1.xml by mir_wayland_generator
 */

#include "wlr-output-management-unstable-v1_wrapper.h"

#include <boost/exception/diagnostic_information.hpp>
#include <wayland-server-core.h>

#include "mir/log.h"
#include "mir/wayland/protocol_error.h"
#include "mir/wayland/client.h"

namespace mir
{
namespace wayland
{
extern struct wl_interface const zwlr_output_configuration_head_v1_interface_data;
extern struct wl_interface const zwlr_output_configuration_v1_interface_data;
extern struct wl_interface const zwlr_output_head_v1_interface_data;
extern struct wl_interface const zwlr_output_manager_v1_interface_data;
extern struct wl_interface const zwlr_output_mode_v1_interface_data;
}
}

namespace mw = mir::wayland;

namespace
{
struct wl_interface const* all_null_types [] {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr};
}

// ZwlrOutputManagerV1

struct mw::ZwlrOutputManagerV1::Thunks
{
    static int const supported_version;

    static void create_configuration_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id, uint32_t serial)
    {
        wl_resource* id_resolved{
            wl_resource_create(client, &zwlr_output_configuration_v1_interface_data, wl_resource_get_version(resource), id)};
        if (id_resolved == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        try
        {
            auto me = static_cast<ZwlrOutputManagerV1*>(wl_resource_get_user_data(resource));
            me->create_configuration(id_resolved, serial);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputManagerV1::create_configuration()");
        }
    }

    static void stop_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        try
        {
            auto me = static_cast<ZwlrOutputManagerV1*>(wl_resource_get_user_data(resource));
            me->stop();
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputManagerV1::stop()");
        }
    }

    static void resource_destroyed_thunk(wl_resource* resource)
    {
        delete static_cast<ZwlrOutputManagerV1*>(wl_resource_get_user_data(resource));
    }

    static void bind_thunk(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = static_cast<ZwlrOutputManagerV1::Global*>(data);
        auto resource = wl_resource_create(
            client,
            &zwlr_output_manager_v1_interface_data,
            std::min((int)version, Thunks::supported_version),
            id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        try
        {
            me->bind(resource);
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputManagerV1 global bind");
        }
    }

    static struct wl_interface const* create_configuration_types[];
    static struct wl_interface const* head_types[];
    static struct wl_message const request_messages[];
    static struct wl_message const event_messages[];
    static void const* request_vtable[];
};

int const mw::ZwlrOutputManagerV1::Thunks::supported_version = 2;

mw::ZwlrOutputManagerV1::ZwlrOutputManagerV1(struct wl_resource* resource, Version<2>)
    : Resource{resource}
{
    wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);
}

mw::ZwlrOutputManagerV1::~ZwlrOutputManagerV1()
{
    wl_resource_set_implementation(resource, nullptr, nullptr, nullptr);
}

void mw::ZwlrOutputManagerV1::send_head_event(struct wl_resource* head) const
{
    wl_resource_post_event(resource, Opcode::head, head);
}

void mw::ZwlrOutputManagerV1::send_done_event(uint32_t serial) const
{
    wl_resource_post_event(resource, Opcode::done, serial);
}

void mw::ZwlrOutputManagerV1::send_finished_event() const
{
    wl_resource_post_event(resource, Opcode::finished);
}

bool mw::ZwlrOutputManagerV1::is_instance(wl_resource* resource)
{
    return wl_resource_instance_of(resource, &zwlr_output_manager_v1_interface_data, Thunks::request_vtable);
}

void mw::ZwlrOutputManagerV1::destroy_and_delete() const
{
    // Will result in this object being deleted
    wl_resource_destroy(resource);
}

mw::ZwlrOutputManagerV1::Global::Global(wl_display* display, Version<2>)
    : wayland::Global{
          wl_global_create(
              display,
              &zwlr_output_manager_v1_interface_data,
              Thunks::supported_version,
              this,
              &Thunks::bind_thunk)}
{
}

auto mw::ZwlrOutputManagerV1::Global::interface_name() const -> char const*
{
    return ZwlrOutputManagerV1::interface_name;
}

struct wl_interface const* mw::ZwlrOutputManagerV1::Thunks::create_configuration_types[] {
    &zwlr_output_configuration_v1_interface_data,
    nullptr};

struct wl_interface const* mw::ZwlrOutputManagerV1::Thunks::head_types[] {
    &zwlr_output_head_v1_interface_data};

struct wl_message const mw::ZwlrOutputManagerV1::Thunks::request_messages[] {
    {"create_configuration", "nu", create_configuration_types},
    {"stop", "", all_null_types}};

struct wl_message const mw::ZwlrOutputManagerV1::Thunks::event_messages[] {
    {"head", "n", head_types},
    {"done", "u", all_null_types},
    {"finished", "", all_null_types}};

void const* mw::ZwlrOutputManagerV1::Thunks::request_vtable[] {
    (void*)Thunks::create_configuration_thunk,
    (void*)Thunks::stop_thunk};

mw::ZwlrOutputManagerV1* mw::ZwlrOutputManagerV1::from(struct wl_resource* resource)
{
    if (resource &&
        wl_resource_instance_of(resource, &zwlr_output_manager_v1_interface_data, ZwlrOutputManagerV1::Thunks::request_vtable))
    {
        return static_cast<ZwlrOutputManagerV1*>(wl_resource_get_user_data(resource));
    }
    else
    {
        return nullptr;
    }
}

// ZwlrOutputHeadV1

struct mw::ZwlrOutputHeadV1::Thunks
{
    static int const supported_version;

    static void resource_destroyed_thunk(wl_resource* resource)
    {
        delete static_cast<ZwlrOutputHeadV1*>(wl_resource_get_user_data(resource));
    }

    static struct wl_interface const* mode_types[];
    static struct wl_interface const* current_mode_types[];
    static struct wl_message const event_messages[];
    static void const* request_vtable[];
};

int const mw::ZwlrOutputHeadV1::Thunks::supported_version = 2;

mw::ZwlrOutputHeadV1::ZwlrOutputHeadV1(ZwlrOutputManagerV1 const& parent)
    : Resource{wl_resource_create(
          wl_resource_get_client(parent.resource),
          &zwlr_output_head_v1_interface_data,
          wl_resource_get_version(parent.resource), 0)}
{
    wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);
}

mw::ZwlrOutputHeadV1::~ZwlrOutputHeadV1()
{
    wl_resource_set_implementation(resource, nullptr, nullptr, nullptr);
}

void mw::ZwlrOutputHeadV1::send_name_event(std::string const& name) const
{
    const char* name_resolved = name.c_str();
    wl_resource_post_event(resource, Opcode::name, name_resolved);
}

void mw::ZwlrOutputHeadV1::send_description_event(std::string const& description) const
{
    const char* description_resolved = description.c_str();
    wl_resource_post_event(resource, Opcode::description, description_resolved);
}

void mw::ZwlrOutputHeadV1::send_physical_size_event(int32_t width, int32_t height) const
{
    wl_resource_post_event(resource, Opcode::physical_size, width, height);
}

void mw::ZwlrOutputHeadV1::send_mode_event(struct wl_resource* mode) const
{
    wl_resource_post_event(resource, Opcode::mode, mode);
}

void mw::ZwlrOutputHeadV1::send_enabled_event(int32_t enabled) const
{
    wl_resource_post_event(resource, Opcode::enabled, enabled);
}

void mw::ZwlrOutputHeadV1::send_current_mode_event(struct wl_resource* mode) const
{
    wl_resource_post_event(resource, Opcode::current_mode, mode);
}

void mw::ZwlrOutputHeadV1::send_position_event(int32_t x, int32_t y) const
{
    wl_resource_post_event(resource, Opcode::position, x, y);
}

void mw::ZwlrOutputHeadV1::send_transform_event(int32_t transform) const
{
    wl_resource_post_event(resource, Opcode::transform, transform);
}

void mw::ZwlrOutputHeadV1::send_scale_event(double scale) const
{
    wl_fixed_t scale_resolved{wl_fixed_from_double(scale)};
    wl_resource_post_event(resource, Opcode::scale, scale_resolved);
}

void mw::ZwlrOutputHeadV1::send_finished_event() const
{
    wl_resource_post_event(resource, Opcode::finished);
}

bool mw::ZwlrOutputHeadV1::version_supports_make()
{
    return wl_resource_get_version(resource) >= 2;
}

void mw::ZwlrOutputHeadV1::send_make_event_if_supported(std::string const& make) const
{
    if (wl_resource_get_version(resource) >= 2)
    {
        const char* make_resolved = make.c_str();
        wl_resource_post_event(resource, Opcode::make, make_resolved);
    }
}

void mw::ZwlrOutputHeadV1::send_make_event(std::string const& make) const
{
    if (wl_resource_get_version(resource) >= 2)
    {
        const char* make_resolved = make.c_str();
        wl_resource_post_event(resource, Opcode::make, make_resolved);
    }
    else
    {
        tried_to_send_unsupported_event(client->raw_client(), resource, "make", 2);
    }
}

bool mw::ZwlrOutputHeadV1::version_supports_model()
{
    return wl_resource_get_version(resource) >= 2;
}

void mw::ZwlrOutputHeadV1::send_model_event_if_supported(std::string const& model) const
{
    if (wl_resource_get_version(resource) >= 2)
    {
        const char* model_resolved = model.c_str();
        wl_resource_post_event(resource, Opcode::model, model_resolved);
    }
}

void mw::ZwlrOutputHeadV1::send_model_event(std::string const& model) const
{
    if (wl_resource_get_version(resource) >= 2)
    {
        const char* model_resolved = model.c_str();
        wl_resource_post_event(resource, Opcode::model, model_resolved);
    }
    else
    {
        tried_to_send_unsupported_event(client->raw_client(), resource, "model", 2);
    }
}

bool mw::ZwlrOutputHeadV1::version_supports_serial_number()
{
    return wl_resource_get_version(resource) >= 2;
}

void mw::ZwlrOutputHeadV1::send_serial_number_event_if_supported(std::string const& serial_number) const
{
    if (wl_resource_get_version(resource) >= 2)
    {
        const char* serial_number_resolved = serial_number.c_str();
        wl_resource_post_event(resource, Opcode::serial_number, serial_number_resolved);
    }
}

void mw::ZwlrOutputHeadV1::send_serial_number_event(std::string const& serial_number) const
{
    if (wl_resource_get_version(resource) >= 2)
    {
        const char* serial_number_resolved = serial_number.c_str();
        wl_resource_post_event(resource, Opcode::serial_number, serial_number_resolved);
    }
    else
    {
        tried_to_send_unsupported_event(client->raw_client(), resource, "serial_number", 2);
    }
}

bool mw::ZwlrOutputHeadV1::is_instance(wl_resource* resource)
{
    return wl_resource_instance_of(resource, &zwlr_output_head_v1_interface_data, Thunks::request_vtable);
}

void mw::ZwlrOutputHeadV1::destroy_and_delete() const
{
    // Will result in this object being deleted
    wl_resource_destroy(resource);
}

struct wl_interface const* mw::ZwlrOutputHeadV1::Thunks::mode_types[] {
    &zwlr_output_mode_v1_interface_data};

struct wl_interface const* mw::ZwlrOutputHeadV1::Thunks::current_mode_types[] {
    &zwlr_output_mode_v1_interface_data};

struct wl_message const mw::ZwlrOutputHeadV1::Thunks::event_messages[] {
    {"name", "s", all_null_types},
    {"description", "s", all_null_types},
    {"physical_size", "ii", all_null_types},
    {"mode", "n", mode_types},
    {"enabled", "i", all_null_types},
    {"current_mode", "o", current_mode_types},
    {"position", "ii", all_null_types},
    {"transform", "i", all_null_types},
    {"scale", "f", all_null_types},
    {"finished", "", all_null_types},
    {"make", "2s", all_null_types},
    {"model", "2s", all_null_types},
    {"serial_number", "2s", all_null_types}};

void const* mw::ZwlrOutputHeadV1::Thunks::request_vtable[] {
    nullptr};

mw::ZwlrOutputHeadV1* mw::ZwlrOutputHeadV1::from(struct wl_resource* resource)
{
    if (resource &&
        wl_resource_instance_of(resource, &zwlr_output_head_v1_interface_data, ZwlrOutputHeadV1::Thunks::request_vtable))
    {
        return static_cast<ZwlrOutputHeadV1*>(wl_resource_get_user_data(resource));
    }
    else
    {
        return nullptr;
    }
}

// ZwlrOutputModeV1

struct mw::ZwlrOutputModeV1::Thunks
{
    static int const supported_version;

    static void resource_destroyed_thunk(wl_resource* resource)
    {
        delete static_cast<ZwlrOutputModeV1*>(wl_resource_get_user_data(resource));
    }

    static struct wl_message const event_messages[];
    static void const* request_vtable[];
};

int const mw::ZwlrOutputModeV1::Thunks::supported_version = 2;

mw::ZwlrOutputModeV1::ZwlrOutputModeV1(ZwlrOutputHeadV1 const& parent)
    : Resource{wl_resource_create(
          wl_resource_get_client(parent.resource),
          &zwlr_output_mode_v1_interface_data,
          wl_resource_get_version(parent.resource), 0)}
{
    wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);
}

mw::ZwlrOutputModeV1::~ZwlrOutputModeV1()
{
    wl_resource_set_implementation(resource, nullptr, nullptr, nullptr);
}

void mw::ZwlrOutputModeV1::send_size_event(int32_t width, int32_t height) const
{
    wl_resource_post_event(resource, Opcode::size, width, height);
}

void mw::ZwlrOutputModeV1::send_refresh_event(int32_t refresh) const
{
    wl_resource_post_event(resource, Opcode::refresh, refresh);
}

void mw::ZwlrOutputModeV1::send_preferred_event() const
{
    wl_resource_post_event(resource, Opcode::preferred);
}

void mw::ZwlrOutputModeV1::send_finished_event() const
{
    wl_resource_post_event(resource, Opcode::finished);
}

bool mw::ZwlrOutputModeV1::is_instance(wl_resource* resource)
{
    return wl_resource_instance_of(resource, &zwlr_output_mode_v1_interface_data, Thunks::request_vtable);
}

void mw::ZwlrOutputModeV1::destroy_and_delete() const
{
    // Will result in this object being deleted
    wl_resource_destroy(resource);
}

struct wl_message const mw::ZwlrOutputModeV1::Thunks::event_messages[] {
    {"size", "ii", all_null_types},
    {"refresh", "i", all_null_types},
    {"preferred", "", all_null_types},
    {"finished", "", all_null_types}};

void const* mw::ZwlrOutputModeV1::Thunks::request_vtable[] {
    nullptr};

mw::ZwlrOutputModeV1* mw::ZwlrOutputModeV1::from(struct wl_resource* resource)
{
    if (resource &&
        wl_resource_instance_of(resource, &zwlr_output_mode_v1_interface_data, ZwlrOutputModeV1::Thunks::request_vtable))
    {
        return static_cast<ZwlrOutputModeV1*>(wl_resource_get_user_data(resource));
    }
    else
    {
        return nullptr;
    }
}

// ZwlrOutputConfigurationV1

struct mw::ZwlrOutputConfigurationV1::Thunks
{
    static int const supported_version;

    static void enable_head_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* head)
    {
        wl_resource* id_resolved{
            wl_resource_create(client, &zwlr_output_configuration_head_v1_interface_data, wl_resource_get_version(resource), id)};
        if (id_resolved == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
            me->enable_head(id_resolved, head);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationV1::enable_head()");
        }
    }

    static void disable_head_thunk(struct wl_client* client, struct wl_resource* resource, struct wl_resource* head)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
            me->disable_head(head);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationV1::disable_head()");
        }
    }

    static void apply_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
            me->apply();
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationV1::apply()");
        }
    }

    static void test_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
            me->test();
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationV1::test()");
        }
    }

    static void destroy_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
            me->destroy();
            wl_resource_destroy(resource);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationV1::destroy()");
        }
    }

    static void resource_destroyed_thunk(wl_resource* resource)
    {
        delete static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
    }

    static struct wl_interface const* enable_head_types[];
    static struct wl_interface const* disable_head_types[];
    static struct wl_message const request_messages[];
    static struct wl_message const event_messages[];
    static void const* request_vtable[];
};

int const mw::ZwlrOutputConfigurationV1::Thunks::supported_version = 2;

mw::ZwlrOutputConfigurationV1::ZwlrOutputConfigurationV1(struct wl_resource* resource, Version<2>)
    : Resource{resource}
{
    wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);
}

mw::ZwlrOutputConfigurationV1::~ZwlrOutputConfigurationV1()
{
    wl_resource_set_implementation(resource, nullptr, nullptr, nullptr);
}

void mw::ZwlrOutputConfigurationV1::send_succeeded_event() const
{
    wl_resource_post_event(resource, Opcode::succeeded);
}

void mw::ZwlrOutputConfigurationV1::send_failed_event() const
{
    wl_resource_post_event(resource, Opcode::failed);
}

void mw::ZwlrOutputConfigurationV1::send_cancelled_event() const
{
    wl_resource_post_event(resource, Opcode::cancelled);
}

bool mw::ZwlrOutputConfigurationV1::is_instance(wl_resource* resource)
{
    return wl_resource_instance_of(resource, &zwlr_output_configuration_v1_interface_data, Thunks::request_vtable);
}

uint32_t const mw::ZwlrOutputConfigurationV1::Error::already_configured_head;
uint32_t const mw::ZwlrOutputConfigurationV1::Error::unconfigured_head;
uint32_t const mw::ZwlrOutputConfigurationV1::Error::already_used;

struct wl_interface const* mw::ZwlrOutputConfigurationV1::Thunks::enable_head_types[] {
    &zwlr_output_configuration_head_v1_interface_data,
    &zwlr_output_head_v1_interface_data};

struct wl_interface const* mw::ZwlrOutputConfigurationV1::Thunks::disable_head_types[] {
    &zwlr_output_head_v1_interface_data};

struct wl_message const mw::ZwlrOutputConfigurationV1::Thunks::request_messages[] {
    {"enable_head", "no", enable_head_types},
    {"disable_head", "o", disable_head_types},
    {"apply", "", all_null_types},
    {"test", "", all_null_types},
    {"destroy", "", all_null_types}};

struct wl_message const mw::ZwlrOutputConfigurationV1::Thunks::event_messages[] {
    {"succeeded", "", all_null_types},
    {"failed", "", all_null_types},
    {"cancelled", "", all_null_types}};

void const* mw::ZwlrOutputConfigurationV1::Thunks::request_vtable[] {
    (void*)Thunks::enable_head_thunk,
    (void*)Thunks::disable_head_thunk,
    (void*)Thunks::apply_thunk,
    (void*)Thunks::test_thunk,
    (void*)Thunks::destroy_thunk};

mw::ZwlrOutputConfigurationV1* mw::ZwlrOutputConfigurationV1::from(struct wl_resource* resource)
{
    if (resource &&
        wl_resource_instance_of(resource, &zwlr_output_configuration_v1_interface_data, ZwlrOutputConfigurationV1::Thunks::request_vtable))
    {
        return static_cast<ZwlrOutputConfigurationV1*>(wl_resource_get_user_data(resource));
    }
    else
    {
        return nullptr;
    }
}

// ZwlrOutputConfigurationHeadV1

struct mw::ZwlrOutputConfigurationHeadV1::Thunks
{
    static int const supported_version;

    static void set_mode_thunk(struct wl_client* client, struct wl_resource* resource, struct wl_resource* mode)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
            me->set_mode(mode);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationHeadV1::set_mode()");
        }
    }

    static void set_custom_mode_thunk(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height, int32_t refresh)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
            me->set_custom_mode(width, height, refresh);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationHeadV1::set_custom_mode()");
        }
    }

    static void set_position_thunk(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
            me->set_position(x, y);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationHeadV1::set_position()");
        }
    }

    static void set_transform_thunk(struct wl_client* client, struct wl_resource* resource, int32_t transform)
    {
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
            me->set_transform(transform);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationHeadV1::set_transform()");
        }
    }

    static void set_scale_thunk(struct wl_client* client, struct wl_resource* resource, wl_fixed_t scale)
    {
        double scale_resolved{wl_fixed_to_double(scale)};
        try
        {
            auto me = static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
            me->set_scale(scale_resolved);
        }
        catch(ProtocolError const& err)
        {
            wl_resource_post_error(err.resource(), err.code(), "%s", err.message());
        }
        catch(...)
        {
            internal_error_processing_request(client, "ZwlrOutputConfigurationHeadV1::set_scale()");
        }
    }

    static void resource_destroyed_thunk(wl_resource* resource)
    {
        delete static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
    }

    static struct wl_interface const* set_mode_types[];
    static struct wl_message const request_messages[];
    static void const* request_vtable[];
};

int const mw::ZwlrOutputConfigurationHeadV1::Thunks::supported_version = 2;

mw::ZwlrOutputConfigurationHeadV1::ZwlrOutputConfigurationHeadV1(struct wl_resource* resource, Version<2>)
    : Resource{resource}
{
    wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);
}

mw::ZwlrOutputConfigurationHeadV1::~ZwlrOutputConfigurationHeadV1()
{
    wl_resource_set_implementation(resource, nullptr, nullptr, nullptr);
}

bool mw::ZwlrOutputConfigurationHeadV1::is_instance(wl_resource* resource)
{
    return wl_resource_instance_of(resource, &zwlr_output_configuration_head_v1_interface_data, Thunks::request_vtable);
}

void mw::ZwlrOutputConfigurationHeadV1::destroy_and_delete() const
{
    // Will result in this object being deleted
    wl_resource_destroy(resource);
}

uint32_t const mw::ZwlrOutputConfigurationHeadV1::Error::already_set;
uint32_t const mw::ZwlrOutputConfigurationHeadV1::Error::invalid_mode;
uint32_t const mw::ZwlrOutputConfigurationHeadV1::Error::invalid_custom_mode;
uint32_t const mw::ZwlrOutputConfigurationHeadV1::Error::invalid_transform;
uint32_t const mw::ZwlrOutputConfigurationHeadV1::Error::invalid_scale;

struct wl_interface const* mw::ZwlrOutputConfigurationHeadV1::Thunks::set_mode_types[] {
    &zwlr_output_mode_v1_interface_data};

struct wl_message const mw::ZwlrOutputConfigurationHeadV1::Thunks::request_messages[] {
    {"set_mode", "o", set_mode_types},
    {"set_custom_mode", "iii", all_null_types},
    {"set_position", "ii", all_null_types},
    {"set_transform", "i", all_null_types},
    {"set_scale", "f", all_null_types}};

void const* mw::ZwlrOutputConfigurationHeadV1::Thunks::request_vtable[] {
    (void*)Thunks::set_mode_thunk,
    (void*)Thunks::set_custom_mode_thunk,
    (void*)Thunks::set_position_thunk,
    (void*)Thunks::set_transform_thunk,
    (void*)Thunks::set_scale_thunk};

mw::ZwlrOutputConfigurationHeadV1* mw::ZwlrOutputConfigurationHeadV1::from(struct wl_resource* resource)
{
    if (resource &&
        wl_resource_instance_of(resource, &zwlr_output_configuration_head_v1_interface_data, ZwlrOutputConfigurationHeadV1::Thunks::request_vtable))
    {
        return static_cast<ZwlrOutputConfigurationHeadV1*>(wl_resource_get_user_data(resource));
    }
    else
    {
        return nullptr;
    }
}

namespace mir
{
namespace wayland
{

struct wl_interface const zwlr_output_manager_v1_interface_data {
    mw::ZwlrOutputManagerV1::interface_name,
    mw::ZwlrOutputManagerV1::Thunks::supported_version,
    2, mw::ZwlrOutputManagerV1::Thunks::request_messages,
    3, mw::ZwlrOutputManagerV1::Thunks::event_messages};

struct wl_interface const zwlr_output_head_v1_interface_data {
    mw::ZwlrOutputHeadV1::interface_name,
    mw::ZwlrOutputHeadV1::Thunks::supported_version,
    0, nullptr,
    13, mw::ZwlrOutputHeadV1::Thunks::event_messages};

struct wl_interface const zwlr_output_mode_v1_interface_data {
    mw::ZwlrOutputModeV1::interface_name,
    mw::ZwlrOutputModeV1::Thunks::supported_version,
    0, nullptr,
    4, mw::ZwlrOutputModeV1::Thunks::event_messages};

struct wl_interface const zwlr_output_configuration_v1_interface_data {
    mw::ZwlrOutputConfigurationV1::interface_name,
    mw::ZwlrOutputConfigurationV1::Thunks::supported_version,
    5, mw::ZwlrOutputConfigurationV1::Thunks::request_messages,
    3, mw::ZwlrOutputConfigurationV1::Thunks::event_messages};

struct wl_interface const zwlr_output_configuration_head_v1_interface_data {
    mw::ZwlrOutputConfigurationHeadV1::interface_name,
    mw::ZwlrOutputConfigurationHeadV1::Thunks::supported_version,
    5, mw::ZwlrOutputConfigurationHeadV1::Thunks::request_messages,
    0, nullptr};

}
}
