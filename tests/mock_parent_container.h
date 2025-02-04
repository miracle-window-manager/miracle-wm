#ifndef MIRACLE_MOCK_PARENT_CONTAINER_H
#define MIRACLE_MOCK_PARENT_CONTAINER_H

#include "mock_container.h"
#include "parent_container.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{

    class MockParentContainer : public ParentContainer, public MockContainer
    {
    public:
        MockParentContainer(
            CompositorState const& state,
            OutputManager* output_manager,
            WindowController& window_controller,
            std::shared_ptr<Config> const& config,
            geom::Rectangle area,
            Workspace* workspace,
            std::shared_ptr<ParentContainer> const& parent,
            bool is_anchored) :
            ParentContainer(state, output_manager, window_controller, config, area, workspace, parent, is_anchored)
        {
        }
    };

} // namespace test
} // namespace miracle

#endif // MIRACLE_MOCK_PARENT_CONTAINER_H
