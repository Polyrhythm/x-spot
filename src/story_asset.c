struct tm_tag_component_api* tm_tag_component_api;

#include "dialogue_component.h"
#include "dialogue_loader.h"
#include "story_asset.h"
#include "story_node_component.h"

#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/asset_io.h>
#include <foundation/buffer.h>
#include <foundation/config.h>
#include <foundation/json.h>
#include <foundation/localizer.h>
#include <foundation/log.h>
#include <foundation/murmurhash64a.inl>
#include <foundation/os.h>
#include <foundation/path.h>
#include <foundation/string.inl>
#include <foundation/task_system.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_assets.h>
#include <foundation/the_truth_types.h>
#include <foundation/undo.h>

#include <plugins/entity/entity.h>
#include <plugins/entity/tag_component.h>
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

static char* int_to_s(int i)
{
    int length = snprintf(NULL, 0, "%d", i);
    char* str = malloc(length + 1);
    snprintf(str, length + 1, "%d", i);

    return str;
}

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

void add_node_cmp(struct tm_the_truth_o* tt, tm_the_truth_object_o* entity, const char* name, const char* content, const tm_tt_undo_scope_t* undo_scope)
{
    tm_tt_type_t story_node_type = tm_the_truth_api->object_type_from_name_hash(tt, TM_TT_TYPE_HASH__STORY_NODE_COMPONENT);

	const tm_tt_id_t cmp = tm_the_truth_api->create_object_of_type(tt, story_node_type, *undo_scope);
	tm_the_truth_object_o* cmp_w = tm_the_truth_api->write(tt, cmp);
    tm_the_truth_api->set_string(tt, cmp_w, TM_TT_PROP__STORY_NODE_COMPONENT__NAME, name);
	tm_the_truth_api->set_string(tt, cmp_w, TM_TT_PROP__STORY_NODE_COMPONENT__CONTENT, content);
	tm_the_truth_api->add_to_subobject_set(tt, entity, TM_TT_PROP__ENTITY__COMPONENTS, &cmp_w, 1);
	tm_the_truth_api->commit(tt, cmp_w, *undo_scope);
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

    // parse json
    {
        TM_INIT_TEMP_ALLOCATOR(ta);

        tm_allocator_i allocator;
        tm_temp_allocator_api->allocator(&allocator, ta);
        const char* data = tm_the_truth_api->get_buffer(tt, tm_tt_read(tt, asset), TM_TT_PROP__DIALOGUE_STORY__DATA).data;
        tm_config_i* cd = tm_config_api->create(&allocator);
        tm_json_parse_info_t* pi = tm_json_api->parse_with_line_info(data, cd, 0, ta);

        if (!pi->success)
        {
            tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Error parsing json: %s", pi->error);
        }

        tm_config_item_t root = cd->root(cd->inst);
        tm_config_item_t resources = cd->object_get(cd->inst, root, TM_STATIC_HASH("resources", 0xea2d07788c0118deULL));
		tm_config_item_t nodes = cd->object_get(cd->inst, resources, TM_STATIC_HASH("nodes", 0x6ea600aa4b4a3195ULL));
        tm_config_item_t* node_keys;
        tm_config_item_t* node_vals;
        cd->to_object(cd->inst, nodes, &node_keys, &node_vals);

		tm_config_item_t scenes = cd->object_get(cd->inst, resources, TM_STATIC_HASH("scenes", 0x9e01cdce1780b727ULL));
        tm_config_item_t* scene_keys;
        tm_config_item_t* scene_vals;
        int num_scenes = cd->to_object(cd->inst, scenes, &scene_keys, &scene_vals);

		tm_config_item_t characters = cd->object_get(cd->inst, resources, TM_STATIC_HASH("characters", 0xa94318105571cca4ULL));
        tm_config_item_t* char_keys;
        tm_config_item_t* char_vals;
        int num_chars = cd->to_object(cd->inst, characters, &char_keys, &char_vals);

		tm_tt_id_t scenes_root_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
        tm_the_truth_object_o* scenes_root_e_w = tm_the_truth_api->write(tt, scenes_root_e);
        tm_the_truth_api->set_string(tt, scenes_root_e_w, TM_TT_PROP__ENTITY__NAME, "scenes");
        add_node_cmp(tt, scenes_root_e_w, "scenes", "", &undo_scope);
        tm_the_truth_api->commit(tt, scenes_root_e_w, undo_scope);
        tm_scene_common_api->place_entity(tt, scenes_root_e, local_transform, entity, undo_scope);

        // add scenes
        for (int scene_idx = 0; scene_idx < num_scenes; ++scene_idx)
        {
            const char* curr_scene = cd->to_string(cd->inst, scene_keys[scene_idx]);
            const char* scene_name = cd->to_string(cd->inst, cd->object_get(cd->inst, scene_vals[scene_idx], TM_STATIC_HASH("name", 0xd4c943cba60c270bULL)));
            tm_tt_id_t scene_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
            tm_the_truth_object_o* scene_e_w = tm_the_truth_api->write(tt, scene_e);
            tm_the_truth_api->set_string(tt, scene_e_w, TM_TT_PROP__ENTITY__NAME, curr_scene);

            // add component
            add_node_cmp(tt, scene_e_w, curr_scene, scene_name, &undo_scope);

            tm_the_truth_api->commit(tt, scene_e_w, undo_scope);
            tm_scene_common_api->place_entity(tt, scene_e, local_transform, scenes_root_e, undo_scope);

            // add nodes
            tm_config_item_t map = cd->object_get(cd->inst, scene_vals[scene_idx], TM_STATIC_HASH("map", 0xda34211fe66bc38bULL));
            tm_config_item_t* map_node_keys;
            tm_config_item_t* map_node_vals;
            int num_map_nodes = cd->to_object(cd->inst, map, &map_node_keys, &map_node_vals);

            for (int map_node_idx = 0; map_node_idx < num_map_nodes; ++map_node_idx)
            {
                const char* node_name = cd->to_string(cd->inst, map_node_keys[map_node_idx]);
                tm_config_item_t node = cd->object_get(cd->inst, nodes, tm_murmur_hash_string(node_name));
                tm_config_item_t node_data = cd->object_get(cd->inst, node, TM_STATIC_HASH("data", 0x8fd0d44d20650b68ULL));

                tm_config_item_t node_io = cd->object_get(cd->inst, map_node_vals[map_node_idx], TM_STATIC_HASH("io", 0x2c8639e08c0f0fddULL));
                tm_config_item_t* node_io_arrs;
                int num_io = (int)cd->to_array(cd->inst, node_io, &node_io_arrs);

                const char* node_type = cd->to_string(cd->inst, cd->object_get(cd->inst, node, TM_STATIC_HASH("type", 0xa21bd0e01ac8f01fULL)));

                tm_tt_id_t node_root_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
                tm_the_truth_object_o* node_root_e_w = tm_the_truth_api->write(tt, node_root_e);
                tm_the_truth_api->set_string(tt, node_root_e_w, TM_TT_PROP__ENTITY__NAME, node_name);

				// content
                if (tm_strcmp_ignore_case(node_type, STORY_NODE_TYPE_CONTENT) == 0)
                {
                    const char* content = cd->to_string(cd->inst, cd->object_get(cd->inst, node_data, TM_STATIC_HASH("content", 0x8f0e5bf58c4edcf6ULL)));
                    add_node_cmp(tt, node_root_e_w, node_name, content, &undo_scope);
                }
                // entry or dialog
                else
                {
                    add_node_cmp(tt, node_root_e_w, node_name, "", &undo_scope);
                }

                tm_the_truth_api->commit(tt, node_root_e_w, undo_scope);
                tm_scene_common_api->place_entity(tt, node_root_e, local_transform, scene_e, undo_scope);
                
                // type
                tm_tt_id_t node_type_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
                tm_the_truth_object_o* node_type_e_w = tm_the_truth_api->write(tt, node_type_e);
                tm_the_truth_api->set_string(tt, node_type_e_w, TM_TT_PROP__ENTITY__NAME, "type");

                add_node_cmp(tt, node_type_e_w, "type", node_type, &undo_scope);

                tm_the_truth_api->commit(tt, node_type_e_w, undo_scope);
                tm_scene_common_api->place_entity(tt, node_type_e, local_transform, node_root_e, undo_scope);

                // entry and content
                if (tm_strcmp_ignore_case(node_type, STORY_NODE_TYPE_CONTENT) == 0 ||
                    tm_strcmp_ignore_case(node_type, STORY_NODE_TYPE_ENTRY) == 0)
                {
                    // next
                    if (num_io > 0)
                    {
                        tm_tt_id_t next_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
						tm_the_truth_object_o* line_next_e_w = tm_the_truth_api->write(tt, next_e);
						tm_the_truth_api->set_string(tt, line_next_e_w, TM_TT_PROP__ENTITY__NAME, "next");
						
						tm_config_item_t* node_io_arr;
						cd->to_array(cd->inst, node_io_arrs[0], &node_io_arr);
						int next_id = (int)cd->to_number(cd->inst, node_io_arr[2]);
						add_node_cmp(tt, line_next_e_w, "next", int_to_s(next_id), &undo_scope);

						tm_the_truth_api->commit(tt, line_next_e_w, undo_scope);
						tm_scene_common_api->place_entity(tt, next_e, local_transform, node_root_e, undo_scope);
                    }
                }

                // dialog
                if (tm_strcmp_ignore_case(node_type, STORY_NODE_TYPE_DIALOG) == 0)
                {
                    int char_id = (int)cd->to_number(cd->inst, cd->object_get(cd->inst, node_data, TM_STATIC_HASH("character", 0x394e4fef3b347ba9ULL)));
                    tm_config_item_t node_char = cd->object_get(cd->inst, characters, tm_murmur_hash_string(int_to_s(char_id)));
                    const char* node_char_name = cd->to_string(cd->inst, cd->object_get(cd->inst, node_char, TM_STATIC_HASH("name", 0xd4c943cba60c270bULL)));

					tm_tt_id_t node_char_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
					tm_the_truth_object_o* node_char_e_w = tm_the_truth_api->write(tt, node_char_e);
					tm_the_truth_api->set_string(tt, node_char_e_w, TM_TT_PROP__ENTITY__NAME, "character");

					add_node_cmp(tt, node_char_e_w, "character", node_char_name, &undo_scope);

					tm_the_truth_api->commit(tt, node_char_e_w, undo_scope);
					tm_scene_common_api->place_entity(tt, node_char_e, local_transform, node_root_e, undo_scope);

                    // lines
					tm_tt_id_t node_lines_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
					tm_the_truth_object_o* node_lines_e_w = tm_the_truth_api->write(tt, node_lines_e);
					tm_the_truth_api->set_string(tt, node_lines_e_w, TM_TT_PROP__ENTITY__NAME, "lines");
                    add_node_cmp(tt, node_lines_e_w, "lines", "", &undo_scope);
                    tm_the_truth_api->commit(tt, node_lines_e_w, undo_scope);
                    tm_scene_common_api->place_entity(tt, node_lines_e, local_transform, node_root_e, undo_scope);

                    tm_config_item_t* lines;
                    int num_lines = cd->to_array(cd->inst, cd->object_get(cd->inst, node_data, TM_STATIC_HASH("lines", 0xfd431731eedb241dULL)), &lines);

                    for (int line_idx = 0; line_idx < num_lines; ++line_idx)
                    {
						tm_tt_id_t node_line_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
						tm_the_truth_object_o* node_line_e_w = tm_the_truth_api->write(tt, node_line_e);
						tm_the_truth_api->set_string(tt, node_line_e_w, TM_TT_PROP__ENTITY__NAME, int_to_s(line_idx));

                        add_node_cmp(tt, node_line_e_w, int_to_s(line_idx), cd->to_string(cd->inst, lines[line_idx]), &undo_scope);

						tm_the_truth_api->commit(tt, node_line_e_w, undo_scope);
						tm_scene_common_api->place_entity(tt, node_line_e, local_transform, node_lines_e, undo_scope);
                        
                        // next
                        if (num_io >= line_idx + 1)
                        {
							tm_tt_id_t line_next_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
							tm_the_truth_object_o* line_next_e_w = tm_the_truth_api->write(tt, line_next_e);
							tm_the_truth_api->set_string(tt, line_next_e_w, TM_TT_PROP__ENTITY__NAME, "next");
							
							tm_config_item_t* node_io_arr;
							cd->to_array(cd->inst, node_io_arrs[line_idx], &node_io_arr);
							int line_next = (int)cd->to_number(cd->inst, node_io_arr[2]);
							add_node_cmp(tt, line_next_e_w, "next", int_to_s(line_next), &undo_scope);

							tm_the_truth_api->commit(tt, line_next_e_w, undo_scope);
							tm_scene_common_api->place_entity(tt, line_next_e, local_transform, node_line_e, undo_scope);
                        }
					}
                }
            }

        }

        // add characters
        tm_tt_id_t char_root_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
        tm_the_truth_object_o* char_root_e_w = tm_the_truth_api->write(tt, char_root_e);
        tm_the_truth_api->set_string(tt, char_root_e_w, TM_TT_PROP__ENTITY__NAME, "characters");
        add_node_cmp(tt, char_root_e_w, "characters", "", &undo_scope);
        tm_the_truth_api->commit(tt, char_root_e_w, undo_scope);
        tm_scene_common_api->place_entity(tt, char_root_e, local_transform, entity, undo_scope);

        
        for (int i = 0; i < num_chars; ++i)
        {
            const char* char_name = cd->to_string(cd->inst, cd->object_get(cd->inst, char_vals[i], TM_STATIC_HASH("name", 0xd4c943cba60c270bULL)));
            tm_tt_id_t char_e = tm_the_truth_api->create_object_of_type(tt, entity_type, undo_scope);
            tm_the_truth_object_o* char_e_w = tm_the_truth_api->write(tt, char_e);
            const char* char_id = cd->to_string(cd->inst, char_keys[i]);
            tm_the_truth_api->set_string(tt, char_e_w, TM_TT_PROP__ENTITY__NAME, char_id);

            // add component
            add_node_cmp(tt, char_e_w, char_id, char_name, &undo_scope);

            tm_the_truth_api->commit(tt, char_e_w, undo_scope);
            tm_scene_common_api->place_entity(tt, char_e, local_transform, char_root_e, undo_scope);
        }

        TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
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
        [TM_TT_PROP__DIALOGUE_STORY__FILE] = {"import path", TM_THE_TRUTH_PROPERTY_TYPE_STRING},
        [TM_TT_PROP__DIALOGUE_STORY__DATA] = {"data", TM_THE_TRUTH_PROPERTY_TYPE_BUFFER},
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
    tm_tag_component_api = reg->get(TM_TAG_COMPONENT_API_NAME);

    tm_add_or_remove_implementation(reg, load, TM_THE_TRUTH_CREATE_TYPES_INTERFACE_NAME, truth__create_types);
    tm_add_or_remove_implementation(reg, load, TM_ASSET_BROWSER_CREATE_ASSET_INTERFACE_NAME, &asset_browser_create_dialogue_story);
}

