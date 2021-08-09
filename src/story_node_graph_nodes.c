struct tm_dialogue_component_api* tm_dialogue_component_api;

#include "dialogue_component.h"
#include "dialogue_loader.h"
#include "story_node_component.h"

#include <foundation/api_registry.h>
#include <foundation/api_types.h>
#include <foundation/carray.inl>
#include <foundation/localizer.h>
#include <foundation/log.h>
#include <foundation/macros.h>
#include <foundation/string.inl>
#include <foundation/the_truth.h>
#include <foundation/the_truth_types.h>

#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_node_helpers.inl>
#include <plugins/graph_interpreter/graph_node_macros.h>
#include <plugins/graph_interpreter/graph_interpreter.h>
#include <plugins/graph_interpreter/graph_interpreter_loader.h>

#include <plugins/the_machinery_shared/scene_common.h>

// Helper function for quickly writing a string to a specific wire.
static inline void write_string_to_wire(tm_graph_interpreter_context_t *ctx, uint32_t wire, const char *str)
{
    char *result = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[wire], 1, (uint32_t)strlen(str) + 1);
    strcpy(result, str);
}


static const char* get_content(tm_entity_context_o* ctx, tm_entity_t e)
{
    tm_component_type_t type = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__STORY_NODE_COMPONENT);
	tm_story_node_component_t* cmp = tm_entity_api->get_component(ctx, e, type);
    
    return cmp->content.data;
}

static const char* get_name(tm_entity_context_o* ctx, tm_entity_t e)
{
    tm_component_type_t type = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__STORY_NODE_COMPONENT);
    tm_story_node_component_t* cmp = tm_entity_api->get_component(ctx, e, type);

    return cmp->name.data;
}

static tm_entity_t get_child_node(tm_entity_context_o* ctx, tm_entity_t root, const char* name)
{
    TM_INIT_TEMP_ALLOCATOR(ta);

    tm_entity_t* children = tm_entity_api->children(ctx, root, ta);
    int num_children = tm_carray_size(children);
    for (int i = 0; i < num_children; ++i)
    {
        const char* node_name = get_name(ctx, children[i]);
        if (tm_strcmp_ignore_case(name, node_name) == 0)
        {
            return children[i];
        }
    }

    tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Child name not found: %s", name);

    TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
    return root;
}

static const char* get_line(tm_entity_context_o* ctx, tm_entity_t dialog_root, int line_idx)
{
    TM_INIT_TEMP_ALLOCATOR(ta);
    tm_entity_t* line_nodes = tm_entity_api->children(ctx, get_child_node(ctx, dialog_root, "lines"), ta);
    int num_lines = tm_carray_size(line_nodes);
    TM_SHUTDOWN_TEMP_ALLOCATOR(ta);

    if (line_idx >= num_lines)
    {
        return "";
    }


    return get_content(ctx, line_nodes[line_idx]);
}


static tm_entity_t get_child_by_type(tm_entity_context_o* ctx, tm_entity_t root, const char* type)
{
    TM_INIT_TEMP_ALLOCATOR(ta);

    tm_entity_t* children = tm_entity_api->children(ctx, root, ta);
    int num_children = tm_carray_size(children);
    for (int i = 0; i < num_children; ++i)
    {
        tm_entity_t type_node = get_child_node(ctx, children[i], "type");
        const char* child_type = get_content(ctx, type_node);
        if (tm_strcmp_ignore_case(type, child_type) == 0)
        {
            return children[i];
        }
    }

    tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Child type not found: %s", type);
    return root;

    TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
}

static tm_entity_t get_next_node(tm_entity_context_o* ctx, tm_entity_t root, tm_entity_t scene_root)
{
    tm_logger_api->printf(TM_LOG_TYPE_INFO, "get next called: %s", get_name(ctx, root));
    const char* next_name = get_content(ctx, get_child_node(ctx, root, "next"));
    return get_child_node(ctx, scene_root, next_name);
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
    int num_scene = tm_carray_size(scene_root_children);
    
    if (idx >= num_scene)
    {
        tm_logger_api->printf(TM_LOG_TYPE_ERROR, "No scene index found!");
        return;
    }

    *scene_entity = scene_root_children[idx];

	TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
}

static inline void get_next_node_graph(tm_graph_interpreter_context_t* ctx, tm_entity_t scene_root, tm_entity_t current_node, tm_entity_t* next_node)
{
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    tm_entity_t parent_node = tm_entity_api->parent(entity_ctx, current_node);
    const char* parent_name = get_name(entity_ctx, parent_node);

    // go through entry first if at root
    if (tm_strcmp_ignore_case(parent_name, "scenes") == 0)
    {
        tm_entity_t entry_node = get_child_by_type(entity_ctx, scene_root, "entry");
        *next_node = get_next_node(entity_ctx, entry_node, scene_root);
        return;
    }

    tm_logger_api->printf(TM_LOG_TYPE_INFO, "currnode name: %s", get_name(entity_ctx, current_node));
    *next_node = get_next_node(entity_ctx, current_node, scene_root);
}

