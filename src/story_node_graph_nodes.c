struct tm_dialogue_component_api* tm_dialogue_component_api;

#include "dialogue_component.h"
#include "dialogue_loader.h"
#include "story_node_component.h"

#include <foundation/api_registry.h>
#include <foundation/api_types.h>
#include <foundation/localizer.h>
#include <foundation/log.h>
#include <foundation/macros.h>
#include <foundation/string.inl>
#include <foundation/the_truth.h>
#include <foundation/the_truth_types.h>

#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_node_helpers.inl>
#include <plugins/graph_interpreter/graph_node_macros.h>

#include <plugins/the_machinery_shared/scene_common.h>

static const char* get_content(tm_entity_context_o* ctx, tm_entity_t e)
{
    tm_component_type_t type = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__STORY_NODE_COMPONENT);
	tm_story_node_component_t* cmp = tm_entity_api->get_component(ctx, e, type);
    
    return cmp->content.data;
}

GGN_BEGIN("Story");
static inline void toot(tm_graph_interpreter_context_t* ctx)
{
    tm_dialogue_component_api->toot();
}

GGN_NODE_QUERY();
GGN_PARAM_OPTIONAL(scene_idx);
GGN_PARAM_DEFAULT_VALUE(scene_idx, 0);
static inline void get_scene(tm_graph_interpreter_context_t* ctx, tm_entity_t root_entity, float scene_idx, tm_entity_t* scene_entity)
{
	TM_INIT_TEMP_ALLOCATOR(ta);
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    tm_entity_t scene_root = tm_entity_api->children(entity_ctx, root_entity, ta)[0];
    tm_entity_t* scene_root_children = tm_entity_api->children(entity_ctx, scene_root, ta);
    int idx = (int)scene_idx;
    int num_scene = TM_ARRAY_COUNT(scene_root_children);
    
    if (idx >= num_scene)
    {
        tm_logger_api->printf(TM_LOG_TYPE_ERROR, "No scene index found!");
        return;
    }

    *scene_entity = scene_root_children[idx];

	TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
}

GGN_NODE_QUERY()
GGN_PARAM_OPTIONAL(line_idx);
GGN_PARAM_DEFAULT_VALUE(line_idx, 0);
static inline void get_line(tm_graph_interpreter_context_t* ctx, tm_entity_t dialog_entity, float line_idx, const char* content)
{
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    
}
GGN_END();

#include "story_node_graph_nodes.inl"
void load_story_node_graph_nodes(struct tm_api_registry_api* reg, bool load)
{
    tm_graph_interpreter_api = reg->get(TM_GRAPH_INTERPRETER_API_NAME);
    tm_dialogue_component_api = reg->get(TM_DIALOGUE_COMPONENT_API);

    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &toot_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_scene_node);

}
