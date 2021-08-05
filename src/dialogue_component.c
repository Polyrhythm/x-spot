
#include "dialogue_loader.h"

#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/asset_io.h>
#include <foundation/buffer.h>
#include <foundation/carray.inl>
#include <foundation/carray_print.inl>
#include <foundation/config.h>
#include <foundation/error.h>
#include <foundation/hash.inl>
#include <foundation/json.h>
#include <foundation/localizer.h>
#include <foundation/log.h>
#include <foundation/math.inl>
#include <foundation/murmurhash64a.inl>
#include <foundation/os.h>
#include <foundation/path.h>
#include <foundation/sprintf.h>
#include <foundation/string.inl>
#include <foundation/task_system.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_assets.h>
#include <foundation/the_truth_types.h>
#include <foundation/undo.h>

#include <plugins/editor_views/asset_browser.h>
#include <plugins/editor_views/properties.h>
#include <plugins/entity/entity.h>
#include <plugins/entity/transform_component.h>
#include <plugins/the_machinery_shared/asset_aspects.h>
#include <plugins/the_machinery_shared/component_interfaces/editor_ui_interface.h>
#include <plugins/the_machinery_shared/scene_common.h>

#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_component.h>
#include <plugins/graph_interpreter/graph_node_helpers.inl>
#include <plugins/graph_interpreter/graph_node_macros.h>
#include <plugins/simulate/simulate_entry.h>

#include "dialogue_component.h"

struct tm_config_api* tm_config_api;
struct tm_entity_api* tm_entity_api;
struct tm_error_api* tm_error_api;
struct tm_transform_component_api* tm_transform_component_api;
struct tm_temp_allocator_api* tm_temp_allocator_api;
struct tm_the_truth_api* tm_the_truth_api;
struct tm_localizer_api* tm_localizer_api;
struct tm_logger_api* tm_logger_api;
struct tm_json_api* tm_json_api;
struct tm_os_api* tm_os_api;
struct tm_graph_interpreter_api* tm_graph_interpreter_api;
struct tm_properties_view_api* tm_properties_view_api;
struct tm_sprintf_api* tm_sprintf_api;
struct tm_asset_browser_add_asset_api* add_asset;
struct tm_path_api* tm_path_api;
struct tm_allocator_api* tm_allocator_api;
struct tm_task_system_api* task_system;
struct tm_scene_common_api* tm_scene_common_api;

enum {
    TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET,
};

struct tm_dialogue_component_t {
    char* text;
    uint64_t size;
};

typedef struct tm_component_manager_o {
    tm_entity_context_o* ctx;
    tm_allocator_i allocator;

    bool is_parsed;

    uint64_t scene_idx;

    tm_config_i* cd;
    tm_config_item_t characters;
    tm_config_item_t scene;
    tm_config_item_t nodes;
    tm_config_item_t map;
} tm_component_manager_o;

static char* int_to_s(int i)
{
    int length = snprintf(NULL, 0, "%d", i);
    char* str = malloc(length + 1);
    snprintf(str, length + 1, "%d", i);

    return str;
}

static const char* component__category(void)
{
    return TM_LOCALIZE("Story");
}

static tm_ci_editor_ui_i* editor_aspect = &(tm_ci_editor_ui_i){
    .category = component__category
};

static float properties__component_custom_ui(struct tm_properties_ui_args_t* args, tm_rect_t item_rect, tm_tt_id_t object, uint32_t indent)
{
    TM_INIT_TEMP_ALLOCATOR(ta);

    tm_tt_type_t asset_type = tm_the_truth_api->object_type_from_name_hash(args->tt, TM_TT_TYPE_HASH__DIALOGUE_STORY);
    tm_tt_id_t* ids = tm_the_truth_api->all_objects_of_type(args->tt, asset_type, ta);
    tm_tt_id_t* items = 0;
    const char** names = 0;
    tm_carray_temp_push(names, "Select", ta);
    tm_carray_temp_push(items, (tm_tt_id_t) { 0 }, ta);
    for (uint32_t i = 0; i < tm_carray_size(ids); ++i)
    {
        tm_tt_id_t owner = tm_the_truth_api->owner(args->tt, ids[i]);
        if (true)//tm_tt_type(owner).u64 == asset_type.u64)
        {
            tm_carray_temp_push(names, tm_the_truth_api->get_string(args->tt, tm_tt_read(args->tt, owner), TM_TT_PROP__ASSET__NAME), ta);
            tm_carray_temp_push(items, ids[i], ta);
        }
    }

    item_rect.y = tm_properties_view_api->ui_reference_popup_picker(args, item_rect, "Asset", NULL, object, TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET, names, items, (uint32_t)tm_carray_size(items));

    TM_SHUTDOWN_TEMP_ALLOCATOR(ta);

    return item_rect.y;
}

