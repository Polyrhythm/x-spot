#include "dialogue_component.h"
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

#include <plugins/creation_graph/creation_graph.h>
#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_component.h>
#include <plugins/graph_interpreter/graph_node_helpers.inl>
#include <plugins/graph_interpreter/graph_node_macros.h>
#include <plugins/simulate/simulate_entry.h>
#include <plugins/ui/ui.h>

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
struct tm_creation_graph_api* tm_creation_graph_api;
struct tm_the_truth_common_types_api* tm_the_truth_common_types_api;
struct tm_api_registry_api* tm_global_api_registry;

extern void load_dialogue_component(struct tm_api_registry_api* reg, bool load);
extern void load_story_asset(struct tm_api_registry_api* reg, bool load);
extern void load_story_node_component(struct tm_api_registry_api* reg, bool load);
extern void load_story_node_graph_nodes(struct tm_api_registry_api* reg, bool load);
extern void load_story_text_ui_component(struct tm_api_registry_api* reg, bool load);

TM_DLL_EXPORT void tm_load_plugin(struct tm_api_registry_api* reg, bool load)
{
    tm_global_api_registry = reg;
    tm_entity_api = reg->get(TM_ENTITY_API_NAME);
    tm_transform_component_api = reg->get(TM_TRANSFORM_COMPONENT_API_NAME);
    tm_the_truth_api = reg->get(TM_THE_TRUTH_API_NAME);
    tm_temp_allocator_api = reg->get(TM_TEMP_ALLOCATOR_API_NAME);
    tm_localizer_api = reg->get(TM_LOCALIZER_API_NAME);

    tm_config_api = reg->get(TM_CONFIG_API_NAME);
    tm_error_api = reg->get(TM_ERROR_API_NAME);
    tm_logger_api = reg->get(TM_LOGGER_API_NAME);
    tm_json_api = reg->get(TM_JSON_API_NAME);
    tm_os_api = reg->get(TM_OS_API_NAME);
    tm_properties_view_api = reg->get(TM_PROPERTIES_VIEW_API_NAME);
    tm_sprintf_api = reg->get(TM_SPRINTF_API_NAME);
    add_asset = reg->get(TM_ASSET_BROWSER_ADD_ASSET_API_NAME);
    tm_path_api = reg->get(TM_PATH_API_NAME);
    tm_allocator_api = reg->get(TM_ALLOCATOR_API_NAME);
    task_system = reg->get(TM_TASK_SYSTEM_API_NAME);
    tm_scene_common_api = reg->get(TM_SCENE_COMMON_API_NAME);
    tm_transform_component_api = reg->get(TM_TRANSFORM_COMPONENT_API_NAME);
    tm_ui_api = reg->get(TM_UI_API_NAME);
    tm_creation_graph_api = reg->get(TM_CREATION_GRAPH_API_NAME);
    tm_the_truth_common_types_api = reg->get(TM_THE_TRUTH_COMMON_TYPES_API_NAME);

    tm_logger_api->printf(TM_LOG_TYPE_INFO, "hello dialogue world");
    
    load_dialogue_component(reg, load);
    load_story_asset(reg, load);
    load_story_node_component(reg, load);
    load_story_node_graph_nodes(reg, load);
    load_story_text_ui_component(reg, load);
}
