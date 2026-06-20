#define APP_NAME "3D Model Viewer"
#define APP_GPU_RENDERING
#define APP_IMGUI

#if defined(WINDOWS)
#include <windows/hk_windows.h>
#else
static_assert(0, "no supported platform is defined");
#endif

typedef enum
{
    GRAPHICS_BUFFER_PER_FRAME_CB,
    GRAPHICS_BUFFER_VERTICES_SB,
    GRAPHICS_BUFFER_INDICES_SB,
    GRAPHICS_BUFFER_JOINT_TRANSFORMS_SB,
    GRAPHICS_BUFFER_MATERIAL_PROPERTIES_SB,
    GRAPHICS_BUFFER_COUNT
} graphics_buffer;

typedef enum
{
    INPUT_ACTION_TYPE_NONE,
    INPUT_ACTION_TYPE_NEXT_MODEL,
    INPUT_ACTION_TYPE_PREVIOUS_MODEL,
    INPUT_ACTION_TYPE_NEXT_ANIMATION,
    INPUT_ACTION_TYPE_PREVIOUS_ANIMATION,
    INPUT_ACTION_TYPE_ROTATE,
    INPUT_ACTION_TYPE_ZOOM_IN,
    INPUT_ACTION_TYPE_ZOOM_OUT,
    INPUT_ACTION_TYPE_ZOOM,
    INPUT_ACTION_TYPE_COUNT
} input_action_type;

// NOTE: This is used as a shader resource so padding must be made explicit.
// It is bound as a constant buffer so the size must be a multiple of 16 bytes.
typedef struct
{
    hk_f32_4x4 global_transform; // align: 16
    u32 vertex_offset;
    u32 index_offset;
    u32 material_id;
    u32 texture_id;
} per_draw;

// NOTE: This is used as a shader resource so padding must be made explicit.
// It is bound as a constant buffer so the size must be a multiple of 16 bytes.
typedef struct
{
    hk_f32_4x4 world_from_model; // align: 16
    hk_f32_4x4 clip_from_world;  // align: 16
    hk_f32_3x camera_pos;
    f32 _padding_0;
} per_frame;

typedef struct
{
    input_action_type type; // align: 4
    f32 repeat_rate;
} input_action;

typedef struct
{
    hk_f32_4x rotation; // align: 16
    hk_graphics_metrics* metrics;
    hk_animation animation;                                   // align: 4
    hk_camera camera;                                         // align: 4
    input_action input_action_map[HK_INPUT_EVENT_TYPE_COUNT]; // align: 4
    hk_graphics_api gfx_api;                                  // align: 4
    hk_graphics_api supported_gfx_apis;                       // align: 4
    hk_f32_3x scaling;
    hk_f32_3x translation;
    u32 model_id;
    u32 model_animation_count;
    b8 fullscreen;
    b8 vsync;
    b8 auto_rotate;
    b8 wireframe_mode;
} application_state;

typedef struct
{
    u32 model_id_last_frame;
    u32 max_vertex_count;
    u32 max_index_count;
    u32 max_joint_count;
    u32 max_material_count;
    u32 total_texture_count;
} models_metadata;

typedef enum
{
    MODEL_NONE,
    MODEL_ABSTRACT_RAINBOW_TRANSLUCENT_PENDANT,
    MODEL_BOX_ANIMATED,
    MODEL_BRAINSTEM,
    MODEL_CORSET,
    MODEL_DAMAGED_HELMET,
    MODEL_FOX,
    MODEL_FTM,
    MODEL_PLAYSTATION_1,
    MODEL_VIRTUAL_CITY,
    MODEL_WATER_BOTTLE,
    MODEL_COUNT
} asset_type_model;

GLOBAL c8* model_names[] = {"None",
                            "Abstract Rainbow Translucent Pendant",
                            "Box Animated",
                            "Brainstem",
                            "Corset",
                            "Damaged Helmet",
                            "Fox",
                            "Ftm",
                            "PlayStation 1",
                            "Virtual City",
                            "Water Bottle"};

GLOBAL hk_config config
    = {.permanent_mem_size = HK_MEBIBYTE(1024),
       .transient_mem_size = HK_KIBIBYTE(256),
       .min_gpu_mem_size = HK_MEBIBYTE(512),
       .input_queue_event_count = 10,
       .gamepad_deadzone = HK_INPUT_GAMEPAD_DEFAULT_DEADZONE,
       .gamepad_count = 1};