// -- API functions

static void toot(void)
{
    tm_logger_api->printf(TM_LOG_TYPE_INFO, "toot");
}

// -- end API functions

static int get_next(tm_component_manager_o* man, const char* node_idx)
{
    tm_config_i* cd = man->cd;
    tm_config_item_t* io_items;
    tm_config_item_t map_node = cd->object_get(cd->inst, man->map, tm_murmur_hash_string(node_idx));
    uint32_t io_count = cd->to_array(cd->inst, cd->object_get(cd->inst, map_node, TM_STATIC_HASH("io", 0x2c8639e08c0f0fddULL)), &io_items);

    if (io_count < 1)
    {
        return -1;
    }

    tm_config_item_t* io_inner_items;
    cd->to_array(cd->inst, io_items[0], &io_inner_items);

    return (int)cd->to_number(cd->inst, io_inner_items[2]);
}

typedef enum d_type
{
    D_NULL,
    ENTRY,
    DIALOG,
    CONTENT,
} d_type;

typedef struct d_data
{
    const char* character;
    const char* line;
    d_type type;
} d_data;

static void draw_text(const char* text)
{

}

static d_data get_text(tm_component_manager_o* man, tm_config_item_t* currnode, int line_idx)
{
    tm_config_i* cd = man->cd;

    tm_config_item_t data = cd->object_get(cd->inst, *currnode, TM_STATIC_HASH("data", 0x8fd0d44d20650b68ULL));
    const char* type = cd->to_string(cd->inst, cd->object_get(cd->inst, *currnode, TM_STATIC_HASH("type", 0xa21bd0e01ac8f01fULL)));

    d_data res;
    res.character = "";
    res.line = "";
    res.type = D_NULL;

    if (tm_strcmp_ignore_case(type, "content") == 0)
    {
        res.line = cd->to_string(cd->inst, cd->object_get(cd->inst, data, TM_STATIC_HASH("content", 0x8f0e5bf58c4edcf6ULL)));
        res.type = CONTENT;

        return res;
    }
    else if (tm_strcmp_ignore_case(type, "dialog") == 0)
    {
        tm_config_item_t* lines;
        cd->to_array(cd->inst, cd->object_get(cd->inst, data, TM_STATIC_HASH("lines", 0xfd431731eedb241dULL)), &lines);

        res.line = cd->to_string(cd->inst, lines[line_idx]);
        
        int char_idx = (int)cd->to_number(cd->inst, cd->object_get(cd->inst, data, TM_STATIC_HASH("character", 0x394e4fef3b347ba9ULL)));
        tm_config_item_t character = cd->object_get(cd->inst, man->characters, tm_murmur_hash_string(int_to_s(char_idx)));
        res.character = cd->to_string(cd->inst, cd->object_get(cd->inst, character, TM_STATIC_HASH("name", 0xd4c943cba60c270bULL)));

        res.type = DIALOG;

        return res;
    }

    return res;
}

static void start_dialogue(tm_component_manager_o* man)
{
    tm_config_i* cd = man->cd;

    int entry = (int)cd->to_number(cd->inst, cd->object_get(cd->inst, man->scene, TM_STATIC_HASH("entry", 0xcc88228c8069a76bULL)));

    int step = 1;
    int next = get_next(man, int_to_s(entry));
	tm_config_item_t currnode = cd->object_get(cd->inst, man->nodes, tm_murmur_hash_string(int_to_s(entry)));
    while (next >= 0)
    {
        if (step > 1)
        {
			currnode = cd->object_get(cd->inst, man->nodes, tm_murmur_hash_string(int_to_s(next)));
			next = get_next(man, int_to_s(next));
        }

        d_data data = get_text(man, &currnode, 0);
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "step: %d", step);
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "character: %s", data.character);
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "text: %s", data.line);
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "type: %d", data.type);
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "next: %d", next);
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "---------\n");

        step++;
    }
}

typedef struct tm_simulate_state_o
{
    float test_float;
} tm_simulate_state_o;

