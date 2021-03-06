// DO NOT EDIT! Generated by generate-graph-nodes.

enum {
    TOOT__IN_EVENT,
    TOOT__OUT_EVENT,
};

static void toot_node_f(tm_graph_interpreter_context_t *ctx)
{

    toot(ctx);

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[TOOT__OUT_EVENT]);
}

static tm_graph_component_node_type_i toot_node = {
    .definition_path = __FILE__,
    .name = "tm_toot",
    .category = TM_LOCALIZE_LATER("Story"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_in = 1,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_out = 1,
    .run = toot_node_f,
};

enum {
    GET_SCENE__ROOT_ENTITY,
    GET_SCENE__SCENE_IDX,
    GET_SCENE__OUT_SCENE_ENTITY,
};

static const tm_graph_generic_value_t get_scene_scene_idx_default_value = { .f = (float[1]){ 0 } };

static void get_scene_node_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t root_entity_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_SCENE__ROOT_ENTITY]);
    const tm_graph_interpreter_wire_content_t scene_idx_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_SCENE__SCENE_IDX]);

    if (root_entity_w.n == 0)
        return;

    const tm_entity_t root_entity = *(tm_entity_t *)root_entity_w.data;
    const float scene_idx = scene_idx_w.n > 0 ? *(float *)scene_idx_w.data : *get_scene_scene_idx_default_value.f;

    tm_entity_t *scene_entity = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[GET_SCENE__OUT_SCENE_ENTITY], 1, sizeof(*scene_entity));

    get_scene(ctx, root_entity, scene_idx, scene_entity);
}

static tm_graph_component_node_type_i get_scene_node = {
    .definition_path = __FILE__,
    .name = "tm_get_scene",
    .category = TM_LOCALIZE_LATER("Story"),
    .static_connectors.in = {
        { "root_entity", TM_TT_TYPE_HASH__ENTITY_T },
        { "scene_idx", TM_TT_TYPE_HASH__FLOAT, .optional = true, .default_value = &get_scene_scene_idx_default_value },
    },
    .static_connectors.num_in = 2,
    .static_connectors.out = {
        { "scene_entity", TM_TT_TYPE_HASH__ENTITY_T },
    },
    .static_connectors.num_out = 1,
    .run = get_scene_node_f,
};

enum {
    GET_NEXT_NODE_GRAPH__IN_EVENT,
    GET_NEXT_NODE_GRAPH__SCENE_ROOT,
    GET_NEXT_NODE_GRAPH__CURRENT_NODE,
    GET_NEXT_NODE_GRAPH__OUT_EVENT,
    GET_NEXT_NODE_GRAPH__OUT_NEXT_NODE,
};

static void get_next_node_graph_node_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t scene_root_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_NEXT_NODE_GRAPH__SCENE_ROOT]);
    const tm_graph_interpreter_wire_content_t current_node_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_NEXT_NODE_GRAPH__CURRENT_NODE]);

    if (scene_root_w.n == 0)
        return;
    if (current_node_w.n == 0)
        return;

    const tm_entity_t scene_root = *(tm_entity_t *)scene_root_w.data;
    const tm_entity_t current_node = *(tm_entity_t *)current_node_w.data;

    tm_entity_t *next_node = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[GET_NEXT_NODE_GRAPH__OUT_NEXT_NODE], 1, sizeof(*next_node));

    get_next_node_graph(ctx, scene_root, current_node, next_node);

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[GET_NEXT_NODE_GRAPH__OUT_EVENT]);
}

static tm_graph_component_node_type_i get_next_node_graph_node = {
    .definition_path = __FILE__,
    .name = "tm_get_next_node_graph",
    .category = TM_LOCALIZE_LATER("Story"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "scene_root", TM_TT_TYPE_HASH__ENTITY_T },
        { "current_node", TM_TT_TYPE_HASH__ENTITY_T },
    },
    .static_connectors.num_in = 3,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "next_node", TM_TT_TYPE_HASH__ENTITY_T },
    },
    .static_connectors.num_out = 2,
    .run = get_next_node_graph_node_f,
};

enum {
    GET_NODE_GRAPH__SCENE_ROOT,
    GET_NODE_GRAPH__NAME,
    GET_NODE_GRAPH__OUT_NODE,
};

static void get_node_graph_node_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t scene_root_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_NODE_GRAPH__SCENE_ROOT]);
    const tm_graph_interpreter_wire_content_t name_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_NODE_GRAPH__NAME]);

    if (scene_root_w.n == 0)
        return;
    if (name_w.n == 0)
        return;

    const tm_entity_t scene_root = *(tm_entity_t *)scene_root_w.data;
    const char *name = (const char *)name_w.data;

    tm_entity_t *node = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[GET_NODE_GRAPH__OUT_NODE], 1, sizeof(*node));

    get_node_graph(ctx, scene_root, name, node);
}

static tm_graph_component_node_type_i get_node_graph_node = {
    .definition_path = __FILE__,
    .name = "tm_get_node_graph",
    .display_name = "Get Node",
    .category = TM_LOCALIZE_LATER("Story"),
    .static_connectors.in = {
        { "scene_root", TM_TT_TYPE_HASH__ENTITY_T },
        { "name", TM_TT_TYPE_HASH__STRING },
    },
    .static_connectors.num_in = 2,
    .static_connectors.out = {
        { "node", TM_TT_TYPE_HASH__ENTITY_T },
    },
    .static_connectors.num_out = 1,
    .run = get_node_graph_node_f,
};

enum {
    PRINT_NODE__IN_EVENT,
    PRINT_NODE__ENTITY,
    PRINT_NODE__OUT_EVENT,
};

static void print_node_node_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t entity_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[PRINT_NODE__ENTITY]);

    if (entity_w.n == 0)
        return;

    const tm_entity_t entity = *(tm_entity_t *)entity_w.data;

    print_node(ctx, entity);

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[PRINT_NODE__OUT_EVENT]);
}

static tm_graph_component_node_type_i print_node_node = {
    .definition_path = __FILE__,
    .name = "tm_print_node",
    .category = TM_LOCALIZE_LATER("Story"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "entity", TM_TT_TYPE_HASH__ENTITY_T },
    },
    .static_connectors.num_in = 2,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_out = 1,
    .run = print_node_node_f,
};