GLOBAL application_state app_state
    = {.camera = {.up_axis = {.y = 1.0f}, .arcball = true},
       .model_id = MODEL_DAMAGED_HELMET,
       .vsync = true};

FUNCTION void
reset_view(void)
{
    app_state.animation = (hk_animation){0};
    app_state.camera.position
        = (hk_f32_3x){.x = HK_PI / 4.0f, .y = HK_PI / 2.0f, .z = 6.0f};
    app_state.scaling = (hk_f32_3x){0};
    app_state.rotation = (hk_f32_4x){0};
    app_state.translation = (hk_f32_3x){0};
    app_state.auto_rotate = true;

    if (app_state.model_id == MODEL_ABSTRACT_RAINBOW_TRANSLUCENT_PENDANT)
    {
        app_state.scaling = hk_f32_3x_pack(0.8f);
        app_state.rotation
            = hk_f32_4x_rotation((hk_f32_3x){.x = 0.0f, .y = 1.0f, .z = 0.0f},
                                 -150.0f);
    }
    else if (app_state.model_id == MODEL_BOX_ANIMATED)
    {
        app_state.camera.position.y = HK_PI / 4.0f;
        app_state.scaling = hk_f32_3x_pack(0.45f);
    }
    else if (app_state.model_id == MODEL_BRAINSTEM)
    {
        app_state.camera.position.x = HK_PI / 2.0f;
        app_state.translation.y = -1.0f;
        app_state.auto_rotate = false;
    }
    else if (app_state.model_id == MODEL_CORSET)
    {
        app_state.scaling = hk_f32_3x_pack(35.0f);
        app_state.translation.y = -1.0f;
    }
    else if (app_state.model_id == MODEL_DAMAGED_HELMET)
    {
        app_state.scaling = hk_f32_3x_pack(1.125f);
    }
    else if (app_state.model_id == MODEL_FOX)
    {
        app_state.scaling = hk_f32_3x_pack(0.015f);
        app_state.translation.y = -1.0f;
    }
    else if (app_state.model_id == MODEL_FTM)
    {
        app_state.camera.position.y = HK_PI / 2.5f;
        app_state.scaling = hk_f32_3x_pack(0.13f);
        app_state.rotation
            = hk_f32_4x_rotation((hk_f32_3x){.x = 0.0f, .y = 1.0f, .z = 0.0f},
                                 180.0f);
    }
    else if (app_state.model_id == MODEL_PLAYSTATION_1)
    {
        app_state.scaling = hk_f32_3x_pack(0.5f);
        app_state.rotation = hk_f32_4x_mul_quaternion(
            hk_f32_4x_rotation((hk_f32_3x){.x = 1.0f, .y = 0.0f, .z = 0.0f},
                               90.0f),
            hk_f32_4x_rotation((hk_f32_3x){.x = 0.0f, .y = 1.0f, .z = 0.0f},
                               -90.0f));
    }
    else if (app_state.model_id == MODEL_VIRTUAL_CITY)
    {
        app_state.camera.position.y = HK_PI / 3.0f;
        app_state.scaling = hk_f32_3x_pack(0.075f);
    }
    else if (app_state.model_id == MODEL_WATER_BOTTLE)
    {
        app_state.scaling = hk_f32_3x_pack(8.0f);
        app_state.rotation
            = hk_f32_4x_rotation((hk_f32_3x){.x = 0.0f, .y = 1.0f, .z = 0.0f},
                                 -90.0f);
    }
}

