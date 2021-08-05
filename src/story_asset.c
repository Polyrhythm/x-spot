#include "dialogue_component.h"
#include "dialogue_loader.h"
#include "story_asset.h"

#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/asset_io.h>
#include <foundation/buffer.h>
#include <foundation/localizer.h>
#include <foundation/log.h>
#include <foundation/os.h>
#include <foundation/path.h>
#include <foundation/string.inl>
#include <foundation/task_system.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_assets.h>
#include <foundation/the_truth_types.h>
#include <foundation/undo.h>

#include <plugins/entity/entity.h>
#include <plugins/editor_views/asset_browser.h>
#include <plugins/editor_views/properties.h>
#include <plugins/the_machinery_shared/asset_aspects.h>
#include <plugins/the_machinery_shared/component_interfaces/editor_ui_interface.h>
#include <plugins/the_machinery_shared/scene_common.h>

enum {
    TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET,
};

enum {
    TM_TT_PROP__DIALOGUE_STORY__FILE,
    TM_TT_PROP__DIALOGUE_STORY__DATA,
};

struct task__import_story
{
    uint64_t bytes;
    struct tm_asset_io_import args;
    char file[8];
};

static void task__import_story(void* data, uint64_t task_id)
{
    struct task__import_story* task = (struct task__import_story*)data;
    const struct tm_asset_io_import* args = &task->args;
    const char* json_file = task->file;
    tm_the_truth_o* tt = args->tt;

    tm_file_stat_t stat = tm_os_api->file_system->stat(json_file);
    if (!stat.exists)
    {
        tm_logger_api->printf(TM_LOG_TYPE_ERROR, "File does not exist: %s", json_file);
        return;
    }

    tm_buffers_i* buffers = tm_the_truth_api->buffers(tt);
    void* buffer = buffers->allocate(buffers->inst, stat.size, false);
    tm_file_o f = tm_os_api->file_io->open_input(json_file);
    const int64_t read = tm_os_api->file_io->read(f, buffer, stat.size);

    if (read != (int64_t)stat.size)
    {
        tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Could not read file: %s", json_file);
    }

    const uint32_t buffer_id = buffers->add(buffers->inst, buffer, stat.size, 0);
    const tm_tt_type_t plugin_asset_type = tm_the_truth_api->object_type_from_name_hash(tt, TM_TT_TYPE_HASH__DIALOGUE_STORY);
    const tm_tt_id_t asset_id = tm_the_truth_api->create_object_of_type(tt, plugin_asset_type, TM_TT_NO_UNDO_SCOPE);
    tm_the_truth_object_o* asset_obj = tm_the_truth_api->write(tt, asset_id);
    tm_the_truth_api->set_buffer(tt, asset_obj, TM_TT_PROP__DIALOGUE_STORY__DATA, buffer_id);
    tm_the_truth_api->set_string(tt, asset_obj, TM_TT_PROP__DIALOGUE_STORY__FILE, json_file);

    if (args->reimport_into.u64)
    {
        tm_the_truth_api->retarget_write(tt, asset_obj, args->reimport_into);
        tm_the_truth_api->commit(tt, asset_obj, args->undo_scope);
        tm_the_truth_api->destroy_object(tt, asset_id, args->undo_scope);
    }
    else
    {
		tm_the_truth_api->commit(tt, asset_obj, args->undo_scope);
        const char* asset_name = tm_path_api->base(tm_str(json_file)).data;
		const tm_tt_id_t current_dir = add_asset->current_directory(add_asset->inst, args->ui);
		const bool should_select = args->asset_browser.u64 && tm_the_truth_api->version(tt, args->asset_browser) == args->asset_browser_version_at_start;

		add_asset->add(add_asset->inst, current_dir, asset_id, asset_name, args->undo_scope, should_select, args->ui, 0, 0);
    }

        
    tm_os_api->file_io->close(f);
    tm_free(args->allocator, task, task->bytes);
}

static float properties__custom_ui(struct tm_properties_ui_args_t* args, tm_rect_t item_rect, tm_tt_id_t object, uint32_t indent)
{
    tm_the_truth_o* tt = args->tt;
    bool picked = false;
    item_rect.y = tm_properties_view_api->ui_open_path(args, item_rect, "Imported Path", "Path the file was imported from", object, TM_TT_PROP__DIALOGUE_STORY__FILE, "json", "*.json (Story files)", &picked);

    if (picked)
    {
        const char* file = tm_the_truth_api->get_string(tt, tm_tt_read(tt, object), TM_TT_PROP__DIALOGUE_STORY__FILE);
        {
            tm_allocator_i* allocator = tm_allocator_api->system;
            const uint64_t bytes = sizeof(struct task__import_story) + strlen(file);
            struct task__import_story* task = tm_alloc(allocator, bytes);
            *task = (struct task__import_story){
                .bytes = bytes,
                .args = {
                    .allocator = allocator,
                    .tt = tt,
                    .reimport_into = object
                }
            };

            strcpy(task->file, file);
            task_system->run_task(task__import_story, task, "Import Story");
        }
    }

    return item_rect.y;
}

