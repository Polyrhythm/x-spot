#include "story_node_component.h"

#include "dialogue_component.h"
#include "dialogue_loader.h"

#include <plugins/entity/entity.h>
#include <plugins/the_machinery_shared/component_interfaces/editor_ui_interface.h>

#include <foundation/api_registry.h>
#include <foundation/carray.inl>
#include <foundation/localizer.h>
#include <foundation/log.h>
#include <foundation/the_truth.h>

static const char* component__category(void)
{
	return TM_LOCALIZE("Dialogue");
}

static tm_ci_editor_ui_i* editor_aspect = &(tm_ci_editor_ui_i) {
	.category = component__category
};

static void truth__create_types(struct tm_the_truth_o* tt)
{
    tm_the_truth_property_definition_t custom_component_properties[] = {
        [TM_TT_PROP__STORY_NODE_COMPONENT__CONTENT] = { "content", TM_THE_TRUTH_PROPERTY_TYPE_STRING },
        [TM_TT_PROP__STORY_NODE_COMPONENT__CHARACTER] = { "character", TM_THE_TRUTH_PROPERTY_TYPE_STRING },
    };

    const tm_tt_type_t custom_component_type = tm_the_truth_api->create_object_type(tt, TM_TT_TYPE__STORY_NODE_COMPONENT, custom_component_properties, TM_ARRAY_COUNT(custom_component_properties));
    const tm_tt_id_t default_object = tm_the_truth_api->quick_create_object(tt, TM_TT_NO_UNDO_SCOPE, TM_TT_TYPE_HASH__STORY_NODE_COMPONENT,
        TM_TT_PROP__STORY_NODE_COMPONENT__CONTENT, "",
        TM_TT_PROP__STORY_NODE_COMPONENT__CHARACTER, "", -1);
    tm_the_truth_api->set_default_object(tt, custom_component_type, default_object);

    tm_the_truth_api->set_aspect(tt, custom_component_type, TM_CI_EDITOR_UI, editor_aspect);
}

static bool component__load_asset(tm_component_manager_o* man, tm_entity_t e, void* c_vp, const tm_the_truth_o* tt, tm_tt_id_t asset)
{
    struct tm_story_node_component_t* c = c_vp;
    const tm_the_truth_object_o* asset_r = tm_tt_read(tt, asset);

    c->content = tm_str(tm_the_truth_api->get_string(tt, asset_r, TM_TT_PROP__STORY_NODE_COMPONENT__CONTENT));
    c->character = tm_str(tm_the_truth_api->get_string(tt, asset_r, TM_TT_PROP__STORY_NODE_COMPONENT__CHARACTER));

    return true;
}

static void component__create(struct tm_entity_context_o* ctx)
{
    tm_component_i component = {
        .name = TM_TT_TYPE__STORY_NODE_COMPONENT,
        .bytes = sizeof(struct tm_story_node_component_t),
        .load_asset = component__load_asset,
    };

    tm_entity_api->register_component(ctx, &component);
}

void load_story_node_component(struct tm_api_registry_api* reg, bool load)
{
	tm_logger_api->printf(TM_LOG_TYPE_INFO, "WOOHOO!");

    tm_add_or_remove_implementation(reg, load, TM_THE_TRUTH_CREATE_TYPES_INTERFACE_NAME, truth__create_types);
    tm_add_or_remove_implementation(reg, load, TM_ENTITY_CREATE_COMPONENT_INTERFACE_NAME, component__create);
}