FUNCTION void
imgui_ui(void)
{
#if defined(APP_IMGUI)
    hk_imgui_graphics_header(app_state.supported_gfx_apis,
                             app_state.metrics,
                             &app_state.gfx_api,
                             &app_state.fullscreen,
                             &app_state.vsync,
                             &app_state.wireframe_mode);

    u32 model_id = app_state.model_id;

    b8 model_selection_active
        = ImGui_CollapsingHeader("Models", ImGuiTreeNodeFlags_DefaultOpen);
    if (model_selection_active)
    {
        for (asset_type_model i = 1; i < MODEL_COUNT; i += 1)
        {
            ImGui_RadioButtonIntPtr(model_names[i],
                                    (s32*)&app_state.model_id,
                                    i);
        }
        if (app_state.model_id != model_id)
        {
            reset_view();
        }
    }

    if (app_state.model_animation_count)
    {
        b8 animation_selection_active
            = ImGui_CollapsingHeader("Animations",
                                     ImGuiTreeNodeFlags_DefaultOpen);
        if (animation_selection_active)
        {
            for (u32 i = 1; i <= app_state.model_animation_count; i += 1)
            {
                c8 animation_label[15] = {0};
                StringCchPrintfA(animation_label,
                                 sizeof(animation_label),
                                 "Animation %u",
                                 i);
                ImGui_RadioButtonIntPtr(animation_label,
                                        (s32*)&app_state.animation.id,
                                        i - 1);
            }
        }
    }

    b8 mouse_controls_active
        = ImGui_CollapsingHeader("Mouse Controls",
                                 ImGuiTreeNodeFlags_DefaultOpen);
    if (mouse_controls_active)
    {
        ImGui_Text("[Left Click + Drag]: Rotate");
        ImGui_Text("[Scroll]: Zoom In/Zoom Out");
    }

    b8 keyboard_controls_active
        = ImGui_CollapsingHeader("Keyboard Controls",
                                 ImGuiTreeNodeFlags_DefaultOpen);
    if (keyboard_controls_active)
    {
        ImGui_Text("[D/S/Right/Down]: Next Model");
        ImGui_Text("[W/A/Left/Up]: Previous Model");
        ImGui_Text("[E]: Next Animation");
        ImGui_Text("[Q]: Previous Animation");
        ImGui_Text("[Alt+Enter]: Toggle Fullscreen");
    }

    b8 gamepad_controls_active
        = ImGui_CollapsingHeader("Gamepad Controls",
                                 ImGuiTreeNodeFlags_DefaultOpen);
    if (gamepad_controls_active)
    {
        ImGui_Text("[Right/Down]: Next Model");
        ImGui_Text("[Left/Up]: Previous Model");
        ImGui_Text("[Right Bumper]: Next Animation");
        ImGui_Text("[Left Bumper]: Previous Animation");
        ImGui_Text("[Left/Right Stick]: Rotate");
        ImGui_Text("[Right Trigger/Left Trigger]: Zoom In/Zoom Out");
    }
#endif
}

