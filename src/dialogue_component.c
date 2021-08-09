
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

#include <plugins/the_machinery_shared/component_interfaces/editor_ui_interface.h>
#include <plugins/editor_views/asset_browser.h>
#include <plugins/editor_views/properties.h>
#include <plugins/entity/entity.h>
#include <plugins/entity/transform_component.h>
#include <plugins/the_machinery_shared/asset_aspects.h>
#include <plugins/the_machinery_shared/component_interfaces/editor_ui_interface.h>
#include <plugins/the_machinery_shared/scene_common.h>

#include <plugins/creation_graph/creation_graph.h>
#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_component.h>
#include <plugins/graph_interpreter/graph_node_helpers.inl>
#include <plugins/graph_interpreter/graph_node_macros.h>
#include <plugins/simulate/simulate_entry.h>
#include <plugins/ui/ui.h>
#include <plugins/ui/ui_custom.h>

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
struct tm_ui_api* tm_ui_api;
struct tm_ui_renderer_api* tm_ui_renderer_api;

enum {
    TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET,
};

struct tm_dialogue_component_t {
    char* text;
    bool should_notify;
    uint64_t size;
};

typedef struct tm_component_manager_o {
    tm_entity_context_o* ctx;
    tm_allocator_i allocator;
} tm_component_manager_o;

static const char* component__category(void)
{
    return TM_LOCALIZE("Story");
}

static tm_ci_editor_ui_i* editor_aspect = &(tm_ci_editor_ui_i){
    .category = component__category,
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

typedef struct tm_simulate_state_o
{
    float test_float;
} tm_simulate_state_o;

static tm_simulate_state_o* start(tm_simulate_start_args_t* args)
{
    tm_simulate_state_o state = (tm_simulate_state_o){
        .test_float = 1.0f,
    };

    return &state;
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

    c->should_notify = false;

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
    //struct tm_entity_context_o* ctx = (struct tm_entity_context_o*)inst;
    tm_ui_o* ui;
    tm_rect_t* rect;
    tm_ui_style_t* style;
    for (const tm_entity_blackboard_value_t* bb = data->blackboard_start; bb != data->blackboard_end; ++bb) {
        if (TM_STRHASH_EQUAL(bb->id, TM_ENTITY_BB__UI))
        {
            ui = bb->ptr_value;
        }

        if (TM_STRHASH_EQUAL(bb->id, TM_ENTITY_BB__UI_RECT))
        {
            rect = bb->ptr_value;
        }

        if (TM_STRHASH_EQUAL(bb->id, TM_ENTITY_BB__UI_STYLE))
        {
            style = bb->ptr_value;
        }
    }

    tm_rect_t r = (tm_rect_t){
        .h = 50,
        .w = 200,
        .x = 400,
        .y = 400,
    };

    const tm_ui_button_t button = (tm_ui_button_t){
        .text = "step",
        .rect = r,
    };

    
    bool clicked = tm_ui_api->button(ui, style, &button);

    const char* curr_content = "lol";
    const char* lines[MAX_LINES] = {"", "", "", ""};

    for (tm_engine_update_array_t* a = data->arrays; a < data->arrays + data->num_arrays; ++a) {
        struct tm_graph_component_t* graph_cmp = a->components[1];

        for (uint32_t i = 0; i < a->n; ++i)
        {
            curr_content = tm_graph_interpreter_api->read_variable(graph_cmp->gr, TM_STATIC_HASH("curr_content", 0xc6a0c239392ec385ULL).u64).data;

            lines[0] = tm_graph_interpreter_api->read_variable(graph_cmp->gr, TM_STATIC_HASH("line0", 0xc247fedbdde616d1ULL).u64).data;
            lines[1] = tm_graph_interpreter_api->read_variable(graph_cmp->gr, TM_STATIC_HASH("line1", 0x97c85cd1b0cc9372ULL).u64).data;
            lines[2] = tm_graph_interpreter_api->read_variable(graph_cmp->gr, TM_STATIC_HASH("line2", 0x56c7ef7abb5f6399ULL).u64).data;
            lines[3] = tm_graph_interpreter_api->read_variable(graph_cmp->gr, TM_STATIC_HASH("line3", 0x22faf8dd917b6a7aULL).u64).data;

            if (clicked)
            {
                tm_graph_interpreter_api->trigger_event(graph_cmp->gr, TM_STATIC_HASH("story_step", 0xb7f87569121db7adULL));
            }
        }
    }

    const tm_rect_t content_rect = (tm_rect_t){
        .x = 10,
        .y = 10,
        .h = 10,
        .w = 10,
    };

    const tm_ui_text_t content_text = (tm_ui_text_t){
        .text = curr_content,
        .rect = content_rect,
    };

    tm_ui_api->text(ui, style, &content_text);

    tm_rect_t line_rect = (tm_rect_t){
        .x = 10,
        .y = 40,
        .h = 20,
        .w = 20,
    };

    for (int i = 0; i < MAX_LINES; ++i)
    {
        line_rect.y += 20;
        //uint8_t blue = tm_ui_api->is_hovering(ui, line_rect, style->clip) ? 0 : 255;
        tm_color_srgb_t clr = (tm_color_srgb_t){
            .a = 255,
            .r = 255,
            .g = 255,
            .b = 255,
        };
        const tm_ui_text_t line_text = (tm_ui_text_t){
            .text = lines[i],
            .rect = line_rect,
            .color = &clr,
        };
        
        tm_ui_api->text(ui, style, &line_text);
    }
}

static bool engine_filter__dialogue_component(tm_engine_o* inst, const tm_component_type_t* components, uint32_t num_components, const tm_component_mask_t* mask)
{
    return tm_entity_mask_has_component(mask, components[0]) && tm_entity_mask_has_component(mask, components[1]);
}

static struct tm_dialogue_component_api* tm_dialogue_component_api = &(struct tm_dialogue_component_api) {
    .toot = toot,
};

static void component__register_engine(struct tm_entity_context_o* ctx)
{
    const tm_component_type_t dialogue_component = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__DIALOGUE_COMPONENT);
    const tm_component_type_t graph_component = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__GRAPH_COMPONENT);

    const tm_engine_i dialogue_component_engine = {
        .ui_name = TM_LOCALIZE_LATER("Engine: Dialogue + Graph"),
        .hash  = TM_STATIC_HASH("DIALOGUE_COMPONENT", 0xc6cec009ae9b950aULL),
        .num_components = 2,
        .components = { dialogue_component, graph_component },
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
    //tm_add_or_remove_implementation(reg, load, TM_SIMULATE_ENTRY_INTERFACE_NAME, &simulate_entry_i);

    tm_set_or_remove_api(reg, load, TM_DIALOGUE_COMPONENT_API, tm_dialogue_component_api);
}