GGN_NODE_QUERY();
GGN_NODE_DISPLAY_NAME("Get Node");
static inline void get_node_graph(tm_graph_interpreter_context_t* ctx, tm_entity_t scene_root, const char *name, tm_entity_t* node)
{
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    *node = get_child_node(entity_ctx, scene_root, name);
}

static inline void print_node(tm_graph_interpreter_context_t* ctx, tm_entity_t entity)
{
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    const char* name = get_name(entity_ctx, entity);
    const char* content = get_content(entity_ctx, entity);

    tm_logger_api->printf(TM_LOG_TYPE_INFO, "Name: %s", name);
    tm_logger_api->printf(TM_LOG_TYPE_INFO, "Content: %s", content);
}
GGN_END();

static inline void get_name_and_content_graph_node_f(tm_graph_interpreter_context_t* ctx)
{
    tm_graph_interpreter_wire_content_t in_entity;
    tm_graph_interpreter_api->read_wires_indirect(ctx->interpreter, (tm_graph_interpreter_context_t* []){ &in_entity }, ctx->wires, 1);
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    write_string_to_wire(ctx, 1, get_name(entity_ctx, *(tm_entity_t*)in_entity.data));
    write_string_to_wire(ctx, 2, get_content(entity_ctx, *(tm_entity_t*)in_entity.data));
}

static tm_graph_component_node_type_i get_name_and_content_graph_node = {
    .category = "Story",
    .name = "tm_get_node_name_and_content",
    .static_connectors.num_in = 1,
    .static_connectors.in = {
        { "entity", TM_TT_TYPE_HASH__ENTITY_T, .optional = false },
    },
    .static_connectors.num_out = 2,
    .static_connectors.out = {
        { "name", TM_TT_TYPE_HASH__STRING },
        { "content", TM_TT_TYPE_HASH__STRING },
    },
    .run = get_name_and_content_graph_node_f,
};

static inline void get_type_graph_node_f(tm_graph_interpreter_context_t* ctx)
{
    tm_graph_interpreter_wire_content_t in_entity;
    tm_graph_interpreter_api->read_wires_indirect(ctx->interpreter, (tm_graph_interpreter_context_t* []){ &in_entity }, ctx->wires, 1);
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    tm_entity_t type_node = get_child_node(entity_ctx, *(tm_entity_t*)in_entity.data, "type");
    write_string_to_wire(ctx, 1, get_content(entity_ctx, type_node));
}

static tm_graph_component_node_type_i get_type_graph_node = {
    .category = "Story",
    .name = "tm_get_type",
    .static_connectors.num_in = 1,
    .static_connectors.in = {
        { "entity", TM_TT_TYPE_HASH__ENTITY_T, .optional = false },
    },
    .static_connectors.num_out = 1,
    .static_connectors.out = {
        { "type", TM_TT_TYPE_HASH__STRING },
    },
    .run = get_type_graph_node_f,
};

static inline void get_line_graph_node_f(tm_graph_interpreter_context_t* ctx)
{
    tm_graph_interpreter_wire_content_t in_entity, in_line_idx;
    tm_graph_interpreter_api->read_wires_indirect(ctx->interpreter, (tm_graph_interpreter_context_t* []){ &in_entity, &in_line_idx }, ctx->wires, 2);
    tm_entity_context_o* entity_ctx = private__get_entity_context(ctx);
    const char* line = get_line(entity_ctx, *(tm_entity_t*)in_entity.data, *(int*)in_line_idx.data);
    write_string_to_wire(ctx, 2, line);
}

static tm_graph_component_node_type_i get_line_graph_node = {
    .category = "Story",
    .name = "tm_get_line",
    .static_connectors.num_in = 2,
    .static_connectors.in = {
        { "entity", TM_TT_TYPE_HASH__ENTITY_T, .optional = false },
        { "line_idx", TM_TT_TYPE_HASH__UINT32_T, .optional = false },
    },
    .static_connectors.num_out = 1,
    .static_connectors.out = {
        { "line content", TM_TT_TYPE_HASH__STRING },

    },
    .run = get_line_graph_node_f,
};

#include "story_node_graph_nodes.inl"
void load_story_node_graph_nodes(struct tm_api_registry_api* reg, bool load)
{
    tm_graph_interpreter_api = reg->get(TM_GRAPH_INTERPRETER_API_NAME);
    tm_dialogue_component_api = reg->get(TM_DIALOGUE_COMPONENT_API);

    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &toot_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_scene_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &print_node_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_next_node_graph_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_name_and_content_graph_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_node_graph_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_type_graph_node);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, &get_line_graph_node);
}