FUNCTION void
init_app(hk_file_read_fp hk_file_read,
         hk_scratch_allocator* permanent_mem,
         hk_assets** assets,
         models_metadata* metadata,
         hk_input_queue* input_queue,
         hk_graphics_renderer_data* renderer_data,
         hk_error* err)
{
    // Read assets file.
    *assets = hk_assets_read_hka(hk_string_create(HK_ASSET_FILE_NAME, 0, err),
                                 0,
                                 0,
                                 0,
                                 0,
                                 MODEL_COUNT,
                                 hk_file_read,
                                 permanent_mem,
                                 err);
    static_assert(CAP(model_names) == MODEL_COUNT,
                  "unexpected model names count");

    // Get models metadata.
    for (u32 i = 0; i < (*assets)->model_count; i += 1)
    {
        hk_asset_model* model = &(*assets)->models[i];

        if (model->vertex_count > metadata->max_vertex_count)
        {
            metadata->max_vertex_count = model->vertex_count;
        }

        if (model->index_count > metadata->max_index_count)
        {
            metadata->max_index_count = model->index_count;
        }

        if (model->joint_count > metadata->max_joint_count)
        {
            metadata->max_joint_count = model->joint_count;
        }

        if (model->material_count > metadata->max_material_count)
        {
            metadata->max_material_count = model->material_count;
        }

        for (u32 j = 0; j < model->material_count; j += 1)
        {
            metadata->total_texture_count += model->materials[j].texture_count;
        }
    }

    // Initialize input queue.
    hk_scratch_alloc(permanent_mem,
                     config.input_queue_event_count * sizeof(hk_input_event),
                     alignof(hk_input_event),
                     &input_queue->events,
                     err);
    input_queue->event_count = config.input_queue_event_count;

    // Set input action map.
    for (hk_input_event_type et = 0; et < HK_INPUT_EVENT_TYPE_COUNT; et += 1)
    {
        input_action* at = &app_state.input_action_map[et];
        switch (et)
        {
            case HK_KEYBOARD_S:
            case HK_KEYBOARD_DOWN:
            case HK_GAMEPAD_DOWN:
            case HK_KEYBOARD_D:
            case HK_KEYBOARD_RIGHT:
            case HK_GAMEPAD_RIGHT:
            {
                at->type = INPUT_ACTION_TYPE_NEXT_MODEL;
                at->repeat_rate = HK_MILLISECOND(1.0f / 2.0f);
                break;
            }
            case HK_KEYBOARD_W:
            case HK_KEYBOARD_UP:
            case HK_GAMEPAD_UP:
            case HK_KEYBOARD_A:
            case HK_KEYBOARD_LEFT:
            case HK_GAMEPAD_LEFT:
            {
                at->type = INPUT_ACTION_TYPE_PREVIOUS_MODEL;
                at->repeat_rate = HK_MILLISECOND(1.0f / 2.0f);
                break;
            }
            case HK_KEYBOARD_E:
            case HK_GAMEPAD_RB:
            {
                at->type = INPUT_ACTION_TYPE_NEXT_ANIMATION;
                break;
            }
            case HK_KEYBOARD_Q:
            case HK_GAMEPAD_LB:
            {
                at->type = INPUT_ACTION_TYPE_PREVIOUS_ANIMATION;
                break;
            }
            case HK_MOUSE_MOVED:
            case HK_GAMEPAD_LS_MOVED:
            case HK_GAMEPAD_RS_MOVED:
            {
                at->type = INPUT_ACTION_TYPE_ROTATE;
                break;
            }
            case HK_GAMEPAD_LT:
            {
                at->type = INPUT_ACTION_TYPE_ZOOM_OUT;
                break;
            }
            case HK_GAMEPAD_RT:
            {
                at->type = INPUT_ACTION_TYPE_ZOOM_IN;
                break;
            }
            case HK_MOUSE_SCROLLED:
            {
                at->type = INPUT_ACTION_TYPE_ZOOM;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    // Initialize renderer data.
    {
        hk_graphics_buffer_data buffer_data[]
            = {{.elem_size = sizeof(per_frame),
                .shader_stage = HK_SHADER_STAGE_VERTEX,
                .max_elem_count = 1,
                .id = GRAPHICS_BUFFER_PER_FRAME_CB},
               {.elem_size = sizeof(hk_vertex),
                .shader_stage = HK_SHADER_STAGE_VERTEX,
                .max_elem_count = metadata->max_vertex_count,
                .id = GRAPHICS_BUFFER_VERTICES_SB},
               {.elem_size = sizeof(HK_GRAPHICS_INDEX_TYPE),
                .shader_stage = HK_SHADER_STAGE_VERTEX,
                .max_elem_count = metadata->max_index_count,
                .id = GRAPHICS_BUFFER_INDICES_SB},
               {.elem_size = sizeof(hk_f32_4x4),
                .shader_stage = HK_SHADER_STAGE_VERTEX,
                .max_elem_count = metadata->max_joint_count,
                .id = GRAPHICS_BUFFER_JOINT_TRANSFORMS_SB},
               {.elem_size = sizeof(hk_asset_material_properties),
                .shader_stage = HK_SHADER_STAGE_PIXEL,
                .max_elem_count = metadata->max_material_count,
                .id = GRAPHICS_BUFFER_MATERIAL_PROPERTIES_SB}};
        static_assert(CAP(buffer_data) == GRAPHICS_BUFFER_COUNT,
                      "unexpected buffer data count");

        *renderer_data = (hk_graphics_renderer_data){
            .constant_count = sizeof(per_draw) / sizeof(u32),
            .buffer_count = CAP(buffer_data),
            .max_texture_count = (*assets)->model_count
                                 * metadata->max_material_count
                                 * HK_TEXTURE_TYPE_COUNT,
            .wireframe = app_state.wireframe_mode,
            .render_target_srgb = true,
            .depth_buffer_bit_count = 32};

        hk_scratch_alloc(permanent_mem,
                         renderer_data->buffer_count
                             * sizeof(hk_graphics_buffer_data),
                         alignof(hk_graphics_buffer_data),
                         &renderer_data->buffer_data,
                         err);
        hk_copy(buffer_data,
                renderer_data->buffer_count * sizeof(hk_graphics_buffer_data),
                renderer_data->buffer_data,
                renderer_data->buffer_count * sizeof(hk_graphics_buffer_data),
                err);
    }

    reset_view();
}

FUNCTION void
process_action(input_action_type at, hk_f32_2x event_value, hk_error* err)
{
    f32 frame_time = app_state.metrics->cpu_last_frame_time;

    switch (at)
    {
        case INPUT_ACTION_TYPE_NEXT_MODEL:
        {
            u32 model_id = (app_state.model_id == MODEL_COUNT - 1)
                               ? 1
                               : app_state.model_id + 1;
            b8 model_changed = (app_state.model_id != model_id);
            app_state.model_id = model_id;
            if (model_changed)
            {
                reset_view();
            }
            break;
        }
        case INPUT_ACTION_TYPE_PREVIOUS_MODEL:
        {
            u32 model_id = (app_state.model_id == 1) ? MODEL_COUNT - 1
                                                     : app_state.model_id - 1;
            b8 model_changed = (app_state.model_id != model_id);
            app_state.model_id = model_id;
            if (model_changed)
            {
                reset_view();
            }
            break;
        }
        case INPUT_ACTION_TYPE_NEXT_ANIMATION:
        {
            app_state.animation.id = (app_state.animation.id
                                      == app_state.model_animation_count - 1)
                                         ? 0
                                         : app_state.animation.id + 1;
            break;
        }
        case INPUT_ACTION_TYPE_PREVIOUS_ANIMATION:
        {
            app_state.animation.id = (app_state.animation.id == 0)
                                         ? app_state.model_animation_count - 1
                                         : app_state.animation.id - 1;
            break;
        }
        case INPUT_ACTION_TYPE_ROTATE:
        {
            app_state.auto_rotate = false;

            f32 manual_rotation_rate = hk_f32_deg_to_rad(135.0f); // degrees/s
            hk_f32_2x rotation_speed
                = hk_f32_2x_mul(hk_f32_2x_pack((manual_rotation_rate
                                                * (1.0f / HK_MILLISECOND(1)))
                                               * frame_time),
                                event_value);
            app_state.camera.position.x += rotation_speed.x;
            app_state.camera.position.y += rotation_speed.y;
            hk_camera_clamp((hk_f32_2x){.min = 0.0f, .max = 2.0f * HK_PI},
                            (hk_f32_2x){.min = 0.0f, .max = HK_PI},
                            (hk_f32_2x){.min = 2.0f, .max = 10.0f},
                            true,
                            &app_state.camera);
            break;
        }
        case INPUT_ACTION_TYPE_ZOOM_IN:
        case INPUT_ACTION_TYPE_ZOOM_OUT:
        case INPUT_ACTION_TYPE_ZOOM:
        {
            if (at == INPUT_ACTION_TYPE_ZOOM_OUT)
            {
                event_value.x *= -1.0f;
            }
            f32 zoom_rate = 1.0f / 150.0f;
            f32 zoom_speed = zoom_rate * event_value.x * frame_time;
            app_state.camera.position.z -= zoom_speed;
            hk_camera_clamp((hk_f32_2x){.min = 0.0f, .max = 2.0f * HK_PI},
                            (hk_f32_2x){.min = 0.0f, .max = HK_PI},
                            (hk_f32_2x){.min = 2.0f, .max = 10.0f},
                            true,
                            &app_state.camera);
            break;
        }
        default:
        {
            HK_ERROR_MINOR("unexpected input action type");
            break;
        }
    }
}

FUNCTION void
update_app(hk_assets* assets,
           hk_input_queue* iq,
           models_metadata* metadata,
           hk_f32_2x render_res,
           hk_scratch_allocator* transient_mem,
           hk_graphics_renderer_data* renderer_data,
           hk_error* err)
{
    f32 frame_time = app_state.metrics->cpu_last_frame_time;

    // Process input.
    {
        // Process inputs in event queue.
        for (; iq->read_idx != iq->write_idx;
             iq->read_idx = (iq->read_idx + 1) % iq->event_count)
        {
            hk_input_event ie = iq->events[iq->read_idx];
            input_action* ia = &app_state.input_action_map[ie.event_type];

            // Skip any input event that does not have a mapped input action.
            if (!ia->type)
            {
                continue;
            }

            hk_f32_2x event_value = ie.value;
            {
                // Handle mouse click-to-drag.
                if (ia->type == INPUT_ACTION_TYPE_ROTATE
                    && ie.input_type == HK_INPUT_TYPE_MOUSE)
                {
                    // Skip if left mouse button is not held down.
                    if (iq->duration_held[HK_MOUSE_LEFT] == 0.0f)
                    {
                        continue;
                    }

                    // Compute the vector from the previous mouse position to
                    // the current one.
                    b8 drag = false;
                    for (u32 read_idx = (iq->read_idx + iq->event_count - 1)
                                        % iq->event_count;
                         read_idx != iq->write_idx;
                         read_idx
                         = (read_idx + iq->event_count - 1) % iq->event_count)
                    {
                        hk_input_event prev_ie = iq->events[read_idx];

                        // Don't look past the start of the chord.
                        if (prev_ie.event_type == HK_MOUSE_LEFT)
                        {
                            break;
                        }

                        if (prev_ie.event_type == HK_MOUSE_MOVED)
                        {
                            f32 cursor_multipler = 100.0f;
                            event_value = hk_f32_2x_mul(
                                hk_f32_2x_sub(ie.value, prev_ie.value),
                                hk_f32_2x_pack(cursor_multipler));
                            drag = true;
                            break;
                        }
                    }

                    // Skip if mouse not dragged.
                    if (!drag)
                    {
                        continue;
                    }
                }

                // Handle mouse scrolling.
                if (ia->type == INPUT_ACTION_TYPE_ZOOM
                    && ie.event_type == HK_MOUSE_SCROLLED)
                {
                    f32 scroll_multiplier = 5.0f;
                    event_value
                        = hk_f32_2x_mul(ie.value,
                                        hk_f32_2x_pack(scroll_multiplier));
                }
            }

            if (iq->duration_held[ie.event_type] > 0.0f)
            {
                process_action(ia->type, event_value, err);
            }
        }

        // Process held inputs.
        for (hk_input_event_type et = 0; et < (s32)CAP(iq->duration_held);
             et += 1)
        {
            input_action* ia = &app_state.input_action_map[et];

            if (ia->repeat_rate != 0.0f
                && iq->duration_held[et] > ia->repeat_rate)
            {
                process_action(ia->type, hk_f32_2x_pack(0.0f), err);
                iq->duration_held[et] -= ia->repeat_rate;
            }
        }
    }

    hk_asset_model* model = &assets->models[app_state.model_id];

    // Animate.
    {
        app_state.model_animation_count = model->animation_count;

        if (app_state.auto_rotate)
        {
            f32 auto_rotation_rate = hk_f32_deg_to_rad(30.0f); // degrees/s
            f32 rotation_speed
                = (auto_rotation_rate * (1.0f / HK_MILLISECOND(1)))
                  * frame_time;
            app_state.camera.position.x += rotation_speed;
            hk_camera_clamp((hk_f32_2x){.min = 0.0f, .max = 2.0f * HK_PI},
                            (hk_f32_2x){.min = 0.0f, .max = HK_PI},
                            (hk_f32_2x){.min = 2.0f, .max = 10.0f},
                            true,
                            &app_state.camera);
        }

        if (app_state.model_id != metadata->model_id_last_frame)
        {
            app_state.animation.time = 0.0f;
        }

        if (app_state.animation.id < model->animation_count)
        {
            app_state.animation.time += frame_time;

            // Loop the animation.
            if (app_state.animation.time
                > model->animations[app_state.animation.id].duration)
            {
                app_state.animation.time
                    -= model->animations[app_state.animation.id].duration;
            }
        }
    }

    // Compute transformation matrices.
    hk_f32_4x4 world_from_model
        = hk_f32_4x4_world_from_model(app_state.scaling,
                                      app_state.rotation,
                                      app_state.translation);
    hk_f32_3x camera_position
        = hk_camera_get_cartesian_position(&app_state.camera);
    hk_f32_4x4 view_from_world
        = hk_f32_4x4_view_from_world(camera_position,
                                     app_state.camera.focal_point,
                                     app_state.camera.up_axis);
    hk_f32_4x4 view_from_model
        = hk_f32_4x4_mul(view_from_world, world_from_model);
    hk_f32_4x4 clip_from_view = hk_f32_4x4_clip_from_view_perspective(
        27.0f,
        render_res.width / render_res.height,
        0.01f,
        16.0f);

    // Get drawables.
    hk_f32_4x4* joint_transforms = 0;
    hk_graphics_drawables drawables = {0};
    {
        u32 model_ids[] = {app_state.model_id};
        hk_animation animations[] = {app_state.animation};
        hk_assets_get_3d_drawables(assets,
                                   model_ids,
                                   animations,
                                   CAP(model_ids),
                                   &view_from_model,
                                   transient_mem,
                                   &joint_transforms,
                                   &drawables,
                                   err);
    }

    // Update renderer data.
    {
        // Update buffers.
        for (graphics_buffer gb = 0; gb < GRAPHICS_BUFFER_COUNT; gb += 1)
        {
            if (gb == GRAPHICS_BUFFER_PER_FRAME_CB)
            {
                hk_f32_4x4 clip_from_world
                    = hk_f32_4x4_mul(clip_from_view, view_from_world);

                per_frame* per_frame_data;
                hk_scratch_alloc(transient_mem,
                                 sizeof(per_frame),
                                 alignof(per_frame),
                                 &per_frame_data,
                                 err);
                *per_frame_data
                    = (per_frame){.world_from_model = world_from_model,
                                  .clip_from_world = clip_from_world,
                                  .camera_pos = camera_position};

                renderer_data->buffer_data[gb].buffer = per_frame_data;
                renderer_data->buffer_data[gb].elem_count = 1;
            }
            else if (gb == GRAPHICS_BUFFER_VERTICES_SB)
            {
                if (app_state.model_id != metadata->model_id_last_frame)
                {
                    renderer_data->buffer_data[gb].buffer = model->vertices;
                    renderer_data->buffer_data[gb].elem_count
                        = model->vertex_count;
                }
            }
            else if (gb == GRAPHICS_BUFFER_INDICES_SB)
            {
                if (app_state.model_id != metadata->model_id_last_frame)
                {
                    renderer_data->buffer_data[gb].buffer = model->indices;
                    renderer_data->buffer_data[gb].elem_count
                        = model->index_count;
                }
            }
            else if (gb == GRAPHICS_BUFFER_JOINT_TRANSFORMS_SB)
            {
                renderer_data->buffer_data[gb].buffer = joint_transforms;
                renderer_data->buffer_data[gb].elem_count = model->joint_count;
            }
            else if (gb == GRAPHICS_BUFFER_MATERIAL_PROPERTIES_SB)
            {
                hk_asset_material_properties* material_properties;
                hk_scratch_alloc(transient_mem,
                                 model->material_count
                                     * sizeof(hk_asset_material_properties),
                                 alignof(hk_asset_material_properties),
                                 &material_properties,
                                 err);

                for (u32 i = 0; i < model->material_count; i += 1)
                {
                    material_properties[i] = model->materials[i].properties;
                }

                renderer_data->buffer_data[gb].buffer = material_properties;
                renderer_data->buffer_data[gb].elem_count
                    = model->material_count;
            }

            if (renderer_data->buffer_data[gb].elem_count
                > renderer_data->buffer_data[gb].max_elem_count)
            {
                HK_ERROR_MAJOR(
                    "renderer buffer element count exceeds max element count");
            }
        }

        // Declare (required and optional) textures for upcoming frame.
        {
            hk_scratch_alloc(transient_mem,
                             metadata->total_texture_count
                                 * sizeof(hk_graphics_texture_data),
                             alignof(hk_graphics_texture_data),
                             &renderer_data->texture_data,
                             err);

            // Consider textures for the current model required and textures
            // for all other models in priority order (i.e. +/-1, +/-2, etc)
            // optional.
            u32 required_texture_count = 0;
            u32 optional_texture_count = 0;
            if (app_state.model_id != metadata->model_id_last_frame)
            {
                s32 offset = 0;
                b8 positive = true;
                for (u32 i = 0; i < assets->model_count; i += 1)
                {
                    s32 model_id = 0;
                    if (i == 0)
                    {
                        model_id = (s32)app_state.model_id;
                    }
                    else if (positive)
                    {
                        model_id = ((s32)app_state.model_id + offset)
                                   % (s32)assets->model_count;
                    }
                    else
                    {
                        model_id = ((s32)app_state.model_id - offset)
                                   % (s32)assets->model_count;
                        if (model_id < 0)
                        {
                            model_id += assets->model_count;
                        }
                    }

                    hk_asset_model* m = &assets->models[model_id];
                    for (u32 j = 0; j < m->material_count; j += 1)
                    {
                        for (u32 k = 0; k < m->materials[j].texture_count;
                             k += 1)
                        {
                            renderer_data
                                ->texture_data[required_texture_count
                                               + optional_texture_count]
                                = (hk_graphics_texture_data){
                                    .texture = &(m->materials[j].textures[k]),
                                    .id = (u32)hk_3d_to_1d_index(
                                        m->materials[j].textures[k].type,
                                        j,
                                        model_id,
                                        HK_TEXTURE_TYPE_COUNT,
                                        metadata->max_material_count)};

                            if (i == 0)
                            {
                                required_texture_count += 1;
                            }
                            else
                            {
                                optional_texture_count += 1;
                            }
                        }
                    }

                    if (i != 0)
                    {
                        positive = !positive;
                    }

                    if (positive)
                    {
                        offset += 1;
                    }
                }
            }

            renderer_data->required_texture_count = required_texture_count;
            renderer_data->optional_texture_count = optional_texture_count;
        }

        // Set draw data.
        {
            hk_scratch_alloc(transient_mem,
                             drawables.drawable_count
                                 * sizeof(hk_graphics_draw_data),
                             alignof(hk_graphics_draw_data),
                             &renderer_data->draw_data,
                             err);

            for (u32 i = 0; i < drawables.drawable_count; i += 1)
            {
                hk_graphics_drawable* d = &drawables.drawables[i];
                per_draw* per_draw_data;
                hk_scratch_alloc(transient_mem,
                                 sizeof(per_draw),
                                 alignof(per_draw),
                                 &per_draw_data,
                                 err);
                *per_draw_data
                    = (per_draw){.global_transform = d->global_transform,
                                 .vertex_offset = d->vertex_offset,
                                 .index_offset = d->index_offset,
                                 .material_id = d->material_id,
                                 .texture_id = (u32)hk_3d_to_1d_index(
                                     0,
                                     d->material_id,
                                     d->art_id,
                                     HK_TEXTURE_TYPE_COUNT,
                                     metadata->max_material_count)};

                renderer_data->draw_data[i] = (hk_graphics_draw_data){
                    .constants = per_draw_data,
                    .vertex_count = d->index_count,
                    .instance_count = 1,
                    .start_texture_id = per_draw_data->texture_id,
                    .texture_count = HK_TEXTURE_TYPE_COUNT,
                    .opaque = (i < drawables.opaque_drawable_count),
                };
            }

            renderer_data->draw_count = drawables.drawable_count;
            renderer_data->wireframe = app_state.wireframe_mode;
        }
    }
}

#if defined(WINDOWS)
s32 WINAPI
wWinMain(HINSTANCE inst, HINSTANCE prev_inst, WCHAR* cmd_args, s32 show_code)
{
    (void)prev_inst;
    (void)cmd_args;
    (void)show_code;

    hk_windows windows = {0};
    hk_error error = {.log = &hk_windows_error_log};
    hk_error* err = &error;

    hk_assets* assets = 0;
    models_metadata metadata = {0};

    hk_windows_init_window(&windows,
                           inst,
                           config.fixed_aspect_ratio_width,
                           config.fixed_aspect_ratio_height,
                           &app_state.fullscreen,
                           err);
    hk_windows_init_memory(&windows,
                           config.permanent_mem_size,
                           config.transient_mem_size,
                           err);

    init_app(&hk_windows_file_read,
             &windows.permanent_mem,
             &assets,
             &metadata,
             &windows.input_queue,
             &windows.gfx.renderer_data,
             err);

    hk_windows_init_graphics(&windows,
                             config.min_gpu_mem_size,
                             windows.gfx.renderer_data,
                             app_state.vsync,
                             &app_state.gfx_api,
                             &app_state.supported_gfx_apis,
                             err);
    hk_windows_init_metrics(&windows.metrics, err);
    app_state.metrics = &windows.metrics.gfx_metrics;

    while (windows.msg.message != WM_QUIT)
    {
        if (PeekMessageW(&windows.msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&windows.msg);
            DispatchMessageW(&windows.msg);
            continue;
        }

        hk_graphics_api gfx_api = app_state.gfx_api;

        hk_windows_update_input(&windows,
                                config.gamepad_deadzone,
                                config.gamepad_count,
                                err);

        update_app(assets,
                   &windows.input_queue,
                   &metadata,
                   windows.window.render_res,
                   &windows.transient_mem,
                   &windows.gfx.renderer_data,
                   err);
        metadata.model_id_last_frame = app_state.model_id;

        hk_windows_update_graphics(&windows,
                                   app_state.gfx_api,
                                   windows.gfx.renderer_data,
                                   app_state.fullscreen,
                                   app_state.vsync,
                                   &imgui_ui,
                                   err);

        hk_windows_update_metrics(&windows.metrics, err);

        if (app_state.gfx_api != gfx_api)
        {
            metadata.model_id_last_frame = 0;
            hk_windows_reload_graphics(&windows,
                                       inst,
                                       config.min_gpu_mem_size,
                                       windows.gfx.renderer_data,
                                       config.fixed_aspect_ratio_width,
                                       config.fixed_aspect_ratio_height,
                                       &app_state.fullscreen,
                                       app_state.vsync,
                                       &app_state.gfx_api,
                                       &app_state.supported_gfx_apis,
                                       err);
        }

        hk_scratch_free(&windows.transient_mem);
    }

    hk_windows_release(&windows);

    return 0;
}
#endif