static tm_simulate_state_o* start(tm_simulate_start_args_t* args)
{
    tm_logger_api->printf(TM_LOG_TYPE_INFO, "start");

    tm_simulate_state_o* state = tm_alloc(args->allocator, sizeof(*state));
    *state = (tm_simulate_state_o){
        .test_float = 0,
    };

    TM_INIT_TEMP_ALLOCATOR(ta);
    tm_entity_context_o* e_ctx = args->entity_ctx;
    tm_component_type_t type = tm_entity_api->lookup_component_type(e_ctx, TM_TT_TYPE_HASH__DIALOGUE_COMPONENT);
    tm_component_manager_o* man = tm_entity_api->component_manager(e_ctx, type);
    const char* file = "E:/machinery/projects/x_spot/data/lol.json";
    tm_file_o f = tm_os_api->file_io->open_input(file);
    tm_file_stat_t stat = tm_os_api->file_system->stat(file);

    char* data = tm_temp_alloc(ta, stat.size);
    tm_os_api->file_io->read(f, data, stat.size);
    tm_os_api->file_io->close(f);

    tm_allocator_i allocator;
    tm_temp_allocator_api->allocator(&allocator, ta);
    tm_config_i* cd = tm_config_api->create(&allocator);
    man->cd = cd;
    tm_json_parse_info_t* pi = tm_json_api->parse_with_line_info(data, cd, 0, ta);

    if (!pi->success)
    {
        tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Error: %s", pi->error);
        return state;
    }

    tm_config_item_t root = cd->root(cd->inst);
    tm_config_item_t resources = cd->object_get(cd->inst, root, TM_STATIC_HASH("resources", 0xea2d07788c0118deULL));
    man->characters = cd->object_get(cd->inst, resources, TM_STATIC_HASH("characters", 0xa94318105571cca4ULL));
    tm_config_item_t scenes = cd->object_get(cd->inst, resources, TM_STATIC_HASH("scenes", 0x9e01cdce1780b727ULL));
    man->nodes = cd->object_get(cd->inst, resources, TM_STATIC_HASH("nodes", 0x6ea600aa4b4a3195ULL));

    tm_config_item_t* scene_keys;
    tm_config_item_t* scene_vals;
    uint32_t scene_count = cd->to_object(cd->inst, scenes, &scene_keys, &scene_vals);

    if (man->scene_idx > scene_count - 1)
    {
        tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Not enough scenes!");
        return state;
    }

    man->scene = scene_vals[man->scene_idx];

    man->map = cd->object_get(cd->inst, man->scene, TM_STATIC_HASH("map", 0xda34211fe66bc38bULL));

    start_dialogue(man);
    TM_SHUTDOWN_TEMP_ALLOCATOR(ta);

    return state;
}

static void stop(tm_simulate_state_o* state)
{
    tm_logger_api->printf(TM_LOG_TYPE_INFO, "stop");
}

enum {
    TM_TT_PROP__DIALOGUE_STORY__FILE,
    TM_TT_PROP__DIALOGUE_STORY__DATA,
};

static void truth__create_types(struct tm_the_truth_o* tt)
{
    tm_the_truth_property_definition_t dialogue_component_properties[] = {
        [TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET] = { "story_asset", .type = TM_THE_TRUTH_PROPERTY_TYPE_REFERENCE, .type_hash = TM_TT_TYPE_HASH__DIALOGUE_STORY},
    };

    const tm_tt_type_t dialogue_component_type = tm_the_truth_api->create_object_type(tt, TM_TT_TYPE__DIALOGUE_COMPONENT, dialogue_component_properties, TM_ARRAY_COUNT(dialogue_component_properties));

    static tm_properties_aspect_i properties_component_aspect = {
        .custom_ui = properties__component_custom_ui,
    };

    tm_the_truth_api->set_aspect(tt, dialogue_component_type, TM_CI_EDITOR_UI, editor_aspect);
    tm_the_truth_api->set_aspect(tt, dialogue_component_type, TM_TT_ASPECT__PROPERTIES, &properties_component_aspect);
}

static bool component__load_asset(tm_component_manager_o* man, tm_entity_t e, void* c_vp, const tm_the_truth_o* tt, tm_tt_id_t asset)
{
    struct tm_dialogue_component_t* c = c_vp;
    const tm_the_truth_object_o* asset_r = tm_tt_read(tt, asset);
    tm_tt_id_t id = tm_the_truth_api->get_reference(tt, asset_r, TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET);
    if (!id.u64)
    {
        return true;
    }

    tm_tt_buffer_t buffer = tm_the_truth_api->get_buffer(tt, tm_tt_read(tt, id), TM_TT_PROP__DIALOGUE_STORY__DATA);
    c->text = tm_alloc(&man->allocator, buffer.size);
    c->size = buffer.size;
    memcpy(c->text, buffer.data, buffer.size);

    return true;
}

static void component__remove(tm_component_manager_o* man, tm_entity_t e, void* data)
{
    struct tm_dialogue_component_t* sc = (struct tm_dialogue_component_t*)data;
    tm_free(&man->allocator, sc->text, sc->size);
}

static void component__destroy(tm_component_manager_o* man)
{
    tm_entity_api->call_remove_on_all_entities(man->ctx, tm_entity_api->lookup_component_type(man->ctx, TM_TT_TYPE_HASH__DIALOGUE_COMPONENT));
    tm_entity_context_o* ctx = man->ctx;
    tm_allocator_i a = man->allocator;
    tm_free(&a, man, sizeof(*man));
    tm_entity_api->destroy_child_allocator(ctx, &a);
}

