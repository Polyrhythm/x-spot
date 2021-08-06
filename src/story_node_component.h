#pragma once

#include <foundation/api_types.h>

#define TM_TT_TYPE__STORY_NODE_COMPONENT "tm_dialogue_story_node_component"
#define TM_TT_TYPE_HASH__STORY_NODE_COMPONENT TM_STATIC_HASH("tm_dialogue_story_node_component", 0x4d6d7070d9247cb1ULL)

enum {
	TM_TT_PROP__STORY_NODE_COMPONENT__CONTENT,
};

typedef struct tm_story_node_component_t
{
	tm_str_t content;
} tm_story_node_component_t;