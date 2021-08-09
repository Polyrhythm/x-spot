#include "dialogue_loader.h"
#include "dialogue_component.h"
#include "story_text_ui.h"

#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/log.h>
#include <foundation/macros.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_types.h>

#include <plugins/entity/entity.h>
#include <plugins/renderer/render_backend.h>
#include <plugins/the_machinery_shared/component_interfaces/editor_ui_interface.h>
#include <plugins/ui_components/ui_components_loader.h>

typedef struct tm_component_manager_o
{
    tm_entity_context_o* ctx;
    tm_allocator_i allocator;
    tm_the_truth_object_o* tt;

    tm_renderer_backend_i* rb;
} tm_component_manager_o;

typedef struct tm_story_text_ui_component_t
{
    tm_rect_t rect;
} tm_story_text_ui_component_t;

static const char* component__category(void)
{
    return "Story";
}

static bool component__disabled(void)
{

}

static tm_ci_editor_ui_i* editor_aspect = &(tm_ci_editor_ui_i){
    .disabled = component__disabled,
    .category = component__category,
};

static void truth__create_types(struct tm_the_truth_o* tt)
{
   tm_the_truth_property_definition_t story_text_ui_properties[] = {
       [TM_TT_PROP__STORY_TEXT_UI_COMPONENT__RECT] = { "rect", TM_THE_TRUTH_PROPERTY_TYPE_SUBOBJECT, .type_hash = TM_TT_TYPE_HASH__RECT},
   };

   const tm_tt_type_t story_text_ui_type = tm_the_truth_api->create_object_type(tt, TM_TT_PROP__STORY_TEXT_UI_COMPONENT__RECT, story_text_ui_properties, TM_ARRAY_COUNT(story_text_ui_properties));

   tm_rect_t default_rect = { 45.f, 45.f, 100.f, 100.f };

   const tm_tt_id_t story_text_ui = tm_the_truth_api->create_object_of_type(tt, story_text_ui_type, TM_TT_NO_UNDO_SCOPE);
   tm_the_truth_object_o* story_text_ui_w = tm_the_truth_api->write(tt, story_text_ui);
   tm_the_truth_common_types_api->set_rect(tt, story_text_ui_w, TM_TT_PROP__STORY_TEXT_UI_COMPONENT__RECT, default_rect, TM_TT_NO_UNDO_SCOPE);
   tm_the_truth_api->commit(tt, story_text_ui_w, TM_TT_NO_UNDO_SCOPE);

   tm_the_truth_api->set_aspect(tt, story_text_ui_type, TM_CI_EDITOR_UI, editor_aspect);
   tm_the_truth_api->set_default_object(tt, story_text_ui_type, story_text_ui);
}

static bool component__load_asset(tm_component_manager_o* man, tm_entity_t e, void* data, const struct tm_the_truth_o* tt, tm_tt_id_t asset)
{
    tm_story_text_ui_component_t* c = data;
    const tm_the_truth_object_o* asset_r = tm_tt_read(tt, asset);

    c->rect = tm_the_truth_common_types_api->get_rect(tt, asset_r, TM_TT_PROP__STORY_TEXT_UI_COMPONENT__RECT);

    return true;
}

static void component__destroy(tm_component_manager_o* man)
{
    tm_entity_context_o* ctx = man->ctx;
    tm_allocator_i a = man->allocator;
    const tm_component_type_t type = tm_entity_api->lookup_component_type(ctx, TM_TT_TYPE_HASH__STORY_TEXT_UI_COMPONENT);
    tm_entity_api->call_remove_on_all_entities(ctx, type);
    tm_free(&a, man, sizeof(*man));
    tm_entity_api->destroy_child_allocator(ctx, &a);
}

static void component__create(struct tm_entity_context_o* ctx)
{
    tm_renderer_backend_i* rb = tm_global_api_registry->implementations(TM_RENDER_BACKEND_INTERFACE_NAME)[0];
    tm_allocator_i a;
    tm_entity_api->create_child_allocator(ctx, TM_TT_TYPE__STORY_TEXT_UI_COMPONENT, &a);
    tm_component_manager_o* man = tm_alloc(&a, sizeof(&man));
    *man = (tm_component_manager_o){
        .ctx = ctx,
        .tt = tm_entity_api->the_truth(ctx),
        .allocator = a,
        .rb = rb,
    };

    tm_component_i component = {
        .name = TM_TT_TYPE__STORY_TEXT_UI_COMPONENT,
        .bytes = sizeof(tm_story_text_ui_component_t),
        .manager = (tm_component_manager_o*)man,
        .destroy = component__destroy,
        .load_asset = component__load_asset,
    };

    tm_entity_api->register_component(ctx, &component);
}

void load_story_text_ui_component(struct tm_registry_api* reg, bool load)
{
    tm_logger_api->printf(TM_LOG_TYPE_INFO, "text ui loaded");
}