static void component__create(struct tm_entity_context_o* ctx)
{
	tm_allocator_i a;
    tm_entity_api->create_child_allocator(ctx, TM_TT_TYPE__DIALOGUE_COMPONENT, &a);
    tm_component_manager_o* manager = tm_alloc(&a, sizeof(*manager));
    *manager = (tm_component_manager_o){
        .ctx = ctx,
        .allocator = a,
    };

	tm_component_i component = {
        .name = TM_TT_TYPE__DIALOGUE_COMPONENT,
        .bytes = sizeof(struct tm_dialogue_component_t),
        .manager = (tm_component_manager_o*)manager,
        .load_asset = component__load_asset,
        .remove = component__remove,
        .destroy = component__destroy,
    };

    tm_entity_api->register_component(ctx, &component);
}

// Runs on (dialogue_component, transform_component)
static void engine_update__dialogue_component(tm_engine_o* inst, tm_engine_update_set_t* data)
{
    struct tm_entity_context_o* ctx = (struct tm_entity_context_o*)inst;

    tm_logger_api->printf(TM_LOG_TYPE_INFO, "holy fuck");

    double t = 0;
    for (const tm_entity_blackboard_value_t* bb = data->blackboard_start; bb != data->blackboard_end; ++bb) {
        if (TM_STRHASH_EQUAL(bb->id, TM_ENTITY_BB__TIME))
            t = bb->double_value;
    }

    tm_component_type_t dialogue_type = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__DIALOGUE_COMPONENT);

    for (tm_engine_update_array_t* a = data->arrays; a < data->arrays + data->num_arrays; ++a) {
        //struct tm_dialogue_component_t* dialogue_component = a->components[0];
        //tm_transform_component_t* transform = a->components[1];
        tm_component_manager_o* man = tm_entity_api->component_manager(ctx, dialogue_type);
        if (man->is_parsed)
        {
            continue;
            tm_logger_api->printf(TM_LOG_TYPE_INFO, "dialogue component continue");
        }

        man->is_parsed = false;
        tm_logger_api->printf(TM_LOG_TYPE_INFO, "parse that shit");

        for (uint32_t i = 0; i < a->n; ++i) {
            //const float y = dialogue_component[i].y0 + dialogue_component[i].amplitude * sinf((float)t * dialogue_component[i].frequency);
        }
    }

    //tm_entity_api->notify(ctx, data->engine->components[1], mod_transform, (uint32_t)tm_carray_size(mod_transform));

}

static bool engine_filter__dialogue_component(tm_engine_o* inst, const tm_component_type_t* components, uint32_t num_components, const tm_component_mask_t* mask)
{
    return true;
}

static void component__register_engine(struct tm_entity_context_o* ctx)
{
    const tm_component_type_t dialogue_component = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__DIALOGUE_COMPONENT);
    const tm_component_type_t transform_component = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__TRANSFORM_COMPONENT);

    const tm_engine_i dialogue_component_engine = {
        .ui_name = "Dialogue Component",
        .hash  = TM_STATIC_HASH("DIALOGUE_COMPONENT", 0xc6cec009ae9b950aULL),
        .num_components = 2,
        .components = { dialogue_component, transform_component },
        .writes = { false, true },
        .update = engine_update__dialogue_component,
        .filter = engine_filter__dialogue_component,
        .inst = (tm_engine_o*)ctx,
    };
    tm_entity_api->register_engine(ctx, &dialogue_component_engine);
}

static tm_simulate_entry_i simulate_entry_i = {
    .id = TM_STATIC_HASH("tm_dialogue_simulate_entry", 0xee1be97eadeab0a5ULL),
    .display_name = TM_LOCALIZE_LATER("Dialogue Simulate Entry"),
    .start = start,
    .stop = stop,
    //.tick = tick,
};

 void load_dialogue_component(struct tm_api_registry_api* reg, bool load)
{
    tm_add_or_remove_implementation(reg, load, TM_THE_TRUTH_CREATE_TYPES_INTERFACE_NAME, truth__create_types);
    tm_add_or_remove_implementation(reg, load, TM_ENTITY_CREATE_COMPONENT_INTERFACE_NAME, component__create);
    tm_add_or_remove_implementation(reg, load, TM_ENTITY_SIMULATION_REGISTER_ENGINES_INTERFACE_NAME, component__register_engine);
    tm_add_or_remove_implementation(reg, load, TM_SIMULATE_ENTRY_INTERFACE_NAME, &simulate_entry_i);
}
