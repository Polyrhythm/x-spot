#pragma once

#include <foundation/api_types.h>

#define TM_DIALOGUE_COMPONENT_API "tm_dialogue_component_api"

#define TM_TT_TYPE__DIALOGUE_COMPONENT "tm_dialogue_component"
#define TM_TT_TYPE_HASH__DIALOGUE_COMPONENT TM_STATIC_HASH("tm_dialogue_component", 0x2672fc2c845f0a49ULL)

// asset stuff
#define TM_TT_TYPE__DIALOGUE_STORY__FILE "tm_dialogue_story_file"
#define TM_TT_TYPE__DIALOGUE_STORY__DATA "tm_dialogue_story_data"

#define TM_TT_TYPE__DIALOGUE_STORY "tm_dialogue_story"
#define TM_TT_TYPE_HASH__DIALOGUE_STORY TM_STATIC_HASH("tm_dialogue_story", 0xfbf20104546ea8bfULL)


struct tm_dialogue_component_api
{
	void (*toot)(void);
};