static bool droppable(struct tm_asset_scene_o* inst, struct tm_the_truth_o* tt, tm_tt_id_t asset)
{
    return true;
}

tm_tt_id_t create_entity(struct tm_asset_scene_o* inst, struct tm_the_truth_o* tt,
    tm_tt_id_t asset, const char* name, const tm_transform_t* local_transform,
    tm_tt_id_t parent_entity, struct tm_undo_stack_i* undo_stack)
{
    const tm_tt_undo_scope_t undo_scope = tm_the_truth_api->create_undo_scope(tt, TM_LOCALIZE_LATER("Create Entity From Creation Graph"));
    const tm_tt_type_t entity_type = tm_the_truth_api->optional_object_type_from_name_hash(tt, TM_TT_TYPE_HASH__ENTITY);
    const tm_tt_id_t entity = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
    tm_the_truth_object_o* entity_w = tm_the_truth_api->write(tt, entity);
    tm_the_truth_api->set_string(tt, entity_w, TM_TT_PROP__ENTITY__NAME, name);

    // add story
    {
        tm_tt_type_t asset_type = tm_the_truth_api->object_type_from_name_hash(tt, TM_TT_TYPE_HASH__DIALOGUE_COMPONENT);
        const tm_tt_id_t component = tm_the_truth_api->create_object_of_type(tt, asset_type, undo_scope);
        tm_the_truth_object_o* component_w = tm_the_truth_api->write(tt, component);
        tm_the_truth_api->set_reference(tt, component_w, TM_TT_PROP__DIALOGUE_COMPONENT__STORY_ASSET, asset);
        tm_the_truth_api->add_to_subobject_set(tt, entity_w, TM_TT_PROP__ENTITY__COMPONENTS, &component_w, 1);
        tm_the_truth_api->commit(tt, component_w, undo_scope);
    }

    tm_the_truth_api->commit(tt, entity_w, undo_scope);
    tm_scene_common_api->place_entity(tt, entity, local_transform, parent_entity, undo_scope);

    // add child
    {
       const tm_tt_id_t child_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope); 
    }

    undo_stack->add(undo_stack->inst, tt, undo_scope);

    return entity;
}

tm_asset_scene_api scene_api = {
    .droppable = droppable,
    .create_entity = create_entity,
};



static bool asset_io__enabled(struct tm_asset_io_o* inst)
{
    return true;
}

static bool asset_io__can_import(struct tm_asset_io_o* inst, const char* extension)
{
    return tm_strcmp_ignore_case(extension, "json") == 0;
}

static bool asset_io__can_reimport(struct tm_asset_io_o* inst, struct tm_the_truth_o* tt, tm_tt_id_t asset)
{
    const tm_tt_id_t object = tm_the_truth_api->get_subobject(tt, tm_tt_read(tt, asset), TM_TT_PROP__ASSET__OBJECT);
    return tm_tt_type(object).u64 == tm_the_truth_api->object_type_from_name_hash(tt, TM_TT_TYPE_HASH__DIALOGUE_STORY).u64;
}

static void truth__create_types(struct tm_the_truth_o* tt)
{
	static tm_the_truth_property_definition_t story_properties[] = {
        {"import path", TM_THE_TRUTH_PROPERTY_TYPE_STRING},
        {"data", TM_THE_TRUTH_PROPERTY_TYPE_BUFFER},
    };

    static tm_properties_aspect_i properties_aspect = {
        .custom_ui = properties__custom_ui,
    };

    const tm_tt_type_t dialogue_story_type = tm_the_truth_api->create_object_type(tt, TM_TT_TYPE__DIALOGUE_STORY, story_properties, TM_ARRAY_COUNT(story_properties));
    tm_the_truth_api->set_aspect(tt, dialogue_story_type, TM_TT_ASPECT__FILE_EXTENSION, "story");
    tm_the_truth_api->set_aspect(tt, dialogue_story_type, TM_TT_ASPECT__PROPERTIES, &properties_aspect);
    tm_the_truth_api->set_aspect(tt, dialogue_story_type, TM_TT_ASPECT__ASSET_SCENE, &scene_api);
}

static tm_tt_id_t asset_browser_create(struct tm_asset_browser_create_asset_o* inst, tm_the_truth_o* tt, tm_tt_undo_scope_t undo_scope)
{
    const tm_tt_type_t type = tm_the_truth_api->object_type_from_name_hash(tt, TM_TT_TYPE_HASH__DIALOGUE_STORY);
    return tm_the_truth_api->create_object_of_type(tt, type, undo_scope);
}

static tm_asset_browser_create_asset_i asset_browser_create_dialogue_story = {
    .menu_name = TM_LOCALIZE_LATER("New Story"),
    .asset_name = TM_LOCALIZE_LATER("Story"),
    .create = asset_browser_create,
};

void load_story_asset(struct tm_api_registry_api* reg, bool load)
{
    tm_add_or_remove_implementation(reg, load, TM_THE_TRUTH_CREATE_TYPES_INTERFACE_NAME, truth__create_types);
    tm_add_or_remove_implementation(reg, load, TM_ASSET_BROWSER_CREATE_ASSET_INTERFACE_NAME, &asset_browser_create_dialogue_story);
}

