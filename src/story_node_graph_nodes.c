struct tm_dialogue_component_api* tm_dialogue_component_api;

#include "dialogue_component.h"
#include "dialogue_loader.h"

#include <foundation/api_registry.h>
#include <foundation/api_types.h>
#include <foundation/localizer.h>
#include <foundation/macros.h>
#include <foundation/string.inl>
#include <foundation/the_truth_types.h>

#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_node_helpers.inl>
#include <plugins/graph_interpreter/graph_node_macros.h>

#include <plugins/the_machinery_shared/scene_common.h>

GGN_BEGIN("Story");
GGN_NODE_QUERY();
static inline void toot(tm_graph_interpreter_context_t* ctx)
{
    tm_dialogue_component_api->toot();
}

static inline void get_scene(tm_graph_interpreter_context_t* ctx, tm_entity_t root_entity, int scene_idx, tm_entity_t* scene_entity)
{
	TM_INIT_TEMP_ALLOCATOR(ta);
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
	TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
}
GGN_END();

#include "story_node_graph_nodes.inl"
void load_story_node_graph_nodes(struct tm_api_registry_api* reg, bool load)
{
    tm_graph_interpreter_api = reg->get(TM_GRAPH_INTERPRETER_API_NAME);
    tm_dialogue_component_api = reg->get(TM_DIALOGUE_COMPONENT_API);

    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &toot_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_entity_child_node);

}
