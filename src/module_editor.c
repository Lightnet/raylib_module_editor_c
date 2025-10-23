// module_editor.c
// for editor gui and others
#include "ecs_components.h"
#include "module_editor.h"
#include "raygui.h"

// non transform 3d example config, game logic, event, props
void render_2d_entity_scene_list_system(ecs_iter_t *it){

}

// filter out the transform3d entity list
void render_2d_transform_scene_list_system(ecs_iter_t *it){

}

// transform 3d position, rotation, scale
void render_2d_transform_control_system(ecs_iter_t *it){

}


void render_2d_menu_bar_editor_system(ecs_iter_t *it){
    DrawText(TextFormat("[editor] test ..."), 2, 2, 20, DARKGRAY);
    GuiButton((Rectangle){ 0, 0, 120, 24 }, "Menu");
    GuiButton((Rectangle){ 120, 0, 120, 24 }, "Scene"); 
    GuiButton((Rectangle){ 240, 0, 120, 24 }, "Blocks"); 
}

void setup_systems_editor(ecs_world_t *world){
    // ONLY 2D
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "render_2d_menu_bar_editor_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      //.query.terms = {
        //   { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        //   { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      //},
      .callback = render_2d_menu_bar_editor_system
    });
}

void setup_components_editor(ecs_world_t *world){

}

void module_init_editor(ecs_world_t *world){
    setup_components_editor(world);
    setup_systems_editor(world);
} 